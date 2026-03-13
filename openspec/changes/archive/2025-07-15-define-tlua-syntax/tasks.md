## 1. EBNF 语法定义

- [x] 1.1 编写 `.tlua` 类型表达式的 EBNF 产生式（base_type, union_type, optional_type, array_type, table_type, function_type）
- [x] 1.2 编写类型标注注入点的 EBNF 产生式（typed_local_stat, typed_global_assign, typed_param, typed_return_type）
- [x] 1.3 编写完整的 `.tlua` 语法 EBNF 文档，整合类型表达式与注入点，验证其为 Lua 5.4 语法的严格超集
- [x] 1.4 验证 EBNF 无歧义性：手动推导多返回值逗号消歧、冒号消歧、多重赋值消歧的 parse tree

## 2. 参考文档

- [x] 2.1 编写 `.tlua` 语法参考文档（`docs/tlua-syntax-reference.md`），包含所有标注位置和类型表达式的说明与示例
- [x] 2.2 编写转译规则参考文档（`docs/tlua-transpile-reference.md`），包含输入/输出对照表和边界情况说明

## 3. 示例文件

- [x] 3.1 创建 `.tlua` 示例源文件（`examples/demo.tlua`），覆盖所有标注位置和类型表达式
- [x] 3.2 创建对应的预期转译输出文件（`examples/demo.lua`），展示类型擦除后的结果
- [x] 3.3 验证 `examples/demo.lua` 可被 `build/Release/lua.exe` 正常执行

## 4. 转译器核心实现

- [x] 4.1 确定转译器实现语言和项目结构（在 CMake 构建系统中添加新目标）
- [x] 4.2 实现 Lexer：支持标准 Lua 5.4 token 以及新增的类型标注 token（`:` 在类型上下文、`|`、`?`、`[]`、`<>`、`fun` 关键字）
- [x] 4.3 实现 Parser：基于 EBNF 语法解析 `.tlua` 源文件，构建 AST（含类型标注节点）
- [x] 4.4 实现 CodeGen（类型擦除）：遍历 AST，输出去除所有类型标注的标准 Lua 5.4 代码
- [x] 4.5 实现代码保留逻辑：确保注释、空白、缩进、字符串字面量在转译过程中不被修改
- [x] 4.6 实现字符串/注释内容跳过逻辑：确保引号和 `[[ ]]` 内的类型标注语法不被误识别

## 5. CLI 与文件 I/O

- [x] 5.1 实现转译器 CLI 入口：接受 `.tlua` 输入文件路径，输出 `.lua` 文件（`.tlua` → `.lua` 扩展名替换）
- [x] 5.2 实现错误报告：输出包含文件名、行号和问题描述的诊断信息
- [x] 5.3 实现逐文件独立转译模式（不依赖其他文件内容）

## 6. 测试与验证

- [x] 6.1 为每个标注位置编写转译测试用例（local 变量、global 变量、函数参数、函数返回值）
- [x] 6.2 为每种类型表达式编写转译测试用例（基础类型、联合、可选、数组、表泛型、函数签名）
- [x] 6.3 编写边界情况测试：多重赋值、混合标注/未标注、方法调用共存、字符串内类型语法
- [x] 6.4 编写纯 Lua 文件直通测试：验证无标注 `.tlua` 转译后与输入完全一致
- [x] 6.5 编写错误处理测试：畸形类型标注、未闭合泛型等语法错误的诊断输出
- [x] 6.6 运行所有转译输出通过 `build/Release/lua.exe` 执行验证