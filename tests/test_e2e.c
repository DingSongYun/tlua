/*
** test_e2e.c — End-to-end integration tests for the TypingLua transpiler.
** Tests fixture-based transpilation: reads .tlua files, transpiles them
** via the tlua_transpile API, and compares output to .expected.lua files.
**
** FIXTURES_DIR is passed via CMake compile definition.
*/

#include "tlua_test.h"
#include "tlua_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================ */
/* Helper: read entire file into malloc'd buffer                     */
/* ================================================================ */

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc(len + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t read = fread(buf, 1, len, f);
    buf[read] = '\0';
    fclose(f);
    return buf;
}

/* Helper: count lines */
static int count_lines(const char *s) {
    if (!s || !*s) return 0;
    int n = 1;
    for (const char *p = s; *p; p++) {
        if (*p == '\n') n++;
    }
    return n;
}

/* Helper: build fixture path */
static char pathbuf[1024];
static const char *fixture_path(const char *name) {
    snprintf(pathbuf, sizeof(pathbuf), "%s/%s", FIXTURES_DIR, name);
    return pathbuf;
}

/* ================================================================ */
/* Fixture-based test macro                                          */
/* Reads <name>.tlua, transpiles, compares with <name>.expected.lua  */
/* ================================================================ */

#define FIXTURE_TEST(test_name, fixture_name) \
TEST(test_name) { \
    /* Read input */ \
    char *input = read_file(fixture_path(fixture_name ".tlua")); \
    ASSERT_NOT_NULL(input); \
    \
    /* Read expected output */ \
    char *expected = read_file(fixture_path(fixture_name ".expected.lua")); \
    ASSERT_NOT_NULL(expected); \
    \
    /* Transpile */ \
    char *output = NULL; \
    char err[512]; \
    int rc = tlua_transpile(input, fixture_name ".tlua", &output, err, sizeof(err)); \
    if (rc != 0) { \
        printf("\n    Transpile error: %s\n", err); \
        free(input); free(expected); \
        ASSERT_TRUE(0); /* fail */ \
        return; \
    } \
    ASSERT_NOT_NULL(output); \
    \
    /* Compare output to expected */ \
    ASSERT_STR_EQ(output, expected); \
    \
    /* Verify line count preservation */ \
    ASSERT_EQ(count_lines(input), count_lines(output)); \
    \
    free(input); \
    free(expected); \
    free(output); \
}

/* ================================================================ */
/* 5.1  Basic types fixture                                          */
/* ================================================================ */
FIXTURE_TEST(e2e_basic_types, "basic_types")

/* ================================================================ */
/* 5.2  Functions fixture                                            */
/* ================================================================ */
FIXTURE_TEST(e2e_functions, "functions")

/* ================================================================ */
/* 5.3  Comments fixture                                             */
/* ================================================================ */
FIXTURE_TEST(e2e_comments, "comments")

/* ================================================================ */
/* 5.4  Plain Lua passthrough fixture                                */
/* ================================================================ */
FIXTURE_TEST(e2e_plain_lua, "plain_lua")

/* ================================================================ */
/* 5.5  Complex types fixture                                        */
/* ================================================================ */
FIXTURE_TEST(e2e_complex_types, "complex_types")

/* ================================================================ */
/* 5.6  Advanced types fixture                                       */
/* ================================================================ */
FIXTURE_TEST(e2e_advanced_types, "advanced_types")

/* ================================================================ */
/* 5.7  Line count verification (standalone)                         */
/* ================================================================ */
TEST(e2e_line_count_all_fixtures) {
    const char *fixtures[] = {
        "basic_types", "functions", "comments", "plain_lua", "complex_types",
        "advanced_types"
    };
    int nfixtures = sizeof(fixtures) / sizeof(fixtures[0]);

    for (int i = 0; i < nfixtures; i++) {
        char input_path[1024], expected_path[1024];
        snprintf(input_path, sizeof(input_path), "%s/%s.tlua", FIXTURES_DIR, fixtures[i]);
        snprintf(expected_path, sizeof(expected_path), "%s/%s.expected.lua", FIXTURES_DIR, fixtures[i]);

        char *input = read_file(input_path);
        ASSERT_NOT_NULL(input);

        char *output = NULL;
        char err[512];
        int rc = tlua_transpile(input, fixtures[i], &output, err, sizeof(err));
        ASSERT_EQ(rc, 0);
        ASSERT_NOT_NULL(output);
        ASSERT_EQ(count_lines(input), count_lines(output));

        free(input);
        free(output);
    }
}

/* ================================================================ */
/* main                                                              */
/* ================================================================ */

int main(void) {
    printf("=== E2E Integration Tests ===\n\n");

    /* 5.1-5.5 Fixture-based tests */
    RUN_TEST(e2e_basic_types);
    RUN_TEST(e2e_functions);
    RUN_TEST(e2e_comments);
    RUN_TEST(e2e_plain_lua);
    RUN_TEST(e2e_complex_types);
    RUN_TEST(e2e_advanced_types);

    /* 5.7 Line count verification */
    RUN_TEST(e2e_line_count_all_fixtures);

    TEST_REPORT();
}
