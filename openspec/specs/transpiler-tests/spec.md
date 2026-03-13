## Requirements

### Requirement: 局部变量类型擦除测试

转译器测试 SHALL 验证所有局部变量声明中的类型注解被正确擦除。

#### Scenario: 单个局部变量带类型和初始值

- **WHEN** 输入 `local x: number = 42` 被转译
- **THEN** 输出 SHALL 为 `local x = 42`

#### Scenario: 单个局部变量带类型无初始值

- **WHEN** 输入 `local name: string` 被转译
- **THEN** 输出 SHALL 为 `local name`

#### Scenario: 多个局部变量各带类型

- **WHEN** 输入 `local a: string, b: number = "hi", 1` 被转译
- **THEN** 输出 SHALL 为 `local a, b = "hi", 1`

#### Scenario: 部分局部变量带类型

- **WHEN** 输入 `local a: string, b = "hi", 1` 被转译
- **THEN** 输出 SHALL 为 `local a, b = "hi", 1`

### Requirement: 全局变量类型擦除测试

转译器测试 SHALL 验证全局变量赋值中的类型注解被正确擦除。

#### Scenario: 全局变量带类型

- **WHEN** 输入 `x: number = 42` 被转译
- **THEN** 输出 SHALL 为 `x = 42`

### Requirement: 函数参数类型擦除测试

转译器测试 SHALL 验证函数参数中的类型注解被正确擦除。

#### Scenario: 单参数带类型

- **WHEN** 输入 `function square(x: number)` 被转译
- **THEN** 输出 SHALL 为 `function square(x)`

#### Scenario: 多参数带类型

- **WHEN** 输入 `function add(a: number, b: number)` 被转译
- **THEN** 输出 SHALL 为 `function add(a, b)`

#### Scenario: 混合有类型和无类型参数

- **WHEN** 输入 `function f(a: number, b, c: string)` 被转译
- **THEN** 输出 SHALL 为 `function f(a, b, c)`

#### Scenario: 匿名函数参数带类型

- **WHEN** 输入 `local f = function(x: number)` 被转译
- **THEN** 输出 SHALL 为 `local f = function(x)`

#### Scenario: local 函数参数带类型

- **WHEN** 输入 `local function f(a: number)` 被转译
- **THEN** 输出 SHALL 为 `local function f(a)`

### Requirement: 函数返回类型擦除测试

转译器测试 SHALL 验证函数声明中的返回类型注解被正确擦除。

#### Scenario: 单返回类型

- **WHEN** 输入 `function greet(): string` 被转译
- **THEN** 输出 SHALL 为 `function greet()`

#### Scenario: 参数和返回类型同时存在

- **WHEN** 输入 `function add(a: number, b: number): number` 被转译
- **THEN** 输出 SHALL 为 `function add(a, b)`

#### Scenario: 多返回类型

- **WHEN** 输入 `function get_info(): number, string` 被转译
- **THEN** 输出 SHALL 为 `function get_info()`

#### Scenario: 带括号的多返回类型

- **WHEN** 输入 `function get_info(): (number, string)` 被转译
- **THEN** 输出 SHALL 为 `function get_info()`

#### Scenario: 匿名函数带返回类型

- **WHEN** 输入 `local f = function(x: number): string` 被转译
- **THEN** 输出 SHALL 为 `local f = function(x)`

#### Scenario: void 返回类型

- **WHEN** 输入 `function do_nothing(): void` 被转译
- **THEN** 输出 SHALL 为 `function do_nothing()`

### Requirement: 复杂类型表达式擦除测试

转译器测试 SHALL 验证联合类型、可选类型、数组类型、泛型 table 类型和函数签名类型均被正确擦除。

#### Scenario: 联合类型

- **WHEN** 输入 `local x: number|string = 42` 被转译
- **THEN** 输出 SHALL 为 `local x = 42`

#### Scenario: 多联合类型

- **WHEN** 输入 `local x: number|string|boolean` 被转译
- **THEN** 输出 SHALL 为 `local x`

#### Scenario: 带空格的联合类型

- **WHEN** 输入 `local x: number | string` 被转译
- **THEN** 输出 SHALL 为 `local x`

#### Scenario: 可选类型

