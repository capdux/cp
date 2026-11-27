# governor@1.0.0 (Agent Quickref)

> Zero-allocation IETF HTTP RateLimit headers (RateLimit-Policy / RateLimit / X-RateLimit-*) parser, formatter, and reset delay calculator for CP. | License: MIT
> Import: `import { ... } from "governor";` | Full types: `src/index.d.cp`

## Functions
- `export fn parseUintHeader(value: string): i64;`
- `export fn parsePolicy(header: string): RateLimitPolicy;`
- `export fn formatPolicy(quota: u64, window_secs: u64, out: []u8): usize;`
- `export fn parseStructuredRateLimit(header: string): RateLimitState;`
- `export fn formatStructuredRateLimit(state: RateLimitState, out: []u8): usize;`
- `export fn isExhausted(state: RateLimitState): bool;`
- `export fn computeResetDelaySecs(reset_val: u64, now_unix_secs: u64): u64;`
