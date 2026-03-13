# TypingLua — `.tlua` 内联类型标注参考手册

> **版本**: 2.0  
> **适用范围**: Lua 5.4  
> **语法风格**: 内联冒号标注（类似 TypeScript / Python type hints）  
> **工作流**: `.tlua` 源文件 → 转译（Type Erasure）→ `.lua` 输出文件

---

## 1. 概述

TypingLua 使用 `.tlua` 文件扩展名来定义带有内联类型标注的 Lua 代码。通过转译器（transpiler），类型标注会被**擦除（Type Erasure）**，生成标准的 Lua 5.4 代码。

**核心理念：**

| 特性 | 说明 |
|------|------|
| 文件扩展名 | `.tlua`（源文件），`.lua`（转译输出） |
| 标注语法 | 冒号 `:` 后跟类型表达式 |
| 转译策略 | Type Erasure — 剥离所有类型标注 |
| Lua 兼容性 | 转译输出为合法 Lua 5.4 代码 |

**对比旧版（v1.0 注释风格）：**

```lua
-- v1.0 (archived): 基于注释
---@type number
local x = 42
---@param a number
---@param b number
---@return number
local function add(a, b)
    return a + b
end
```

```
-- v2.0: 内联冒号标注 (.tlua)
local x: number = 42
local function add(a: number, b: number): number
    return a + b
end
```

---

## 2. 变量类型标注

### 2.1 局部变量

在变量名后使用 `:` 指定类型：

```
local <name>: <type> = <value>
```

**示例：**

```lua
-- 基本类型
local x: number = 42
local name: string = "hello"
local flag: boolean = true

-- 可以省略初始值（类型仍然有效）
local count: number

-- 无类型标注（等价于 any）
local y = 100
```

### 2.2 多变量声明

每个变量可以独立标注类型：

```lua
local a: string, b: number = "hi", 1
local x: number, y: number, z: number = 1, 2, 3
```

> **注意**：逗号是变量分隔符。联合类型使用 `|`（如 `number|string`），不会产生歧义。

### 2.3 全局变量

全局变量同样使用冒号标注，但**必须**同时带有赋值：

```lua
name: string = "hello"
count: number = 0
```

> **限制**：全局类型标注仅适用于简单的 `Name` 赋值，不适用于表字段赋值（如 `t.x = 1`）。

---

## 3. 函数类型标注

### 3.1 参数类型

在参数名后使用 `:` 指定类型：

```lua
local function square(x: number): number
    return x * x
end

-- 多参数
local function add(a: number, b: number): number
    return a + b
end

-- 部分参数标注（未标注的参数隐含 any）
local function greet(name: string, times)
    for i = 1, times do
        print("Hello, " .. name)
    end
end
```

### 3.2 返回类型

返回类型在参数列表的 `)` 之后使用 `:` 指定：

```lua
-- 单返回值
local function greet(): string
    return "hello"
end

-- 多返回值（逗号分隔）
local function get_info(): number, string
    return 42, "answer"
end

-- 无返回值
local function do_nothing(): void
    -- nothing here
end
```

### 3.3 多返回值的括号形式（可选）

为了增强可读性，多返回值可以用括号包裹：

```lua
local function get_info(): (number, string)
    return 42, "answer"
end
```

两种写法等价。

### 3.4 所有函数声明形式

所有 Lua 函数声明形式均支持类型标注：

```lua
-- 普通函数声明
function f(a: number): string
    return tostring(a)
end

-- 局部函数声明
local function f(a: number): string
    return tostring(a)
end

-- 表函数 (dot syntax)
function mylib.calc(x: number): number
    return x * 2
end

-- 方法函数 (colon syntax)
function obj:getName(): string
    return self.name
end

-- 匿名函数
local fn = function(x: number): number
    return x + 1
end
```

### 3.5 冒号歧义消解

Lua 方法语法中的 `:` （如 `obj:method`）与类型标注的 `:` 不会冲突：

