# `cap:rate` — Zero-Alloc Rate Limiting Suite

> 参考来源：/data/dev/bun/src/http/HTTPContext.cp | 吸收日期：2026-11-22

Pure-CP zero-allocation rate limiting algorithms converged from `rate_sliding_window`, `rate_token_bucket`, and `token_bucket`.

## Exports

### Sliding Window & GCRA (`./sliding.cp`)
- `SlidingWindowCounter` — Weighted sliding-window request counter
- `SlidingGcraLimiter` — Generic Cell Rate Algorithm (GCRA) leaky-bucket limiter

### Token Bucket & GCRA (`./bucket.cp`)
- `RateLimitError`
- `TokenBucket` — Precision token bucket rate limiter
- `GcraLimiter` — GCRA limiter with error/retry metadata

### Fixed Window & Simple Token Bucket (`./limiter.cp`)
- `SimpleTokenBucket` — Lightweight integer token bucket limiter
- `FixedWindowLimiter` — Fixed-interval window request limiter

## 上游行为契约与限流选型边界

1. **三种算法适用场景对比**：
   - **`TokenBucket` / `SimpleTokenBucket`**：允许受控突发（burst），适合 API 网关与外部 LLM 调用令牌配额控制；
   - **`GcraLimiter` / `SlidingGcraLimiter`**：基于理论到达时间（TAT）的恒定内存漏桶算法，每条限流键仅需单时间戳状态，平滑流量且精准给出下一次允许重试时间；
   - **`SlidingWindowCounter` / `FixedWindowLimiter`**：加权滑动窗口消除固定窗口临界双倍突发问题，固定窗口适合轻量级计数统计。
2. **时钟单调性与零堆分配边界**：
   - 所有限流器均由调用方显式传入单调时间戳，内部对时钟回拨与大时间跨度执行饱和钳制，无后台定时器线程、零堆分配。
