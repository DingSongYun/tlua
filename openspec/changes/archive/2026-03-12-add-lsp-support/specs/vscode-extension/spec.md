## ADDED Requirements

### Requirement: 语言注册

VSCode 扩展 SHALL 将 `.tlua` 文件扩展名注册为 `tlua` 语言标识符。

#### Scenario: 文件关联

- **WHEN** 用户在 VSCode 中打开 `.tlua` 文件
- **THEN** 编辑器底部状态栏 SHALL 显示语言为 `TypingLua`
- **AND** 文件 SHALL 使用 `tlua` 语法高亮规则

#### Scenario: 语言配置

- **WHEN** 扩展的 `package.json` 中定义 `contributes.languages`
- **THEN** SHALL 包含 `id: "tlua"`
- **AND** SHALL 包含 `extensions: [".tlua"]`
- **AND** SHALL 包含 `aliases: ["TypingLua", "tlua"]`

### Requirement: TextMate 语法高亮

VSCode 扩展 SHALL 提供 TextMate 语法定义文件，为 `.tlua` 文件提供语法高亮。

#### Scenario: Lua 关键词高亮

- **WHEN** 文件包含 Lua 关键词（`local`, `function`, `if`, `then`, `end`, `return` 等）
- **THEN** SHALL 以关键词颜色高亮显示

#### Scenario: 类型注解高亮

- **WHEN** 文件包含类型注解如 `local x: number = 1`
- **THEN** 类型名称 `number` SHALL 以类型颜色高亮显示（scope: `support.type.tlua`）

#### Scenario: 类型关键词高亮

- **WHEN** 文件包含类型关键词 `fun`（用于函数类型签名）
- **THEN** `fun` SHALL 以存储类型颜色高亮显示（scope: `storage.type.tlua`）

#### Scenario: 类型操作符高亮

- **WHEN** 文件包含类型操作符 `|`（联合类型）和 `?`（可选类型）出现在类型上下文中
- **THEN** SHALL 以操作符颜色高亮显示

#### Scenario: 字符串和注释高亮

- **WHEN** 文件包含字符串字面量或注释
- **THEN** SHALL 分别以字符串和注释颜色高亮显示
- **AND** 长字符串 `[[ ... ]]` 和块注释 `--[[ ... ]]` SHALL 被正确识别

### Requirement: LSP 客户端集成

VSCode 扩展 SHALL 内嵌 LSP 客户端，自动启动 `tlua-lsp` 服务器进程。

#### Scenario: 服务器启动

- **WHEN** 用户打开一个包含 `.tlua` 文件的工作区
- **THEN** 扩展 SHALL 启动 `tlua-lsp` 进程
- **AND** 通过 stdin/stdout 与服务器通信

#### Scenario: 服务器路径配置

- **WHEN** 用户在 VSCode 设置中配置 `tluaLsp.serverPath`
- **THEN** 扩展 SHALL 使用配置的路径启动服务器
- **AND** 若未配置，SHALL 从系统 PATH 中查找 `tlua-lsp` 可执行文件

#### Scenario: 服务器崩溃恢复

- **WHEN** `tlua-lsp` 进程意外退出
- **THEN** `vscode-languageclient` SHALL 按默认策略尝试重启服务器

### Requirement: 扩展打包

VSCode 扩展 SHALL 可以打包为 `.vsix` 文件供本地安装。

#### Scenario: 打包命令

- **WHEN** 在 `vscode-tlua/` 目录执行 `npx vsce package`
- **THEN** SHALL 生成一个 `.vsix` 文件
- **AND** 该文件可通过 `code --install-extension <file>.vsix` 安装到 VSCode
