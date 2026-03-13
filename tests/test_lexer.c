/*
** test_lexer.c — Unit tests for tlua_lexer
** Covers: keywords, identifiers, numbers, strings, long strings,
**         operators, comments, whitespace/newlines, edge cases,
**         column tracking, peek, and error recovery.
*/

#include "tlua_test.h"
#include "tlua_lexer.h"

/* ================================================================ */
/* Helper: lex all tokens into an array and return count             */
/* ================================================================ */
#define MAX_TOKENS 256

typedef struct {
    TluaToken tokens[MAX_TOKENS];
    int count;
} TokenList;

static TokenList lex_all(const char *source) {
    TokenList list;
    list.count = 0;
    TluaLexer lex;
    tlua_lexer_init(&lex, source, "test");
    for (;;) {
        tlua_lexer_next(&lex);
        if (lex.token.type == TK_EOF) break;
        if (list.count < MAX_TOKENS) {
            list.tokens[list.count++] = lex.token;
        }
    }
    return list;
}

/* Helper: lex all non-whitespace/non-newline tokens */
static TokenList lex_significant(const char *source) {
    TokenList list;
    list.count = 0;
    TluaLexer lex;
    tlua_lexer_init(&lex, source, "test");
    for (;;) {
        tlua_lexer_next(&lex);
        if (lex.token.type == TK_EOF) break;
        if (lex.token.type != TK_WHITESPACE && lex.token.type != TK_NEWLINE) {
            if (list.count < MAX_TOKENS) {
                list.tokens[list.count++] = lex.token;
            }
        }
    }
    return list;
}

/* Helper: extract token text into a buffer */
static char tokbuf[512];
static const char *tok_text(const TluaToken *t) {
    int len = t->length < 511 ? t->length : 511;
    memcpy(tokbuf, t->start, len);
    tokbuf[len] = '\0';
    return tokbuf;
}

/* ================================================================ */
/* 2.1  All Lua 5.4 keywords                                        */
/* ================================================================ */

TEST(keywords_lua54) {
    const char *src =
        "and break do else elseif end false for function goto "
        "if in local nil not or repeat return then true until while";
    TokenList tl = lex_significant(src);

    TluaTokenType expected[] = {
        TK_AND, TK_BREAK, TK_DO, TK_ELSE, TK_ELSEIF, TK_END,
        TK_FALSE, TK_FOR, TK_FUNCTION, TK_GOTO,
        TK_IF, TK_IN, TK_LOCAL, TK_NIL, TK_NOT, TK_OR,
        TK_REPEAT, TK_RETURN, TK_THEN, TK_TRUE, TK_UNTIL, TK_WHILE
    };
    int n = sizeof(expected) / sizeof(expected[0]);
    ASSERT_EQ(tl.count, n);
    for (int i = 0; i < n; i++) {
        ASSERT_EQ(tl.tokens[i].type, expected[i]);
    }
}

/* ================================================================ */
/* 2.2  TK_FUN keyword                                              */
/* ================================================================ */

TEST(keyword_fun) {
    TokenList tl = lex_significant("fun");
    ASSERT_EQ(tl.count, 1);
    ASSERT_EQ(tl.tokens[0].type, TK_FUN);
}

TEST(fun_not_in_identifier) {
    /* "funny" should be TK_NAME, not TK_FUN */
    TokenList tl = lex_significant("funny");
    ASSERT_EQ(tl.count, 1);
    ASSERT_EQ(tl.tokens[0].type, TK_NAME);
}

/* ================================================================ */
/* 2.3  Identifiers                                                  */
/* ================================================================ */

TEST(identifiers_basic) {
    TokenList tl = lex_significant("foo _bar _123 a1b2c3");
    ASSERT_EQ(tl.count, 4);
    for (int i = 0; i < 4; i++) {
        ASSERT_EQ(tl.tokens[i].type, TK_NAME);
    }
    ASSERT_STR_EQ(tok_text(&tl.tokens[0]), "foo");
    ASSERT_STR_EQ(tok_text(&tl.tokens[1]), "_bar");
    ASSERT_STR_EQ(tok_text(&tl.tokens[2]), "_123");
    ASSERT_STR_EQ(tok_text(&tl.tokens[3]), "a1b2c3");
}

/* ================================================================ */
/* 2.4  Numeric literals                                             */
/* ================================================================ */

TEST(numbers_integer) {
    TokenList tl = lex_significant("42 0 999");
    ASSERT_EQ(tl.count, 3);
    for (int i = 0; i < 3; i++) {
        ASSERT_EQ(tl.tokens[i].type, TK_NUMBER);
    }
}

TEST(numbers_float) {
    TokenList tl = lex_significant("3.14 .5 1e10 2.5e-3");
    ASSERT_EQ(tl.count, 4);
    for (int i = 0; i < 4; i++) {
        ASSERT_EQ(tl.tokens[i].type, TK_NUMBER);
    }
}

TEST(numbers_hex) {
    TokenList tl = lex_significant("0xFF 0x1A3F");
    ASSERT_EQ(tl.count, 2);
    for (int i = 0; i < 2; i++) {
        ASSERT_EQ(tl.tokens[i].type, TK_NUMBER);
    }
}

