/*
** test_transpiler.c — Unit tests for tlua_parser (transpiler)
** Covers: type erasure for variables, function params, return types,
**         comment/whitespace preservation, line-count equality,
**         nested types, union types, optional types, method calls,
**         and passthrough of plain Lua.
*/

#include "tlua_test.h"
#include "tlua_parser.h"

/* ================================================================ */
/* Helper: transpile and return the output (caller must free)        */
/* Returns NULL on error                                             */
/* ================================================================ */

static char *transpile(const char *source) {
    char *out = NULL;
    char err[512];
    int rc = tlua_transpile(source, "test.tlua", &out, err, sizeof(err));
    if (rc != 0) {
        if (out) free(out);
        return NULL;
    }
    return out;
}

/* Helper: count lines (number of '\n' + 1 if non-empty) */
static int count_lines(const char *s) {
    if (!s || !*s) return 0;
    int n = 1;
    for (const char *p = s; *p; p++) {
        if (*p == '\n') n++;
    }
    return n;
}

/* ================================================================ */
/* 3.1  Variable type annotations (local x: number = 10)            */
/* ================================================================ */

TEST(erase_local_var_type) {
    char *out = transpile("local x: number = 10");
    ASSERT_NOT_NULL(out);
    ASSERT_STR_EQ(out, "local x = 10");
    free(out);
}

TEST(erase_local_multiple_vars) {
    char *out = transpile("local a: number, b: string = 1, \"hi\"");
    ASSERT_NOT_NULL(out);
    ASSERT_STR_EQ(out, "local a, b = 1, \"hi\"");
    free(out);
}

/* 3.1b  Local variable without initializer */
TEST(erase_local_var_type_no_init) {
    char *out = transpile("local name: string");
    ASSERT_NOT_NULL(out);
    ASSERT_STR_EQ(out, "local name");
    free(out);
}

/* 3.1c  Partial typing in multi-variable declaration */
TEST(erase_local_partial_typed) {
    char *out = transpile("local a: string, b = \"hi\", 1");
    ASSERT_NOT_NULL(out);
    ASSERT_STR_EQ(out, "local a, b = \"hi\", 1");
    free(out);
}

/* ================================================================ */
/* 3.2  Function parameter type annotations                          */
/* ================================================================ */

TEST(erase_function_params) {
    char *out = transpile("function add(a: number, b: number)\nend");
    ASSERT_NOT_NULL(out);
    /* Should not contain ': number' */
    ASSERT_TRUE(strstr(out, ": number") == NULL);
    /* Should still contain the parameter names */
    ASSERT_TRUE(strstr(out, "a, b") != NULL || strstr(out, "a,b") != NULL ||
                (strstr(out, "a") != NULL && strstr(out, "b") != NULL));
    free(out);
}

/* 3.2b  Mixed typed/untyped parameters */
TEST(erase_function_params_mixed) {
    char *out = transpile("function f(a: number, b, c: string)\nend");
    ASSERT_NOT_NULL(out);
    ASSERT_TRUE(strstr(out, ": number") == NULL);
    ASSERT_TRUE(strstr(out, ": string") == NULL);
    ASSERT_TRUE(strstr(out, "a, b, c") != NULL || strstr(out, "a,b,c") != NULL);
    free(out);
}

/* 3.2c  Anonymous function with typed parameters */
TEST(erase_anon_function_params) {
    char *out = transpile("local f = function(x: number)\nend");
    ASSERT_NOT_NULL(out);
    ASSERT_TRUE(strstr(out, ": number") == NULL);
    ASSERT_TRUE(strstr(out, "function(x)") != NULL);
    free(out);
}

/* 3.2d  Local function with typed parameters */
TEST(erase_local_function_params) {
    char *out = transpile("local function f(a: number)\nend");
    ASSERT_NOT_NULL(out);
    ASSERT_TRUE(strstr(out, ": number") == NULL);
    ASSERT_TRUE(strstr(out, "local function f(a)") != NULL);
    free(out);
}

/* ================================================================ */
/* 3.3  Function return type annotations                             */
/* ================================================================ */

TEST(erase_function_return_type) {
    char *out = transpile("function foo(): number\nend");
    ASSERT_NOT_NULL(out);
    /* The ': number' after ')' should be erased */
    ASSERT_TRUE(strstr(out, ": number") == NULL);
    ASSERT_TRUE(strstr(out, "function") != NULL);
    ASSERT_TRUE(strstr(out, "foo") != NULL);
    free(out);
}

/* 3.3b  Params AND return type both erased */
TEST(erase_params_and_return_type) {
    char *out = transpile("function add(a: number, b: number): number\nend");
    ASSERT_NOT_NULL(out);
    ASSERT_TRUE(strstr(out, ": number") == NULL);
    ASSERT_TRUE(strstr(out, "function add(a, b)") != NULL);
    free(out);
}

