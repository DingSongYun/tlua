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

-- 无返回值（省略返回类型即可）
local function do_nothing()
    -- nothing here
end
```

### 3.3 多返回值的括号形式（可选，仅转译器支持）

为了增强可读性，多返回值可以用括号包裹：

```lua
local function get_info(): (number, string)
    return 42, "answer"
end
```

两种写法等价。

> **注意**：括号形式目前仅 `tluac` 转译器支持。Language Server 使用逗号分隔形式（`): number, string`）。

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

TypingLua 定义 10 种基础类型关键字（区分大小写）：

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

**子类型关系**：`integer` 是 `number` 的子类型。

> **注意**：`void` 已弃用。无返回值的函数不需要声明返回类型，直接省略即可：
> ```lua
> local function doWork(x: number)  -- 无返回类型 = 无返回值
>     print(x)
> end
> ```

### 4.1 自定义类型（User-Defined Types）

除基础类型外，任何标识符都可以作为类型名使用，但**必须**通过 `---@class` 或 `---@alias` 注解预先定义。

```lua
-- 通过 @class 定义类型（通常在独立的类型声明文件中）
---@class Vector3
---@field x number
---@field y number
---@field z number

---@class Player
---@field name string
---@field health number
---@field position Vector3

-- 在业务代码中使用 inline 标注引用
local pos: Vector3 = { x = 0, y = 0, z = 0 }
local player: Player = createPlayer("Alice")
```

#### UE5 集成示例

在 UE5 + UnLua 工程中，使用 UnLua IntelliSense 导出的 stub 文件会自动提供 `@class` 定义：

```lua
-- UnLua 自动导出的 stub（无需手动编写）：
-- ---@class AActor : UObject
-- ---@class UWidget : UVisual
-- ---@class FVector
-- ---@field X number
-- ---@field Y number
-- ---@field Z number

-- 业务代码中直接使用 inline 标注
local actor: AActor = self:GetOwner()
local widget: UWidget = self:GetWidget()
local location: FVector = actor:K2_GetActorLocation()
```

#### 泛型类型

自定义类型支持泛型参数（通过 `<T>` 语法）：

```lua
-- UE 容器类型
local enemies: TArray<AActor> = self:GetEnemies()
local lookup: TMap<string, AActor> = self:GetActorMap()
local items: TSet<UItemData> = self:GetItemSet()

-- 嵌套泛型
local groups: TMap<string, TArray<AActor>> = self:GetGroups()
```

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
local cleanup: fun() = function() end
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

## 8. 混合模式（Inline + EmmyLua 共存）

TypingLua 采用**混合模式**：inline 标注用于变量/参数/返回值的类型声明，而类型定义（`---@class`、`---@alias`）仍使用 EmmyLua 注释语法。

### 8.1 设计理念

```
┌─────────────────────────────────────────────────────────┐
│  类型定义层（Type Definitions）                          │
│  → 使用 EmmyLua 注释：---@class, ---@alias, ---@enum    │
│  → 通常在 stub 文件或文件头部声明                        │
└─────────────────────────────────┬───────────────────────┘
                                  │ 引用
┌─────────────────────────────────▼───────────────────────┐
│  类型引用层（Type Annotations）                          │
│  → 使用 inline 冒号标注：local x: MyType = ...          │
│  → 在业务逻辑代码中使用                                  │
└─────────────────────────────────────────────────────────┘
```

这种分层是有意为之的：

1. **类型定义**需要描述字段、继承、泛型参数等复杂结构，注释语法更适合
2. **类型引用**只需要写类型名，inline 标注简洁直观
3. 与现有 LuaLS 生态完全兼容（所有 `@class` 注解库都可直接使用）

### 8.2 冲突解析规则

当 inline 标注与 EmmyLua 注释作用于同一目标时，**inline 标注优先**：

```lua
-- ✅ inline 类型优先于 ---@type
---@type string
local x: number = 42  -- hover 显示: number（inline 优先）

-- ✅ inline 参数类型优先于 ---@param
---@param a string
local function add(a: number, b: number): number
    return a + b
end
-- hover 显示 a: number（inline 优先）

-- ✅ inline 返回类型优先于 ---@return
---@return string
local function getName(): number
    return 42
end
-- hover 显示 -> number（inline 优先）
```

**最佳实践**：避免在同一目标上混用两种标注。如果代码中已有 inline 标注，删除对应的 EmmyLua 注释。

### 8.3 推荐工作流

#### 纯 TypingLua 项目（新项目）

```lua
-- 文件头：类型定义（EmmyLua 注释）
---@class Player
---@field name string
---@field health number

-- 业务逻辑：inline 标注
local function createPlayer(name: string): Player
    return { name = name, health = 100 }
end
```

#### UE5 + UnLua 项目（接入现有工程）

```lua
-- stub 文件由 UnLua IntelliSense 自动生成（无需手动编写）
-- 业务脚本中直接使用 inline 标注引用 UE 类型：

---@class BP_PlayerCharacter_C : ACharacter
---@field Health number
---@field MaxHealth number
local M = {}

function M:ReceiveBeginPlay()
    self.Health = self.MaxHealth
    local loc: FVector = self:K2_GetActorLocation()
end

function M:TakeDamage(amount: number, instigator: AActor): boolean
    self.Health = self.Health - amount
    return self.Health <= 0
end

return M
```

#### self 类型推断

在 UnLua 模式下，`self` 的类型由 `---@class` 标注决定：

```lua
---@class MyClass : AActor
local MyClass = {}

function MyClass:DoSomething()
    -- self 的类型是 MyClass（继承 AActor → UObject）
    -- 可以访问 MyClass 自身字段 + 所有继承链上的方法
    self:K2_GetActorLocation()  -- ✅ 来自 AActor
    self:GetName()              -- ✅ 来自 UObject
end
```

---

## 9. 当前限制与未来扩展

### 9.1 已支持的扩展

以下特性通过 `---@class` / `---@alias` 注解系统间接支持：

- **自定义类型名**：通过 `---@class` 定义后可在 inline 标注中使用（见 4.1）
- **泛型类型**：通过 `<T>` 语法支持（如 `TArray<FVector>`）
- **类型检查**：Language Server 提供 hover、completion、diagnostics

### 9.2 UE5 集成注意事项

在 UE5 + UnLua 工程中使用时：

| 项目 | 说明 |
|------|------|
| Stub 文件 | UnLua IntelliSense 导出的 `@class` 定义，放在 workspace library path |
| 配置开关 | `.luarc.json` 中设置 `"Lua.typingLua.enabled": true` |
| 运行时 | UE 引擎加载 `.lua` 文件时需要类型擦除预处理 |
| 开发阶段 | 通过 UnLua 加载层 hook 自动 strip（推荐方案 R2/R3） |
| 发布阶段 | 构建管线中批量转译，发布包只含标准 `.lua`（方案 R1） |
| 热重载 | strip 过程保持行号 1:1 映射，热重载正常工作 |

### 9.3 未来扩展

以下特性**不在当前版本范围内**：

- 内联类定义语法（`class`）
- 内联类型别名语法（`type alias`）
- 表字段类型（结构体字面量）
- 函数重载
- 枚举类型
- 字面量类型（如 `"hello"` 或 `42`）
