# `cap:rate` — Zero-Alloc Industrial Rate Limiting Suite

> 对标 Rust [`governor`](https://docs.rs/governor) 工业级 GCRA 算法与 IETF Draft `RateLimit` / `Retry-After` 标准响应头规范。  
> 纯 CP 实现、零堆分配、零后台定时器线程、防时钟回拨、全 `u128` 防溢出。

```typescript
import {
    Quota,
    RateDecision,
    GcraLimiter,
    formatRateLimitHeaders,
    formatRetryAfterHeader,
    formatRetryAfterSeconds,
    formatResetAfterSeconds,
    TokenBucket,
    SimpleTokenBucket,
    SlidingWindowCounter,
    SlidingGcraLimiter,
    FixedWindowLimiter,
} from "cap:rate";
```

---

## 一、 算法对比与选型指南

| 算法 / 类型 | 核心原理 | 时间精度 | 内存占用 | 突发控制 | 临界双倍突发 | 推荐场景 |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **`GcraLimiter`** (`gcra.cp`) ⭐ | 理论到达时间（TAT）虚拟漏桶（对标 Rust `governor`） | 纳秒 (`u64`) | 24 字节 (`Quota` + `tat_ns`) | 精确 `withBurst(b)` + 批量 `checkNAt` | 无 | **首选**：API 网关、RPC/LLM 配额控制、HTTP `429` + `RateLimit-*` 响应头 |
| **`TokenBucket`** (`bucket.cp`) | 浮点连续补充令牌桶 | 纳秒 (`u64`) | 32 字节 | 支持小数消耗 (`f64`) | 无 | 非整数权重请求、连续带宽整形 |
| **`SimpleTokenBucket`** (`limiter.cp`) | 纯整数毫令牌（1 token = 1000 milli-tokens）令牌桶 | 毫秒 (`u64`) | 32 字节 | 精确整数突发、亚秒级零舍入损失 | 无 | 嵌入式 / 无浮点场景、确定性整数量化配额 |
| **`SlidingWindowCounter`** (`sliding.cp`) | 双窗口加权滑动计数 | 毫秒 (`u64`) | 40 字节 | 窗口总量上限 | 近似消除 | 分布式/租户级时间窗配额统计 |
| **`SlidingGcraLimiter`** (`sliding.cp`) | 毫秒级轻量 GCRA | 毫秒 (`u64`) | 24 字节 | 整数突发容忍 | 无 | 毫秒时钟下的轻量级平滑限流 |
| **`FixedWindowLimiter`** (`limiter.cp`) | 固定时间窗口计数 | 毫秒 (`u64`) | 32 字节 | 窗口内任意突发 | 存在 | 简单计数统计、低成本粗粒度门控 |

### 时钟单调性与确定性契约
- 所有限流器均不隐式调用系统时钟，由调用方显式传入单调时间戳（`now_ns` / `now_ms`），天然支持确定性单元测试与无锁嵌入；
- `GcraLimiter` 内部使用 `u128` 执行 TAT 与突发窗口比较，免疫 `u64` 乘法/加法溢出；拒绝请求时**绝不推进 `tat_ns`**，且时钟回拨绝不赠送额外配额。

---

## 二、 全部导出 API 一览

### 1. GCRA 限流器与配额 (`./gcra.cp`)

| 符号 | 类型 / 签名 | 说明 |
| :--- | :--- | :--- |
| `NS_PER_SECOND` | `const u64 = 1_000_000_000` | 每秒纳秒数 |
| `NS_PER_MINUTE` | `const u64 = 60_000_000_000` | 每分钟纳秒数 |
| `Quota.perSecond(n: u64): Quota` | 静态工厂 | 每秒 `n` 次，默认突发容量 `n`（`0` 自动钳制为 `1`） |
| `Quota.perMinute(n: u64): Quota` | 静态工厂 | 每分钟 `n` 次，默认突发容量 `n`（`0` 自动钳制为 `1`） |
| `Quota.perIntervalNs(interval_ns: u64, burst: u64): Quota` | 静态工厂 | 自定义单次发射间隔（ns）与突发容量 |
| `quota.withBurst(b: u64): Quota` | 只读方法 | 返回速率不变、突发容量调整为 `b` 的新 `Quota` |
| `quota.emissionIntervalNs(): u64` | 只读方法 | 单次发射间隔 $T$（ns） |
| `quota.burstCapacity(): u64` | 只读方法 | 最大突发容量 $B$ |
| `quota.burstToleranceNs(): u64` | 只读方法 | GCRA 突发容忍度 $\tau = (B - 1) \cdot T$（饱和防溢出） |
| `RateDecision` | `struct` | 字段：`allowed: bool`, `remaining: u64`, `limit: u64`, `retry_after_ns: u64`, `reset_after_ns: u64`；方法：`retryAfterSeconds()`, `resetAfterSeconds()` |
| `GcraLimiter.init(quota: Quota): GcraLimiter` | 静态构造 | 从 `Quota` 初始化 GCRA 限流器 |
| `lim.checkAt(*_, now_ns: u64): RateDecision` | 可变方法 | 检查并消耗 1 个配额，返回完整 `RateDecision` |
| `lim.checkNAt(*_, n: u64, now_ns: u64): RateDecision` | 可变方法 | 原子检查并消耗 `n` 个配额；不足时拒绝且不推进 TAT；`n > max_burst` 返回 `retry_after_ns = U64_MAX` |
| `lim.allow(*_, now_ns: u64): bool` | 可变方法 | `checkAt(now_ns).allowed` 便捷方法 |
| `lim.peek(now_ns: u64): RateDecision` | 只读方法 | 探测当前余量与放行状态，不修改内部 `tat_ns` |
| `lim.reset(*_)` | 可变方法 | 重置 `tat_ns = 0`，立即恢复满突发容量 |
| `lim.getQuota(): Quota` | 只读方法 | 返回当前配置的 `Quota` |

### 2. IETF Draft `RateLimit` / `Retry-After` 响应头格式化 (`./gcra.cp`)

| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `RateLimitHeaderLengths` | `struct { limit_len: usize; remaining_len: usize; reset_len: usize; }` | 三个响应头各自写入的 ASCII 字节长度 |
| `formatRetryAfterSeconds(decision: RateDecision): u64` | 函数 | 计算 HTTP `Retry-After` 整数秒（向上取整；放行时返回 `0`） |
| `formatResetAfterSeconds(decision: RateDecision): u64` | 函数 | 计算 `RateLimit-Reset` 整数秒（向上取整） |
| `formatRateLimitHeaders(decision, out_limit, out_remaining, out_reset): RateLimitHeaderLengths` | 函数 | 零堆分配格式化 `RateLimit-Limit` / `RateLimit-Remaining` / `RateLimit-Reset` 到调用方栈缓冲区 |
| `formatRetryAfterHeader(decision, out): usize` | 函数 | 零堆分配格式化 `Retry-After` ASCII 字节串到 `out` |

### 3. 令牌桶与窗口限流器 (`./bucket.cp`, `./sliding.cp`, `./limiter.cp`)

- **`./bucket.cp`**：`RateLimitError`、`TokenBucket`（`init`, `refill`, `allow`, `available`, `retryAfterNs`）、`BucketGcraLimiter`
- **`./sliding.cp`**：`SlidingWindowCounter`（`init`, `estimatedCount`, `tryAcquire`, `remaining`）、`SlidingGcraLimiter`（`init`, `checkAndAcquire`）
- **`./limiter.cp`**：`SimpleTokenBucket`（`init`, `initWithTokens`, `refill`, `available`, `tryConsume`, `timeUntilReadyMs`）、`FixedWindowLimiter`（`init`, `allow`, `remaining`, `resetAfterMs`）

---

## 三、 完整可运行使用示例

### 示例 1：GCRA 突发控制与平滑流量整形

```typescript
import { Quota, GcraLimiter, NS_PER_SECOND } from "cap:rate";
import { print } from "cap:io";

pub fn main() {
    // 每秒 10 次（间隔 100ms），允许瞬时突发 3 次
    const quota = Quota.perSecond(10).withBurst(3);
    let limiter = GcraLimiter.init(quota);

    let now_ns: u64 = 1 * NS_PER_SECOND;

    // 瞬时连续 4 个请求：前 3 个放行，第 4 个被拒绝并给出精确等待时间 100ms
    let i: u64 = 0;
    while (i < 4) {
        const d = limiter.checkAt(now_ns);
        if (d.allowed) {
            print("req allowed, remaining=" + d.remaining + "/" + d.limit);
        } else {
            print("req rejected, retry_after_ns=" + d.retry_after_ns);
        }
        i += 1;
    }

    // 推进 100ms 后平滑释放 1 个名额
    now_ns += 100_000_000;
    const next = limiter.checkAt(now_ns);
    print("after 100ms allowed=" + next.allowed + ", reset_after_ns=" + next.reset_after_ns);
}
```

### 示例 2：批量配额扣减（`checkNAt` 用于 LLM Token / 批量 RPC 计费）

```typescript
import { Quota, GcraLimiter, NS_PER_SECOND } from "cap:rate";
import { print } from "cap:io";

pub fn main() {
    // 每分钟 6000 token（即每 10ms 恢复 1 token），最大单次突发 1000 token
    const quota = Quota.perMinute(6000).withBurst(1000);
    let limiter = GcraLimiter.init(quota);

    let now_ns: u64 = 10 * NS_PER_SECOND;

    // 一次扣减 700 token -> 放行，剩余 300
    const d1 = limiter.checkNAt(700, now_ns);
    print("batch 700: allowed=" + d1.allowed + ", remaining=" + d1.remaining);

    // 立即申请 500 token -> 原子拒绝（不扣减任何余量），告知需等待 2s（200 * 10ms）
    const d2 = limiter.checkNAt(500, now_ns);
    if (!d2.allowed) {
        print("batch 500 rejected, wait ns=" + d2.retry_after_ns + ", still remaining=" + d2.remaining);
        // 等待 retry_after_ns 后重试必定放行
        now_ns += d2.retry_after_ns;
        const d3 = limiter.checkNAt(500, now_ns);
        print("retry batch 500: allowed=" + d3.allowed + ", remaining=" + d3.remaining);
    }
}
```

### 示例 3：HTTP 中间件集成 `429 Too Many Requests` + IETF `RateLimit-*` & `Retry-After` 响应头

```typescript
import {
    Quota,
    GcraLimiter,
    formatRateLimitHeaders,
    formatRetryAfterHeader,
    formatRetryAfterSeconds,
    NS_PER_SECOND,
} from "cap:rate";
import { print } from "cap:io";

pub struct HttpRateOutcome {
    status: u16;
    retry_after_sec: u64;
    remaining: u64;
}

fn handleApiRequest(limiter: *GcraLimiter, now_ns: u64): HttpRateOutcome {
    const decision = limiter.checkAt(now_ns);

    // 栈上零堆分配格式化 IETF Draft RateLimit-* 响应头
    let limit_buf: [24]u8 = [0; 24];
    let rem_buf: [24]u8 = [0; 24];
    let reset_buf: [24]u8 = [0; 24];
    const lens = formatRateLimitHeaders(decision, limit_buf[0..], rem_buf[0..], reset_buf[0..]);

    if (!decision.allowed) {
        let retry_buf: [24]u8 = [0; 24];
        const retry_len = formatRetryAfterHeader(decision, retry_buf[0..]);
        // 在 HTTP 响应中写入：
        //   HTTP/1.1 429 Too Many Requests
        //   RateLimit-Limit:     limit_buf[0..lens.limit_len]
        //   RateLimit-Remaining: rem_buf[0..lens.remaining_len]
        //   RateLimit-Reset:     reset_buf[0..lens.reset_len]
        //   Retry-After:         retry_buf[0..retry_len]
        if (lens.limit_len > 0 && retry_len > 0) {
            return HttpRateOutcome{
                status: 429,
                retry_after_sec: formatRetryAfterSeconds(decision),
                remaining: decision.remaining,
            };
        }
    }

    return HttpRateOutcome{
        status: 200,
        retry_after_sec: 0,
        remaining: decision.remaining,
    };
}

pub fn main() {
    let limiter = GcraLimiter.init(Quota.perSecond(2).withBurst(2));
    const t0: u64 = 5 * NS_PER_SECOND;

    const r1 = handleApiRequest(*limiter, t0);
    const r2 = handleApiRequest(*limiter, t0);
    const r3 = handleApiRequest(*limiter, t0);
    print("statuses: " + r1.status + ", " + r2.status + ", " + r3.status + " (Retry-After: " + r3.retry_after_sec + "s)");
}
```
