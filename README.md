# TypingLua

为 Lua 添加**内联类型注解**的实验性语言扩展。`.tlua` 文件在运行前会被自动转译为标准 Lua 5.4 代码——类型信息仅用于静态分析，不影响运行时行为。

## 快速体验

```lua
-- examples/demo.tlua
local greeting: string = "Hello, TypingLua!"
local count: number = 42
print(greeting, count)
```

```bash
# 直接运行 .tlua 文件（推荐）
build\Release\tlua.exe examples\demo.tlua

# 或者先转译再运行
build\Release\tluac.exe examples\demo.tlua -o examples\demo.lua
build\Release\lua.exe examples\demo.lua
```

## 前置条件

- **Visual Studio 2022**（含 C/C++ 工作负载）
- **CMake** 3.15+

## 构建

在 **Developer Command Prompt for VS 2022** 中执行：

```bash
cmake -B build
cmake --build build --config Release
```

编译产物在 `build/Release/` 目录下：

| 可执行文件 | 说明 |
|-----------|------|
| `tlua.exe` | **TypingLua 解释器** — 直接运行 `.tlua` 文件，内部自动转译并执行 |
| `tluac.exe` | **TypingLua 转译器** — 将 `.tlua` 文件转译为标准 `.lua` 文件 |
| `lua.exe` | Lua 5.4 标准解释器 |
| `luac.exe` | Lua 5.4 标准编译器 |

## 用法

### `tlua` — 直接运行（推荐）

```bash
# 运行 .tlua 文件
tlua examples/demo.tlua

# 执行内联代码
tlua -e "local x: number = 1 + 2; print(x)"

# 传递命令行参数（脚本中通过 arg[1]、arg[2] 访问）
tlua examples/demo.tlua arg1 arg2
```

### `tluac` — 转译

```bash
# 转译到标准输出
tluac examples/demo.tlua

# 转译到文件
tluac examples/demo.tlua -o output.lua
```

## 类型语法概览

```lua
-- 变量类型注解
local name: string = "Alice"
local age: number = 30
local active: boolean = true

-- 函数参数与返回值
local function add(a: number, b: number): number
    return a + b
end

-- 复合类型
local scores: {number} = {90, 85, 92}
local config: {string: any} = {host = "localhost", port = 8080}
local value: string | nil = nil

-- 方法调用（冒号语法完全兼容）
local upper: string = name:upper()
```

> 完整语法参考见 [`docs/tlua-reference.md`](docs/tlua-reference.md) 和 [`docs/tlua-syntax.ebnf`](docs/tlua-syntax.ebnf)。

## 项目结构

```
TypingLua/
├── tlua/                   # TypingLua 核心源码
│   ├── tlua_lexer.c/.h     #   词法分析器
│   ├── tlua_parser.c/.h    #   语法解析器 / 转译器
│   ├── tlua_main.c         #   tluac CLI 入口
│   └── tlua_run.c          #   tlua 解释器入口（嵌入 Lua VM）
├── lua/                    # Lua 5.4.7 官方源码
├── examples/               # .tlua 示例文件
├── docs/                   # 语法文档与 EBNF
├── CMakeLists.txt          # CMake 构建配置
└── README.md
```

## License

本项目中的 Lua 源码遵循 [MIT License](https://www.lua.org/license.html)。