/* ================================================================ */
/* 2.5  String literals                                              */
/* ================================================================ */

TEST(string_double_quotes) {
    TokenList tl = lex_significant("\"hello world\"");
    ASSERT_EQ(tl.count, 1);
    ASSERT_EQ(tl.tokens[0].type, TK_STRING);
}

TEST(string_single_quotes) {
    TokenList tl = lex_significant("'hello'");
    ASSERT_EQ(tl.count, 1);
    ASSERT_EQ(tl.tokens[0].type, TK_STRING);
}

TEST(string_escape_sequences) {
    TokenList tl = lex_significant("\"line\\nnext\"");
    ASSERT_EQ(tl.count, 1);
    ASSERT_EQ(tl.tokens[0].type, TK_STRING);
}

/* ================================================================ */
/* 2.6  Long strings                                                 */
/* ================================================================ */

TEST(long_string_basic) {
    TokenList tl = lex_significant("[[hello]]");
    ASSERT_EQ(tl.count, 1);
    ASSERT_EQ(tl.tokens[0].type, TK_LONGSTRING);
}

TEST(long_string_level1) {
    TokenList tl = lex_significant("[==[multi\nline]==]");
    ASSERT_EQ(tl.count, 1);
    ASSERT_EQ(tl.tokens[0].type, TK_LONGSTRING);
}

/* ================================================================ */
/* 2.7  All operators (single-char and multi-char)                   */
/* ================================================================ */

TEST(operators_single_char) {
    const char *src = "( ) [ ] { } , ; : . = + - * / % ^ # & ~ | < > ?";
    TokenList tl = lex_significant(src);
    TluaTokenType expected[] = {
        TK_LPAREN, TK_RPAREN, TK_LBRACKET, TK_RBRACKET,
        TK_LBRACE, TK_RBRACE, TK_COMMA, TK_SEMICOLON,
        TK_COLON, TK_DOT, TK_ASSIGN, TK_PLUS, TK_MINUS,
        TK_STAR, TK_SLASH, TK_PERCENT, TK_CARET, TK_HASH,
        TK_AMPERSAND, TK_TILDE, TK_PIPE, TK_LT, TK_GT,
        TK_QUESTION
    };
    int n = sizeof(expected) / sizeof(expected[0]);
    ASSERT_EQ(tl.count, n);
    for (int i = 0; i < n; i++) {
        ASSERT_EQ(tl.tokens[i].type, expected[i]);
    }
}

TEST(operators_multi_char) {
    const char *src = ".. ... == ~= <= >= << >> // ::";
    TokenList tl = lex_significant(src);
    TluaTokenType expected[] = {
        TK_CONCAT, TK_DOTS, TK_EQ, TK_NE,
        TK_LE, TK_GE, TK_SHL, TK_SHR,
        TK_IDIV, TK_DBCOLON
    };
    int n = sizeof(expected) / sizeof(expected[0]);
    ASSERT_EQ(tl.count, n);
    for (int i = 0; i < n; i++) {
        ASSERT_EQ(tl.tokens[i].type, expected[i]);
    }
}

/* ================================================================ */
/* 2.8  Comments                                                     */
/* ================================================================ */

TEST(comment_line) {
    TokenList tl = lex_significant("-- this is a comment");
    ASSERT_EQ(tl.count, 1);
    ASSERT_EQ(tl.tokens[0].type, TK_COMMENT);
}

TEST(comment_block) {
    TokenList tl = lex_significant("--[[ block\ncomment ]]");
    ASSERT_EQ(tl.count, 1);
    ASSERT_EQ(tl.tokens[0].type, TK_COMMENT);
}

TEST(comment_then_code) {
    TokenList tl = lex_significant("-- comment\nlocal x");
    /* Should have: TK_COMMENT, TK_LOCAL, TK_NAME */
    ASSERT_TRUE(tl.count >= 2);
    ASSERT_EQ(tl.tokens[0].type, TK_COMMENT);
}

/* ================================================================ */
/* 2.9  Whitespace and newlines                                      */
/* ================================================================ */

TEST(whitespace_tokens) {
    TokenList tl = lex_all("  \t");
    /* Should produce whitespace token(s) */
    ASSERT_TRUE(tl.count >= 1);
    ASSERT_EQ(tl.tokens[0].type, TK_WHITESPACE);
}

TEST(newline_tokens) {
    TokenList tl = lex_all("a\nb");
    /* Should include a TK_NEWLINE between a and b */
    int found_newline = 0;
    for (int i = 0; i < tl.count; i++) {
        if (tl.tokens[i].type == TK_NEWLINE) found_newline = 1;
    }
    ASSERT_TRUE(found_newline);
}

/* ================================================================ */
/* 2.10  Edge cases                                                  */
/* ================================================================ */

TEST(empty_source) {
    TokenList tl = lex_all("");
    ASSERT_EQ(tl.count, 0);
}

TEST(only_whitespace) {
    TokenList tl = lex_significant("   \t  \n  ");
    ASSERT_EQ(tl.count, 0);
}

