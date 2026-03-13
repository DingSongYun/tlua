## Context

TypingLua 项目的核心目标是让 Lua 开发者获得类似 Python / TypeScript 的内联类型标注体验。前一个迭代（已归档）基于 `---@type` 注释方案，虽然 100% 兼容标准 Lua，但冗余度高、与代码分离。现在转向**转译器模式**：用户编写 `.tlua` 源文件（Lua 超集），通过转译器剥离类型标注输出标准 `.lua` 文件。

当前阶段的重点是**定义语法规范**，不涉及转译器的具体实现代码。

现有基础设施：
- Lua 5.4 源码已编译（`build/Release/lua.exe`）
- 项目使用 CMake 构建系统
- 已有归档的注释方案（EBNF、参考文档、示例）可作为类型表达式部分的参考

## Goals / Non-Goals

**Goals:**
- 定义 `.tlua` 文件格式的完整语法，作为 Lua 5.4 的严格超集
- 支持变量声明、函数参数、函数返回值三个位置的冒号风格内联类型标注
- 支持完整的类型表达式（基础类型、联合、可选、数组、表泛型、函数签名）
- 提供 EBNF 形式化语法定义，覆盖类型标注的注入点和类型表达式
- 定义转译规则：如何从 `.tlua` 剥离类型标注生成合法 `.lua`
- 提供 `.tlua` 示例文件及其对应的预期 `.lua` 转译输出

**Non-Goals:**
- 不实现转译器代码（lexer / parser / codegen），仅定义语法和规则
- 不实现静态类型检查，类型标注当前仅用于文档和未来的分析工具
- 不支持 `@class`、`@field`、`@generic`、`@overload` 等高级指令（留待未来扩展）
- 不修改 Lua 解释器本身
- 不定义 `.tlua` 的 IDE 语法高亮方案

## Decisions

### Decision 1: 冒号风格内联语法（TypeScript/Python 风格）

**选择**：`local x: number = 42`，`function f(a: number): string`

**备选方案**：
- `as` 关键字风格：`local x as number = 42` — 需要引入新关键字，与 Lua 标识符空间冲突
- 箭头风格：`local x -> number = 42` — `->` 在其他语言中通常表示返回类型，用于变量声明会产生语义混乱
- 注释方案：`---@type number` — 已被否决（冗余、与代码分离）

**理由**：冒号风格是 TypeScript、Python、Kotlin、Swift 等现代语言的事实标准，学习成本最低，生态认知度最高。Lua 中冒号 `:` 仅用于方法调用（`obj:method()`），在变量声明和函数签名的上下文中不存在歧义。

### Decision 2: `.tlua` 作为专用文件扩展名

**选择**：使用 `.tlua` 扩展名区分源文件和输出文件

**备选方案**：
- `.tl`：与 Teal 语言冲突
- `.lua.t` / `.luax`：非常规，IDE 支持差
- 直接使用 `.lua`：无法区分源文件和转译输出

**理由**：`.tlua` 明确表达 "Typed Lua" 语义，不与现有生态冲突，且可以为 IDE 编辑器注册独立的文件类型关联。

### Decision 3: 类型标注的注入点

**选择**：在以下三个语法位置支持类型标注：

1. **变量声明**（local / global）：
   ```
   local x: number = 42
   local a: string, b: number = "hi", 1
   x: number = 42          -- 全局赋值（首次赋值时标注）
   ```

2. **函数参数**：
   ```
   function f(a: number, b: string)
   local g = function(x: number)
   ```

3. **函数返回值**：
   ```
   function f(a: number): string
   local g = function(): number, string
   ```

**不支持的位置**（Non-Goal for v1）：
- 表达式级别的类型断言（如 TypeScript 的 `x as number`）
- 表字段类型（如 `{ name: string = "hi" }`）——与 Lua 的表构造器 `{[key] = value}` 语法冲突

**理由**：这三个位置覆盖了日常开发中 90%+ 的类型标注需求，且语法扩展最小。表字段和类型断言涉及更复杂的语法歧义处理，留待后续版本。

