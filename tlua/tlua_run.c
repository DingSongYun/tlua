/*
** tlua_run.c — TypingLua Runner (like ts-node for TypeScript)
**
** Transpiles .tlua to Lua in memory, then executes it directly
** via the Lua 5.4 C API. No intermediate files needed.
**
** Usage:
**   tlua [options] <script.tlua> [args...]
**
** Options:
**   -e <code>   Execute a TypingLua string directly
**   -d          Enable debug mode (wait for debugger attach)
**   -v          Show version information
**   -h          Show help
**
** All arguments after the script name are passed to the script
** via the global 'arg' table (same convention as standard lua).
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* Lua 5.4 headers */
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

/* TypingLua transpiler */
#include "tlua_parser.h"

#define TLUA_VERSION "0.1.0"

/* Debug mode flag */
static int debug_mode = 0;
static const char *debug_host = "127.0.0.1";
static const char *debug_port = "12306";

/* ================================================================ */
/* Utility: file I/O                                                 */
/* ================================================================ */

static char *read_file_contents(const char *path, long *out_size) {
    FILE *f;
    long size;
    char *buf;

    if (strcmp(path, "-") == 0) {
        /* Read from stdin */
        size_t capacity = 4096;
        size_t length = 0;
        size_t n;
        buf = (char *)malloc(capacity);
        if (!buf) return NULL;
        while ((n = fread(buf + length, 1, capacity - length - 1, stdin)) > 0) {
            length += n;
            if (length + 1 >= capacity) {
                capacity *= 2;
                buf = (char *)realloc(buf, capacity);
                if (!buf) return NULL;
            }
        }
        buf[length] = '\0';
        if (out_size) *out_size = (long)length;
        return buf;
    }

    f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);

    buf = (char *)malloc(size + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    if (size > 0) {
        fread(buf, 1, size, f);
    }
    buf[size] = '\0';
    fclose(f);

    if (out_size) *out_size = size;
    return buf;
}

/* ================================================================ */
/* Lua error message handler (traceback)                             */
/* ================================================================ */

static int msghandler(lua_State *L) {
    const char *msg = lua_tostring(L, 1);
    if (msg == NULL) {
        /* Is there an error object with a __tostring metamethod? */
        if (luaL_callmeta(L, 1, "__tostring") &&
            lua_type(L, -1) == LUA_TSTRING) {
            return 1; /* use the result of __tostring */
        } else {
            msg = lua_pushfstring(L, "(error object is a %s value)",
                                  luaL_typename(L, 1));
        }
    }
    luaL_traceback(L, L, msg, 1);
    return 1;
}

/* ================================================================ */
/* Set up the 'arg' table (same convention as lua.c)                 */
/* ================================================================ */

/*
** arg[-2] = "tlua"      (interpreter name placeholder)
** arg[-1] = (none)
** arg[0]  = script name
** arg[1]  = first script argument
** arg[2]  = second script argument
** ...
*/
static void create_arg_table(lua_State *L, int argc, char **argv,
                             int script_index) {
    int i;
    lua_createtable(L, argc - script_index, script_index + 1);
    for (i = 0; i < argc; i++) {
        lua_pushstring(L, argv[i]);
        lua_rawseti(L, -2, i - script_index);
    }
    lua_setglobal(L, "arg");
}

/* ================================================================ */
/* Debug: initialize debugger                                        */
/* ================================================================ */

/*
** Debugger integration for .tlua scripts.
**
** We support two approaches, tried in order:
**
** 1. lua-debug (actboy168) — The recommended VSCode debugger.
**    Uses DAP protocol. We try: require("ldbg"):start("127.0.0.1:port")
**
** 2. mobdebug — A popular pure-Lua remote debugger (ZeroBrane Studio).
**    Uses: require("mobdebug").start(host, port)
**
** 3. Fallback — If no debugger library is found, we use Lua's built-in
**    debug.sethook to provide basic line tracing to stderr.
**
** The debugger is initialized AFTER luaL_openlibs but BEFORE any user
** code runs, so breakpoints on the first line of user code will work.
**
** Key insight: Because our transpiler performs type-erasure without
** changing line numbers, breakpoints set on .tlua source lines map
** directly to the transpiled Lua code lines. No source maps needed.
*/

