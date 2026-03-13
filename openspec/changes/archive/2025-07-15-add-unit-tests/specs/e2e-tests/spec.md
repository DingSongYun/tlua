## ADDED Requirements

### Requirement: tluac CLI 转译验证

端到端测试 SHALL 通过调用 `tluac` 可执行文件验证完整的 `.tlua → .lua` 转译流程。

#### Scenario: tluac 输出到 stdout

- **WHEN** 执行 `tluac -p <fixture>.tlua`（print 模式）
- **THEN** stdout 输出 SHALL 与 `<fixture>.expected.lua` 文件内容逐字符相同
- **AND** 进程退出码为 0

#### Scenario: tluac 写入文件

- **WHEN** 执行 `tluac <fixture>.tlua`（默认模式）
- **THEN** SHALL 在同目录生成 `<fixture>.lua` 文件
- **AND** 生成的文件内容 SHALL 与 `<fixture>.expected.lua` 逐字符相同

#### Scenario: tluac 处理不存在的文件

- **WHEN** 执行 `tluac nonexistent.tlua`
- **THEN** 进程退出码 SHALL 为非 0
- **AND** stderr 或 stdout SHALL 包含错误信息

### Requirement: 转译结果运行时验证

端到端测试 SHALL 验证转译后的 `.lua` 文件能被 `lua.exe` 正确执行。

#### Scenario: 转译后执行成功

- **WHEN** 一个 fixture `.tlua` 文件被 `tluac` 转译为 `.lua`
- **AND** 转译后的 `.lua` 文件被 `lua.exe` 执行
- **THEN** `lua.exe` 进程退出码 SHALL 为 0

#### Scenario: 转译后输出正确

- **WHEN** 一个包含 `print` 语句的 `.tlua` fixture 被转译并执行
- **THEN** `lua.exe` 的 stdout 输出 SHALL 与预期值匹配

#### Scenario: 类型注解不影响运行时行为

- **WHEN** 一个包含类型注解的 `.tlua` 文件和其去除类型注解的等价 `.lua` 文件分别被 `lua.exe` 执行
- **THEN** 两者的 stdout 输出 SHALL 完全相同

### Requirement: Fixture 文件管理

端到端测试 SHALL 使用成对的 fixture 文件作为测试输入和期望输出。

#### Scenario: Fixture 文件成对存在

- **WHEN** 检查 `tests/fixtures/` 目录
- **THEN** 每个 `<name>.tlua` 文件 SHALL 有对应的 `<name>.expected.lua` 文件

#### Scenario: 基础类型擦除 fixture

- **WHEN** 检查 fixture 文件集
- **THEN** SHALL 包含覆盖基础类型擦除场景的 fixture（局部变量、全局变量、函数参数、返回类型）

#### Scenario: 代码保持性 fixture

- **WHEN** 检查 fixture 文件集
- **THEN** SHALL 包含覆盖代码保持性的 fixture（注释、字符串、缩进、空行）

#### Scenario: 纯 Lua 透传 fixture

- **WHEN** 检查 fixture 文件集
- **THEN** SHALL 包含不含类型注解的纯 Lua fixture，其 `.tlua` 和 `.expected.lua` 内容完全相同

### Requirement: 构建产物验证

端到端测试 SHALL 验证 CMake 构建产生的所有可执行文件和库文件存在且可运行。

#### Scenario: 核心可执行文件存在

- **WHEN** CMake 构建完成后
- **THEN** SHALL 存在以下可执行文件：`lua.exe`、`luac.exe`、`tluac.exe`、`tlua.exe`

#### Scenario: 共享库存在

- **WHEN** CMake 构建完成后
- **THEN** SHALL 存在 `lua54.dll` 共享库

#### Scenario: lua.exe 可正常执行

- **WHEN** 执行 `lua.exe -e "print('ok')"`
- **THEN** stdout 输出 SHALL 包含 `ok`
- **AND** 退出码为 0

#### Scenario: tluac.exe 可显示用法信息

- **WHEN** 执行 `tluac.exe`（无参数）
- **THEN** 输出 SHALL 包含用法提示信息
