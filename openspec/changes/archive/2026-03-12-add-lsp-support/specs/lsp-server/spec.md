## ADDED Requirements

### Requirement: JSON-RPC 消息传输

LSP 服务器 SHALL 通过 stdin/stdout 使用 LSP 标准传输协议通信。每条消息由一个 `Content-Length: <n>\r\n\r\n` 头部和紧随其后的 JSON 消息体组成。

#### Scenario: 读取 JSON-RPC 请求

- **WHEN** stdin 接收到 `Content-Length: 52\r\n\r\n{"jsonrpc":"2.0","id":1,"method":"initialize",...}`
- **THEN** 服务器 SHALL 正确解析出 Content-Length 值
- **AND** 读取恰好对应长度的 JSON 消息体

#### Scenario: 发送 JSON-RPC 响应

- **WHEN** 服务器需要发送响应
- **THEN** SHALL 先写入 `Content-Length: <n>\r\n\r\n` 头部
- **AND** 紧接写入 JSON 响应体
- **AND** 头部中的长度值 SHALL 精确等于 JSON 体的字节数

### Requirement: LSP 生命周期管理

LSP 服务器 SHALL 实现 LSP 规范中的初始化和关闭生命周期。

#### Scenario: initialize 请求

- **WHEN** 服务器收到 `initialize` 请求
- **THEN** SHALL 返回 `InitializeResult`，包含 `capabilities` 对象
- **AND** capabilities SHALL 声明 `textDocumentSync` 为 `Full`（值 1）
- **AND** capabilities SHALL 声明 `diagnosticProvider` 或通过 publishDiagnostics 推送诊断

#### Scenario: initialized 通知

- **WHEN** 服务器收到 `initialized` 通知
- **THEN** SHALL 接受该通知，不返回响应（notification 无需回复）

#### Scenario: shutdown 请求

- **WHEN** 服务器收到 `shutdown` 请求
- **THEN** SHALL 返回 `null` 结果
- **AND** SHALL 进入关闭状态，不再处理除 `exit` 以外的请求

#### Scenario: exit 通知

- **WHEN** 服务器收到 `exit` 通知
- **THEN** SHALL 以状态码 0 退出进程（若已收到 shutdown）
- **OR** 以状态码 1 退出进程（若未收到 shutdown）

### Requirement: 文本文档同步

LSP 服务器 SHALL 跟踪编辑器中打开的 `.tlua` 文件内容。

#### Scenario: textDocument/didOpen

- **WHEN** 服务器收到 `textDocument/didOpen` 通知
- **THEN** SHALL 在内存中存储该文档的 URI 和完整文本内容
- **AND** SHALL 立即触发一次诊断检查

#### Scenario: textDocument/didChange（Full Sync）

- **WHEN** 服务器收到 `textDocument/didChange` 通知
- **THEN** SHALL 使用通知中的完整文本内容更新内存中的文档
- **AND** SHALL 立即触发一次诊断检查

#### Scenario: textDocument/didClose

- **WHEN** 服务器收到 `textDocument/didClose` 通知
- **THEN** SHALL 从内存中移除该文档
- **AND** SHALL 发送空的 diagnostics 数组以清除编辑器中的诊断标记

### Requirement: JSON 解析与生成

LSP 服务器 SHALL 包含一个轻量级 JSON 解析器和生成器（`tlua_json.c`/`tlua_json.h`），用于处理 JSON-RPC 消息。

#### Scenario: 解析 JSON 对象

- **WHEN** 输入 `{"method":"initialize","id":1}`
- **THEN** SHALL 正确解析出字符串值 `"initialize"` 和数字值 `1`

#### Scenario: 解析嵌套对象

- **WHEN** 输入包含嵌套对象如 `{"params":{"textDocument":{"uri":"file:///a.tlua"}}}`
- **THEN** SHALL 正确导航到嵌套字段并提取值

#### Scenario: 解析 JSON 数组

- **WHEN** 输入包含数组如 `{"items":[1,"a",true,null]}`
- **THEN** SHALL 正确解析数组中的各类型元素

#### Scenario: 生成 JSON 响应

- **WHEN** 需要构建 LSP 响应
- **THEN** SHALL 生成合法的 JSON 字符串
- **AND** 字符串中的特殊字符（`"`, `\`, 控制字符）SHALL 被正确转义

### Requirement: 文档存储

LSP 服务器 SHALL 维护一个内存中的文档存储，用于管理当前打开的文件。

#### Scenario: 存储容量

- **WHEN** 多个 `.tlua` 文件同时打开
- **THEN** SHALL 支持同时管理至少 64 个打开的文档

#### Scenario: 按 URI 查找文档

- **WHEN** 根据 URI 查找已打开的文档
- **THEN** SHALL 返回该文档最新的完整文本内容