### Decision 4: 类型表达式复用归档方案

**选择**：类型表达式体系（基础类型、联合、可选、数组、泛型表、函数签名）直接复用之前归档方案的设计

**理由**：类型表达式本身与标注方式（注释 vs 内联）无关。之前已经过充分设计和 EBNF 验证的 11 种基础类型、`T|U`、`T?`、`T[]`、`table<K,V>`、`fun(...)` 语法可以直接搬用，无需重新设计。

### Decision 5: 转译策略 — 纯语法剥离

**选择**：转译器仅做"类型擦除"——删除所有类型标注，保留其余代码不变

**转译规则**：
| `.tlua` 输入 | `.lua` 输出 |
|---|---|
| `local x: number = 42` | `local x = 42` |
| `local a: string, b: number = "hi", 1` | `local a, b = "hi", 1` |
| `function f(a: number, b: string): boolean` | `function f(a, b)` |
| `local g = function(x: number): string` | `local g = function(x)` |

**理由**：类型擦除是最简单、最可靠的转译策略，等价于 TypeScript 的 `--isolatedModules` 模式。不需要全局类型推断，可以逐文件独立转译。

### Decision 6: 冒号歧义处理

**潜在歧义**：Lua 使用 `:` 作为方法调用语法糖（`obj:method()`），需要确保类型标注不与之冲突。

**分析**：
- 方法调用 `obj:method()` 出现在**表达式**中
- 类型标注 `x: number` 出现在 `local` 声明和 `function` 参数列表中
- 两者的语法上下文完全不同，不存在歧义

**函数返回值标注**的特殊处理：
- `function f(): number` — 这里 `)` 后的 `:` 明确是返回值类型标注
- 在标准 Lua 中 `)` 后只能跟函数体，不存在 `:` 的合法用法

**结论**：冒号在类型标注场景中不存在语法歧义。

## Risks / Trade-offs

### Risk 1: 多返回值类型标注的解析复杂度
**风险**：`function f(): number, string` 中逗号可能被误解析为语句分隔而非类型列表
**缓解**：在 EBNF 中明确定义返回值类型列表的产生式，要求 `:` 后的类型列表在遇到换行或 `end`/语句边界时终止

### Risk 2: 与 Teal 语言的生态位重叠
**风险**：Teal（`.tl`）已经是一个成熟的 Typed Lua 方案，TypingLua 可能面临"重复造轮子"的质疑
**缓解**：TypingLua 的定位是**轻量级类型标注**（类似 Python 的 type hint），不要求完整的类型系统和类型检查。与 Teal 的全功能类型系统不同，TypingLua 是渐进式的、可选的

### Risk 3: 类型标注位置的扩展性
**风险**：v1 仅支持三个标注位置（变量、参数、返回值），用户可能期望更多（如表字段、类型断言）
**缓解**：EBNF 语法设计时预留扩展点，确保未来添加新标注位置不会破坏现有语法

### Risk 4: 多重赋值中的类型标注语法
**风险**：`local a: string, b: number = "hi", 1` 中逗号可能引起解析歧义——逗号属于类型表达式（联合类型？）还是变量列表分隔符？
**缓解**：明确规定在多重赋值上下文中，逗号始终作为变量列表分隔符。如果需要联合类型，必须使用管道符 `|` 而非逗号。这与 TypeScript 的处理方式一致。

## Open Questions

1. **可选类型标注**：是否所有变量/参数都必须标注类型？还是支持部分标注（未标注的默认为 `any`）？
   → 倾向于：类型标注完全可选，未标注等同于 `any`（渐进式类型系统）

2. **多返回值的括号包裹**：`function f(): (number, string)` 是否需要用括号包裹多返回值类型？
   → 倾向于：不强制括号，但允许括号作为可选的可读性增强

3. **转译输出格式**：是否保留源码中的空白和注释？还是输出最小化的 Lua？
   → 倾向于：保留原始格式，仅删除类型标注部分，保持可读性和可调试性
