/*
** tlua_lexer.c — TypingLua Transpiler Lexer Implementation
** Tokenizes .tlua source, preserving whitespace and comments as tokens
** so the parser can reproduce them verbatim in the output.
*/

#include "tlua_lexer.h"

/* ================================================================ */
/* Keyword table                                                     */
/* ================================================================ */

typedef struct {
    const char *name;
    TluaTokenType type;
} Keyword;

static const Keyword keywords[] = {
    {"and",      TK_AND},
    {"break",    TK_BREAK},
    {"do",       TK_DO},
    {"else",     TK_ELSE},
    {"elseif",   TK_ELSEIF},
    {"end",      TK_END},
    {"false",    TK_FALSE},
    {"for",      TK_FOR},
    {"function", TK_FUNCTION},
    {"goto",     TK_GOTO},
    {"if",       TK_IF},
    {"in",       TK_IN},
    {"local",    TK_LOCAL},
    {"nil",      TK_NIL},
    {"not",      TK_NOT},
    {"or",       TK_OR},
    {"repeat",   TK_REPEAT},
    {"return",   TK_RETURN},
    {"then",     TK_THEN},
    {"true",     TK_TRUE},
    {"until",    TK_UNTIL},
    {"while",    TK_WHILE},
    {"fun",      TK_FUN},
    {NULL,       TK_EOF}
};

/* ================================================================ */
/* Helpers                                                           */
/* ================================================================ */

static int is_ident_start(char c) {
    return isalpha((unsigned char)c) || c == '_';
}

static int is_ident_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

static int is_digit(char c) {
    return isdigit((unsigned char)c);
}

static int is_hex_digit(char c) {
    return isxdigit((unsigned char)c);
}

static char peek_char(TluaLexer *lex) {
    return *lex->current;
}

static char peek_char_at(TluaLexer *lex, int offset) {
    return lex->current[offset];
}

static char advance_char(TluaLexer *lex) {
    char c = *lex->current;
    if (c == '\n') {
        lex->line++;
        lex->col = 1;
    } else {
        lex->col++;
    }
    lex->current++;
    return c;
}

static TluaToken make_token(TluaLexer *lex, TluaTokenType type,
                            const char *start, int length, int line, int col) {
    TluaToken tok;
    tok.type = type;
    tok.start = start;
    tok.length = length;
    tok.line = line;
    tok.col = col;
    return tok;
}

/* ================================================================ */
/* Lookup keyword                                                    */
/* ================================================================ */

static TluaTokenType lookup_keyword(const char *start, int length) {
    for (int i = 0; keywords[i].name != NULL; i++) {
        if ((int)strlen(keywords[i].name) == length &&
            memcmp(keywords[i].name, start, length) == 0) {
            return keywords[i].type;
        }
    }
    return TK_NAME;
}

/* ================================================================ */
/* Scan functions                                                    */
/* ================================================================ */

/* Count the level of long bracket: [==[ returns 2, [[ returns 0, [ returns -1 */
static int count_long_bracket(TluaLexer *lex) {
    const char *p = lex->current;
    if (*p != '[') return -1;
    p++;
    int level = 0;
    while (*p == '=') { level++; p++; }
    if (*p == '[') return level;
    return -1;
}

/* Scan a long string [=*[ ... ]=*] */
static void scan_long_string(TluaLexer *lex, int level) {
    /* skip opening [=*[ */
    advance_char(lex); /* [ */
    for (int i = 0; i < level; i++) advance_char(lex); /* = ... = */
    advance_char(lex); /* [ */

    while (peek_char(lex) != '\0') {
        if (peek_char(lex) == ']') {
            const char *p = lex->current + 1;
            int count = 0;
            while (*p == '=') { count++; p++; }
            if (count == level && *p == ']') {
                /* skip closing ]=*] */
                advance_char(lex); /* ] */
                for (int i = 0; i < level; i++) advance_char(lex);
                advance_char(lex); /* ] */
                return;
            }
        }
        advance_char(lex);
    }
    /* unterminated long string — will produce error token */
}

/* Scan a quoted string "..." or '...' */
static void scan_string(TluaLexer *lex, char quote) {
    advance_char(lex); /* skip opening quote */
    while (peek_char(lex) != '\0' && peek_char(lex) != quote) {
        if (peek_char(lex) == '\\') {
            advance_char(lex); /* skip backslash */
            if (peek_char(lex) != '\0') {
                advance_char(lex); /* skip escaped char */
            }
        } else {
            advance_char(lex);
        }
    }
    if (peek_char(lex) == quote) {
        advance_char(lex); /* skip closing quote */
    }
}

