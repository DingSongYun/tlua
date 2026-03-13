/*
** tlua_main.c — TypingLua Transpiler CLI Entry Point
**
** Usage:
**   tluac [options] input.tlua [output.lua]
**
** Options:
**   -o <file>   Specify output file (default: replace .tlua with .lua)
**   -p          Print to stdout instead of writing file
**   -v          Show version information
**   -h          Show help
**
** If no output file is specified:
**   input.tlua  ->  input.lua  (replaces extension)
**   input.xyz   ->  input.xyz.lua  (appends .lua)
**   -           ->  reads stdin, writes stdout
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tlua_parser.h"

#define TLUAC_VERSION "0.1.0"

/* ================================================================ */
/* Utility: file I/O                                                 */
/* ================================================================ */

/* Read entire file into a malloc'd null-terminated string.
   Returns NULL on failure. */
static char *read_file(const char *path, long *out_size) {
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

/* Write string to file. Returns 0 on success. */
static int write_file(const char *path, const char *data) {
    FILE *f;
    size_t len;

    if (strcmp(path, "-") == 0) {
        fputs(data, stdout);
        return 0;
    }

    f = fopen(path, "wb");
    if (!f) return 1;

    len = strlen(data);
    if (len > 0) {
        fwrite(data, 1, len, f);
    }
    fclose(f);
    return 0;
}

/* ================================================================ */
/* Utility: path manipulation                                        */
/* ================================================================ */

/* Generate default output path:
   "foo.tlua" -> "foo.lua"
   "foo.bar"  -> "foo.bar.lua" */
static char *make_output_path(const char *input_path) {
    size_t len = strlen(input_path);
    const char *ext;
    char *out;

    /* Find last '.' */
    ext = strrchr(input_path, '.');

    /* Also check we don't pick up a dot in the directory part */
    {
        const char *sep1 = strrchr(input_path, '/');
        const char *sep2 = strrchr(input_path, '\\');
        const char *sep = sep1 > sep2 ? sep1 : sep2;
        if (ext && sep && ext < sep) {
            ext = NULL; /* dot is in directory, not filename */
        }
    }

    if (ext && (strcmp(ext, ".tlua") == 0 || strcmp(ext, ".TLUA") == 0)) {
        /* Replace .tlua with .lua */
        size_t base_len = (size_t)(ext - input_path);
        out = (char *)malloc(base_len + 5); /* .lua\0 */
        memcpy(out, input_path, base_len);
        memcpy(out + base_len, ".lua", 4);
        out[base_len + 4] = '\0';
    } else {
        /* Append .lua */
        out = (char *)malloc(len + 5);
        memcpy(out, input_path, len);
        memcpy(out + len, ".lua", 4);
        out[len + 4] = '\0';
    }

    return out;
}

/* ================================================================ */
/* Usage / version                                                   */
/* ================================================================ */

static void print_usage(const char *prog) {
    fprintf(stderr,
        "TypingLua Transpiler v" TLUAC_VERSION "\n"
        "Usage: %s [options] <input.tlua> [output.lua]\n"
        "\n"
        "Options:\n"
        "  -o <file>   Specify output file\n"
        "  -p          Print transpiled output to stdout\n"
        "  -v          Show version\n"
        "  -h          Show this help\n"
        "\n"
        "If no output path is given:\n"
        "  input.tlua  ->  input.lua\n"
        "  -           ->  reads stdin, writes stdout\n",
        prog);
}

static void print_version(void) {
    printf("tluac " TLUAC_VERSION " (TypingLua Transpiler)\n");
}

/* ================================================================ */
/* Main                                                              */
/* ================================================================ */

int main(int argc, char **argv) {
    const char *input_path = NULL;
    const char *output_path = NULL;
    char *output_path_alloc = NULL;  /* if we generated the output path */
    int print_stdout = 0;
    int i;

    /* Parse command line */
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            if (strcmp(argv[i], "-o") == 0) {
                if (i + 1 >= argc) {
                    fprintf(stderr, "Error: -o requires an argument\n");
                    return 1;
                }
                output_path = argv[++i];
            } else if (strcmp(argv[i], "-p") == 0) {
                print_stdout = 1;
            } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
                print_version();
                return 0;
            } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                print_usage(argv[0]);
                return 0;
            } else if (strcmp(argv[i], "-") == 0) {
                input_path = "-"; /* stdin */
            } else {
                fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
                print_usage(argv[0]);
                return 1;
            }
        } else {
            /* Positional argument */
            if (input_path == NULL) {
                input_path = argv[i];
            } else if (output_path == NULL) {
                output_path = argv[i];
            } else {
                fprintf(stderr, "Error: Too many arguments\n");
                print_usage(argv[0]);
                return 1;
            }
        }
    }

    if (!input_path) {
        fprintf(stderr, "Error: No input file specified\n");
        print_usage(argv[0]);
        return 1;
    }

    /* Determine output path */
    if (print_stdout) {
        output_path = "-";
    } else if (!output_path) {
        if (strcmp(input_path, "-") == 0) {
            output_path = "-";
        } else {
            output_path_alloc = make_output_path(input_path);
            output_path = output_path_alloc;
        }
    }

    /* Read input */
    long source_size = 0;
    char *source = read_file(input_path, &source_size);
    if (!source) {
        fprintf(stderr, "Error: Cannot read '%s'\n",
                strcmp(input_path, "-") == 0 ? "<stdin>" : input_path);
        free(output_path_alloc);
        return 1;
    }

    /* Transpile */
    char *lua_output = NULL;
    char error_buf[1024] = {0};
    const char *display_name = (strcmp(input_path, "-") == 0) ? "<stdin>" : input_path;

    int result = tlua_transpile(source, display_name, &lua_output, error_buf, sizeof(error_buf));

    if (result != 0) {
        fprintf(stderr, "Transpile error in %s: %s\n", display_name, error_buf);
        free(source);
        free(output_path_alloc);
        return 1;
    }

    /* Write output */
    if (write_file(output_path, lua_output) != 0) {
        fprintf(stderr, "Error: Cannot write '%s'\n",
                strcmp(output_path, "-") == 0 ? "<stdout>" : output_path);
        free(lua_output);
        free(source);
        free(output_path_alloc);
        return 1;
    }

    /* Report success (only if writing to file, not stdout) */
    if (strcmp(output_path, "-") != 0) {
        fprintf(stderr, "%s -> %s\n", display_name, output_path);
    }

    /* Cleanup */
    free(lua_output);
    free(source);
    free(output_path_alloc);
    return 0;
}
