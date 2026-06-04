# TypingLua UE5 集成计划 — Phase 3 & Phase 4

> **状态**: 待实施
> **前置条件**: UE5 工程环境 + UnLua/sluaunreal 插件

---

## Phase 3：运行时类型擦除集成（预计 3-5 天）

### 目标

UE5 引擎加载含 inline 类型标注的 `.lua` 文件时，标准 Lua parser 会报语法错误。需要在加载链路中自动执行 type erasure。

### 方案选项

| 方案 | 描述 | 改动量 | 适用阶段 |
|------|------|--------|----------|
| R1: 构建时批量转译 | cook/package 时自动 strip 所有 `.lua` 文件 | CI/CD 管线 | 发布 |
| R2: UnLua 加载 hook | 在 `FLuaContext::LoadFile` 层 hook，加载前 strip | C++ 几十行 | 开发 |
| R3: 自定义 lua_Reader | 实现 `lua_Reader` 级 type strip | C 层几十行 | 开发 |
| R4: 双文件模式 | 编辑 `.lua`，watch + auto-transpile | 文件监听服务 | 不推荐 |

**推荐组合**: 开发阶段用 R2/R3，发布阶段用 R1。

### 任务清单

#### 3.1 将 tluac 编译为静态库 `libtlua_strip`

当前 `tluac` 是独立的命令行工具。需要重构为可链接的库形式。

```
目标产物:
  - libtlua_strip.lib (Windows)
  - libtlua_strip.a (Linux/Mac)

导出 API:
  // 输入: 含类型标注的 Lua 源码
  // 输出: 标准 Lua 源码（行号 1:1 保持）
  int tlua_strip(const char* input, size_t input_len,
                 char** output, size_t* output_len);
  void tlua_free(char* buffer);
```

工作项：
- [ ] 从 `tluac` 中分离 strip 核心逻辑为独立编译单元
- [ ] 编写 CMakeLists.txt 支持 `add_library(tlua_strip STATIC ...)`
- [ ] 确保无全局状态（支持多线程调用）
- [ ] 编写单元测试验证 strip 正确性

#### 3.2 实现 UnLua 加载层 hook（方案 R2）

在 UnLua 的文件加载路径中插入 strip 预处理。

```cpp
// 示例: 在 UnLua 的 FLuaContext 中 hook
// 位置: UnLua/Source/UnLua/Private/LuaContext.cpp

// 原始流程:
//   LoadFile(path) → lua_load(L, reader, data, chunkname)
//
// Hook 后流程:
//   LoadFile(path) → ReadFile(path) → tlua_strip(content) → lua_load(L, ...)

#include "tlua_strip.h"

bool FLuaContext::LoadFile(const FString& FilePath)
{
    TArray<uint8> FileContent;
    if (!FFileHelper::LoadFileToArray(FileContent, *FilePath))
        return false;

    char* stripped = nullptr;
    size_t stripped_len = 0;

    int result = tlua_strip(
        (const char*)FileContent.GetData(), FileContent.Num(),
        &stripped, &stripped_len);

    if (result == 0 && stripped)
    {
        // 使用 stripped 内容加载
        int loadResult = luaL_loadbuffer(L, stripped, stripped_len,
            TCHAR_TO_UTF8(*FilePath));
        tlua_free(stripped);
        return loadResult == LUA_OK;
    }

    // fallback: 原始内容加载
    return luaL_loadbuffer(L, (const char*)FileContent.GetData(),
        FileContent.Num(), TCHAR_TO_UTF8(*FilePath)) == LUA_OK;
}
```

工作项：
- [ ] 确认目标 UE5 版本 + UnLua/sluaunreal 版本
- [ ] 找到 Lua 文件加载入口点（`FLuaContext::LoadFile` 或等价函数）
- [ ] 集成 `libtlua_strip` 到 UE5 Build.cs
- [ ] 实现 hook 代码
- [ ] 验证热重载在 hook 模式下正常工作
- [ ] 添加 `bEnableTypingLua` 编译开关（UE Project Settings）

#### 3.3 实现构建管线批量转译（方案 R1）

发布构建时批量 strip 所有 Lua 文件。

```
工具: tluac --strip-dir <input-dir> <output-dir>
  - 递归处理所有 .lua 文件
  - 保持目录结构
  - 保持行号 1:1
  - 跳过已是标准 Lua 的文件（无 inline 标注）

集成点:
  - UE Build pipeline (RunUAT.bat)
  - 或 CI/CD 中 cook 前执行
```

