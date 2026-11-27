[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @web/ratelimit

Official zero-heap fixed-slot sliding-window rate limiter plugin for cap:web

| Metadata | Value |
| --- | --- |
| Package | `@web/ratelimit` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @web/ratelimit
```

```cp
import {
    RateLimitConfig,
    RateLimitDecision,
    RateLimitSlot,
    RateLimiter,
    monotonicNowSec,
    formatU64,
    extractFirstToken,
    setTestClock,
} from "@web/ratelimit";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RateLimitConfig` | `class` | Fields: `max_requests: u32, window_secs: u64, key_header: string` |
| `RateLimitDecision` | `class` | Fields: `allowed: bool, limit: u32, remaining: u32, retry_after: u64` |
| `RateLimitSlot` | `class` | Fields: `used: bool, key_buf: [RATELIMIT_MAX_KEY_LEN]u8, key_len: usize, window_start: u64, count: u32` · Methods: `matchesKey, setKey` |
| `RateLimiter` | `class` | Fields: `config: RateLimitConfig, slots: [RATELIMIT_MAX_SLOTS]RateLimitSlot` · Methods: `init, reset, check` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `monotonicNowSec` | `(): u64` | Invokes `monotonicNowSec` and returns `u64`. |
| `formatU64` | `(val: u64, out_buf: []u8): string` | Serializes/formats data via `formatU64` returning `string`. |
| `extractFirstToken` | `(raw: string): string` | Invokes `extractFirstToken` and returns `string`. |
| `setTestClock` | `(now_sec: u64)` | Mutates or appends state via `setTestClock`. |
| `resetGlobal` | `()` | Executes `resetGlobal` in-place. |
| `rateLimitHook` | `(ctx: &mut Context): bool` | Invokes `rateLimitHook` and returns `bool`. |
| `createRateLimitPlugin` | `(cfg: RateLimitConfig): Plugin` | Constructs/initializes an instance via `createRateLimitPlugin`. |
| `registerRateLimit` | `(app: &mut App, cfg: RateLimitConfig): &mut App` | Registers plugin/middleware into the target application. |

## Minimal Example

```cp
import { extractFirstToken } from "@web/ratelimit";

pub fn main() {
    let result = extractFirstToken("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
