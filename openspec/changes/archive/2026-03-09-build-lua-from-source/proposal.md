## Why

TypingLua 项目的目标是为 Lua 构建强类型语法规范。作为第一步，我们需要在项目内集成 Lua 源码并能成功编译，确保拥有一个可控的 Lua 运行时环境，为后续的类型系统扩展打下基础。

## What Changes

- 将 Lua 5.4 源码引入项目（放入 `lua/` 目录）
- 使用 CMake + MSVC 构建系统编译 Lua 源码
- 生成 `lua.exe` 解释器和 `luac.exe` 编译器
- 提供一个简单的测试脚本验证编译结果可正常运行

## Capabilities

### New Capabilities
- `lua-build`: 从源码编译 Lua 5.4，生成可执行文件
- `lua-run-script`: 使用编译出的 lua.exe 运行 .lua 脚本

## Impact

- `lua/` — Lua 5.4 源码
- `CMakeLists.txt` — CMake 构建配置
- `test/hello.lua` — 验证用测试脚本
- `README.md` — 构建说明