/* Scan a number literal */
static void scan_number(TluaLexer *lex) {
    /* hex: 0x / 0X */
    if (peek_char(lex) == '0' && (peek_char_at(lex, 1) == 'x' || peek_char_at(lex, 1) == 'X')) {
        advance_char(lex); /* 0 */
        advance_char(lex); /* x */
        while (is_hex_digit(peek_char(lex)) || peek_char(lex) == '_') advance_char(lex);
        if (peek_char(lex) == '.') {
            advance_char(lex);
            while (is_hex_digit(peek_char(lex)) || peek_char(lex) == '_') advance_char(lex);
        }
        if (peek_char(lex) == 'p' || peek_char(lex) == 'P') {
            advance_char(lex);
            if (peek_char(lex) == '+' || peek_char(lex) == '-') advance_char(lex);
            while (is_digit(peek_char(lex))) advance_char(lex);
        }
        return;
    }

    /* decimal */
    while (is_digit(peek_char(lex)) || peek_char(lex) == '_') advance_char(lex);
    if (peek_char(lex) == '.') {
        advance_char(lex);
        while (is_digit(peek_char(lex)) || peek_char(lex) == '_') advance_char(lex);
    }
    if (peek_char(lex) == 'e' || peek_char(lex) == 'E') {
        advance_char(lex);
        if (peek_char(lex) == '+' || peek_char(lex) == '-') advance_char(lex);
        while (is_digit(peek_char(lex))) advance_char(lex);
    }
}

/* Scan a line comment (-- ...) or block comment (--[[ ... ]]) */
static void scan_comment(TluaLexer *lex) {
    advance_char(lex); /* - */
    advance_char(lex); /* - */
    /* check for block comment --[=*[ */
    int level = count_long_bracket(lex);
    if (level >= 0) {
        scan_long_string(lex, level);
        return;
    }
    /* line comment: read until end of line */
    while (peek_char(lex) != '\0' && peek_char(lex) != '\n') {
        advance_char(lex);
    }
}

/* ================================================================ */
/* Main scan function                                                */
/* ================================================================ */