static int init_debugger(lua_State *L) {
    int status;

    /*
    ** Strategy 1: Try lua-debug (actboy168)
    **
    ** lua-debug provides a DAP-based debugger. When installed as a
    ** VSCode extension, it places its runtime at a known path.
    ** The typical bootstrap is:
    **
    **   local dbg = require("ldbg")  -- or the path to debugger.lua
    **   dbg:start { host = "127.0.0.1", port = 12306 }
    **
    ** For embedded hosts, we use the "wait" approach: the debugger
    ** connects and we block until it attaches.
    */

    /* First, try to find lua-debug's bootstrap module */
    const char *lua_debug_bootstrap =
        "local function try_lua_debug(host, port)\n"
        "  -- Try the standard lua-debug module name\n"
        "  local ok, dbg = pcall(require, 'ldbg')\n"
        "  if not ok then\n"
        "    -- Try alternate name used by some installations\n"
        "    ok, dbg = pcall(require, 'debugger')\n"
        "  end\n"
        "  if ok and dbg then\n"
        "    if type(dbg) == 'table' and dbg.start then\n"
        "      dbg:start { host = host, port = tonumber(port), wait = true }\n"
        "      return true\n"
        "    elseif type(dbg) == 'function' then\n"
        "      dbg()\n"
        "      return true\n"
        "    end\n"
        "  end\n"
        "  return false\n"
        "end\n"
        "return try_lua_debug(...)\n";

    status = luaL_loadbuffer(L, lua_debug_bootstrap,
                             strlen(lua_debug_bootstrap),
                             "=(debugger bootstrap)");
    if (status == LUA_OK) {
        lua_pushstring(L, debug_host);
        lua_pushstring(L, debug_port);
        status = lua_pcall(L, 2, 1, 0);
        if (status == LUA_OK && lua_toboolean(L, -1)) {
            lua_pop(L, 1);
            fprintf(stderr, "tlua: debugger attached (lua-debug)\n");
            return 0;
        }
        lua_pop(L, 1);
    } else {
        lua_pop(L, 1); /* pop error message */
    }

    /*
    ** Strategy 2: Try mobdebug
    */
    const char *mobdebug_bootstrap =
        "local ok, mdb = pcall(require, 'mobdebug')\n"
        "if ok and mdb then\n"
        "  mdb.start(...)\n"
        "  return true\n"
        "end\n"
        "return false\n";

    status = luaL_loadbuffer(L, mobdebug_bootstrap,
                             strlen(mobdebug_bootstrap),
                             "=(mobdebug bootstrap)");
    if (status == LUA_OK) {
        lua_pushstring(L, debug_host);
        lua_pushinteger(L, atoi(debug_port));
        status = lua_pcall(L, 2, 1, 0);
        if (status == LUA_OK && lua_toboolean(L, -1)) {
            lua_pop(L, 1);
            fprintf(stderr, "tlua: debugger attached (mobdebug)\n");
            return 0;
        }
        lua_pop(L, 1);
    } else {
        lua_pop(L, 1);
    }

    /*
    ** Strategy 3: Fallback — built-in line tracer
    **
    ** If no external debugger is available, provide a basic line-by-line
    ** trace to stderr. This is useful for quick debugging without any
    ** external tools.
    */
    const char *trace_hook =
        "local info = debug.getinfo\n"
        "debug.sethook(function(event, line)\n"
        "  local i = info(2, 'S')\n"
        "  local src = i and i.short_src or '?'\n"
        "  io.stderr:write(string.format('[TRACE] %s:%d\\n', src, line))\n"
        "end, 'l')\n"
        "io.stderr:write('tlua: no external debugger found, using line tracer\\n')\n"
        "io.stderr:write('tlua: install lua-debug (actboy168) for full debugging\\n')\n";

    status = luaL_dostring(L, trace_hook);
    if (status != LUA_OK) {
        const char *msg = lua_tostring(L, -1);
        fprintf(stderr, "tlua: warning: failed to set trace hook: %s\n",
                msg ? msg : "(unknown)");
        lua_pop(L, 1);
    }

    return 0;
}

