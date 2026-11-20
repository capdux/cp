# CP 标准库：熔断器、指数退避抖动与舱壁限流器手册 (`cap:circuit`)

## 一、 架构总览与设计准则

`cap:circuit` 是 CP 语言官方内置的纯 CP **分布式弹性容错与过载保护库**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**。所有时间判定均通过确定性单调毫秒时间戳 `now_ms: i64` 显式注入，不依赖系统时钟或线程睡眠，确保生产可观测性与单元测试 100% 确定性可复现。支持 `import { ... } from "cap:circuit"` 与 `import cap from "cap"; cap.circuit.*` 动态挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/circuit/breaker.cp` | `// Source: cp/lib/circuit/breaker.cp (...)` | `CircuitError`、`CircuitState`、`CircuitConfig`、`CircuitStats`、`CircuitBreaker` 环形滑动窗口熔断状态机 |
| `lib/circuit/policy.cp` | `// Source: cp/lib/circuit/policy.cp (...)` | `JitterMode`、`BackoffPolicy`（SplitMix64 确定性指数退避抖动）、`Bulkhead`（并发舱壁限流器） |
| `lib/circuit/index.cp` | `// Source: cp/lib/circuit/index.cp (...)` | `cap:circuit` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与状态类型

- **`CircuitError`**：
  - `CircuitOpen`：熔断器处于 `Open` 状态拒绝请求。
  - `BulkheadFull`：舱壁并发配额已满。
  - `InvalidConfig`：配置参数非法（如 `window_size == 0`、`minimum_calls == 0`、`minimum_calls > window_size`、`failure_threshold_rate` 不在 `(0.0, 1.0]`、`open_duration_ms < 0`、`base_delay_ms <= 0`、`max_delay_ms < base_delay_ms`、`multiplier < 1.0`、`max_concurrent == 0`）。
  - `OutOfMemory`：环形窗口缓冲区分配失败。
- **`CircuitState`**：`Closed`（闭合放行）、`Open`（熔断拦截）、`HalfOpen`（半开探测）。
- **`JitterMode`**：`None`（无抖动）、`Full`（全抖动 `[0, cap]`）、`Equal`（等半抖动 `[cap/2, cap]`）、`Decorrelated`（去相关抖动 `[base, min(max, prev * 3)]`）。

### 2. `CircuitBreaker` 熔断器

- **`init(alloc: std.mem.Allocator, config: CircuitConfig): CircuitBreaker ! CircuitError`**：校验配置并分配 `window_size` 大小的环形滑动计数窗口。
- **`deinit(*_)`**：释放环形窗口内存。
- **`allowRequest(*_, now_ms: i64): bool`**：
  - `Closed`：放行并返回 `true`；
  - `Open`：若 `now_ms - opened_at_ms >= open_duration_ms`，自动跃迁为 `HalfOpen` 并受限于 `half_open_max_calls` 放行探测请求；否则拒绝并递增 `rejected_count`；
  - `HalfOpen`：在 `half_open_max_calls` 配额内放行探测请求，超出则拒绝并递增 `rejected_count`。
- **`recordSuccess(*_, now_ms: i64)`**：
  - `Closed`：向环形窗口写入成功记录（覆盖最旧槽位）；
  - `HalfOpen`：递增成功计数，达到 `half_open_success_threshold` 后恢复 `Closed` 并重置滑动窗口。
- **`recordFailure(*_, now_ms: i64)`**：
  - `Closed`：向环形窗口写入失败记录；当 `window_calls >= minimum_calls` 且 `failure_rate >= failure_threshold_rate` 时触发熔断进入 `Open`；
  - `HalfOpen`：任一探测失败立即退回 `Open` 并重新计时。
- **`forceOpen(*_, now_ms: i64)` / `forceClose(*_)` / `reset(*_)` / `stats(*_, now_ms: i64): CircuitStats`**。

### 3. `BackoffPolicy` 与 `Bulkhead`

- **`BackoffPolicy.init(base_delay_ms, max_delay_ms, multiplier, max_attempts, jitter): BackoffPolicy ! CircuitError`**：构造指数退避策略。
- **`canRetry(*_, attempt: u32): bool`**：判断 `attempt < max_attempts`。
- **`nextDelayMs(*_, attempt: u32, prev_delay_ms: i64, seed: u64): i64`**：基于 SplitMix64 确定性 PRNG 计算下次退避毫秒数，所有模式均严格封顶于 `max_delay_ms`。
- **`Bulkhead.init(max_concurrent: usize, max_queue: usize): Bulkhead ! CircuitError`**：构造并发舱壁限流器。
- **`tryAcquire(*_): bool`**：非阻塞获取并发许可，满载时递增 `rejected_count` 并返回 `false`。
- **`releasePermit(*_)`**：归还并发许可（注意：方法名为 `releasePermit`，避免与编译器 ARC 关键字冲突）。
- **`activeCount(*_): usize`** / **`rejectedCount(*_): u64`** / **`maxConcurrent(*_): usize`** / **`maxQueue(*_): usize`**。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    CircuitConfig,
    CircuitBreaker,
    JitterMode,
    BackoffPolicy,
    Bulkhead,
} from "cap:circuit";

pub fn demoCircuit(): ! {
    const alloc = std.heap.page_allocator;
    let cb = CircuitBreaker.init(alloc, CircuitConfig{
        failure_threshold_rate: 0.5,
        minimum_calls: 4,
        window_size: 8,
        open_duration_ms: 5000,
        half_open_max_calls: 2,
        half_open_success_threshold: 2,
    })?;
    defer cb.deinit();

    if (cb.allowRequest(1000)) {
        cb.recordFailure(1000);
    }

    let backoff = BackoffPolicy.init(100, 5000, 2.0, 5, JitterMode.Full)?;
    const delay_ms = backoff.nextDelayMs(2, 200, 42);
    * = delay_ms;

    let bh = Bulkhead.init(4, 16)?;
    if (bh.tryAcquire()) {
        defer bh.releasePermit();
    }
}
```
