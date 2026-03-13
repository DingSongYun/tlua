## 1. EBNF 语法规范文档

- [x] 1.1 创建 `docs/type-syntax.ebnf` 文件，定义完整的类型注解 EBNF 语法，包含 `annotation`、`type_annotation`、`param_annotation`、`return_annotation` 产生式
- [x] 1.2 在 EBNF 中定义所有基础类型的产生式：`base_type = "nil" | "boolean" | "number" | "integer" | "string" | "table" | "function" | "thread" | "userdata" | "any" | "void"`
- [x] 1.3 在 EBNF 中定义复合类型产生式：`union_type`（`|` 操作符）、`optional_type`（`?` 后缀）、`array_type`（`[]` 后缀）
- [x] 1.4 在 EBNF 中定义 `table_type` 产生式：`"table" "<" type_expr "," type_expr ">"`
- [x] 1.5 在 EBNF 中定义 `function_type` 产生式：`"fun" "(" param_list ")" ":" type_expr`
- [x] 1.6 验证 EBNF 语法的无歧义性，确保每种合法的类型注解字符串只有唯一的解析树

## 2. 类型系统参考手册

- [x] 2.1 创建 `docs/type-reference.md`，编写文档头部（项目介绍、适用范围、与 LuaLS 的兼容性说明）
- [x] 2.2 编写注解前缀规范章节：`---@` 前缀的精确格式定义（三短线 + 无空格 + `@`）
- [x] 2.3 编写 `---@type` 指令章节：语法格式、变量关联规则、局部/全局变量示例
- [x] 2.4 编写 `---@param` 指令章节：语法格式、参数名匹配规则、多参数注解示例、名称不匹配的诊断行为
- [x] 2.5 编写 `---@return` 指令章节：语法格式、单返回值/多返回值/void 返回的示例
- [x] 2.6 编写基础类型章节：列出所有 11 种基础类型关键字，各自含义及与 Lua `type()` 的对应关系
- [x] 2.7 编写复合类型章节：联合类型 `|`、可选类型 `?`、数组类型 `[]`、表泛型 `table<K,V>`、函数签名 `fun()` 的语法和示例
- [x] 2.8 编写向后兼容性章节：说明注解嵌套在注释中、不影响标准 Lua 执行的保证

## 3. 使用示例

- [x] 3.1 创建 `examples/basic_types.lua`：演示所有基础类型的 `---@type` 注解
- [x] 3.2 创建 `examples/function_annotations.lua`：演示 `---@param` 和 `---@return` 的各种用法（单参数、多参数、void 返回、多返回值）
- [x] 3.3 创建 `examples/composite_types.lua`：演示联合类型、可选类型、数组类型、表泛型、函数签名类型
- [x] 3.4 创建 `examples/real_world.lua`：一个综合示例，模拟真实项目中多种注解组合使用的场景

## 4. 验证与测试

- [x] 4.1 使用编译好的 `lua.exe` 执行所有 `examples/` 目录下的示例文件，确认它们在标准 Lua 5.4 下可正常运行
- [x] 4.2 审查 EBNF 语法与 spec 中 13 个需求的覆盖对应关系，确认每个需求都有对应的语法产生式（13/13 ✅）
- [x] 4.3 审查示例文件与 spec 中 27 个场景的覆盖情况，确认关键场景均有示例代码演示（24/27 ✅，3 个诊断场景待解析器实现）