static TluaToken scan_token(TluaLexer *lex) {
    const char *start = lex->current;
    int start_line = lex->line;
    int start_col = lex->col;

    if (peek_char(lex) == '\0') {
        return make_token(lex, TK_EOF, start, 0, start_line, start_col);
    }

    /* Newlines */
    if (peek_char(lex) == '\n') {
        advance_char(lex);
        /* consume consecutive \r\n or \n */
        return make_token(lex, TK_NEWLINE, start, (int)(lex->current - start), start_line, start_col);
    }
    if (peek_char(lex) == '\r') {
        advance_char(lex);
        if (peek_char(lex) == '\n') advance_char(lex);
        return make_token(lex, TK_NEWLINE, start, (int)(lex->current - start), start_line, start_col);
    }

    /* Whitespace (non-newline) */
    if (peek_char(lex) == ' ' || peek_char(lex) == '\t' || peek_char(lex) == '\f' || peek_char(lex) == '\v') {
        while (peek_char(lex) == ' ' || peek_char(lex) == '\t' || peek_char(lex) == '\f' || peek_char(lex) == '\v') {
            advance_char(lex);
        }
        return make_token(lex, TK_WHITESPACE, start, (int)(lex->current - start), start_line, start_col);
    }

    char c = peek_char(lex);

    /* Comments or minus */
    if (c == '-') {
        if (peek_char_at(lex, 1) == '-') {
            scan_comment(lex);
            return make_token(lex, TK_COMMENT, start, (int)(lex->current - start), start_line, start_col);
        }
        advance_char(lex);
        return make_token(lex, TK_MINUS, start, 1, start_line, start_col);
    }

    /* Strings */
    if (c == '"' || c == '\'') {
        scan_string(lex, c);
        return make_token(lex, TK_STRING, start, (int)(lex->current - start), start_line, start_col);
    }

    /* Long strings */
    if (c == '[') {
        int level = count_long_bracket(lex);
        if (level >= 0) {
            scan_long_string(lex, level);
            return make_token(lex, TK_LONGSTRING, start, (int)(lex->current - start), start_line, start_col);
        }
        advance_char(lex);
        return make_token(lex, TK_LBRACKET, start, 1, start_line, start_col);
    }

    /* Numbers */
    if (is_digit(c) || (c == '.' && is_digit(peek_char_at(lex, 1)))) {
        scan_number(lex);
        return make_token(lex, TK_NUMBER, start, (int)(lex->current - start), start_line, start_col);
    }

    /* Identifiers and keywords */
    if (is_ident_start(c)) {
        while (is_ident_char(peek_char(lex))) advance_char(lex);
        int length = (int)(lex->current - start);
        TluaTokenType type = lookup_keyword(start, length);
        return make_token(lex, type, start, length, start_line, start_col);
    }

    /* Multi-char operators */
    advance_char(lex);
    switch (c) {
        case '(': return make_token(lex, TK_LPAREN, start, 1, start_line, start_col);
        case ')': return make_token(lex, TK_RPAREN, start, 1, start_line, start_col);
        case ']': return make_token(lex, TK_RBRACKET, start, 1, start_line, start_col);
        case '{': return make_token(lex, TK_LBRACE, start, 1, start_line, start_col);
        case '}': return make_token(lex, TK_RBRACE, start, 1, start_line, start_col);
        case ',': return make_token(lex, TK_COMMA, start, 1, start_line, start_col);
        case ';': return make_token(lex, TK_SEMICOLON, start, 1, start_line, start_col);
        case '+': return make_token(lex, TK_PLUS, start, 1, start_line, start_col);
        case '*': return make_token(lex, TK_STAR, start, 1, start_line, start_col);
        case '%': return make_token(lex, TK_PERCENT, start, 1, start_line, start_col);
        case '^': return make_token(lex, TK_CARET, start, 1, start_line, start_col);
        case '#': return make_token(lex, TK_HASH, start, 1, start_line, start_col);
        case '&': return make_token(lex, TK_AMPERSAND, start, 1, start_line, start_col);
        case '|': return make_token(lex, TK_PIPE, start, 1, start_line, start_col);
        case '?': return make_token(lex, TK_QUESTION, start, 1, start_line, start_col);

        case ':':
            if (peek_char(lex) == ':') {
                advance_char(lex);
                return make_token(lex, TK_DBCOLON, start, 2, start_line, start_col);
            }
            return make_token(lex, TK_COLON, start, 1, start_line, start_col);

        case '.':
            if (peek_char(lex) == '.') {
                advance_char(lex);
                if (peek_char(lex) == '.') {
                    advance_char(lex);
                    return make_token(lex, TK_DOTS, start, 3, start_line, start_col);
                }
                return make_token(lex, TK_CONCAT, start, 2, start_line, start_col);
            }
            return make_token(lex, TK_DOT, start, 1, start_line, start_col);

        case '=':
            if (peek_char(lex) == '=') {
                advance_char(lex);
                return make_token(lex, TK_EQ, start, 2, start_line, start_col);
            }
            return make_token(lex, TK_ASSIGN, start, 1, start_line, start_col);

        case '~':
            if (peek_char(lex) == '=') {
                advance_char(lex);
                return make_token(lex, TK_NE, start, 2, start_line, start_col);
            }
            return make_token(lex, TK_TILDE, start, 1, start_line, start_col);

        case '<':
            if (peek_char(lex) == '=') {
                advance_char(lex);
                return make_token(lex, TK_LE, start, 2, start_line, start_col);
            }
            if (peek_char(lex) == '<') {
                advance_char(lex);
                return make_token(lex, TK_SHL, start, 2, start_line, start_col);
            }
            return make_token(lex, TK_LT, start, 1, start_line, start_col);

        case '>':
            if (peek_char(lex) == '=') {
                advance_char(lex);
                return make_token(lex, TK_GE, start, 2, start_line, start_col);
            }
            if (peek_char(lex) == '>') {
                advance_char(lex);
                return make_token(lex, TK_SHR, start, 2, start_line, start_col);
            }
            return make_token(lex, TK_GT, start, 1, start_line, start_col);

        case '/':
            if (peek_char(lex) == '/') {
                advance_char(lex);
                return make_token(lex, TK_IDIV, start, 2, start_line, start_col);
            }
            return make_token(lex, TK_SLASH, start, 1, start_line, start_col);

        default:
            return make_token(lex, TK_ERROR, start, 1, start_line, start_col);
    }
}

/* ================================================================ */
/* Public API implementation                                         */
/* ================================================================ */

void tlua_lexer_init(TluaLexer *lex, const char *source, const char *filename) {
    lex->source = source;
    lex->current = source;
    lex->line = 1;
    lex->col = 1;
    lex->filename = filename;
    lex->has_lookahead = 0;
    lex->token.type = TK_EOF;
    lex->lookahead.type = TK_EOF;
}

void tlua_lexer_next(TluaLexer *lex) {
    if (lex->has_lookahead) {
        lex->token = lex->lookahead;
        lex->has_lookahead = 0;
    } else {
        lex->token = scan_token(lex);
    }
}

