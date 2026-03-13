## Why

Lua 是一门动态类型语言，缺乏原生的类型注解机制。这导致在大型项目中难以进行静态分析、IDE 智能提示和早期错误检测。TypingLua 项目的核心目标是为 Lua 引入类似 Python type hint 的强类型语法规范，而首先需要定义一套清晰、完整的类型注解语法——这是整个类型系统的基石。

## What Changes

- 设计一套基于注释的类型注解语法（使用 `---@` 前缀），兼容标准 Lua 语法，不破坏现有 Lua 代码的可执行性
- 定义基础类型关键字：`number`、`string`、`boolean`、`nil`、`table`、`function`、`any`、`void`
- 定义变量类型注解：`---@type <type>` 用于标注变量类型
- 定义函数参数注解：`---@param <name> <type>` 用于标注函数参数
- 定义函数返回值注解：`---@return <type>` 用于标注返回值
- 定义复合类型语法：联合类型 `type1|type2`、可选类型 `type?`、数组类型 `type[]`
- 编写语法规范文档（BNF/EBNF 格式），作为后续解析器实现的依据
- 提供示例 Lua 文件展示类型注解的实际使用方式

## Capabilities

### New Capabilities
- `type-annotation-syntax`: 类型注解的语法规范定义，包括基础类型、复合类型、注解指令的完整语法
- `type-annotation-examples`: 类型注解的使用示例集，演示各种注解场景

### Modified Capabilities
<!-- 无已有能力需要修改 -->

## Impact

- `docs/type-syntax.md` — 新增：类型注解语法规范文档（BNF/EBNF）
- `docs/type-reference.md` — 新增：类型系统参考手册
- `examples/` — 新增：带类型注解的示例 Lua 文件
- 后续影响：此语法规范将指导 lexer/parser 的修改（`lua/llex.c`、`lua/lparser.c`）