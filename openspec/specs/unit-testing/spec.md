## Requirements

### Requirement: 极简测试宏框架

项目 SHALL 提供一个 header-only 的测试宏文件 `tests/tlua_test.h`，用于编写所有 C 单元测试。该头文件 SHALL 提供以下宏：

- `TEST(name)` — 定义一个测试函数
- `ASSERT_TRUE(expr)` — 断言表达式为真
- `ASSERT_EQ(a, b)` — 断言两个整数相等
- `ASSERT_STR_EQ(a, b)` — 断言两个字符串相等
- `ASSERT_NOT_NULL(ptr)` — 断言指针非空
- `RUN_TEST(name)` — 运行指定测试
- `TEST_REPORT()` — 输出测试汇总（通过/失败计数）并返回退出码

#### Scenario: 测试通过时输出绿色摘要

- **WHEN** 所有 `ASSERT_*` 宏均成功
- **THEN** `TEST_REPORT()` 输出包含通过的测试数量
- **AND** 进程退出码为 0

#### Scenario: 测试失败时输出红色摘要

- **WHEN** 任何一个 `ASSERT_*` 宏失败
- **THEN** 输出包含失败的文件名、行号和失败表达式
- **AND** `TEST_REPORT()` 输出包含失败的测试数量
- **AND** 进程退出码为非 0

#### Scenario: 头文件无外部依赖

- **WHEN** 编译任何包含 `tlua_test.h` 的测试文件
- **THEN** 仅需标准 C 库头文件（`stdio.h`、`string.h`、`stdlib.h`）
- **AND** 不依赖任何第三方库

### Requirement: CTest 集成

构建系统 SHALL 通过 CMake 的 CTest 驱动所有测试。

#### Scenario: 一键运行所有测试

- **WHEN** 在构建目录执行 `ctest --test-dir build --build-config Debug`
- **THEN** 所有注册的测试均被执行
- **AND** 输出每个测试的通过/失败状态

#### Scenario: 测试失败导致 CTest 返回非零退出码

- **WHEN** 任何测试可执行文件返回非零退出码
- **THEN** `ctest` 命令整体返回非零退出码
- **AND** 报告失败的测试名称

### Requirement: 测试可执行文件独立编译

每个测试文件 SHALL 编译为独立的可执行文件，互相隔离。

#### Scenario: 单个测试崩溃不影响其他测试

- **WHEN** `test_lexer` 可执行文件崩溃
- **THEN** `test_transpiler` 和 `test_e2e` 仍能正常执行和报告结果

#### Scenario: 测试可执行文件包含正确的源码依赖

- **WHEN** 编译 `test_lexer`
- **THEN** 链接 `tlua_lexer.c` 源码
- **WHEN** 编译 `test_transpiler`
- **THEN** 链接 `tlua_lexer.c` 和 `tlua_parser.c` 源码