工作项：
- [ ] 给 `tluac` 添加 `--strip-dir` 批量模式
- [ ] 集成到 UE5 cook 管线（Package 前自动执行）
- [ ] 验证发布包中只含标准 Lua 代码

#### 3.4 验证热重载兼容性

工作项：
- [ ] 确认 strip 过程不改变文件大小以外的属性（时间戳可变）
- [ ] 测试 UnLua 文件监听 → 重新加载 → strip → 执行 的完整链路
- [ ] 测试断点在热重载后是否仍然有效（行号 1:1 保证）

---

## Phase 4：UE5 工程集成测试（预计 2-3 天）

### 目标

在真实 UE5 工程中验证 TypingLua 全链路功能。

### 前置条件

- UE5.4+ 工程 + UnLua 插件已集成
- Phase 3 的 `libtlua_strip` 或加载 hook 已就位
- `tlua-language-server` 已配置为 VSCode 的 Lua Language Server

### 测试矩阵

#### 4.1 Language Server 功能验证

| 功能 | 测试内容 | 验证方法 |
|------|----------|----------|
| Hover | 悬停 inline 标注的变量，显示正确类型 | 手动 + 截图 |
| Completion | 输入 `actor:` 后弹出 AActor 的方法列表 | 手动 |
| Go-to-Definition | 点击 `FVector` 跳转到 stub 文件定义 | 手动 |
| Diagnostics | 类型不匹配时显示错误波浪线 | 手动 |
| Find References | 查找所有 `AActor` 的引用 | 手动 |
| Rename | 重命名自定义类型 | 手动 |

#### 4.2 UE 类型兼容性

```lua
-- 测试文件: Content/Script/Test/test_typing.lua

---@class BP_TestActor_C : AActor
---@field TestValue number
local M = {}

function M:ReceiveBeginPlay()
    -- 基础类型
    local loc: FVector = self:K2_GetActorLocation()
    local name: string = self:GetName()

    -- 泛型容器
    local actors: TArray<AActor> = UE.TArray(AActor)
    actors:Add(self)

    -- Delegate
    self.OnDestroyed:Add(self, self.OnActorDestroyed)

    -- 嵌套类型
    local transform: FTransform = self:GetActorTransform()
    local pos: FVector = transform.Translation
end

function M:OnActorDestroyed()
    print("Destroyed: " .. self:GetName())
end

return M
```

#### 4.3 运行时验证

| 场景 | 预期结果 |
|------|----------|
| 加载含 inline 标注的 `.lua` | 无语法错误，正常执行 |
| 热重载修改后的 `.lua` | 重新加载后行为正确 |
| PIE 调试 + 断点 | 断点命中正确行（行号 1:1） |
| 打包后运行 | strip 后的标准 Lua 正常执行 |

#### 4.4 性能基准

| 指标 | 基准 | 可接受范围 |
|------|------|-----------|
| 单文件 strip 耗时 | < 1ms (1KB) | < 10ms |
| 批量 strip 1000 文件 | < 5s | < 30s |
| LSP 首次打开大工程 | 基线 (无 inline) | < 1.2x 基线 |
| LSP 增量更新（单文件修改） | 基线 (无 inline) | < 1.1x 基线 |
| 运行时加载 hook 开销 | 0 (无 hook) | < 5% |

#### 4.5 兼容性矩阵

| 组合 | 状态 |
|------|------|
| UE5.4 + UnLua 2.x | 待测 |
| UE5.5 + UnLua 2.x | 待测 |
| UE5.4 + sluaunreal | 待测 |
| UE5.5 + sluaunreal | 待测 |

---

## 风险与缓解

| 风险 | 影响 | 缓解措施 |
|------|------|----------|
| UnLua 加载路径在不同版本间变化 | hook 点失效 | 提供多版本适配层 |
| 大文件 strip 性能不足 | 加载延迟增加 | C 层实现，O(n) 单遍扫描 |
| UE 热重载与 strip 时序冲突 | 文件状态不一致 | strip 为纯函数，无副作用 |
| 第三方 Lua 库含 `:` 语法冲突 | 误 strip | 只处理 workspace 内文件，库文件跳过 |
