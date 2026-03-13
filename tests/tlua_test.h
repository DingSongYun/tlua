/*
** tlua_test.h — Minimal header-only test framework for TypingLua
** Provides basic test macros: TEST, ASSERT_*, RUN_TEST, TEST_REPORT
** No external dependencies beyond standard C library.
*/

#ifndef TLUA_TEST_H
#define TLUA_TEST_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ================================================================ */
/* ANSI color codes (MSVC console supports them on Win10+)           */
/* ================================================================ */
#define TLUA_COLOR_RED     "\x1b[31m"
#define TLUA_COLOR_GREEN   "\x1b[32m"
#define TLUA_COLOR_YELLOW  "\x1b[33m"
#define TLUA_COLOR_RESET   "\x1b[0m"

/* ================================================================ */
/* Global counters                                                   */
/* ================================================================ */
static int tlua_test_pass_count = 0;
static int tlua_test_fail_count = 0;
static int tlua_test_current_failed = 0;

/* ================================================================ */
/* TEST macro — define a test function                               */
/* ================================================================ */
#define TEST(name) static void test_##name(void)

/* ================================================================ */
/* RUN_TEST macro — run a named test and track results               */
/* ================================================================ */
#define RUN_TEST(name) do { \
    tlua_test_current_failed = 0; \
    printf("  %-60s ", #name); \
    test_##name(); \
    if (tlua_test_current_failed) { \
        printf(TLUA_COLOR_RED "FAIL" TLUA_COLOR_RESET "\n"); \
        tlua_test_fail_count++; \
    } else { \
        printf(TLUA_COLOR_GREEN "PASS" TLUA_COLOR_RESET "\n"); \
        tlua_test_pass_count++; \
    } \
} while (0)

/* ================================================================ */
/* ASSERT macros                                                     */
/* ================================================================ */

#define ASSERT_TRUE(expr) do { \
    if (!(expr)) { \
        printf("\n    " TLUA_COLOR_RED "ASSERT_TRUE failed" TLUA_COLOR_RESET \
               " [%s:%d]: %s\n", __FILE__, __LINE__, #expr); \
        tlua_test_current_failed = 1; \
        return; \
    } \
} while (0)

#define ASSERT_EQ(a, b) do { \
    long long _a = (long long)(a); \
    long long _b = (long long)(b); \
    if (_a != _b) { \
        printf("\n    " TLUA_COLOR_RED "ASSERT_EQ failed" TLUA_COLOR_RESET \
               " [%s:%d]: %s == %lld, expected %s == %lld\n", \
               __FILE__, __LINE__, #a, _a, #b, _b); \
        tlua_test_current_failed = 1; \
        return; \
    } \
} while (0)

#define ASSERT_STR_EQ(a, b) do { \
    const char *_a = (a); \
    const char *_b = (b); \
    if (_a == NULL && _b == NULL) break; \
    if (_a == NULL || _b == NULL || strcmp(_a, _b) != 0) { \
        printf("\n    " TLUA_COLOR_RED "ASSERT_STR_EQ failed" TLUA_COLOR_RESET \
               " [%s:%d]:\n      got:      \"%s\"\n      expected: \"%s\"\n", \
               __FILE__, __LINE__, _a ? _a : "(null)", _b ? _b : "(null)"); \
        tlua_test_current_failed = 1; \
        return; \
    } \
} while (0)

#define ASSERT_NOT_NULL(ptr) do { \
    if ((ptr) == NULL) { \
        printf("\n    " TLUA_COLOR_RED "ASSERT_NOT_NULL failed" TLUA_COLOR_RESET \
               " [%s:%d]: %s is NULL\n", __FILE__, __LINE__, #ptr); \
        tlua_test_current_failed = 1; \
        return; \
    } \
} while (0)

/* ================================================================ */
/* TEST_REPORT — print summary and return exit code                  */
/* ================================================================ */
#define TEST_REPORT() do { \
    printf("\n----------------------------------------\n"); \
    int _total = tlua_test_pass_count + tlua_test_fail_count; \
    if (tlua_test_fail_count == 0) { \
        printf(TLUA_COLOR_GREEN "ALL %d TESTS PASSED" TLUA_COLOR_RESET "\n", _total); \
    } else { \
        printf(TLUA_COLOR_RED "%d/%d TESTS FAILED" TLUA_COLOR_RESET "\n", \
               tlua_test_fail_count, _total); \
    } \
    printf("----------------------------------------\n"); \
    return tlua_test_fail_count > 0 ? 1 : 0; \
} while (0)

#endif /* TLUA_TEST_H */