/* 3.3c  Multiple return types */
TEST(erase_multi_return_type) {
    char *out = transpile("function get_info(): number, string\nend");
    ASSERT_NOT_NULL(out);
    ASSERT_TRUE(strstr(out, ": number") == NULL);
    ASSERT_TRUE(strstr(out, "string") == NULL);
    ASSERT_TRUE(strstr(out, "function get_info()") != NULL);
    free(out);
}

/* 3.3d  Parenthesized multiple return types */
TEST(erase_paren_multi_return_type) {
    char *out = transpile("function get_info(): (number, string)\nend");
    ASSERT_NOT_NULL(out);
    ASSERT_TRUE(strstr(out, ": (") == NULL);
    ASSERT_TRUE(strstr(out, "function get_info()") != NULL);
    free(out);
}

/* 3.3e  Anonymous function with return type */
TEST(erase_anon_function_return_type) {
    char *out = transpile("local f = function(x: number): string\nend");
    ASSERT_NOT_NULL(out);
    ASSERT_TRUE(strstr(out, ": number") == NULL);
    ASSERT_TRUE(strstr(out, ": string") == NULL);
    ASSERT_TRUE(strstr(out, "function(x)") != NULL);
    free(out);
}

/* 3.3f  void return type */
TEST(erase_void_return_type) {
    char *out = transpile("function do_nothing(): void\nend");
    ASSERT_NOT_NULL(out);
    ASSERT_TRUE(strstr(out, "void") == NULL);
    ASSERT_TRUE(strstr(out, "function do_nothing()") != NULL);
    free(out);
}

/* ================================================================ */
/* 3.4  Comment preservation                                         */
/* ================================================================ */

TEST(preserve_line_comment) {
    char *out = transpile("-- hello world\nlocal x = 1");
    ASSERT_NOT_NULL(out);
    ASSERT_TRUE(strstr(out, "-- hello world") != NULL);
    free(out);
}

TEST(preserve_block_comment) {
    char *out = transpile("--[[ block ]]\nlocal x = 1");
    ASSERT_NOT_NULL(out);
    ASSERT_TRUE(strstr(out, "--[[ block ]]") != NULL);
    free(out);
}

/* ================================================================ */
/* 3.5  Line count preservation (1:1 mapping)                        */
/* ================================================================ */

TEST(line_count_preserved_simple) {
    const char *src = "local x: number = 10\nlocal y: string = \"hi\"\nprint(x)";
    char *out = transpile(src);
    ASSERT_NOT_NULL(out);
    ASSERT_EQ(count_lines(src), count_lines(out));
    free(out);
}

TEST(line_count_preserved_function) {
    const char *src =
        "function greet(name: string): string\n"
        "  return \"Hello \" .. name\n"
        "end\n";
    char *out = transpile(src);
    ASSERT_NOT_NULL(out);
    ASSERT_EQ(count_lines(src), count_lines(out));
    free(out);
}

/* ================================================================ */
/* 3.6  Plain Lua passthrough                                        */
/* ================================================================ */

TEST(passthrough_plain_lua) {
    const char *src = "local x = 42\nprint(x)\n";
    char *out = transpile(src);
    ASSERT_NOT_NULL(out);
    ASSERT_STR_EQ(out, src);
    free(out);
}

TEST(passthrough_table_constructor) {
    const char *src = "local t = {1, 2, 3}\n";
    char *out = transpile(src);
    ASSERT_NOT_NULL(out);
    ASSERT_STR_EQ(out, src);
    free(out);
}

/* ================================================================ */
/* 3.7  Optional type (question mark)                                */
/* ================================================================ */

TEST(erase_optional_type) {
    char *out = transpile("local x: number? = nil");
    ASSERT_NOT_NULL(out);
    ASSERT_STR_EQ(out, "local x = nil");
    free(out);
}

/* ================================================================ */
/* 3.8  Union types (pipe)                                           */
/* ================================================================ */

TEST(erase_union_type) {
    char *out = transpile("local x: number | string = 42");
    ASSERT_NOT_NULL(out);
    ASSERT_STR_EQ(out, "local x = 42");
    free(out);
}

/* 3.8b  Multi-union type (3+) */
TEST(erase_multi_union_type) {
    char *out = transpile("local x: number|string|boolean");
    ASSERT_NOT_NULL(out);
    ASSERT_STR_EQ(out, "local x");
    free(out);
}

/* 3.8c  Union type with spaces */
TEST(erase_union_type_spaced) {
    char *out = transpile("local x: number | string");
    ASSERT_NOT_NULL(out);
    ASSERT_STR_EQ(out, "local x");
    free(out);
}

/* ================================================================ */
/* 3.9  Method calls vs type annotations (colon disambiguation)      */
/* ================================================================ */

