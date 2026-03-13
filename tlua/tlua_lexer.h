/*
** tlua_lexer.h — TypingLua Transpiler Lexer
** Tokenizes .tlua source files, recognizing both standard Lua 5.4
** tokens and type-annotation-specific tokens.
*/

#ifndef TLUA_LEXER_H
#define TLUA_LEXER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ================================================================ */
/* Token types                                                       */
/* ================================================================ */

typedef enum {
    /* End / Error */
    TK_EOF = 0,
    TK_ERROR,

    /* Literals & identifiers */
    TK_NAME,           /* identifier */
    TK_STRING,         /* quoted string literal */
    TK_LONGSTRING,     /* [[ ... ]] long string */
    TK_NUMBER,         /* numeric literal */

    /* Keywords — Lua 5.4 */
    TK_AND, TK_BREAK, TK_DO, TK_ELSE, TK_ELSEIF, TK_END,
    TK_FALSE, TK_FOR, TK_FUNCTION, TK_GOTO, TK_IF, TK_IN,
    TK_LOCAL, TK_NIL, TK_NOT, TK_OR, TK_REPEAT, TK_RETURN,
    TK_THEN, TK_TRUE, TK_UNTIL, TK_WHILE,

    /* Type keyword (only in type context) */
    TK_FUN,            /* 'fun' keyword for function types */

    /* Single-char tokens (stored as their ASCII value internally,
       but we give them named constants for clarity) */
    TK_LPAREN,         /* ( */
    TK_RPAREN,         /* ) */
    TK_LBRACKET,       /* [ */
    TK_RBRACKET,       /* ] */
    TK_LBRACE,         /* { */
    TK_RBRACE,         /* } */
    TK_COMMA,          /* , */
    TK_SEMICOLON,      /* ; */
    TK_COLON,          /* : */
    TK_DOT,            /* . */
    TK_ASSIGN,         /* = */
    TK_PLUS,           /* + */
    TK_MINUS,          /* - */
    TK_STAR,           /* * */
    TK_SLASH,          /* / */
    TK_PERCENT,        /* % */
    TK_CARET,          /* ^ */
    TK_HASH,           /* # */
    TK_AMPERSAND,      /* & */
    TK_TILDE,          /* ~ */
    TK_PIPE,           /* | */
    TK_LT,             /* < */
    TK_GT,             /* > */
    TK_QUESTION,       /* ? */

    /* Multi-char tokens */
    TK_CONCAT,         /* .. */
    TK_DOTS,           /* ... */
    TK_EQ,             /* == */
    TK_NE,             /* ~= */
    TK_LE,             /* <= */
    TK_GE,             /* >= */
    TK_SHL,            /* << */
    TK_SHR,            /* >> */
    TK_IDIV,           /* // */
    TK_DBCOLON,        /* :: (label) */
    TK_LBRACKET2,      /* [[ (long string open — handled specially) */

    /* Comment (preserved in output) */
    TK_COMMENT,        /* -- line comment or --[[ block comment ]] */
    TK_NEWLINE,        /* newline(s) — significant for type annotation termination */
    TK_WHITESPACE,     /* spaces/tabs (non-newline) */

    TK_COUNT           /* total number of token types */
} TluaTokenType;

/* ================================================================ */
/* Token structure                                                   */
/* ================================================================ */

typedef struct {
    TluaTokenType type;
    const char *start;   /* pointer into source buffer */
    int length;          /* length of token text */
    int line;            /* 1-based line number */
    int col;             /* 1-based column number */
} TluaToken;

/* ================================================================ */
/* Lexer state                                                       */
/* ================================================================ */

typedef struct {
    const char *source;       /* source buffer (null-terminated) */
    const char *current;      /* current read position */
    int line;                 /* current line (1-based) */
    int col;                  /* current column (1-based) */
    const char *filename;     /* source filename for diagnostics */
    TluaToken token;          /* current token */
    TluaToken lookahead;      /* one-token lookahead (type == TK_EOF if unused) */
    int has_lookahead;        /* 1 if lookahead is valid */
} TluaLexer;

/* ================================================================ */
/* Public API                                                        */
/* ================================================================ */

/* Initialize lexer with source buffer */
void tlua_lexer_init(TluaLexer *lex, const char *source, const char *filename);

/* Advance to next token (skipping nothing — whitespace/comments are tokens) */
void tlua_lexer_next(TluaLexer *lex);

/* Peek at the next non-whitespace/non-comment token without consuming */
TluaToken tlua_lexer_peek(TluaLexer *lex);

/* Get human-readable name for a token type */
const char *tlua_token_name(TluaTokenType type);

/* Check if a TK_NAME token matches a specific keyword string */
int tlua_token_is_keyword(const TluaToken *tok, const char *keyword);

/* Check if token text equals a given string */
int tlua_token_equals(const TluaToken *tok, const char *str);

#endif /* TLUA_LEXER_H */
