# CP 标准库：可组合洋葱中间件与多阶段流水线手册 (`cap:pipeline`)

## 一、 架构总览与设计准则

`cap:pipeline` 是 CP 语言官方内置的纯 CP **可组合洋葱模型（Onion Middleware）与多阶段数据处理流水线引擎**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，专为 Web 中间件、RPC 拦截器与 AI Agent Tool 执行链设计。既可通过 `import { ... } from "cap:pipeline"` 导入，也支持 `import cap from "cap"; cap.pipeline.*` 动态懒挂载调用。

```text
  Request ──▶ A.before ──▶ B.before ──▶ C.before
                                           │
  Response ◀─ A.after  ◀── B.after  ◀── C.after
```

- **短路拦截（Short-Circuit）**：任意阶段调用 `ctx.halt("reason")` 后立即停止向后推进，但仍严格按**逆序**执行已进入阶段的 `after` 清理（如 `B.after -> A.after`）。
- **异常传播与兜底清理**：任意阶段 `before` 抛出错误时，触发阶段级/全局 `onError` 钩子，逆序清理所有先前已进入阶段的 `after` 释放资源，随后向上传播错误。
- **阶段轨迹追踪**：每次执行自动记录各阶段 `StageTrace`（`name`、`outcome`、`duration_ns`、`error_msg`）。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/pipeline/pipeline.cp` | `// Source: cp/lib/pipeline/pipeline.cp (...)` | `PipelineError`、`StageOutcome`、`StageTrace`、`PipelineContext`、`StageSpec`、`Pipeline` |
| `lib/pipeline/index.cp` | `// Source: cp/lib/pipeline/index.cp (Unified Exports for cap:pipeline)` | `cap:pipeline` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误与状态枚举

- **`PipelineError`**：`StageFailed`、`EmptyPipeline`、`DuplicateStage`、`StageNotFound`、`OutOfMemory`
- **`StageOutcome`**：`Continue`、`ShortCircuit`、`Skipped`、`Failed`（提供 `toString()`）

### 2. `StageTrace` 与 `PipelineContext`

- **`StageTrace`**：`name: string`、`outcome: StageOutcome`、`duration_ns: u64`、`error_msg: string`
- **`PipelineContext`**：
  - `PipelineContext.init(alloc, payload)` / `deinit()`
  - `set(key, val): !PipelineError`、`get(key): string?`
  - `setPayload(s): !PipelineError`、`payload(): string`
  - `halt(reason)`、`isHalted(): bool`、`haltReason(): string`
  - `traces(): []const StageTrace`、`clearTraces()`

### 3. 阶段函数类型与 `Pipeline`

- **`StageFn`**：`fn (*PipelineContext) !PipelineError`
- **`ErrorHookFn`**：`fn (*PipelineContext, anyerror) void`
- **`PredicateFn`**：`fn (*PipelineContext) bool`
- **`StageSpec`**：`name`、`before`、`after`、`on_error`、`when`、`enabled`
- **`Pipeline`**：
  - `Pipeline.init(alloc, name)` / `deinit()`
  - `use(name, before_fn)`：注册单向 `before` 阶段（重名返回 `DuplicateStage`）
  - `useOnion(name, before_fn, after_fn)`：注册洋葱阶段（支持 `before`/`after` 任意可空）
  - `useConditional(name, when_fn, before_fn, after_fn)`：注册带条件谓词的洋葱阶段
  - `useStage(spec)`：注册完整 `StageSpec`
  - `onError(hook_fn)`：注册流水线全局错误回调钩子
  - `setStageEnabled(name, enabled)`：动态启停指定阶段（不存在返回 `StageNotFound`）
  - `stageCount(): usize`
  - `execute(ctx: *PipelineContext): !PipelineError`

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { Pipeline, PipelineContext, PipelineError } from "cap:pipeline";
import cap from "cap";

fn authBefore(ctx: *PipelineContext): !PipelineError {
    if (token := ctx.get("token")) {
        if (std.mem.eql(u8, token, "secret")) {
            ctx.set("user", "alice")?;
            return;
        }
    }
    ctx.halt("unauthorized");
}

fn logBefore(ctx: *PipelineContext): !PipelineError {
    ctx.set("logged", "1")?;
}

fn logAfter(ctx: *PipelineContext): !PipelineError {
    ctx.set("flushed", "1")?;
}

pub fn demoPipeline(): ! {
    const alloc = std.heap.page_allocator;
    let pipe = cap.pipeline.Pipeline.init(alloc, "http_api");
    defer pipe.deinit();

    pipe.useOnion("logger", logBefore, logAfter)?;
    pipe.use("auth", authBefore)?;

    let ctx = PipelineContext.init(alloc, "GET /v1/agents");
    defer ctx.deinit();
    ctx.set("token", "secret")?;

    pipe.execute(&ctx)?;
}
```
