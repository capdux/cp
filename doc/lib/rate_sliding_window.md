# `cap:rate_sliding_window` — Weighted Sliding Window Counter & GCRA Rate Limiter

Zero-allocation, pure-integer weighted sliding window counter and Generic Cell Rate Algorithm (GCRA) smooth rate limiters with clock-rollback and overflow protection.

## Exports

- `SlidingWindowCounter` — `init(limit, window_ms, now_ms)`, `estimatedCount(*_, now_ms)`, `tryAcquire(*_, cost, now_ms)`, `remaining(*_, now_ms)`.
- `GcraLimiter` — `init(emission_interval_ms, burst_capacity, now_ms)`, `checkAndAcquire(*_, now_ms): u64` (`0` = admitted, `> 0` = exact retry-after milliseconds).
