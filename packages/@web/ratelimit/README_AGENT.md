# @web/ratelimit@1.0.0 (Agent Quickref)

> Official zero-heap fixed-slot sliding-window rate limiter plugin for cap:web | License: MIT
> Import: `import { ... } from "@web/ratelimit";` | Full types: `src/index.d.cp`

## Functions
- `export fn monotonicNowSec(): u64;`
- `export fn formatU64(val: u64, out_buf: []u8): string;`
- `export fn extractFirstToken(raw: string): string;`
- `export fn setTestClock(now_sec: u64);`
- `export fn resetGlobal();`
- `export fn rateLimitHook(ctx: &mut Context): bool;`
- `export fn createRateLimitPlugin(cfg: RateLimitConfig): Plugin;`
- `export fn registerRateLimit(app: &mut App, cfg: RateLimitConfig): &mut App;`
