## 1. 测试基础设施

- [x] 1.1 创建 `tests/` 目录结构（`tests/`、`tests/fixtures/`）
- [x] 1.2 编写 `tests/tlua_test.h` 极简测试宏头文件（TEST、ASSERT_TRUE、ASSERT_EQ、ASSERT_STR_EQ、ASSERT_NOT_NULL、RUN_TEST、TEST_REPORT）
- [x] 1.3 在 `CMakeLists.txt` 中添加 `enable_testing()` 和测试可执行文件目标（`test_lexer`、`test_transpiler`、`test_e2e`），配置 include 路径和源文件依赖
- [x] 1.4 在 `CMakeLists.txt` 中使用 `add_test()` 将每个测试可执行文件注册为 CTest 测试

## 2. 词法分析器单元测试

- [x] 2.1 创建 `tests/test_lexer.c` 基本框架（include 头文件、main 函数、TEST_REPORT）
- [x] 2.2 编写 Lua 5.4 关键字识别测试（22 个关键字全覆盖）
- [x] 2.3 编写 TypingLua `fun` 关键字识别测试
- [x] 2.4 编写标识符（TK_NAME）识别测试（普通标识符、下划线开头、数字后缀）
- [x] 2.5 编写数字字面量测试（整数、浮点数、十六进制、二进制、科学记数法）
- [x] 2.6 编写字符串字面量测试（短字符串、转义字符、长字符串 `[[ ]]`、`[==[ ]==]`）
- [x] 2.7 编写单字符操作符测试（`( ) [ ] { } , ; : . = + - * / % ^ # & ~ | < > ?`）
- [x] 2.8 编写多字符操作符测试（`.. ... == ~= <= >= << >> // ::`）
- [x] 2.9 编写注释识别测试（行注释 `--`、块注释 `--[[ ]]`）
- [x] 2.10 编写空白和换行 token 测试（TK_WHITESPACE、TK_NEWLINE）
- [x] 2.11 编写行号和列号跟踪测试（多行源码中的 line/col 正确性）
- [x] 2.12 编写 EOF 处理测试（空源码、源码末尾持续返回 TK_EOF）

## 3. 转译器单元测试

- [x] 3.1 创建 `tests/test_transpiler.c` 基本框架（include 头文件、main 函数、辅助比较函数）
- [x] 3.2 编写局部变量类型擦除测试（带初始值、无初始值、多变量、部分类型）
- [x] 3.3 编写全局变量类型擦除测试
- [x] 3.4 编写函数参数类型擦除测试（单参数、多参数、混合类型、匿名函数、local 函数）
- [x] 3.5 编写函数返回类型擦除测试（单返回、参数+返回、多返回、括号多返回、匿名函数、void）
- [x] 3.6 编写复杂类型表达式擦除测试（联合类型、可选类型、数组类型、嵌套数组、泛型 table、嵌套泛型、函数签名类型）
- [x] 3.7 编写代码保持性测试（行注释、块注释、字符串内类型文本、多行字符串、缩进、空行）
- [x] 3.8 编写纯 Lua 透传测试（无类型注解的标准 Lua 代码输出不变、方法调用 `obj:method()` 不被误识别）
- [x] 3.9 编写方法定义冒号消歧测试（`function MyClass:init(name: string): void`）
- [x] 3.10 编写行号保持测试（输出行数等于输入行数）

## 4. 测试 Fixture 文件

- [x] 4.1 创建 `tests/fixtures/basic_types.tlua` 和 `tests/fixtures/basic_types.expected.lua`（覆盖局部变量、全局变量、函数参数、返回类型的基础类型擦除）
- [x] 4.2 创建 `tests/fixtures/complex_types.tlua` 和 `tests/fixtures/complex_types.expected.lua`（覆盖联合类型、可选类型、数组、泛型 table、函数签名类型）
- [x] 4.3 创建 `tests/fixtures/preservation.tlua` 和 `tests/fixtures/preservation.expected.lua`（覆盖注释、字符串、缩进、空行保持）
- [x] 4.4 创建 `tests/fixtures/pure_lua.tlua` 和 `tests/fixtures/pure_lua.expected.lua`（纯 Lua 代码，两个文件内容完全相同）
- [x] 4.5 创建 `tests/fixtures/runnable.tlua` 和 `tests/fixtures/runnable.expected.lua`（包含 print 输出的可执行 fixture，用于 E2E 运行时验证）

## 5. 端到端测试

- [x] 5.1 创建 `tests/test_e2e.c` 基本框架（include 头文件、main 函数、辅助函数用于执行外部进程并捕获输出）
- [x] 5.2 编写 `tluac -p` stdout 输出验证测试（对每个 fixture 文件，比较 stdout 输出与 `.expected.lua` 内容）
- [x] 5.3 编写 `tluac` 文件写入模式验证测试（验证生成的 `.lua` 文件内容正确）
- [x] 5.4 编写 `tluac` 错误处理测试（不存在的文件返回非零退出码）
- [x] 5.5 编写转译后 `lua.exe` 执行测试（转译 runnable fixture 并用 lua.exe 执行，验证输出和退出码）
- [x] 5.6 编写构建产物存在性验证测试（检查 `lua.exe`、`luac.exe`、`tluac.exe`、`tlua.exe`、`lua54.dll` 存在）
- [x] 5.7 通过 CMake 定义宏将 `tluac`、`lua` 可执行文件路径和 fixtures 目录路径传入测试代码（使用 `target_compile_definitions` 或 `configure_file`）

## 6. 构建验证与集成

- [x] 6.1 运行 `cmake -B build` 验证 CMake 配置无错误
- [x] 6.2 运行 `cmake --build build --config Debug` 验证所有测试目标编译成功
- [x] 6.3 运行 `ctest --test-dir build --build-config Debug` 验证所有测试通过
- [x] 6.4 手动引入一个故意的断言失败，验证 CTest 正确报告失败，然后恢复