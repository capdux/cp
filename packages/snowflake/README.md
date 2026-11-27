[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# snowflake

Zero-heap 64-bit Snowflake distributed ID generator, bit-field decoder, range bound calculator, and decimal string formatter/parser in pure CP (Twitter & Discord epoch compatible).

- **Upstream Alignment**: Inspired by Rust [`snowflake`](https://crates.io/crates/snowflake) (`crates.io`, `MIT`) and standard Twitter/Discord 41-5-5-12 bit layouts.
- **Package Version**: `1.0.0` (depends on `itoa`).

## Installation & Import

```bash
cap add snowflake
```

In `cap.json`:

```json
{
  "dependencies": {
    "snowflake": "^1.0.0"
  }
}
```

```typescript
import {
    SnowflakeGenerator, SnowflakeParts,
    TWITTER_EPOCH_MS, DISCORD_EPOCH_MS,
    SF_MAX_DATACENTER, SF_MAX_WORKER, SF_MAX_SEQUENCE,
    encodeSnowflake, compose, decodeSnowflake, decompose,
    timestamp, datacenterId, workerId, sequence,
    minIdForTimestamp, maxIdForTimestamp,
    formatSnowflake, parseSnowflake, parse, isValidSnowflakeString
} from "snowflake";
```

## Core API Reference

### Constants & `class SnowflakeParts`

| Symbol | Value / Fields | Description |
| :--- | :--- | :--- |
| `TWITTER_EPOCH_MS` | `1288834974657` | Twitter Snowflake custom epoch (2010-11-04 UTC) |
| `DISCORD_EPOCH_MS` | `1420070400000` | Discord Snowflake custom epoch (2015-01-01 UTC) |
| `SF_MAX_DATACENTER` / `SF_MAX_WORKER` / `SF_MAX_SEQUENCE` | `31` / `31` / `4095` | 5-bit datacenter, 5-bit worker, 12-bit sequence limits |
| `SF_TIMESTAMP_SHIFT` / `SF_DATACENTER_SHIFT` / `SF_WORKER_SHIFT` / `SF_MAX_TIMESTAMP` | `22` / `17` / `12` / `(1 << 41) - 1` | Bit layout constants |
| `SnowflakeParts` | `timestamp_ms: u64`, `datacenter_id: u16`, `worker_id: u16`, `sequence: u16` | Decoded ID components |

### `class SnowflakeGenerator`

| Method | Signature | Description |
| :--- | :--- | :--- |
| `new` | `SnowflakeGenerator.new(epoch_ms: u64, datacenter_id: u16, worker_id: u16): SnowflakeGenerator` | Create generator with custom epoch |
| `withTwitterEpoch` / `withDiscordEpoch` | `SnowflakeGenerator.withTwitterEpoch(datacenter_id: u16, worker_id: u16): SnowflakeGenerator` | Create generator using Twitter or Discord epoch |
| `nextId` | `nextId(*_, now_ms: u64): u64?` | Generate next monotonic ID; returns `null` on clock rollback or same-ms sequence overflow (`> 4095`) |
| `nextBatch` | `nextBatch(*_, now_ms: u64, out: []u64): usize` | Fill up to `out.len` IDs at `now_ms`; returns count generated |
| `compose` / `decompose` / `timestamp` | `compose(timestamp_ms: u64, seq: u16): u64` / `decompose(id: u64): SnowflakeParts` / `timestamp(id: u64): u64` | Encode/decode using generator's configured epoch and node IDs |
| `reset` | `reset(*_)` | Reset `last_ts_ms` and `sequence` to `0` |

### Free Functions

| Function | Signature | Description |
| :--- | :--- | :--- |
| `encodeSnowflake` / `compose` | `encodeSnowflake(epoch_ms: u64, timestamp_ms: u64, datacenter_id: u16, worker_id: u16, seq: u16): u64` | Bit-pack fields into a 64-bit Snowflake ID |
| `decodeSnowflake` / `decompose` | `decodeSnowflake(id: u64, epoch_ms: u64): SnowflakeParts` | Unpack ID into `SnowflakeParts` |
| `timestamp` / `datacenterId` / `workerId` / `sequence` | `timestamp(id: u64, epoch_ms: u64): u64` / `datacenterId(id: u64): u16` / ... | Extract individual fields |
| `minIdForTimestamp` / `maxIdForTimestamp` | `minIdForTimestamp(epoch_ms: u64, timestamp_ms: u64): u64` | Inclusive ID range bounds for time-window SQL/index queries |
| `formatSnowflake` | `formatSnowflake(id: u64, out: []u8): usize` | Format `u64` ID as decimal ASCII into `out` (zero-heap via `itoa`) |
| `parseSnowflake` / `parse` | `parse(s: string): u64?` | Parse decimal Snowflake string into `u64?` (`null` on invalid/overflow) |
| `isValidSnowflakeString` | `isValidSnowflakeString(s: string): bool` | Validate decimal Snowflake string syntax and `u64` range |

## Examples

### 1. Monotonic ID Generation & Decoding

```typescript
import { SnowflakeGenerator, formatSnowflake } from "snowflake";

fn generateOrderId(now_ms: u64, out: []u8): usize {
    let mut gen = SnowflakeGenerator.withTwitterEpoch(1, 7);
    let id = gen.nextId(now_ms) orelse return 0;

    let parts = gen.decompose(id);
    // parts.datacenter_id == 1, parts.worker_id == 7, parts.timestamp_ms == now_ms
    return formatSnowflake(id, out);
}
```

### 2. Time-Range Bounds & String Parsing

```typescript
import { DISCORD_EPOCH_MS, minIdForTimestamp, maxIdForTimestamp, parse } from "snowflake";

fn isInWindow(raw_id: string, start_ms: u64, end_ms: u64): bool {
    let id = parse(raw_id) orelse return false;
    let lo = minIdForTimestamp(DISCORD_EPOCH_MS, start_ms);
    let hi = maxIdForTimestamp(DISCORD_EPOCH_MS, end_ms);
    return id >= lo && id <= hi;
}
```

## Differences from Upstream & CP Highlights

- **Explicit Clock Injection (`now_ms: u64`)**: `nextId` and `nextBatch` take `now_ms` directly, making ID generation 100% deterministic, unit-testable, and syscall-free inside hot loops.
- **Null-Safe Rollback & Overflow Handling**: Clock rollback (`now_ms < last_ts_ms`) or sequence saturation (`> 4095` in 1ms) returns `null` (`u64?`) instead of blocking or panicking.
- **Zero-Heap Formatting**: `formatSnowflake` uses `itoa` to write up to 20 decimal bytes directly into `out: []u8`.