/* ================================================================ */
/* Transpile + execute                                               */
/* ================================================================ */

static int transpile_and_run(lua_State *L, const char *source,
                             const char *chunk_name) {
    char *lua_code = NULL;
    char error_buf[1024] = {0};
    int status;

    /* Step 1: Transpile .tlua → Lua */
    if (tlua_transpile(source, chunk_name, &lua_code, error_buf, sizeof(error_buf)) != 0) {
        fprintf(stderr, "tlua: transpile error: %s\n", error_buf);
        return 1;
    }

    /* Step 2: Load the transpiled Lua code */
    /* Push the message handler for pcall */
    lua_pushcfunction(L, msghandler);
    int msgh = lua_gettop(L);

    /* Load the chunk */
    status = luaL_loadbuffer(L, lua_code, strlen(lua_code), chunk_name);
    if (status != LUA_OK) {
        const char *msg = lua_tostring(L, -1);
        fprintf(stderr, "tlua: %s\n", msg ? msg : "(unknown load error)");
        free(lua_code);
        return 1;
    }

    /* Step 3: Execute */
    status = lua_pcall(L, 0, LUA_MULTRET, msgh);
    if (status != LUA_OK) {
        const char *msg = lua_tostring(L, -1);
        fprintf(stderr, "%s\n", msg ? msg : "(unknown runtime error)");
        free(lua_code);
        return 1;
    }

    free(lua_code);
    lua_remove(L, msgh); /* remove message handler */
    return 0;
}

/* ================================================================ */
/* Usage / version                                                   */
/* ================================================================ */

static void print_usage(const char *prog) {
    fprintf(stderr,
        "TypingLua Runner v" TLUA_VERSION "\n"
        "Usage: %s [options] <script.tlua> [args...]\n"
        "\n"
        "Options:\n"
        "  -e <code>   Execute a TypingLua string\n"
        "  -d          Enable debug mode (auto-detect debugger)\n"
        "  --debug-host <ip>    Debugger host (default: 127.0.0.1)\n"
        "  --debug-port <port>  Debugger port (default: 12306)\n"
        "  -v          Show version\n"
        "  -h          Show this help\n"
        "\n"
        "Examples:\n"
        "  %s hello.tlua              Run a TypingLua script\n"
        "  %s -e \"print('hi')\"        Run inline code\n"
        "  %s -d hello.tlua           Debug a script (attach debugger first)\n"
        "  %s script.tlua foo bar     Pass arguments to script\n",
        prog, prog, prog, prog, prog);
}

static void print_version(void) {
    printf("tlua " TLUA_VERSION " (TypingLua Runner, Lua 5.4)\n");
}

/* ================================================================ */
/* Main                                                              */
/* ================================================================ */

