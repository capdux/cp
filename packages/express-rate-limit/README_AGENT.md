# express-rate-limit@7.5.0 (Agent Quickref)

> Zero-heap fixed-window HTTP rate limiter with IETF draft-7 RateLimit header formatting | License: MIT
> Import: `import { ... } from "express-rate-limit";` | Full types: `src/index.d.cp`

## Functions
- `export fn erlEmptySlot(): RateLimitSlot;`
- `export fn erlHashKey(key: string): u64;`
- `export fn erlSlotMatches(slot: RateLimitSlot, h: u64, key: string): bool;`
- `export fn erlStoreSlot(h: u64, key: string, count: u32, reset_at_ms: i64): RateLimitSlot;`
- `export fn erlCeilSeconds(reset_at_ms: i64, now_ms: i64): u32;`
- `export fn erlWriteStr(out: []u8, pos: usize, s: string): usize;`
- `export fn erlWriteU32(out: []u8, pos: usize, val: u32): usize;`
- `export fn formatRateLimitHeader(hit: RateLimitHit, out: []u8): usize;`
