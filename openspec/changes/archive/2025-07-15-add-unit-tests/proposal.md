## Why

TypingLua 项目目前没有任何自动化测试。转译器（lexer、parser、type erasure）、CLI 工具（tluac、tlua）和构建系统（CMake）的正确性完全依赖手动验证。随着语法规则和转译逻辑日趋复杂，手动验证既不可靠也不可持续。引入单元测试和端到端测试可以在每次代码修改后快速回归验证，防止功能退化，并为后续特性开发提供安全网。

## What Changes

- 引入 C 语言单元测试框架，集成到 CMake 构建系统中
- 新增 **词法分析器 (lexer)** 单元测试：验证各类 token 的正确识别（类型关键字、操作符、标识符、字面量等）
- 新增 **转译器核心 (parser/transpiler)** 单元测试：验证所有类型注解擦除场景的正确性（本地变量、全局变量、函数参数、返回类型、联合类型、可选类型、数组类型、泛型 table 类型、函数签名类型等）
- 新增 **代码保持性** 测试：验证注释、空白、字符串字面量、缩进在转译后不被修改
- 新增 **端到端 (E2E)** 测试：通过 `tluac` CLI 执行完整的 `.tlua → .lua` 转译流程，并用 `lua.exe` 执行验证运行时正确性
- 新增 **构建验证** 测试：确保 CMake 产物（`lua.exe`、`luac.exe`、`tluac.exe`、`tlua.exe`、`lua54.dll`）正确生成
- 新增 CTest 集成，支持 `cmake --build build --target test` 一键运行所有测试
- 添加测试用的 `.tlua` 和 `.lua` 样本文件

## Capabilities

### New Capabilities
- `unit-testing`: 单元测试基础设施（测试框架选型、CMake/CTest 集成、测试目标构建配置、测试运行和报告机制）
- `lexer-tests`: 词法分析器的单元测试（token 类型识别、边界情况、错误处理）
- `transpiler-tests`: 转译器的单元测试（类型擦除正确性、代码保持性、错误处理、行号保持）
- `e2e-tests`: 端到端测试（CLI 工具行为、完整转译+执行流程、构建产物验证）

### Modified Capabilities
- `lua-build`: 需要在 CMakeLists.txt 中添加测试目标（test executables）和 CTest 配置

## Impact

- **构建系统**：CMakeLists.txt 需要新增测试可执行文件和 `enable_testing()` / `add_test()` 配置
- **目录结构**：新增 `tests/` 目录存放测试源码和测试数据
- **依赖**：引入轻量级 C 测试框架（如 header-only 的单文件方案），无外部运行时依赖
- **CI 兼容**：测试通过 CTest 驱动，天然兼容 CI/CD 流水线
- **现有代码**：不修改现有 `tlua/` 和 `lua/` 源码的功能逻辑
