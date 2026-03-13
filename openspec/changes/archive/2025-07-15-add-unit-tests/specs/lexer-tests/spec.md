## ADDED Requirements

### Requirement: 标识符和关键字识别

词法分析器测试 SHALL 验证所有 Lua 5.4 关键字和 TypingLua 扩展关键字被正确识别为对应的 token 类型。

#### Scenario: Lua 5.4 关键字识别

- **WHEN** 对包含 `and break do else elseif end false for function goto if in local nil not or repeat return then true until while` 的源码执行词法分析
- **THEN** 每个关键字生成对应的 `TK_AND` .. `TK_WHILE` token 类型
- **AND** token 的 `start` 和 `length` 字段正确指向源码中的关键字文本

#### Scenario: TypingLua fun 关键字识别

- **WHEN** 对包含 `fun` 的源码执行词法分析
- **THEN** 生成 `TK_FUN` token

#### Scenario: 普通标识符识别

- **WHEN** 对包含 `myVar _private foo123 _` 的源码执行词法分析
- **THEN** 每个标识符生成 `TK_NAME` token
- **AND** token 文本与源码中的标识符完全匹配

### Requirement: 数字字面量识别

词法分析器测试 SHALL 验证整数和浮点数字面量被正确识别。

#### Scenario: 整数字面量

- **WHEN** 对包含 `42 0 0xFF 0b1010` 的源码执行词法分析
- **THEN** 每个数字生成 `TK_NUMBER` token

#### Scenario: 浮点数字面量

- **WHEN** 对包含 `3.14 .5 1e10 2.5e-3` 的源码执行词法分析
- **THEN** 每个数字生成 `TK_NUMBER` token

### Requirement: 字符串字面量识别

词法分析器测试 SHALL 验证各种字符串字面量被正确识别。

#### Scenario: 短字符串

- **WHEN** 对包含 `"hello" 'world' "with \"escape\""` 的源码执行词法分析
- **THEN** 每个字符串生成 `TK_STRING` token
- **AND** token 文本包含引号

#### Scenario: 长字符串

- **WHEN** 对包含 `[[long string]] [==[nested]==]` 的源码执行词法分析
- **THEN** 生成 `TK_LONGSTRING` token

### Requirement: 操作符和标点符号识别

词法分析器测试 SHALL 验证所有单字符和多字符操作符被正确识别。

#### Scenario: 单字符操作符

- **WHEN** 对包含 `( ) [ ] { } , ; : . = + - * / % ^ # & ~ | < > ?` 的源码执行词法分析
- **THEN** 每个符号生成对应的 `TK_LPAREN` .. `TK_QUESTION` token 类型

#### Scenario: 多字符操作符

- **WHEN** 对包含 `.. ... == ~= <= >= << >> // ::` 的源码执行词法分析
- **THEN** 每个操作符生成对应的 `TK_CONCAT` .. `TK_DBCOLON` token 类型

### Requirement: 注释识别

词法分析器测试 SHALL 验证行注释和块注释被正确识别。

#### Scenario: 行注释

- **WHEN** 对包含 `-- this is a comment` 的源码执行词法分析
- **THEN** 生成 `TK_COMMENT` token
- **AND** token 文本包含完整的注释内容

#### Scenario: 块注释

- **WHEN** 对包含 `--[[ block comment ]]` 的源码执行词法分析
- **THEN** 生成 `TK_COMMENT` token

### Requirement: 空白和换行识别

词法分析器测试 SHALL 验证空白和换行被作为独立 token 识别（因为转译器需要保持它们）。

#### Scenario: 空白 token

- **WHEN** 对包含空格和制表符的源码执行词法分析
- **THEN** 连续空白生成 `TK_WHITESPACE` token

#### Scenario: 换行 token

- **WHEN** 对包含 `\n` 或 `\r\n` 的源码执行词法分析
- **THEN** 换行生成 `TK_NEWLINE` token

### Requirement: 行号和列号跟踪

词法分析器测试 SHALL 验证 token 的行号和列号被正确记录。

#### Scenario: 多行源码中的行号

- **WHEN** 对包含多行的源码执行词法分析
- **THEN** 第一行 token 的 `line` 字段为 1
- **AND** 换行后的 token 的 `line` 字段递增
- **AND** 每个 token 的 `col` 字段反映其在行内的起始位置

### Requirement: EOF 处理

词法分析器测试 SHALL 验证源码末尾的正确处理。

#### Scenario: 空源码

- **WHEN** 对空字符串 `""` 执行词法分析
- **THEN** 第一次调用 `tlua_lexer_next()` 生成 `TK_EOF` token

#### Scenario: 源码末尾生成 EOF

- **WHEN** 所有 token 消费完毕后再次调用 `tlua_lexer_next()`
- **THEN** 持续返回 `TK_EOF` token
