## ADDED Requirements

### Requirement: Lua 源码集成

项目内包含完整的 Lua 5.4 源码，放置在 `lua/` 目录下。

#### Scenario: 源码目录结构正确

- **WHEN** 检查 `lua/` 目录
- **THEN** 包含 Lua 5.4 的所有 `.c` 和 `.h` 源文件
- **AND** 包含 `lua.c`（解释器入口）和 `luac.c`（编译器入口）

### Requirement: CMake 构建配置

使用 CMake 构建系统编译 Lua 源码，支持 MSVC 编译器。

#### Scenario: CMake 配置成功

- **WHEN** 在项目根目录运行 `cmake -B build`
- **THEN** CMake 配置成功完成，无错误
- **AND** 生成 MSVC 解决方案文件

#### Scenario: 编译成功

- **WHEN** 运行 `cmake --build build --config Release`
- **THEN** 编译成功完成，无错误
- **AND** 在 `build/` 目录下生成 `lua.exe` 和 `luac.exe`