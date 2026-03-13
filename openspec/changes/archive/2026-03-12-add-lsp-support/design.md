## Context

TypingLua 是一个为 Lua 添加静态类型注解的预处理器，核心流程为：`.tlua` → lexer/parser → type erasure → `.lua`。当前工具链由两个 C 可执行文件组成：`tluac`（离线转译器）和 `tlua`（transpile + execute runner）。开发者在 VSCode 中编辑 `.tlua` 文件时没有任何语言感知——没有语法高亮，也没有实时错误提示。

现有的 `tlua_parser.c` 使用"流式类型擦除"策略：逐 token 扫描，遇到类型注解注入点时调用 `skip_type_annotation()` / `skip_type_expr()` 跳过类型 token，其余 token 原样输出。lexer 为每个 token 记录了 `line` 和 `col` 信息（1-based）。经过验证，当前 transpiler 的输出与源文件保持**行号 1:1 对齐**（类型擦除不跨行），但**列号有偏移**（同行内类型被删除后后续 token 左移）。

**核心发现：** Lua 生态已有成熟的语言服务器 LuaLS（lua-language-server），提供完整的补全、Hover、Go-to-definition、诊断等功能。与其从零构建 LSP 服务器，不如通过"透明转译代理"架构复用 LuaLS 的全部能力。

**约束：**
- 项目使用纯 C + CMake + MSVC 工具链，transpiler 改动使用 C 语言。
- 不引入外部 C 依赖。
- VSCode 扩展使用 TypeScript。
- 依赖 LuaLS 作为 Lua 智能后端。

## Goals / Non-Goals

**Goals:**
- 提供 `.tlua` 文件在 VSCode 中的 TextMate 语法高亮（含类型注解关键词）。
- 复用 LuaLS 提供完整的 Lua 智能功能：补全、Hover、Go-to-definition、诊断等。
- 提供 TypingLua 特有的诊断：类型注解语法错误。
- 实现空格填充擦除模式，确保源码与擦除后的 Lua 代码行列号完全对齐，使 LuaLS 的结果可直接映射回源文件。
- 将来可渐进式地扩展：在代理层逐步接管更多逻辑（类型检查等）。

**Non-Goals:**
- 从零构建自定义 LSP 服务器（不实现 JSON-RPC、不实现 LSP 协议处理）。
- 自实现 JSON 解析器。
- 语义分析（类型检查、类型推导）——留给将来。
- 在类型注解位置提供补全（v1 不做）。
- 支持 VSCode 以外的编辑器（但架构不排斥扩展）。

## Architecture

### 整体架构：透明转译代理

```
                    TypingLua VSCode Extension
┌───────────────────────────────────────────────────────────┐
│                                                           │
│  ┌──────────────────────────────────────────────────────┐ │
│  │ Shadow Document Manager (TypeScript)                 │ │
│  │                                                      │ │
│  │  .tlua 文件内容 ──▶ tluac --lsp-erase ──▶ shadow .lua│ │
│  │                     (空格填充模式)         (内存中)    │ │
│  │                                                      │ │
│  │  行号 ✅ 完全对齐                                     │ │
│  │  列号 ✅ 完全对齐（空格填充）                          │ │
│  └────────────┬─────────────────────────┬───────────────┘ │
│               │                         │                 │
│               ▼                         ▼                 │
│  ┌────────────────────┐   ┌──────────────────────────┐   │
│  │ LuaLS (标准版)      │   │ tluac --check            │   │
│  │ • Lua 语法检查      │   │ • 类型注解语法检查        │   │
│  │ • 补全              │   │ (通过 child_process 调用) │   │
│  │ • Hover             │   │                          │   │
│  │ • Go-to-definition  │   │                          │   │
│  │ • ...               │   │                          │   │
│  └────────┬───────────┘   └────────────┬─────────────┘   │
│           │ 诊断结果                    │ 诊断结果         │
│           └──────────┬─────────────────┘                  │
│                      ▼                                    │
│           ┌───────────────────────┐                       │
│           │ Diagnostics Merger    │                       │
│           │ 合并诊断 → 推送给编辑器 │                       │
│           └───────────────────────┘                       │
│                                                           │
└───────────────────────────────────────────────────────────┘
```

