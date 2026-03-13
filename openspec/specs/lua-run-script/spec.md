## Requirements

### Requirement: 运行 Lua 脚本

编译出的 lua.exe 能正确执行 Lua 脚本文件。

#### Scenario: 执行 hello world 脚本

- **WHEN** 使用编译出的 `lua.exe` 运行 `test/hello.lua`
- **THEN** 标准输出打印 `Hello from Lua 5.4!`
- **AND** 进程退出码为 0

#### Scenario: 交互式模式可用

- **WHEN** 直接运行 `lua.exe` 不带参数
- **THEN** 进入交互式 REPL 模式
- **AND** 显示 Lua 版本信息

### Requirement: Execute transpiled .tlua output
The compiled `lua.exe` SHALL be able to execute `.lua` files that were produced by transpiling `.tlua` source files. The transpiled output MUST run identically to hand-written Lua code.

#### Scenario: Run transpiled hello world
- **WHEN** a `.tlua` file containing:
  ```tlua
  local greeting: string = "Hello from TypingLua!"
  print(greeting)
  ```
  is transpiled to `.lua` and executed with `lua.exe`
- **THEN** standard output SHALL print `Hello from TypingLua!`
- **AND** the process exit code SHALL be 0

#### Scenario: Run transpiled function with typed parameters
- **WHEN** a `.tlua` file containing:
  ```tlua
  local function add(a: number, b: number): number
    return a + b
  end
  print(add(3, 4))
  ```
  is transpiled to `.lua` and executed with `lua.exe`
- **THEN** standard output SHALL print `7`
- **AND** the process exit code SHALL be 0

#### Scenario: Transpiled output has no residual type syntax
- **WHEN** a transpiled `.lua` file is executed with `lua.exe`
- **THEN** no syntax errors related to type annotations (colons, type names) SHALL occur