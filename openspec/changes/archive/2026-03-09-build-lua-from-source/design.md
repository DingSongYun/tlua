## Context

TypingLua 是一个全新项目，目标是为 Lua 构建强类型语法规范。第一步需要在项目内拥有可编译的 Lua 源码，作为后续修改和扩展的基础。

开发环境：Windows + Visual Studio 2022 Community + CMake。

## Goals / Non-Goals

**Goals:**
- 将 Lua 5.4.7 源码引入项目并能通过 CMake 编译
- 生成可用的 lua.exe 和 luac.exe
- 构建过程简单明了，一条命令即可完成

**Non-Goals:**
- 不修改 Lua 源码（本次只是原样编译）
- 不构建动态链接库（.dll），只做静态编译
- 不支持跨平台构建（先只关注 Windows/MSVC）

## Decisions

### Decision 1: Lua 源码直接放入项目

将 Lua 5.4 的 `src/` 目录内容直接复制到 `lua/` 目录，而非使用 git submodule 或包管理器。

**理由：** 后续我们需要修改 Lua 源码来实现类型系统，直接放入项目最方便管理和修改。

### Decision 2: 使用 CMake 作为构建系统

Lua 官方使用 Makefile，但在 Windows/MSVC 环境下 CMake 是更自然的选择。

**方案：** 手写一个简洁的 `CMakeLists.txt`，定义三个 target：
- `liblua`（静态库）— 核心库文件
- `lua`（可执行文件）— 解释器，链接 liblua + lua.c
- `luac`（可执行文件）— 编译器，链接 liblua + luac.c

### Decision 3: 构建产物放在 build/ 目录

使用 CMake 的 out-of-source 构建，所有中间文件和产物放在 `build/` 目录。该目录应加入 `.gitignore`。