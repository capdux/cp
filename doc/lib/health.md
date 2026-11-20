# CP 标准库：健康检查探针与优雅停机协调器手册 (`cap:health`)

## 一、 架构总览与设计准则

`cap:health` 是 CP 语言官方内置的纯 CP **云原生健康检查探针汇聚（Liveness / Readiness / Startup）与优雅停机生命周期协调器（`ShutdownCoordinator`）**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，专为微服务、Kubernetes 工作负载与分布式 AI Agent 运行时设计。既可通过 `import { ... } from "cap:health"` 导入，也支持 `import cap from "cap"; cap.health.*` 动态懒挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/health/probe.cp` | `// Source: cp/lib/health/probe.cp (...)` | `HealthError`、`ProbeKind`、`HealthStatus`、`CheckOutcome`、`ProbeReport`、`HealthRegistry`、`ShutdownCoordinator` |
| `lib/health/index.cp` | `// Source: cp/lib/health/index.cp (...)` | `cap:health` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与探针类型

- **`HealthError`**：
  - `DuplicateCheck`：重复注册同名健康检查探针。
  - `CheckNotFound`：注销或查找不存在的探针名称。
  - `DuplicateHook`：重复注册同名停机钩子。
  - `AlreadyShuttingDown`：停机已触发后尝试继续注册新钩子。
  - `OutOfMemory`：底层缓冲区分配失败。
- **`ProbeKind`**：`Liveness`、`Readiness`、`Startup`、`All`（通配探针）。
- **`HealthStatus`**：`Up`、`Degraded`、`Down`。
- **`CheckOutcome`**：`status: HealthStatus = .Up`, `message: string = ""`, `latency_ms: u64 = 0`。
- **`ProbeReport`**：
  - `overall: HealthStatus`、`http_status: u16`（`200` 或 `503`）
  - `passing_count: usize`、`degraded_count: usize`、`failing_count: usize`
  - `details_json: string`（合规 RFC 8259 JSON 报告，需调用 `report.deinit()` 释放）

### 2. `HealthRegistry` 核心方法

- **`init(alloc: std.mem.Allocator): HealthRegistry`**：初始化健康检查注册表。
- **`deinit(*_)`**：释放所有已注册探针名称与内部缓冲区。
- **`registerCheck(*_, name: string, kind: ProbeKind, critical: bool, check_fn: CheckFn, ctx: *anyopaque): void ! HealthError`**：注册探针；同名返回 `HealthError.DuplicateCheck`。
- **`unregisterCheck(*_, name: string): void ! HealthError`**：按名称注销探针；不存在返回 `HealthError.CheckNotFound`。
- **`setDraining(*_, draining: bool)` / `isDraining(): bool`**：一键切换流量排空态。开启后 `Readiness` 探针立即返回 `Down` / HTTP `503` 摘除负载均衡流量，而 `Liveness` 仍保持自身检查状态（`Up` / `200`）避免被 K8s 误杀。
- **`evaluate(kind: ProbeKind): ProbeReport ! HealthError`**：聚合该类别与 `ProbeKind.All` 通配探针：
  - 排空态下的 `Readiness` 或任一 `critical == true` 且 `Down` -> `overall = .Down`、`http_status = 503`；
  - 非关键 `Down` 或任一 `Degraded` -> `overall = .Degraded`、`http_status = 200`；
  - 全部正常 -> `overall = .Up`、`http_status = 200`；
  - 生成完整转义的 RFC 8259 JSON 报告 `details_json`。

### 3. `ShutdownCoordinator` 核心方法

- **`init(alloc: std.mem.Allocator): ShutdownCoordinator`**：初始化优雅停机协调器。
- **`deinit(*_)`**：释放已注册钩子名称与执行序列缓冲。
- **`registerHook(*_, name: string, priority: i32, hook_fn: ShutdownHookFn, ctx: *anyopaque): void ! HealthError`**：注册停机钩子；`priority` 越大越先执行（如 HTTP 接入 `100` -> Worker 队列 `50` -> DB 连接池 `10`）。
- **`triggerShutdown(*_): usize`**：严格幂等触发停机。仅首次调用按 `priority` 降序（同优先级按注册顺序）依次执行全部钩子并返回执行总数；重复调用立即返回 `0` 且不重复执行。
- **`executedOrder(): []const string`**：返回实际执行的钩子名称有序切片。
- **`isShuttingDown(): bool`** / **`hookCount(): usize`**：查询停机状态与钩子数量。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    HealthRegistry,
    ShutdownCoordinator,
    ProbeKind,
    HealthStatus,
    CheckOutcome,
} from "cap:health";

fn checkDb(ctx_ptr: *anyopaque): CheckOutcome {
    * = ctx_ptr;
    return CheckOutcome{ status: .Up, message: "pg ok", latency_ms: 2 };
}

fn stopHttp(ctx_ptr: *anyopaque) {
    * = ctx_ptr;
}

pub fn demoHealth(ctx_ptr: *anyopaque): ! {
    const alloc = std.heap.page_allocator;
    let reg = HealthRegistry.init(alloc);
    defer reg.deinit();

    reg.registerCheck("postgres", ProbeKind.Readiness, true, checkDb, ctx_ptr)?;
    let report = reg.evaluate(ProbeKind.Readiness)?;
    defer report.deinit();

    // 流量排空：Readiness 切为 503，Liveness 保持 200
    reg.setDraining(true);

    let coord = ShutdownCoordinator.init(alloc);
    defer coord.deinit();
    coord.registerHook("http_server", 100, stopHttp, ctx_ptr)?;
    const ran = coord.triggerShutdown();
    * = ran;
}
```