### 数据流

```
用户在 VSCode 编辑 .tlua 文件
        │
        ▼
  ① didOpen / didChange 事件
        │
        ├──────────────────────────┐
        ▼                          ▼
  ② tluac --lsp-erase            ⑤ tluac --check
     将 .tlua 转为                   检查类型注解语法
     空格填充的 .lua                 输出 JSON 格式诊断
        │                              │
        ▼                              │
  ③ 将 shadow .lua 喂给 LuaLS          │
     (通过虚拟文件系统或临时文件)        │
        │                              │
        ▼                              │
  ④ LuaLS 返回诊断/补全等结果          │
     (行列号直接对应 .tlua)             │
        │                              │
        ├──────────────────────────────┘
        ▼
  ⑥ 合并两组诊断，推送给编辑器
```

## Decisions

### Decision 1: 架构选型 — LuaLS + 透明转译代理（方案 B）

**选择：** 复用 LuaLS 作为 Lua 智能后端，通过 VSCode 扩展层实现透明转译代理。

**被否决的替代方案：**
- *方案 A — 自定义 C LSP 服务器*：从零实现 JSON-RPC、LSP 协议、JSON 解析器。工作量巨大（2-3 个月），且无法获得补全、Hover、Go-to-definition 等高级功能。
- *方案 C — Fork LuaLS*：分叉 LuaLS 并修改其 Lua 编写的 PEG parser 来理解 TypingLua 语法。初始工作量大（LuaLS ~12万行），且每次上游更新都需要 rebase/merge，长期维护成本极高。
- *方案 D — VSCode Shadow Files*：在磁盘上生成 `.lua` shadow 文件。污染文件系统，且需要处理文件同步、清理等问题。

**理由：**
1. 空格填充解决了列号偏移问题，使 LuaLS 的所有结果可直接使用。
2. 现有 C transpiler 已经过充分测试，只需添加空格填充模式。
3. LuaLS 的全部 Lua 智能功能（补全、Hover、Go-to-definition、诊断）零成本获得。
4. 与上游 LuaLS 完全解耦，可直接升级 LuaLS 版本。
5. 渐进式路径：将来可在代理层逐步接管更多逻辑。

### Decision 2: 空格填充擦除模式

**选择：** 在 transpiler 中新增 `lsp_erase` 模式——擦除类型注解时用等长空格替换而非删除。

**原理：**
```
源文件:   local x: number = 42
标准擦除: local x = 42              ← 列号偏移 9 位
空格填充: local x         = 42      ← ': number' 被替换为 8 个空格，列号对齐
```

**实现方式：** 在 `TluaParser` 结构体中增加 `int lsp_mode` 标志。当 `lsp_mode=1` 时，所有跳过类型注解的函数不再静默跳过，而是记录跳过区域的起止位置，在输出中 emit 等量空格。

**理由：** 行列号完全 1:1 对齐意味着 LuaLS 返回的所有位置信息（诊断、补全、hover 等）可以直接映射到 `.tlua` 源文件，无需任何位置转换逻辑。这极大简化了代理层的实现复杂度。

### Decision 3: 诊断收集 API（tlua_check）

**选择：** 新增 `tlua_check()` C API，用于检查 `.tlua` 文件中的类型注解语法错误。

```c
typedef struct {
    int line;      /* 1-based */
    int col;       /* 1-based */
    int end_col;   /* 错误结束列 */
    int severity;  /* 1=Error, 2=Warning, 3=Info, 4=Hint */
    char message[256];
} TluaDiagnostic;

typedef struct {
    TluaDiagnostic items[64];
    int count;
} TluaDiagnosticList;

int tlua_check(const char *source, const char *filename, TluaDiagnosticList *diags);
```

**CLI 暴露：** `tluac --check <file>` — 输出 JSON 格式的诊断信息，供 VSCode 扩展通过 `child_process` 调用。

### Decision 4: VSCode 扩展与 LuaLS 集成方式

**选择：** VSCode 扩展内嵌 LuaLS 客户端逻辑，使用编程 API 控制 LuaLS。

