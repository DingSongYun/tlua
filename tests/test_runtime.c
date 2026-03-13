/*
** test_runtime.c — Runtime execution tests for TypingLua.
** Verifies that .tlua files can be transpiled AND executed via tlua.exe,
** checking both the exit code (0 = success) and stdout against .expected.out.
**
** Compile definitions from CMake:
**   TLUA_EXE    — absolute path to tlua.exe
**   FIXTURES_DIR — absolute path to tests/fixtures/
*/

#include "tlua_test.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================ */
/* Helper: read entire file into malloc'd buffer (NUL-terminated)    */
/* ================================================================ */

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc(len + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t nread = fread(buf, 1, len, f);
    buf[nread] = '\0';
    fclose(f);
    return buf;
}

/* ================================================================ */
/* Helper: run a command, capture stdout into malloc'd buffer,       */
/*         return the process exit code.                             */
/* ================================================================ */

static int run_capture(const char *cmd, char **out_buf, size_t *out_len) {
    FILE *fp = _popen(cmd, "r");
    if (!fp) return -1;

    size_t cap = 4096;
    size_t len = 0;
    char *buf = (char *)malloc(cap);
    if (!buf) { _pclose(fp); return -1; }

    while (1) {
        size_t nread = fread(buf + len, 1, cap - len - 1, fp);
        if (nread == 0) break;
        len += nread;
        if (len + 1 >= cap) {
            cap *= 2;
            char *tmp = (char *)realloc(buf, cap);
            if (!tmp) { free(buf); _pclose(fp); return -1; }
            buf = tmp;
        }
    }
    buf[len] = '\0';

    int status = _pclose(fp);
    *out_buf = buf;
    *out_len = len;
    return status;
}

/* ================================================================ */
/* Helper: normalize line endings (CRLF -> LF) in-place             */
/* ================================================================ */

static void normalize_crlf(char *s) {
    char *r = s, *w = s;
    while (*r) {
        if (r[0] == '\r' && r[1] == '\n') {
            *w++ = '\n';
            r += 2;
        } else {
            *w++ = *r++;
        }
    }
    *w = '\0';
}

/* ================================================================ */
/* Helper: trim trailing whitespace/newlines                         */
/* ================================================================ */

static void trim_trailing(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r' ||
                        s[len - 1] == ' '  || s[len - 1] == '\t')) {
        s[--len] = '\0';
    }
}

/* ================================================================ */
/* Fixture-based runtime test macro                                  */
/* Runs tlua.exe on <name>.tlua, checks exit code and stdout.        */
/* ================================================================ */

#define RUNTIME_TEST(test_name, fixture_name) \
TEST(test_name) { \
    /* Build command: tlua.exe <fixture>.tlua 2>&1 */ \
    char cmd[2048]; \
    snprintf(cmd, sizeof(cmd), "\"\"%s\" \"%s/%s.tlua\"\" 2>&1", \
             TLUA_EXE, FIXTURES_DIR, fixture_name); \
    \
    /* Run and capture */ \
    char *actual = NULL; \
    size_t actual_len = 0; \
    int exit_code = run_capture(cmd, &actual, &actual_len); \
    \
    /* Check: exit code must be 0 */ \
    if (exit_code != 0) { \
        printf("\n    Exit code: %d\n    Output: %s\n", exit_code, \
               actual ? actual : "(null)"); \
        free(actual); \
        ASSERT_EQ(exit_code, 0); \
        return; \
    } \
    \
    /* Read expected output */ \
    char expected_path[1024]; \
    snprintf(expected_path, sizeof(expected_path), \
             "%s/%s.expected.out", FIXTURES_DIR, fixture_name); \
    char *expected = read_file(expected_path); \
    ASSERT_NOT_NULL(expected); \
    \
    /* Normalize both for comparison */ \
    normalize_crlf(actual); \
    normalize_crlf(expected); \
    trim_trailing(actual); \
    trim_trailing(expected); \
    \
    /* Compare */ \
    ASSERT_STR_EQ(actual, expected); \
    \
    free(actual); \
    free(expected); \
}

/* ================================================================ */
/* Runtime tests for each fixture                                    */
/* ================================================================ */

RUNTIME_TEST(runtime_basic_types,    "basic_types")
RUNTIME_TEST(runtime_functions,      "functions")
RUNTIME_TEST(runtime_comments,       "comments")
RUNTIME_TEST(runtime_plain_lua,      "plain_lua")
RUNTIME_TEST(runtime_complex_types,  "complex_types")
RUNTIME_TEST(runtime_advanced_types, "advanced_types")

/* ================================================================ */
/* main                                                              */
/* ================================================================ */

int main(void) {
    printf("=== Runtime Execution Tests ===\n\n");

    RUN_TEST(runtime_basic_types);
    RUN_TEST(runtime_functions);
    RUN_TEST(runtime_comments);
    RUN_TEST(runtime_plain_lua);
    RUN_TEST(runtime_complex_types);
    RUN_TEST(runtime_advanced_types);

    TEST_REPORT();
}
