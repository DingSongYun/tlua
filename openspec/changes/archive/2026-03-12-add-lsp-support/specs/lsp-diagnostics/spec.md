## ADDED Requirements

### Requirement: 语法错误诊断

LSP 服务器 SHALL 在文件打开或内容变化时，调用现有的 lexer/parser 对 `.tlua` 文件进行语法检查，并将发现的错误转换为 LSP Diagnostic 对象推送给编辑器。

#### Scenario: 检测到词法错误

- **WHEN** 文件内容包含无法识别的字符或不完整的 token（如未闭合的字符串）
- **THEN** SHALL 生成一条 Diagnostic
- **AND** Diagnostic 的 `range` SHALL 指向错误所在的行和列（0-based）
- **AND** Diagnostic 的 `severity` SHALL 为 `Error`（值 1）
- **AND** Diagnostic 的 `message` SHALL 包含描述错误原因的文本

#### Scenario: 无错误的文件

- **WHEN** 文件内容是合法的 `.tlua` 代码
- **THEN** SHALL 发送空的 diagnostics 数组
- **AND** 编辑器中 SHALL 不显示任何错误标记

#### Scenario: 错误被修复后清除诊断

- **WHEN** 用户修改文件修复了之前的语法错误
- **THEN** SHALL 发送空的 diagnostics 数组
- **AND** 之前显示的错误标记 SHALL 被清除

### Requirement: 诊断收集 API

SHALL 新增 C API 函数 `tlua_check()` 用于收集源文件中的语法错误信息。

#### Scenario: 收集单个错误

- **WHEN** 对包含一个语法错误的源码调用 `tlua_check()`
- **THEN** SHALL 返回非零值
- **AND** 诊断列表中 SHALL 包含一条记录，含 `line`、`col`、`severity`、`message` 字段

#### Scenario: 无错误时返回空列表

- **WHEN** 对合法源码调用 `tlua_check()`
- **THEN** SHALL 返回 0
- **AND** 诊断列表的 `count` SHALL 为 0

### Requirement: Diagnostic 推送

LSP 服务器 SHALL 使用 `textDocument/publishDiagnostics` 通知主动向编辑器推送诊断结果。

#### Scenario: 推送诊断通知

- **WHEN** 诊断检查完成且发现错误
- **THEN** SHALL 发送 `textDocument/publishDiagnostics` 通知
- **AND** 通知的 `uri` SHALL 匹配被检查文件的 URI
- **AND** 通知的 `diagnostics` 数组 SHALL 包含所有发现的错误

#### Scenario: 行列号转换

- **WHEN** lexer/parser 报告的错误位置为 1-based（line=3, col=5）
- **THEN** 转换为 LSP Diagnostic 时 SHALL 使用 0-based 坐标（line=2, character=4）