- **WHEN** 输入 `local name: string? = nil` 被转译
- **THEN** 输出 SHALL 为 `local name = nil`

#### Scenario: 数组类型

- **WHEN** 输入 `local nums: number[] = {1, 2, 3}` 被转译
- **THEN** 输出 SHALL 为 `local nums = {1, 2, 3}`

#### Scenario: 嵌套数组类型

- **WHEN** 输入 `local matrix: number[][] = {{1,2},{3,4}}` 被转译
- **THEN** 输出 SHALL 为 `local matrix = {{1,2},{3,4}}`

#### Scenario: 可选数组类型

- **WHEN** 输入 `local items: string[]?` 被转译
- **THEN** 输出 SHALL 为 `local items`

#### Scenario: 泛型 table 类型

- **WHEN** 输入 `local scores: table<string, number> = {}` 被转译
- **THEN** 输出 SHALL 为 `local scores = {}`

#### Scenario: 嵌套泛型 table 类型

- **WHEN** 输入 `local config: table<string, table<string, number>>` 被转译
- **THEN** 输出 SHALL 为 `local config`

#### Scenario: 组合复杂类型

- **WHEN** 输入 `local items: table<string, number[]>? = nil` 被转译
- **THEN** 输出 SHALL 为 `local items = nil`

#### Scenario: 函数签名类型

- **WHEN** 输入 `local cb: fun(x: number): string` 被转译
- **THEN** 输出 SHALL 为 `local cb`

#### Scenario: 无参函数签名类型

- **WHEN** 输入 `local action: fun(): void` 被转译
- **THEN** 输出 SHALL 为 `local action`

### Requirement: 代码保持性测试

转译器测试 SHALL 验证注释、字符串字面量、缩进和空行在转译后不被修改。

#### Scenario: 行注释保持

- **WHEN** 输入包含 `-- this is a comment`
- **THEN** 输出 SHALL 包含完全相同的注释文本

#### Scenario: 块注释保持

- **WHEN** 输入包含 `--[[ block comment ]]`
- **THEN** 输出 SHALL 包含完全相同的块注释

#### Scenario: 字符串中的类型注解文本不被修改

- **WHEN** 输入 `local s: string = "local x: number = 42"` 被转译
- **THEN** 输出 SHALL 为 `local s = "local x: number = 42"`（字符串内容不变）

#### Scenario: 多行字符串中的类型注解文本不被修改

- **WHEN** 输入包含多行字符串 `[[ local x: number ]]`
- **THEN** 字符串内容 SHALL 不被修改

#### Scenario: 缩进保持

- **WHEN** 输入包含带缩进的类型注解代码
- **THEN** 输出 SHALL 保持完全相同的缩进结构

#### Scenario: 空行保持

- **WHEN** 输入包含语句间的空行
- **THEN** 输出 SHALL 保持这些空行

### Requirement: 纯 Lua 透传测试

转译器测试 SHALL 验证不包含类型注解的标准 Lua 代码在转译后保持不变。

#### Scenario: 纯 Lua 文件透传

- **WHEN** 输入是不含任何类型注解的标准 Lua 5.4 代码
- **THEN** 输出 SHALL 与输入完全相同

#### Scenario: 方法调用不被误识别

- **WHEN** 输入包含 `obj:method(arg)`
- **THEN** 输出 SHALL 保持方法调用语法不变

#### Scenario: 方法定义中冒号不被误识别

- **WHEN** 输入包含 `function MyClass:init(name: string): void`
- **THEN** `MyClass:init` 中的冒号 SHALL 被解析为方法语法
- **AND** `name: string` SHALL 被擦除为 `name`
- **AND** `: void` SHALL 被擦除

### Requirement: 行号保持测试

转译器测试 SHALL 验证转译前后的行数一致（1:1 行映射），以确保调试体验不受影响。

#### Scenario: 输出行数等于输入行数

- **WHEN** 一个多行 `.tlua` 文件被转译
- **THEN** 输出的总行数 SHALL 等于输入的总行数

#### Scenario: 类型擦除不减少行数

- **WHEN** 输入包含跨行的类型注解
- **THEN** 转译器 SHALL 通过适当处理（如保留空行）确保行数不变