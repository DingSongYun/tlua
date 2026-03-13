/*
** tlua_parser.h — TypingLua Transpiler Parser / Code Generator
** Parses .tlua source and performs Type Erasure, producing standard Lua output.
**
** Design: Stream-based transpilation.
** The parser reads tokens sequentially. When it detects a type annotation
** injection point (after local/global var names, after function params,
** after ')' for return types), it skips the type tokens. All other tokens
** (including comments, whitespace, newlines) are emitted verbatim.
*/

#ifndef TLUA_PARSER_H
#define TLUA_PARSER_H

#include "tlua_lexer.h"

/* ================================================================ */
/* Output buffer                                                     */
/* ================================================================ */

typedef struct {
    char *data;
    int length;
    int capacity;
} TluaBuffer;

/* ================================================================ */
/* Parser state                                                      */
/* ================================================================ */

typedef struct {
    TluaLexer lexer;
    TluaBuffer output;
    const char *filename;
    int error_count;
    char error_msg[1024];    /* last error message */
} TluaParser;

/* ================================================================ */
/* Public API                                                        */
/* ================================================================ */

/*
** Transpile a .tlua source string to Lua.
** Returns 0 on success, non-zero on error.
** On success, *out_lua receives a malloc'd string (caller must free).
** On error, *out_lua is NULL and error info is in parser.error_msg.
*/
int tlua_transpile(const char *source, const char *filename,
                   char **out_lua, char *error_buf, int error_buf_size);

#endif /* TLUA_PARSER_H */
