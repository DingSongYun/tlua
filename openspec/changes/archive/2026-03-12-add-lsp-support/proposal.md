## Why

TypingLua 当前只能通过命令行工具（`tluac`/`tlua`）使用，开发者在 VSCode 中编辑 `.tlua` 文件时没有实时反馈——既没有语法高亮，也不会在编写代码时提示语法错误。这导致开发体验远逊于成熟的类型化语言。通过复用现有的 Lua 语言服务器（LuaLS）并结合透明转译代理架构，开发者可以获得完整的 Lua 智能功能（补全、Hover、Go-to-definition、诊断等），同时还能获得 TypingLua 类型注解的语法高亮和实时错误检测。

## What Changes

- **新增 tluac `--lsp-erase` 模式**：在现有 transpiler 中添加"空格填充"擦除模式——擦除类型注解时用等长空格替换，确保输出 `.lua` 与源 `.tlua` 保持行号和列号完全 1:1 对齐。
- **新增诊断收集 API**（`tlua_check`）：运行 lexer/parser 但不生成输出，仅收集类型注解语法错误的位置和消息。
- **VSCode 扩展**（`vscode-tlua`）：提供 `.tlua` 文件的 TextMate 语法高亮，集成 LuaLS 作为 Lua 智能后端，管理透明转译流程，合并 LuaLS 诊断与 TypingLua 自有诊断。
- **不再新增自定义 LSP 服务器**：不实现自定义 JSON-RPC 解析、不实现自定义 LSP 协议处理，全部复用 LuaLS。

## Capabilities

### New Capabilities
- `lsp-proxy`: 透明转译代理——在 VSCode 扩展中拦截 `.tlua` 文件事件，通过 tluac 空格填充模式转译为等长 `.lua`，喂给 LuaLS 获取完整 Lua 智能功能，合并诊断结果。
- `lsp-diagnostics`: TypingLua 自有诊断——调用 `tlua_check` 对 `.tlua` 源码进行类型注解语法检查，报告类型注解区域的语法错误。
- `vscode-extension`: VSCode 扩展——TextMate 语法高亮定义（`.tlua` 文件关联）、LuaLS 集成与代理逻辑、扩展打包与安装。

### Modified Capabilities
- `tlua-transpile`: 新增 `--lsp-erase` 空格填充模式（`tlua_transpile_lsp`），擦除类型注解时用等长空格替换而非删除。
- `lua-build`: CMake 构建配置不需要新增 LSP 服务器目标（复用 LuaLS），但需确保 tluac 支持新的 `--lsp-erase` 参数。

## Impact

- **修改源文件**：`tlua/tlua_parser.c`（添加空格填充模式）、`tlua/tlua_parser.h`（新增 API 声明）、`tlua/tlua_main.c`（添加 `--lsp-erase` CLI 参数）。
- **新增目录**：`vscode-tlua/`（VSCode 扩展项目，含 `package.json`、`syntaxes/`、`src/`）。
- **依赖**：C 端不引入外部依赖；VSCode 扩展依赖 `vscode-languageclient` npm 包和 LuaLS 扩展（`sumneko.lua`）。
- **不再新增**：`tlua_lsp.c`、`tlua_json.c` 等自定义 LSP 实现文件。
- **测试**：新增空格填充模式的单元测试，验证行列号对齐。