## Why

Lua 缺乏原生类型标注能力，现有的注释方案（如 EmmyLua `---@type`）虽然兼容标准 Lua，但冗余度高——每个变量/参数都需要额外的注释行，与代码分离，阅读和维护成本大。TypingLua 的目标是提供类似 Python type hint / TypeScript 的内联类型标注体验：在代码中直接书写类型，然后通过转译器（transpiler）剥离类型标注生成标准 Lua 5.4 代码。

## What Changes

- 定义 `.tlua` 文件格式：TypingLua 源文件扩展名，包含内联类型标注的 Lua 超集语法
- 定义冒号风格的内联类型标注语法：
  - 变量声明：`local x: number = 42`
  - 函数参数：`function add(a: number, b: number)`
  - 函数返回值：`function add(a: number, b: number): number`
- 定义完整的类型表达式系统：基础类型（11 种）、联合类型 `|`、可选类型 `?`、数组类型 `[]`、表泛型 `table<K,V>`、函数签名 `fun()`
- 提供 EBNF 形式化语法定义
- 提供参考文档和示例
- **BREAKING**：废弃之前的纯注释方案（`---@type`），转为内联语法 + 转译器模式

## Capabilities

### New Capabilities
- `tlua-syntax`: TypingLua `.tlua` 文件的完整语法规范，包括内联类型标注语法、类型表达式、EBNF 语法定义
- `tlua-transpile`: `.tlua` → `.lua` 转译管线的行为规范（当前阶段仅定义语法剥离规则，不含类型检查）

### Modified Capabilities
- `lua-run-script`: 转译后的 `.lua` 输出文件必须能被已编译的 `lua.exe` 正常执行

## Impact

- **新文件格式**：引入 `.tlua` 扩展名，IDE/编辑器需配置语法高亮支持
- **工具链**：后续需实现转译器（lexer → parser → codegen），将 `.tlua` 转为 `.lua`
- **现有示例**：`examples/` 目录下的 `.lua` 示例将新增对应的 `.tlua` 版本
- **向后兼容**：转译后的 `.lua` 输出必须是标准 Lua 5.4，可被任何 Lua 解释器执行