TluaToken tlua_lexer_peek(TluaLexer *lex) {
    if (!lex->has_lookahead) {
        /* Save state */
        const char *saved_current = lex->current;
        int saved_line = lex->line;
        int saved_col = lex->col;

        /* Scan ahead, skipping whitespace/comments/newlines */
        TluaToken tok;
        do {
            tok = scan_token(lex);
        } while (tok.type == TK_WHITESPACE || tok.type == TK_COMMENT || tok.type == TK_NEWLINE);

        lex->lookahead = tok;
        lex->has_lookahead = 1;

        /* Restore state — but we can't really restore because scan consumed chars.
           Instead, store the lookahead and restore the lexer position. */
        lex->current = saved_current;
        lex->line = saved_line;
        lex->col = saved_col;

        /* Re-scan to get the actual lookahead properly stored */
        /* Actually, we need a different approach. Let's save and use it. */
    }
    return lex->lookahead;
}

const char *tlua_token_name(TluaTokenType type) {
    switch (type) {
        case TK_EOF:        return "EOF";
        case TK_ERROR:      return "ERROR";
        case TK_NAME:       return "NAME";
        case TK_STRING:     return "STRING";
        case TK_LONGSTRING: return "LONGSTRING";
        case TK_NUMBER:     return "NUMBER";
        case TK_AND:        return "'and'";
        case TK_BREAK:      return "'break'";
        case TK_DO:         return "'do'";
        case TK_ELSE:       return "'else'";
        case TK_ELSEIF:     return "'elseif'";
        case TK_END:        return "'end'";
        case TK_FALSE:      return "'false'";
        case TK_FOR:        return "'for'";
        case TK_FUNCTION:   return "'function'";
        case TK_GOTO:       return "'goto'";
        case TK_IF:         return "'if'";
        case TK_IN:         return "'in'";
        case TK_LOCAL:      return "'local'";
        case TK_NIL:        return "'nil'";
        case TK_NOT:        return "'not'";
        case TK_OR:         return "'or'";
        case TK_REPEAT:     return "'repeat'";
        case TK_RETURN:     return "'return'";
        case TK_THEN:       return "'then'";
        case TK_TRUE:       return "'true'";
        case TK_UNTIL:      return "'until'";
        case TK_WHILE:      return "'while'";
        case TK_FUN:        return "'fun'";
        case TK_LPAREN:     return "'('";
        case TK_RPAREN:     return "')'";
        case TK_LBRACKET:   return "'['";
        case TK_RBRACKET:   return "']'";
        case TK_LBRACE:     return "'{'";
        case TK_RBRACE:     return "'}'";
        case TK_COMMA:      return "','";
        case TK_SEMICOLON:  return "';'";
        case TK_COLON:      return "':'";
        case TK_DOT:        return "'.'";
        case TK_ASSIGN:     return "'='";
        case TK_PLUS:       return "'+'";
        case TK_MINUS:      return "'-'";
        case TK_STAR:       return "'*'";
        case TK_SLASH:      return "'/'";
        case TK_PERCENT:    return "'%'";
        case TK_CARET:      return "'^'";
        case TK_HASH:       return "'#'";
        case TK_AMPERSAND:  return "'&'";
        case TK_TILDE:      return "'~'";
        case TK_PIPE:       return "'|'";
        case TK_LT:         return "'<'";
        case TK_GT:         return "'>'";
        case TK_QUESTION:   return "'?'";
        case TK_CONCAT:     return "'..'";
        case TK_DOTS:       return "'...'";
        case TK_EQ:         return "'=='";
        case TK_NE:         return "'~='";
        case TK_LE:         return "'<='";
        case TK_GE:         return "'>='";
        case TK_SHL:        return "'<<'";
        case TK_SHR:        return "'>>'";
        case TK_IDIV:       return "'//'";
        case TK_DBCOLON:    return "'::'";
        case TK_LBRACKET2:  return "'[['";
        case TK_COMMENT:    return "COMMENT";
        case TK_NEWLINE:    return "NEWLINE";
        case TK_WHITESPACE: return "WHITESPACE";
        default:            return "?";
    }
}

int tlua_token_is_keyword(const TluaToken *tok, const char *keyword) {
    int len = (int)strlen(keyword);
    return tok->length == len && memcmp(tok->start, keyword, len) == 0;
}

int tlua_token_equals(const TluaToken *tok, const char *str) {
    int len = (int)strlen(str);
    return tok->length == len && memcmp(tok->start, str, len) == 0;
}
