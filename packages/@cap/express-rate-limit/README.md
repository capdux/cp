# @cap/express-rate-limit

Zero-heap fixed-window HTTP rate limiter in pure CP with fixed-capacity bucket table (64 slots), expired-slot eviction, and IETF draft-7 `RateLimit` header formatting.

## API

- `RateLimitHit { allowed: bool, limit: u32, current: u32, remaining: u32, reset_at_ms: i64, retry_after_sec: u32 }`
- `RateLimiter`
  - `RateLimiter.init(window_ms: i64, max_requests: u32): RateLimiter`
  - `limiter.hit(key: string, now_ms: i64): RateLimitHit`
  - `limiter.resetKey(key: string)`
  - `limiter.resetAll()`
  - `limiter.activeCount(now_ms: i64): usize`
- `formatRateLimitHeader(hit: RateLimitHit, out: []u8): usize` — formats IETF draft-7 `limit=X, remaining=Y, reset=Z`.

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`express-rate-limit@7.5.0`)
- **Upstream Repository**: https://github.com/express-rate-limit/express-rate-limit
- **License**: MIT (clean-room CP implementation)
