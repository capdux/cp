# `cap:rate` — Zero-Alloc Rate Limiting Suite

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