int main(int argc, char **argv) {
    const char *input_path = NULL;
    const char *eval_code = NULL;
    int script_index = 0;  /* index in argv[] of the script name */
    int i;
    int status;
    lua_State *L;

    /* Parse command line — stop at first non-option (the script name) */
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            if (strcmp(argv[i], "-e") == 0) {
                if (i + 1 >= argc) {
                    fprintf(stderr, "tlua: -e requires an argument\n");
                    return 1;
                }
                eval_code = argv[++i];
                script_index = i + 1;  /* args after -e <code> start here */
                break;
            } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
                debug_mode = 1;
            } else if (strcmp(argv[i], "--debug-host") == 0) {
                if (i + 1 >= argc) {
                    fprintf(stderr, "tlua: --debug-host requires an argument\n");
                    return 1;
                }
                debug_host = argv[++i];
                debug_mode = 1;
            } else if (strcmp(argv[i], "--debug-port") == 0) {
                if (i + 1 >= argc) {
                    fprintf(stderr, "tlua: --debug-port requires an argument\n");
                    return 1;
                }
                debug_port = argv[++i];
                debug_mode = 1;
            } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
                print_version();
                return 0;
            } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                print_usage(argv[0]);
                return 0;
            } else if (strcmp(argv[i], "-") == 0) {
                input_path = "-";
                script_index = i;
                break;
            } else {
                fprintf(stderr, "tlua: unrecognized option '%s'\n", argv[i]);
                print_usage(argv[0]);
                return 1;
            }
        } else {
            /* First non-option = script file */
            input_path = argv[i];
            script_index = i;
            break;
        }
    }

    if (!input_path && !eval_code) {
        fprintf(stderr, "tlua: no input file or -e code specified\n");
        print_usage(argv[0]);
        return 1;
    }

    /* Create Lua state */
    L = luaL_newstate();
    if (L == NULL) {
        fprintf(stderr, "tlua: cannot create Lua state (not enough memory)\n");
        return 1;
    }
    luaL_openlibs(L);

    /* Initialize debugger if -d was specified */
    if (debug_mode) {
        init_debugger(L);
    }

    /* Handle -e mode */
    if (eval_code) {
        /* Build arg table manually for -e mode:
        ** arg[-1] = argv[0] (interpreter)
        ** arg[0]  = "-e"
        ** arg[1]  = first extra arg
        ** arg[2]  = second extra arg
        ** ...
        */
        int extra_start = script_index;  /* index of first arg after -e <code> */
        lua_newtable(L);
        lua_pushstring(L, argv[0]);
        lua_rawseti(L, -2, -1);
        lua_pushstring(L, "-e");
        lua_rawseti(L, -2, 0);
        for (i = extra_start; i < argc; i++) {
            lua_pushstring(L, argv[i]);
            lua_rawseti(L, -2, i - extra_start + 1);
        }
        lua_setglobal(L, "arg");

        status = transpile_and_run(L, eval_code, "=(command line)");
        lua_close(L);
        return status;
    }

    /* Handle file mode */
    /* Set up arg table */
    create_arg_table(L, argc, argv, script_index);

    /* Read source */
    long source_size = 0;
    char *source = read_file_contents(input_path, &source_size);
    if (!source) {
        fprintf(stderr, "tlua: cannot open '%s': No such file or directory\n",
                strcmp(input_path, "-") == 0 ? "<stdin>" : input_path);
        lua_close(L);
        return 1;
    }

    /* Build chunk name: @<absolute_path> (Lua convention for file-loaded chunks)
    **
    ** lua-debug (actboy168) matches breakpoints by comparing the chunk's
    ** source name (debug.getinfo().source) against the file path where
    ** the breakpoint was set in VSCode.
    **
    ** We must use the ABSOLUTE path so it matches VSCode's file URI.
    ** We also normalize backslashes to forward slashes for consistency.
    */
    const char *display_name;
    char *chunk_name = NULL;
    if (strcmp(input_path, "-") == 0) {
        display_name = "=stdin";
    } else {
        char abs_path[1024];
#ifdef _WIN32
        DWORD len = GetFullPathNameA(input_path, sizeof(abs_path), abs_path, NULL);
        if (len == 0 || len >= sizeof(abs_path)) {
            /* Fallback to original path if GetFullPathName fails */
            strncpy(abs_path, input_path, sizeof(abs_path) - 1);
            abs_path[sizeof(abs_path) - 1] = '\0';
        }
        /* Keep native backslashes on Windows — lua-debug (actboy168)
        ** matches breakpoint paths using the OS-native separator.
        ** VSCode sends paths like F:\...\file.tlua on Windows,
        ** so the chunk name must use backslashes too. */
#else
        if (realpath(input_path, abs_path) == NULL) {
            strncpy(abs_path, input_path, sizeof(abs_path) - 1);
            abs_path[sizeof(abs_path) - 1] = '\0';
        }
#endif
        chunk_name = (char *)malloc(strlen(abs_path) + 2);
        chunk_name[0] = '@';
        strcpy(chunk_name + 1, abs_path);
        display_name = chunk_name;
    }

    status = transpile_and_run(L, source, display_name);

    /* Cleanup */
    free(chunk_name);
    free(source);
    lua_close(L);
    return status;
}
