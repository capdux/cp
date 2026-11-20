# CP 标准库：确定性限流器手册 (`cap:ratelimit`)

## 一、 架构总览与设计准则

`cap:ratelimit` 是 CP 语言官方内置的纯 CP **高性能确定性限流算法库**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**。所有时间判定均通过显式单调毫秒时间戳 `now_ms: i64` 注入，不依赖系统时钟或 libc，确保高并发网关与单元测试 100% 确定性可复现。支持 `import { ... } from "cap:ratelimit"` 与 `import cap from "cap"; cap.ratelimit.*` 动态挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/ratelimit/ratelimit.cp` | `// Source: cp/lib/ratelimit/ratelimit.cp (...)` | `RateLimitError`、`RateDecision`、`TokenBucket`、`SlidingWindowCounter`、`GcraLimiter`、`KeyedRateLimiter` |
| `lib/ratelimit/index.cp` | `// Source: cp/lib/ratelimit/index.cp (...)` | `cap:ratelimit` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与决策结构

- **`RateLimitError`**：
  - `InvalidRate`：补给速率 `refill_per_sec <= 0`（或 NaN/Inf）或 `emission_interval_ms <= 0`。
  - `InvalidBurst`：突发容量 `capacity == 0` 或窗口上限 `limit == 0`。
  - `InvalidWindow`：滑动窗口时长 `window_ms <= 0`。
  - `InvalidCost`：单次扣减配额 `cost == 0` 或 `cost` 超过最大突发容量。
  - `OutOfMemory`：`KeyedRateLimiter` 键拷贝或哈希表扩容失败。
- **`RateDecision`**：
  - `allowed: bool`：本次请求是否放行。
  - `remaining: u64`：扣减后（或探测时）当前可用剩余配额。
  - `retry_after_ms: i64`：若被拒绝，距离下次可放行的最小等待毫秒数（放行时为 `0`）。
  - `reset_after_ms: i64`：恢复到满配额/窗口重置所需的毫秒数。

### 2. `TokenBucket` 令牌桶

- **`init(capacity: u64, refill_per_sec: f64, now_ms: i64): TokenBucket ! RateLimitError`**：构造满令牌的令牌桶。
- **`initWithTokens(capacity: u64, refill_per_sec: f64, initial_tokens: u64, now_ms: i64): TokenBucket ! RateLimitError`**：以指定初始令牌数构造令牌桶（自动钳位至 `capacity`）。
- **`tryAcquire(*_, cost: u64, now_ms: i64): RateDecision ! RateLimitError`**：按 `now_ms` 平滑补给并尝试扣减 `cost`；遭遇时钟回拨（`now_ms <= last_refill_ms`）时安全钳位不倒扣。
- **`peek(*_, now_ms: i64): RateDecision`**：零配额非消耗探测，返回当前可用令牌与重置时间。
- **`reset(*_, now_ms: i64)`**：重置令牌桶至满容量。

### 3. `SlidingWindowCounter` 双窗口平滑计数

- **`init(limit: u64, window_ms: i64, now_ms: i64): SlidingWindowCounter ! RateLimitError`**：构造双窗口平滑限流器。
- **`tryAcquire(*_, cost: u64, now_ms: i64): RateDecision ! RateLimitError`**：按加权公式 `prev_count * (1 - elapsed / window_ms) + curr_count` 判定放行，消除固定窗口边界双倍突发毛刺。
- **`currentCount(*_, now_ms: i64): f64`**：返回当前时刻的平滑加权请求数。
- **`reset(*_, now_ms: i64)`**：清空前后窗口计数并重置窗口起点。

### 4. `GcraLimiter` 通用信元速率算法（O(1) 单状态）

- **`init(emission_interval_ms: i64, burst_tolerance: u64): GcraLimiter ! RateLimitError`**：以发射间隔 `T` 与突发容忍度构造 GCRA 限流器（最大可突发 `burst_tolerance + 1`）。
- **`tryAcquire(*_, cost: u64, now_ms: i64): RateDecision ! RateLimitError`**：仅维护单一 `tat_ms` 状态变量即可完成 O(1) 精确限流，适合无锁单变量及 Redis 状态映射。
- **`tatMs(*_): i64` / `reset(*_)`**：读取内部理论到达时间 `tat_ms` 或重置状态。

### 5. `KeyedRateLimiter` 多租户/API Key 隔离限流器

- **`init(alloc: std.mem.Allocator, capacity: u64, refill_per_sec: f64): KeyedRateLimiter ! RateLimitError`**：构造按租户 Key 隔离的多键令牌桶容器。
- **`deinit(*_)`**：释放全部内部拷贝键与哈希表内存。
- **`tryAcquireFor(*_, key: string, cost: u64, now_ms: i64): RateDecision ! RateLimitError`**：按 `key` 路由限流并刷新最近活跃时间。
- **`evictIdle(*_, now_ms: i64, idle_ttl_ms: i64): usize`**：清理空闲时长 `>= idle_ttl_ms` 的过期租户键并归还内存，返回清理数量。
- **`keyCount(*_): usize`**：返回当前活跃跟踪的租户键数量。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    TokenBucket,
    SlidingWindowCounter,
    GcraLimiter,
    KeyedRateLimiter,
} from "cap:ratelimit";

pub fn demoRateLimit(): ! {
    let tb = TokenBucket.init(10, 5.0, 1000)?;
    const d1 = tb.tryAcquire(4, 1000)?;
    * = d1;

    let sw = SlidingWindowCounter.init(100, 1000, 0)?;
    const d2 = sw.tryAcquire(10, 250)?;
    * = d2;

    let gcra = GcraLimiter.init(100, 2)?;
    const d3 = gcra.tryAcquire(1, 1000)?;
    * = d3;

    let keyed = KeyedRateLimiter.init(std.heap.page_allocator, 5, 2.0)?;
    defer keyed.deinit();
    const d4 = keyed.tryAcquireFor("tenant-alpha", 2, 1000)?;
    * = d4;
}
```
