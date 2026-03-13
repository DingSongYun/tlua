## ADDED Requirements

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