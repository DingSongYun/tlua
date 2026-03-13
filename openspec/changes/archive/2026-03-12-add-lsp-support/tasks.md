## 1. 空格填充擦除模式（C 端）

- [ ] 1.1 在 `tlua/tlua_parser.h` 中：`TluaParser` 结构体新增 `int lsp_mode` 字段；新增 `tlua_transpile_lsp()` API 声明（与 `tlua_transpile` 签名相同，内部 `lsp_mode=1`）
- [ ] 1.2 在 `tlua/tlua_parser.c` 中：实现空格填充逻辑——当 `lsp_mode=1` 时，所有跳过类型注解的位置（`skip_type_annotation`、`skip_return_typelist` 调用处、`try_process_global_typed_assign`）改为 emit 等长空格而非静默跳过。核心思路：记录跳过区域的源码起止指针，计算字符数差值，emit 等量空格
- [ ] 1.3 实现 `tlua_transpile_lsp()` 公共 API：初始化 parser 时设置 `lsp_mode=1`，其余逻辑复用 `transpile_stream()`
- [ ] 1.4 在 `tlua/tlua_main.c` 中：为 `tluac` 添加 `--lsp-erase` 命令行参数，调用 `tlua_transpile_lsp()` 并输出结果
- [ ] 1.5 新增空格填充模式单元测试（`tests/test_transpiler.c` 或新文件）：验证 `local x: number = 42` → `local x         = 42`（列号完全对齐），覆盖函数参数、返回类型、全局类型赋值等所有注入点

## 2. 诊断收集 API（tlua_check）

- [ ] 2.1 在 `tlua/tlua_parser.h` 中定义 `TluaDiagnostic` 和 `TluaDiagnosticList` 结构体
- [ ] 2.2 在 `tlua/tlua_parser.c` 中实现 `tlua_check()` 函数：调用 lexer/parser 收集错误位置和消息，不生成输出
- [ ] 2.3 在 `tlua/tlua_main.c` 中：为 `tluac` 添加 `--check` 命令行参数，调用 `tlua_check()` 并以 JSON 格式输出诊断结果（`[{"line":1,"col":5,"message":"...","severity":1}, ...]`）
- [ ] 2.4 编写 `tlua_check()` 单元测试：验证合法代码返回空诊断列表、非法代码返回正确行列号和消息

## 3. VSCode 扩展 — 基础框架

- [ ] 3.1 创建 `vscode-tlua/package.json`：扩展清单，包含语言注册（`.tlua` → `tlua`）、TextMate grammar 引用、配置项（`typingLua.tluacPath`）、`extensionDependencies` 声明 LuaLS
- [ ] 3.2 创建 `vscode-tlua/tsconfig.json`：TypeScript 编译配置
- [ ] 3.3 创建 `vscode-tlua/.vscodeignore` 和 `vscode-tlua/.gitignore`

## 4. VSCode 扩展 — TextMate 语法高亮

- [ ] 4.1 创建 `vscode-tlua/syntaxes/tlua.tmLanguage.json`：基于 Lua 语法的 TextMate 定义，新增 `support.type.tlua`（类型关键词）、`storage.type.tlua`（`fun`）、`keyword.operator.type.tlua`（`|`, `?`）scope
- [ ] 4.2 验证语法高亮：打开 `.tlua` 文件，确认 Lua 基础语法和类型注解关键词都正确高亮

## 5. VSCode 扩展 — 透明转译代理

- [ ] 5.1 创建 `vscode-tlua/src/extension.ts`：扩展入口，激活/销毁生命周期管理
- [ ] 5.2 实现 Shadow Document Manager：监听 `.tlua` 文件的 didOpen/didChange 事件，调用 `tluac --lsp-erase`（通过 `child_process`）转译为空格填充的 `.lua` 内容
- [ ] 5.3 实现 LuaLS 集成：将 shadow `.lua` 内容喂给 LuaLS（通过 `vscode-languageclient` 启动独立 LuaLS 进程，或通过临时文件 + workspace 方式）
- [ ] 5.4 实现 TypingLua 诊断集成：调用 `tluac --check`（通过 `child_process`），解析 JSON 输出，生成 VSCode Diagnostic 对象
- [ ] 5.5 实现 Diagnostics Merger：合并 LuaLS 诊断与 TypingLua 诊断，推送给编辑器
- [ ] 5.6 实现 debounce 逻辑：在扩展层对频繁的 didChange 事件做防抖处理（如 300ms 延迟）

## 6. 构建与打包

- [ ] 6.1 确保 `CMakeLists.txt` 中 `tluac` 目标包含新增的 `--lsp-erase` 和 `--check` 功能（源文件无变化，只是 CLI 参数增加）
- [ ] 6.2 在 `vscode-tlua/` 中运行 `npm install`，验证 `npm run compile` 成功
- [ ] 6.3 验证 `npx vsce package` 成功生成 `.vsix` 文件

## 7. 端到端验证

- [ ] 7.1 手动测试：在 VSCode 中安装扩展，打开 `.tlua` 文件，确认语法高亮生效（Lua 语法 + 类型注解高亮）
- [ ] 7.2 手动测试：确认 LuaLS 的 Lua 智能功能可用（补全、Hover、Go-to-definition）且位置映射正确
- [ ] 7.3 手动测试：输入错误的类型注解，确认 TypingLua 诊断出现
- [ ] 7.4 手动测试：输入错误的 Lua 语法，确认 LuaLS 诊断出现在正确位置
- [ ] 7.5 运行全部 CTest 测试，确认无回归