TEST(method_call_preserved) {
    /* obj:method() should NOT be treated as a type annotation */
    const char *src = "obj:method()\n";
    char *out = transpile(src);
    ASSERT_NOT_NULL(out);
    ASSERT_STR_EQ(out, src);
    free(out);
}

/* ================================================================ */
/* 3.10  fun keyword in type annotations                             */
/* ================================================================ */

TEST(erase_fun_type_annotation) {
    char *out = transpile("local f: fun(number): string = myFunc");
    ASSERT_NOT_NULL(out);
    /* The type annotation with 'fun' should be erased */
    ASSERT_TRUE(strstr(out, "fun") == NULL);
    ASSERT_TRUE(strstr(out, "local f") != NULL);
    ASSERT_TRUE(strstr(out, "= myFunc") != NULL);
    free(out);
}

/* 3.10b  No-arg fun with void return */
TEST(erase_fun_no_arg_void) {
    char *out = transpile("local action: fun(): void");
    ASSERT_NOT_NULL(out);
    ASSERT_STR_EQ(out, "local action");
    free(out);
}

/* ================================================================ */
/* Additional transpiler tests                                       */
/* ================================================================ */

/* NOTE: The transpiler does not yet support {K: V} brace-style table types.
   Use 'table' or 'table<K,V>' syntax instead. */
TEST(erase_table_type_keyword) {
    char *out = transpile("local t: table = {}");
    ASSERT_NOT_NULL(out);
    ASSERT_STR_EQ(out, "local t = {}");
    free(out);
}

/* 3.11b  Generic table<K, V> */
TEST(erase_generic_table_type) {
    char *out = transpile("local scores: table<string, number> = {}");
    ASSERT_NOT_NULL(out);
    ASSERT_STR_EQ(out, "local scores = {}");
    free(out);
}

/* 3.11c  Nested generic table */
TEST(erase_nested_generic_table_type) {
    char *out = transpile("local config: table<string, table<string, number>>");
    ASSERT_NOT_NULL(out);
    ASSERT_STR_EQ(out, "local config");
    free(out);
}

/* ================================================================ */
/* 3.12  Array type T[]                                              */
/* ================================================================ */

TEST(erase_array_type) {
    char *out = transpile("local nums: number[] = {1, 2, 3}");
    ASSERT_NOT_NULL(out);
    ASSERT_STR_EQ(out, "local nums = {1, 2, 3}");
    free(out);
}

TEST(erase_nested_array_type) {
    char *out = transpile("local matrix: number[][] = {{1,2},{3,4}}");
    ASSERT_NOT_NULL(out);
    ASSERT_STR_EQ(out, "local matrix = {{1,2},{3,4}}");
    free(out);
}

TEST(erase_optional_array_type) {
    char *out = transpile("local items: string[]?");
    ASSERT_NOT_NULL(out);
    ASSERT_STR_EQ(out, "local items");
    free(out);
}

/* ================================================================ */
/* 3.13  Combined complex types                                      */
/* ================================================================ */

TEST(erase_combined_complex_type) {
    char *out = transpile("local items: table<string, number[]>? = nil");
    ASSERT_NOT_NULL(out);
    ASSERT_STR_EQ(out, "local items = nil");
    free(out);
}

/* ================================================================ */
/* 3.14  Global variable type annotation (EBNF §2.2)                 */
/* ================================================================ */

TEST(erase_global_var_type) {
    char *out = transpile("x: number = 42");
    ASSERT_NOT_NULL(out);
    ASSERT_STR_EQ(out, "x = 42");
    free(out);
}

/* ================================================================ */
/* 3.15  Method declaration with typed params                        */
/* ================================================================ */

TEST(method_declaration_typed) {
    char *out = transpile("function MyClass:init(name: string): void\nend");
    ASSERT_NOT_NULL(out);
    /* Method colon should be preserved */
    ASSERT_TRUE(strstr(out, "MyClass:init") != NULL);
    /* Type annotations should be erased */
    ASSERT_TRUE(strstr(out, ": string") == NULL);
    ASSERT_TRUE(strstr(out, ": void") == NULL);
    ASSERT_TRUE(strstr(out, "function MyClass:init(name)") != NULL);
    free(out);
}

/* ================================================================ */
/* 3.16  Dotted function name (function t.f)                        */
/* ================================================================ */

TEST(dotted_function_typed) {
    char *out = transpile("function mod.init(x: number): string\nend");
    ASSERT_NOT_NULL(out);
    ASSERT_TRUE(strstr(out, "function mod.init(x)") != NULL);
    ASSERT_TRUE(strstr(out, ": number") == NULL);
    ASSERT_TRUE(strstr(out, ": string") == NULL);
    free(out);
}