| 上下文 | 冒号含义 | 示例 |
|--------|---------|------|
| `funcname` 中，`(` 之前 | 方法分隔符 | `function obj:init()` |
| 参数名之后，参数列表内 | 类型标注 | `(x: number)` |
| `)` 之后，函数体之前 | 返回类型标注 | `): string` |

---

## 4. 基础类型

TypingLua 定义 11 种基础类型关键字（区分大小写）：

| 类型关键字 | 含义 | 对应 Lua `type()` |
|-----------|------|-------------------|
| `nil` | 空值 | `"nil"` |
| `boolean` | 布尔值 | `"boolean"` |
| `number` | 数值（含整数和浮点数） | `"number"` |
| `integer` | 整数子类型（Lua 5.4） | `"number"` |
| `string` | 字符串 | `"string"` |
| `table` | 表（无泛型约束） | `"table"` |
| `function` | 函数（无签名约束） | `"function"` |
| `thread` | 协程 | `"thread"` |
| `userdata` | 用户数据 | `"userdata"` |
| `any` | 任意类型（跳过类型检查） | — |
| `void` | 无返回值（仅用于返回类型） | — |

**子类型关系**：`integer` 是 `number` 的子类型。

---

## 5. 复合类型

### 5.1 联合类型 (`|`)

表示值可以是多种类型之一：

```lua
local id: number|string = "abc"
local flexible: number | string | boolean = true  -- 管道符两侧允许空格
```

### 5.2 可选类型 (`?`)

`T?` 等价于 `T|nil`：

```lua
local name: string? = nil
local function register(callback: function?)
    if callback then callback() end
end
```

### 5.3 数组类型 (`[]`)

`T[]` 等价于 `table<integer, T>`：

```lua
local scores: number[] = {95, 87, 92}
local matrix: string[][] = {{"a", "b"}, {"c", "d"}}   -- 嵌套数组
local maybe_list: number[]? = nil                       -- 可选数组
```

### 5.4 表泛型类型 (`table<K, V>`)

```lua
local ages: table<string, number> = {Alice = 30, Bob = 25}
local nested: table<string, table<string, number>> = {
    engineering = {Alice = 30},
}
```

### 5.5 函数签名类型 (`fun()`)

```lua
local formatter: fun(x: number): string = function(x) return tostring(x) end
local cleanup: fun(): void = function() end
local validator: fun(a: number, b: string): boolean
```

### 5.6 括号分组

当联合类型与后缀操作符组合时，使用括号明确优先级：

```lua
local a: (number|string)[]    -- number 或 string 的数组
local b: number|string[]      -- number 或 string 数组（string[] 优先级更高）
```

---

## 6. 转译（Type Erasure）

### 6.1 转译原理

转译器读取 `.tlua` 文件，剥离所有类型标注，输出合法的 `.lua` 文件。

**转译前（`.tlua`）：**

```lua
local x: number = 42
local function add(a: number, b: number): number
    return a + b
end
```

**转译后（`.lua`）：**

```lua
local x = 42
local function add(a, b)
    return a + b
end
```

### 6.2 转译规则

| 源构造 | 擦除行为 |
|--------|---------|
| `local x: T = v` | → `local x = v` |
| `local x: T` | → `local x` |
| `name: T = v` | → `name = v` |
| `function f(p: T): R` | → `function f(p)` |
| `fun(p: T): R` (类型表达式中) | 整个类型表达式被移除 |

### 6.3 保证

> 转译后的 `.lua` 文件在运行时行为与手动移除所有类型标注后的代码**完全一致**。

---

## 7. 形式化语法

完整的 EBNF 语法定义参见 [`docs/tlua-syntax.ebnf`](tlua-syntax.ebnf)。

---

## 8. 当前限制与未来扩展

以下特性**不在当前版本范围内**，将在后续版本中支持：

- 类定义（`class`）
- 类型别名（`type alias`）
- 表字段类型（结构体）
- 函数重载
- 泛型
- 枚举类型
- 字面量类型（如 `"hello"` 或 `42`）
- 静态类型检查（当前仅做 Type Erasure）
