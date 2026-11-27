# @sapphire/snowflake — AI Agent Quick Ref

Zero-heap 64-bit Twitter/Discord Snowflake distributed ID generator, decomposer, clock-rollback guard, and decimal formatter. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SnowflakeParts,
    SnowflakeGenerator,
    compose,
    decompose,
    initGenerator,
    nextId,
    formatId,
    parseId,
} from "@sapphire/snowflake";
```

## Core Types & Constants

```cp
class SnowflakeParts { timestamp_ms: u64, datacenter_id: u8, worker_id: u8, sequence: u16 }
class SnowflakeGenerator { epoch_ms: u64, datacenter_id: u8, worker_id: u8, last_ts_ms: u64, /* +1 fields */ }
```

## API Signatures

```cp
fn compose(epoch_ms: u64, timestamp_ms: u64, datacenter_id: u8, worker_id: u8, sequence: u16): u64;
fn decompose(id: u64, epoch_ms: u64): SnowflakeParts;
fn initGenerator(epoch_ms: u64, datacenter_id: u8, worker_id: u8): SnowflakeGenerator;
fn nextId(gen: &mut SnowflakeGenerator, now_ms: u64): i64;
fn formatId(id: u64, out: []u8): usize;
fn parseId(s: string): i64;
```

## Minimal Usage

```cp
import { parseId } from "@sapphire/snowflake";

pub fn main() {
    let result = parseId("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@sapphire/snowflake"`; install via `cap add @sapphire/snowflake`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
