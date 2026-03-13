## Context

TypingLua 是一个带类型注解的 Lua 方言，包含以下核心组件：

- **词法分析器** (`tlua_lexer.c/h`)：将 `.tlua` 源码拆分为 token 流，支持 80+ 种 token 类型（Lua 5.4 关键字、类型注解符号、空白/注释等）
- **转译器/解析器** (`tlua_parser.c/h`)：流式读取 token，识别类型注解注入点并擦除类型信息，生成标准 Lua 5.4 输出
- **CLI 工具**：`tluac`（独立转译器）和 `tlua`（转译+执行一体化运行器）
- **构建系统**：CMake，产出 `lua54.dll`、`lua.exe`、`luac.exe`、`tluac.exe`、`tlua.exe`

当前没有任何自动化测试。所有公共 API（`tlua_lexer_init`、`tlua_lexer_next`、`tlua_transpile` 等）都是纯 C 函数，适合直接编写 C 单元测试。

## Goals / Non-Goals

**Goals:**

- 建立可持续的测试基础设施，一条命令即可运行全部测试
- 覆盖词法分析器的 token 识别正确性
- 覆盖转译器所有类型擦除场景（与 `tlua-transpile` spec 中的 scenario 一一对应）
- 覆盖代码保持性（注释、空白、字符串、缩进不被篡改）
- 覆盖行号保持（转译前后 1:1 行映射）
- 验证 CLI 端到端流程和构建产物
- 测试框架零外部依赖，不影响项目编译速度

**Non-Goals:**

- 不做类型检查器测试（类型检查尚未实现）
- 不做性能/压力测试
- 不做 `tlua.exe` 调试流程的自动化测试（涉及 VSCode 扩展交互，难以自动化）
- 不做跨平台测试（当前仅支持 Windows/MSVC）
- 不做代码覆盖率分析

## Decisions

### Decision 1: 测试框架 — 自写极简宏 vs 第三方框架

**选择**：自写极简测试宏（header-only，约 50 行）

**理由**：
- 项目是纯 C（非 C++），可选的 C 测试框架较少
- 第三方方案（如 Unity、CMocka、Check）引入额外依赖管理负担
- 需求极简：断言 + 测试注册 + pass/fail 计数 + 彩色输出即可
- 自写宏完全可控，无版本兼容风险

**替代方案**：
- **Unity**（ThrowTheSwitch）：成熟但需要 3 个文件，对本项目略重
- **CMocka**：需要 CMake find_package，增加构建复杂度
- **直接 assert.h**：无测试统计、无可读输出，不推荐

### Decision 2: 测试目录结构

**选择**：

```
tests/
├── tlua_test.h            # 极简测试宏（header-only）
├── test_lexer.c           # 词法分析器单元测试
├── test_transpiler.c      # 转译器单元测试
├── test_e2e.c             # 端到端测试（调用 tluac CLI）
└── fixtures/              # 测试用 .tlua/.lua 文件
    ├── basic_types.tlua
    ├── basic_types.expected.lua
    ├── preservation.tlua
    ├── preservation.expected.lua
    ├── pure_lua.tlua
    ├── pure_lua.expected.lua
    └── ...
```

**理由**：
- 按被测组件分文件，职责清晰
- `fixtures/` 目录存放输入/期望输出对，便于维护和扩展
- 每个测试文件编译为独立可执行文件，失败隔离

### Decision 3: 词法分析器和转译器测试方式

**选择**：直接调用 C API（in-process 单元测试）

- **Lexer 测试**：调用 `tlua_lexer_init()` + 循环 `tlua_lexer_next()`，断言 token 类型、文本、行号
- **Transpiler 测试**：调用 `tlua_transpile()`，比较输出字符串与期望字符串

**理由**：
- Lexer 和 Transpiler 都暴露了干净的 C API，无需 mock
- In-process 测试执行速度极快（微秒级）
- 直接 `strcmp` 比较转译输出，简单可靠

### Decision 4: 端到端测试方式

**选择**：C 程序中使用 `system()` / `_popen()` 调用 `tluac.exe` 和 `lua.exe`

**流程**：
1. 将 fixture `.tlua` 文件通过 `tluac -p` 转译到 stdout
2. 捕获输出，与 `.expected.lua` 文件比较
3. 执行 `lua.exe` 运行转译后的 `.lua`，验证退出码和输出

**理由**：
- 验证了完整工具链（文件 I/O、CLI 参数解析、转译、执行）
- 使用 CTest 的 `add_test()` 也可以直接运行可执行文件并检查退出码

**替代方案**：
- **Shell 脚本**：Windows 上 batch/powershell 脚本语法不统一，维护成本高
- **Python 测试脚本**：引入 Python 依赖，与纯 C 项目风格不一致

### Decision 5: CTest 集成策略

**选择**：每个测试可执行文件注册为一个 CTest 测试

```cmake
enable_testing()
add_executable(test_lexer tests/test_lexer.c tlua/tlua_lexer.c)
add_test(NAME test_lexer COMMAND test_lexer)
```

**理由**：
- `ctest` 命令天然支持并行执行、过滤、超时、CI 集成
- 每个测试独立运行，一个崩溃不影响其他测试
- `cmake --build build --target test` 即可一键运行

### Decision 6: Fixture 文件格式

**选择**：成对文件 `<name>.tlua` + `<name>.expected.lua`

**理由**：
- 直观：输入文件和期望输出文件一一对应
- 方便手动检查：可以直接 diff 两个文件
- 新增测试只需添加文件对，无需修改测试代码（如果使用目录扫描方式）

**初期方案**：先在 C 代码中硬编码 fixture 路径列表；后续可改为目录扫描

## Risks / Trade-offs

**[自写测试框架] → 功能有限**
极简宏只提供基本断言（相等、字符串、非空）。如果未来需要 mock、参数化测试等高级功能，需要升级到第三方框架。当前需求简单，此风险可接受。

**[字符串精确比较] → 空白敏感**
转译器输出与期望值的比较是逐字符的。如果转译器对尾随空白/换行的处理发生变化，大量测试可能同时失败。缓解：规范化输出中的行尾（统一 `\n`）。

**[system() 调用] → 路径依赖**
端到端测试需要知道 `tluac.exe` 和 `lua.exe` 的路径。缓解：通过 CMake 的 `$<TARGET_FILE:tluac>` generator expression 传入路径。

**[Windows 独占] → 跨平台扩展成本**
`_popen()` 是 Windows 扩展。如果未来需要 Linux 支持，需要条件编译为 `popen()`。当前非目标，风险低。

## Open Questions

- 是否需要在 CI（如 GitHub Actions）中自动运行测试？当前设计支持，但尚未配置 CI workflow。
- fixture 目录扫描是否值得在初期实现，还是先用硬编码列表？