**方案选项：**
- *依赖已安装的 LuaLS 扩展*：通过 VSCode 扩展 API 获取已安装的 `sumneko.lua` 扩展。简单但要求用户额外安装。
- *自行启动 LuaLS 进程*：扩展自行下载/捆绑 LuaLS 二进制，通过 `vscode-languageclient` 启动。完全自包含。
- *中间文件 + LuaLS workspace*：在临时目录生成 shadow `.lua` 文件，配置 LuaLS 的 workspace 指向该目录。

**选择先用依赖方式（v0.1）**：要求用户安装 LuaLS 扩展，扩展通过 `vscode-languageclient` 与 LuaLS 通信。将来可考虑自行捆绑。

### Decision 5: TextMate 语法高亮定义

**选择：** 基于 Lua TextMate 语法，扩展类型注解相关的 token scope。

**内容：**
- 复用/派生社区 Lua TextMate grammar（JSON 格式）。
- 新增 scope：`support.type.tlua`（类型名如 `number`, `string`, `boolean`）、`keyword.operator.type.tlua`（`|`, `?`）、`storage.type.tlua`（`fun` 关键词）。
- 文件关联：`.tlua` 扩展名 → `source.tlua` 语言 ID。

### Decision 6: 渐进式演化路径

```
v0.1          v0.5           v1.0           v2.0
────────────────────────────────────────────────▶
│              │              │              │
│ LuaLS 提供   │ 加入自有    │ 自有类型检查  │ 完全自有
│ 全部智能     │ 类型注解    │ 逐步接管      │ LSP
│ + 基础语法   │ 错误报告    │ LuaLS 的诊断  │ (可选)
│ 高亮         │             │               │
```

当前 scope 为 v0.1 + v0.5，即：LuaLS 提供完整 Lua 智能 + TypingLua 自有类型注解语法检查。

## Risks / Trade-offs

**[Risk] LuaLS 与 shadow 文件的集成复杂性** → 如何将内存中的 shadow `.lua` 内容喂给 LuaLS 需要仔细设计。可能需要使用 Virtual Document 或临时文件。
- *Mitigation*：先用最简单的方案（临时目录 + workspace 配置），验证可行后再优化为内存方案。

**[Risk] 空格填充可能导致 LuaLS 的某些启发式规则异常** → 多余空格可能影响 LuaLS 的缩进检测或格式化建议。
- *Mitigation*：LuaLS 对空白不敏感（Lua 本身对空白不敏感），影响应该极小。如果有问题，可以在诊断合并时过滤掉空白相关的诊断。

**[Risk] 用户需要同时安装 LuaLS** → 增加了安装步骤。
- *Mitigation*：在扩展的 `package.json` 中声明 `extensionDependencies`，VSCode 会自动提示安装。将来可考虑捆绑 LuaLS。

**[Trade-off] 类型位置无法补全** → 在 `: number` 的 `number` 位置按 Ctrl+Space 不会提供类型补全（因为 LuaLS 看到的是空格）。
- *Mitigation*：v1 不需要此功能。将来可在扩展层拦截类型位置的补全请求，自行提供候选列表。

**[Trade-off] Hover 显示 Lua 推断类型而非用户声明类型** → LuaLS 推断 `local x = 42` 为 `integer`，但用户声明的是 `: number`。
- *Mitigation*：LuaLS 的推断信息本身有价值。将来可在 Hover 结果中附加用户声明的类型信息。

## Open Questions

1. **LuaLS 集成的具体技术方案**：是通过 `vscode-languageclient` 编程启动一个独立的 LuaLS 进程，还是依赖已安装的 LuaLS 扩展？需要原型验证。
2. **Shadow 文件管理**：是使用磁盘临时文件还是 VSCode 的 Virtual Document API？需要调研 LuaLS 是否支持虚拟文件。
3. **增量更新**：每次按键都需要重新调用 tluac 进行转译吗？是否需要在扩展层实现 debounce？
4. **扩展发布**：VSCode 扩展是否需要发布到 Marketplace，还是仅作为本地 `.vsix` 分发？初期建议本地 `.vsix`。