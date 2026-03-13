# TypingLua (.tlua) 调试方案

> 本文档记录了在 VSCode 中使用 `actboy168.lua-debug` 扩展调试 `.tlua` 文件的完整方案。
> 编写日期：2026-03-09

---

## 目录

1. [背景与目标](#1-背景与目标)
2. [方案概述](#2-方案概述)
3. [项目构建修改](#3-项目构建修改)
4. [运行时路径修改](#4-运行时路径修改)
5. [lua-debug 扩展修改](#5-lua-debug-扩展修改)
   - 5.1 [parser.lua — 断点行信息解析](#51-parserlua--断点行信息解析)
   - 5.2 [breakpoint.lua — 传递文件路径](#52-breakpointlua--传递文件路径)
   - 5.3 [source.lua — 源文件路径映射](#53-sourcelua--源文件路径映射)
6. [launch.json 配置](#6-launchjson-配置)
7. [注意事项](#7-注意事项)

---

## 1. 背景与目标

TypingLua (`.tlua`) 是带有类型注解的 Lua 方言。`tluac` 编译器会将 `.tlua` 转译为标准 `.lua` 文件，**转译过程保持 1:1 行号映射**（类型注解被擦除，行号不变）。

**目标**：在 VSCode 中直接对 `.tlua` 文件设置断点，命中后在 `.tlua` 源码中查看和调试，获得与调试普通 `.lua` 完全一致的体验。

**核心挑战**：
- `lua-debug` 扩展通过 DLL 注入方式 hook Lua 运行时，需要**共享库** (`lua54.dll`)
- 调试器使用 `load(content)` 验证源码行信息，`.tlua` 的类型注解语法会导致解析失败
- 调试器的路径映射需要将运行时的 `.lua` 路径映射到编辑器中的 `.tlua` 路径

---

## 2. 方案概述

```
┌─────────────────────────────────────────────────────┐
│  VSCode Editor                                      │
│  ┌─────────────┐                                    │
│  │ demo.tlua   │  ← 用户在此设置断点                │
│  │ (带类型注解) │                                    │
│  └──────┬──────┘                                    │
│         │                                           │
│  ┌──────▼──────────────────────────────────┐        │
│  │ lua-debug 扩展 (已修改)                 │        │
│  │                                         │        │
│  │ 1. parser.lua: 用 demo.lua 验证行信息   │        │
│  │ 2. source.lua: .lua → .tlua 路径映射    │        │
│  └──────┬──────────────────────────────────┘        │
│         │                                           │
└─────────┼───────────────────────────────────────────┘
          │
  ┌───────▼───────┐     ┌──────────────┐
  │  tlua.exe     │────▶│  lua54.dll   │  ← 共享库，供调试器 hook
  │  (运行时)     │     │  (Lua 引擎)  │
  └───────────────┘     └──────────────┘
          │
  ┌───────▼───────┐
  │  demo.lua     │  ← 实际执行的转译文件
  │  (无类型注解)  │
  └───────────────┘
```

关键点：由于 tluac 保持 **1:1 行号映射**，`.lua` 的行信息可以直接用于 `.tlua`。

---

## 3. 项目构建修改

### CMakeLists.txt

将 `lua54` 从**静态库**改为**共享库 (DLL)**，这样 `lua-debug` 才能找到 Lua 符号并注入调试钩子。

```cmake
# 修改前
add_library(lua54 STATIC ${LUA_SRC})

# 修改后
add_library(lua54 SHARED ${LUA_SRC})
```

同时需要确保 `tlua.exe` 链接到 `lua54.dll`（动态链接）。

---

## 4. 运行时路径修改

### src/tlua_run.c

调试器使用**绝对路径**来匹配断点位置，而 `luaL_loadfile` 默认使用调用时传入的路径（可能是相对路径）。需要在加载文件前将路径转换为绝对路径：

```c
#include <windows.h>

// 在执行 luaL_loadfile 前，将相对路径转换为绝对路径
char absPath[MAX_PATH];
DWORD len = GetFullPathNameA(filename, MAX_PATH, absPath, NULL);
if (len > 0 && len < MAX_PATH) {
    filename = absPath;
}
// 然后使用 filename 调用 luaL_loadfile
```

这确保了 chunk name（以 `@` 开头）中的路径与 VSCode 编辑器中打开的文件路径一致。

---

## 5. lua-debug 扩展修改

> ⚠️ 扩展更新后需重新应用以下修改！
>
> 扩展路径（典型位置）：
> `%USERPROFILE%\.vscode\extensions\actboy168.lua-debug-<version>\`
>
> 需要修改的文件位于扩展目录下的 `script/backend/worker/` 子目录。

### 5.1 parser.lua — 断点行信息解析

**文件**：`script/backend/worker/parser.lua`

**问题**：调试器使用 `load(content)` 编译源码来提取有效的行号信息。`.tlua` 文件包含类型注解（如 `: number`），这不是合法的 Lua 语法，`load` 会报错，导致断点无法被验证（显示为空心灰色圆圈）。

**修改方案**：当 `load(content)` 失败时，检查文件路径是否以 `.tlua` 结尾。如果是，尝试加载同目录下同名的 `.lua` 文件来获取行信息。

```lua
-- 原始代码（大致结构）：
-- return function(content)
--     local f = load(content)
--     ...
-- end

-- 修改后：
return function(content, filepath)
    local f, err = load(content)
    if not f and filepath then
        -- 如果是 .tlua 文件且 load 失败，尝试加载对应的 .lua 文件
        local luaPath = filepath:gsub("%.tlua$", ".lua")
        if luaPath ~= filepath then
            local fh = io.open(luaPath, "r")
            if fh then
                local luaContent = fh:read("*a")
                fh:close()
                f = load(luaContent)
            end
        end
    end
    if not f then
        return nil
    end
    -- ... 后续行信息提取逻辑不变 ...
end
```

### 5.2 breakpoint.lua — 传递文件路径

**文件**：`script/backend/worker/breakpoint.lua`

**问题**：原始 `parser(content)` 调用没有传递文件路径，修改后的 `parser` 需要路径来判断是否需要回退到 `.lua` 文件。

**修改方案**：在 `calcLineInfo` 函数中，将 `src.path` 传给 `parser`：

```lua
-- 原始代码：
-- local lineInfo = parser(content)

-- 修改后：
local lineInfo = parser(content, src.path)
```

### 5.3 source.lua — 源文件路径映射

**文件**：`script/backend/worker/source.lua`

**问题**：运行时加载的是 `.lua` 文件，调试器将此路径报告给 VSCode。但用户希望在 `.tlua` 文件中看到断点命中。需要将服务端的 `.lua` 路径自动映射到客户端的 `.tlua` 路径。

**修改方案**：在 `serverPathToClientPath` 函数中（`return` 之前），插入 `.lua → .tlua` 映射逻辑：

```lua
-- 在函数 serverPathToClientPath 的最终 return 之前，加入以下代码：

-- .tlua 映射：如果存在同名的 .tlua 文件，优先返回 .tlua 路径
local tluaPath = path:gsub("%.lua$", ".tlua")
if tluaPath ~= path then
    local fh = io.open(tluaPath, "r")
    if fh then
        fh:close()
        path = tluaPath
        nativePath = fs.source_native(path)
    end
end
return skip, covertPath(path)
```

> **注意**：确保最终 `return` 语句使用的是映射后的 `path`（即 `covertPath(path)`），而不是映射前的旧变量。

---

## 6. launch.json 配置

```jsonc
{
    "version": "0.2.0",
    "configurations": [
        {
            "type": "lua",
            "request": "launch",
            "name": "Debug TLua Script",
            "program": "${workspaceFolder}/build/Debug/tlua.exe",
            "arg": [
                "${workspaceFolder}/examples/demo.lua"
            ],
            "cwd": "${workspaceFolder}",
            "stopOnEntry": false,
            "luaVersion": "lua54",
            "luaArch": "x86_64"
        }
    ]
}
```

> **注意**：`arg` 中传入的是转译后的 `.lua` 文件路径（运行时实际执行的文件）。
> 扩展修改会自动将调试界面映射回 `.tlua` 文件。
> **不需要** 配置 `sourceMaps`（扩展内置逻辑的 glob 替换有 bug，会产生错误映射）。

---

## 7. 注意事项

### 扩展更新
`lua-debug` 扩展更新后，上述 3 个文件（`parser.lua`、`breakpoint.lua`、`source.lua`）的修改会被覆盖。更新后需要重新应用修改。

建议：
- 保留本文档作为修改参考
- 考虑用脚本自动化 patch 过程（如 `patch` 命令或 Python 脚本）

### 1:1 行映射的前提
整个方案依赖 `tluac` 转译器**保持行号一致**。如果未来转译器改变了行号映射策略（如合并行、插入辅助代码），需要引入真正的 source map 机制。

### 构建产物
确保 `lua54.dll` 与 `tlua.exe` 在同一目录下（或在系统 PATH 中），否则运行时找不到 DLL。

### 平台兼容性
- `GetFullPathNameA` 是 Windows API，如需跨平台支持需替换为 `realpath`（POSIX）
- 扩展路径中的 `io.open` 在 Windows 上使用反斜杠，`gsub` 替换 `.lua` → `.tlua` 不受影响

---

## 附录：修改文件清单

| 文件 | 位置 | 修改类型 |
|------|------|----------|
| `CMakeLists.txt` | 项目根目录 | lua54 改为 SHARED |
| `src/tlua_run.c` | 项目源码 | 添加绝对路径转换 |
| `parser.lua` | 扩展 `script/backend/worker/` | 添加 .tlua 回退解析 |
| `breakpoint.lua` | 扩展 `script/backend/worker/` | 传递 filepath 参数 |
| `source.lua` | 扩展 `script/backend/worker/` | 添加 .lua→.tlua 映射 |
| `launch.json` | `.vscode/` | 调试配置（无 sourceMaps） |
