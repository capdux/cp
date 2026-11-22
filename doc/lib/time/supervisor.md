# `cap:supervisor` — OTP 监督树状态机与分布式弹性原语

> [!NOTE]
> 本子模块已归并至统一主包 `cap:time`，导入请使用 `import { ... } from "cap:time"`。

> **模块路径**: `lib/supervisor/`  
> **导入协议**: `import { SupervisorTree, BackoffPolicy, Bulkhead, RetryBudget, ... } from "cap:time";`  
> **动态挂载**: `import cap from "cap";` -> `cap.supervisor.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:supervisor` 紧扣 CP 语言「BEAM 容错调度 + 百万 AI Agent 并发」核心架构，提供零外部依赖、100% 确定性的工业级容错与防雪崩原语：

1. **OTP 监督树状态机 (`SupervisorTree`)**：
   - 严格实现 Erlang/OTP 三大重启拓扑策略：`OneForOne`（单点重启）、`OneForAll`（全树逆序停止 + 顺序重启）与 `RestForOne`（故障节点及下游依赖逆序停止 + 顺序重启）；
   - 精确区分三种子进程重启策略：`Permanent`（总是重启）、`Transient`（仅 `Abnormal` 异常重启）、`Temporary`（永不重启，且在 `OneForAll`/`RestForOne` 联动停止后不重启）；
   - 内置滑动时间窗 `[now_ms - period_ms, now_ms]` 重启风暴熔断保护：窗口内超 `max_restarts` 自动触发 `EscalateShutdown`，逆序输出所有需停止的活跃子项并锁定监督者状态。
2. **确定性退避抖动策略 (`BackoffPolicy`)**：
   - 支持 `Fixed`、`Linear`、`Exponential`、`FullJitter`、`DecorrelatedJitter` 五种退避模式；
   - 内置确定性 SplitMix64 伪随机数生成器与 `max_ms` 硬上限钳位，支持 `resetSeed` 复现任意抖动序列。
3. **并发舱壁限流器 (`Bulkhead`)**：
   - 限制同时在飞执行槽位（`max_concurrent`）与可选排队容量（`max_queue`），统计实时并发、拒绝计数与历史峰值水位（`peakActive`），采用明确命名的 `releaseSlot()` 避免与 ARC `.release()` 冲突。
4. **滑动窗口重试预算 (`RetryBudget`)**：
   - 参考 Envoy / Finagle 设计，基于 O(1) 空间时间轮统计滑动窗口内主请求与重试请求比例，保障低流量基线（`min_retries_per_window`）同时按基点上限（`retry_ratio_bps`）拦截全链路重试风暴。

---

## 二、 核心 API 速查表

### 1. OTP 监督树状态机 (`lib/supervisor/strategy.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `SupervisorError` | `ChildNotFound`, `DuplicateChildId`, `InvalidConfig`, `MaxRestartsExceeded`, `BulkheadFull`, `OutOfMemory` |
| `RestartStrategy` | `OneForOne`, `OneForAll`, `RestForOne` |
| `ChildRestartPolicy` | `Permanent`, `Transient`, `Temporary`（附 `shouldRestart(reason)`） |
| `ExitReason` | `Normal`, `Shutdown`, `Abnormal` |
| `ChildState` | `Running`, `Stopped`, `Restarting`, `Terminated`（附 `isActive()`） |
| `SupervisionDecisionKind` | `Ignore`, `RestartChildren`, `EscalateShutdown` |
| `ChildSpec` | `{ id: string, restart_policy: ChildRestartPolicy, shutdown_timeout_ms: u64 }` |
| `SupervisorConfig` | `{ strategy: RestartStrategy, max_restarts: u32, period_ms: u64 }` |
| `SupervisionDecision` | `{ kind, stop_order, restart_order, window_restarts }`，调用 `deinit()` 释放自有切片内存 |
| `SupervisorTree.init(alloc, config)` | 初始化监督树（`period_ms == 0` 返回 `InvalidConfig`） |
| `deinit(*_)` | 释放全部子项与时间窗历史 |
| `addChild(*_, spec)` | 按注册顺序添加子项（初始为 `Running`，拒绝空 ID 或重复 ID） |
| `removeChild(*_, id)` | 按 ID 移除子项并保持剩余拓扑顺序 |
| `childState(id)` / `runningCount()` | 查询子项状态与当前 `Running` 数量 |
| `handleChildExit(*_, id, reason, now_ms)` | 评估退出原因、校验重启风暴窗口并生成精确停止/重启拓扑计划 |

### 2. 分布式弹性原语 (`lib/supervisor/resilience.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `BackoffKind` | `Fixed`, `Linear`, `Exponential`, `FullJitter`, `DecorrelatedJitter` |
| `BackoffPolicy.init(kind, base_ms, max_ms, multiplier_x100, seed)` | 创建确定性 SplitMix64 退避策略 |
| `nextDelayMs(*_, attempt, prev_delay_ms)` / `resetSeed(*_, seed)` | 计算受 `max_ms` 约束的下一步退避延迟或重置随机种子 |
| `Bulkhead.init(max_concurrent, max_queue)` | 创建并发舱壁限流器 |
| `tryAcquire(*_)` / `acquire(*_)` / `tryEnqueue(*_)` / `releaseSlot(*_)` | 非阻塞抢占/报错获取、入队与释放槽位（释放时自动唤醒排队项） |
| `activeCount()` / `availableSlots()` / `rejectedCount()` / `peakActive()` | 查询当前并发、可用槽位、拒绝计数与历史并发峰值 |
| `RetryBudget.init(window_ms, min_retries_per_window, retry_ratio_bps)` | 创建零堆分配滑动窗口重试预算器 |
| `recordRequest(*_, now_ms)` / `tryConsumeRetry(*_, now_ms)` | 记录主请求扩容预算 / 尝试消耗重试配额 |
| `currentRatioBps(*_, now_ms)` / `remainingRetries(*_, now_ms)` / `rejectedRetries()` | 查询当前窗口重试率（bps）、剩余配额与拦截总数 |

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";

fn runSupervisorExample(): !void {
    const alloc = std.heap.page_allocator;

    let tree = cap.supervisor.SupervisorTree.init(alloc, cap.supervisor.SupervisorConfig{
        strategy: cap.supervisor.RestartStrategy.RestForOne,
        max_restarts: 3,
        period_ms: 5000,
    })?;
    defer tree.deinit();

    tree.addChild(cap.supervisor.ChildSpec{ id: "db", restart_policy: .Permanent })?;
    tree.addChild(cap.supervisor.ChildSpec{ id: "cache", restart_policy: .Permanent })?;
    tree.addChild(cap.supervisor.ChildSpec{ id: "worker", restart_policy: .Transient })?;

    let decision = tree.handleChildExit("cache", .Abnormal, 1000)?;
    defer decision.deinit();
    // decision.stop_order == ["worker"], decision.restart_order == ["cache", "worker"]

    let bh = cap.supervisor.Bulkhead.init(4, 0);
    if (bh.tryAcquire()) {
        defer bh.releaseSlot();
    }
}
```
