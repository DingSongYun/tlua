## MODIFIED Requirements

### Requirement: CMake 构建配置

使用 CMake 构建系统编译 Lua 源码和 TypingLua 工具，支持 MSVC 编译器。构建系统 SHALL 同时管理测试目标的编译和 CTest 集成。

#### Scenario: CMake 配置成功

- **WHEN** 在项目根目录运行 `cmake -B build`
- **THEN** CMake 配置成功完成，无错误
- **AND** 生成 MSVC 解决方案文件

#### Scenario: 编译成功

- **WHEN** 运行 `cmake --build build --config Release`
- **THEN** 编译成功完成，无错误
- **AND** 在 `build/` 目录下生成 `lua.exe` 和 `luac.exe`

#### Scenario: 测试目标编译

- **WHEN** 运行 `cmake --build build --config Debug`
- **THEN** SHALL 同时编译测试可执行文件 `test_lexer`、`test_transpiler`、`test_e2e`
- **AND** 测试可执行文件生成在构建输出目录中

#### Scenario: CTest 注册

- **WHEN** CMakeLists.txt 被处理
- **THEN** SHALL 包含 `enable_testing()` 调用
- **AND** 每个测试可执行文件 SHALL 通过 `add_test()` 注册为 CTest 测试

#### Scenario: 测试包含路径配置

- **WHEN** 编译测试可执行文件
- **THEN** 测试目标 SHALL 能访问 `tlua/` 目录下的头文件
- **AND** 测试目标 SHALL 链接所需的 `tlua_lexer.c` 和/或 `tlua_parser.c` 源文件