/* ================================================================ */
/* 3.17  String content not modified by type erasure                 */
/* ================================================================ */

TEST(string_content_preserved) {
    char *out = transpile("local s: string = \"local x: number = 42\"");
    ASSERT_NOT_NULL(out);
    ASSERT_STR_EQ(out, "local s = \"local x: number = 42\"");
    free(out);
}

/* ================================================================ */
/* 3.18  Whitespace and structural preservation                     */
/* ================================================================ */

TEST(whitespace_preservation) {
    /* Indentation should be preserved exactly */
    const char *src = "  local x: number = 1\n";
    char *out = transpile(src);
    ASSERT_NOT_NULL(out);
    /* Output should start with the same indentation */
    ASSERT_TRUE(out[0] == ' ' && out[1] == ' ');
    free(out);
}

TEST(empty_lines_preserved) {
    const char *src = "local x = 1\n\n\nlocal y = 2\n";
    char *out = transpile(src);
    ASSERT_NOT_NULL(out);
    ASSERT_STR_EQ(out, src);
    free(out);
}

TEST(for_loop_passthrough) {
    const char *src = "for i = 1, 10 do\n  print(i)\nend\n";
    char *out = transpile(src);
    ASSERT_NOT_NULL(out);
    ASSERT_STR_EQ(out, src);
    free(out);
}

/* ================================================================ */
/* 3.19  Varargs passthrough                                         */
/* ================================================================ */

TEST(varargs_passthrough) {
    char *out = transpile("function f(a: number, ...)\nend");
    ASSERT_NOT_NULL(out);
    ASSERT_TRUE(strstr(out, "function f(a, ...)") != NULL);
    ASSERT_TRUE(strstr(out, ": number") == NULL);
    free(out);
}

/* ================================================================ */
/* main                                                              */
/* ================================================================ */

int main(void) {
    printf("=== Transpiler Unit Tests ===\n\n");

    /* 3.1 Variable type erasure */
    RUN_TEST(erase_local_var_type);
    RUN_TEST(erase_local_multiple_vars);
    RUN_TEST(erase_local_var_type_no_init);
    RUN_TEST(erase_local_partial_typed);

    /* 3.2 Function param type erasure */
    RUN_TEST(erase_function_params);
    RUN_TEST(erase_function_params_mixed);
    RUN_TEST(erase_anon_function_params);
    RUN_TEST(erase_local_function_params);

    /* 3.3 Return type erasure */
    RUN_TEST(erase_function_return_type);
    RUN_TEST(erase_params_and_return_type);
    RUN_TEST(erase_multi_return_type);
    RUN_TEST(erase_paren_multi_return_type);
    RUN_TEST(erase_anon_function_return_type);
    RUN_TEST(erase_void_return_type);

    /* 3.4 Comment preservation */
    RUN_TEST(preserve_line_comment);
    RUN_TEST(preserve_block_comment);

    /* 3.5 Line count preservation */
    RUN_TEST(line_count_preserved_simple);
    RUN_TEST(line_count_preserved_function);

    /* 3.6 Plain Lua passthrough */
    RUN_TEST(passthrough_plain_lua);
    RUN_TEST(passthrough_table_constructor);

    /* 3.7 Optional type */
    RUN_TEST(erase_optional_type);

    /* 3.8 Union type */
    RUN_TEST(erase_union_type);
    RUN_TEST(erase_multi_union_type);
    RUN_TEST(erase_union_type_spaced);

    /* 3.9 Method call disambiguation */
    RUN_TEST(method_call_preserved);

    /* 3.10 fun keyword */
    RUN_TEST(erase_fun_type_annotation);
    RUN_TEST(erase_fun_no_arg_void);

    /* 3.11 table types */
    RUN_TEST(erase_table_type_keyword);
    RUN_TEST(erase_generic_table_type);
    RUN_TEST(erase_nested_generic_table_type);

    /* 3.12 Array types */
    RUN_TEST(erase_array_type);
    RUN_TEST(erase_nested_array_type);
    RUN_TEST(erase_optional_array_type);

    /* 3.13 Combined complex types */
    RUN_TEST(erase_combined_complex_type);

    /* 3.14 Global variable type */
    RUN_TEST(erase_global_var_type);

    /* 3.15 Method declaration */
    RUN_TEST(method_declaration_typed);

    /* 3.16 Dotted function name */
    RUN_TEST(dotted_function_typed);

    /* 3.17 String content preserved */
    RUN_TEST(string_content_preserved);

    /* 3.18 Whitespace/structure preservation */
    RUN_TEST(whitespace_preservation);
    RUN_TEST(empty_lines_preserved);
    RUN_TEST(for_loop_passthrough);

    /* 3.19 Varargs */
    RUN_TEST(varargs_passthrough);

    TEST_REPORT();
}