TEST(consecutive_operators) {
    /* <=>> should be LE, GT, GT or LE, SHR depending on implementation */
    TokenList tl = lex_significant("<=>");
    ASSERT_TRUE(tl.count >= 2);
    ASSERT_EQ(tl.tokens[0].type, TK_LE);
    ASSERT_EQ(tl.tokens[1].type, TK_GT);
}

/* ================================================================ */
/* 2.11  Line and column tracking                                    */
/* ================================================================ */

TEST(line_tracking) {
    TokenList tl = lex_significant("a\nb\nc");
    ASSERT_EQ(tl.count, 3);
    ASSERT_EQ(tl.tokens[0].line, 1);
    ASSERT_EQ(tl.tokens[1].line, 2);
    ASSERT_EQ(tl.tokens[2].line, 3);
}

TEST(column_tracking) {
    /* "ab cd" — 'ab' at col 1, 'cd' at col 4 */
    TokenList tl = lex_significant("ab cd");
    ASSERT_EQ(tl.count, 2);
    ASSERT_EQ(tl.tokens[0].col, 1);
    ASSERT_EQ(tl.tokens[1].col, 4);
}

/* ================================================================ */
/* 2.12  Peek functionality                                          */
/* ================================================================ */

TEST(peek_does_not_consume) {
    TluaLexer lex;
    tlua_lexer_init(&lex, "local x = 10", "test");
    tlua_lexer_next(&lex); /* consume 'local' */
    ASSERT_EQ(lex.token.type, TK_LOCAL);

    /* Peek should see 'x' (skipping whitespace) without advancing */
    TluaToken peeked = tlua_lexer_peek(&lex);
    ASSERT_EQ(peeked.type, TK_NAME);

    /* Next call to next should still give whitespace or 'x' after ws */
    tlua_lexer_next(&lex);
    /* After consuming whitespace, next should be the name 'x' */
    if (lex.token.type == TK_WHITESPACE) {
        tlua_lexer_next(&lex);
    }
    ASSERT_EQ(lex.token.type, TK_NAME);
    ASSERT_STR_EQ(tok_text(&lex.token), "x");
}

/* ================================================================ */
/* Additional edge-case tests                                        */
/* ================================================================ */

TEST(colon_vs_dbcolon) {
    TokenList tl = lex_significant(": ::");
    ASSERT_EQ(tl.count, 2);
    ASSERT_EQ(tl.tokens[0].type, TK_COLON);
    ASSERT_EQ(tl.tokens[1].type, TK_DBCOLON);
}

TEST(dot_concat_dots) {
    TokenList tl = lex_significant(". .. ...");
    ASSERT_EQ(tl.count, 3);
    ASSERT_EQ(tl.tokens[0].type, TK_DOT);
    ASSERT_EQ(tl.tokens[1].type, TK_CONCAT);
    ASSERT_EQ(tl.tokens[2].type, TK_DOTS);
}

TEST(keyword_like_identifiers) {
    /* "donut" should be TK_NAME, not TK_DO */
    TokenList tl = lex_significant("donut fortune iffy");
    ASSERT_EQ(tl.count, 3);
    for (int i = 0; i < 3; i++) {
        ASSERT_EQ(tl.tokens[i].type, TK_NAME);
    }
}

/* ================================================================ */
/* main                                                              */
/* ================================================================ */

int main(void) {
    printf("=== Lexer Unit Tests ===\n\n");

    /* 2.1 Keywords */
    RUN_TEST(keywords_lua54);

    /* 2.2 fun keyword */
    RUN_TEST(keyword_fun);
    RUN_TEST(fun_not_in_identifier);

    /* 2.3 Identifiers */
    RUN_TEST(identifiers_basic);

    /* 2.4 Numbers */
    RUN_TEST(numbers_integer);
    RUN_TEST(numbers_float);
    RUN_TEST(numbers_hex);

    /* 2.5 Strings */
    RUN_TEST(string_double_quotes);
    RUN_TEST(string_single_quotes);
    RUN_TEST(string_escape_sequences);

    /* 2.6 Long strings */
    RUN_TEST(long_string_basic);
    RUN_TEST(long_string_level1);

    /* 2.7 Operators */
    RUN_TEST(operators_single_char);
    RUN_TEST(operators_multi_char);

    /* 2.8 Comments */
    RUN_TEST(comment_line);
    RUN_TEST(comment_block);
    RUN_TEST(comment_then_code);

    /* 2.9 Whitespace & Newlines */
    RUN_TEST(whitespace_tokens);
    RUN_TEST(newline_tokens);

    /* 2.10 Edge cases */
    RUN_TEST(empty_source);
    RUN_TEST(only_whitespace);
    RUN_TEST(consecutive_operators);

    /* 2.11 Line & Column */
    RUN_TEST(line_tracking);
    RUN_TEST(column_tracking);

    /* 2.12 Peek */
    RUN_TEST(peek_does_not_consume);

    /* Extra edge cases */
    RUN_TEST(colon_vs_dbcolon);
    RUN_TEST(dot_concat_dots);
    RUN_TEST(keyword_like_identifiers);

    TEST_REPORT();
}
