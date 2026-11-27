[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# snowflake

Zero-heap 64-bit Snowflake distributed ID generator, encoder, decoder, and parser for CP (Twitter & Discord epoch compatible).

## Provenance & Zero-Friction Migration

| Field | Value |
| :--- | :--- |
| **Source Registry** | crates.io |
| **Upstream Package** | `snowflake` |
| **CP Package** | `snowflake` (`v1.0.0`) |
| **License** | MIT |

Keep the exact package name in CP — no `@cap/*` prefix or renaming required:

```rust
// Rust (crates.io: snowflake)
use snowflake::SnowflakeIdGenerator;
let mut gen = SnowflakeIdGenerator::new(1, 2);
let id = gen.real_time_generate();
```

```typescript
// CP (same package name: "snowflake")
import { SnowflakeGenerator, TWITTER_EPOCH_MS } from "snowflake";
let mut gen = SnowflakeGenerator.withTwitterEpoch(1, 2);
let id = gen.nextId(1700000000000);
```

## Installation

```bash
cap add snowflake
```

## Bit Layout (64-bit Unsigned)

- **41 bits**: milliseconds since custom epoch (`SF_TIMESTAMP_SHIFT = 22`, max `SF_MAX_TIMESTAMP`)
- **5 bits**: datacenter ID (`0..31`, `SF_MAX_DATACENTER = 31`, `SF_DATACENTER_SHIFT = 17`)
- **5 bits**: worker ID (`0..31`, `SF_MAX_WORKER = 31`, `SF_WORKER_SHIFT = 12`)
- **12 bits**: per-millisecond sequence (`0..4095`, `SF_MAX_SEQUENCE = 4095`)

Built-in epoch constants:
- `TWITTER_EPOCH_MS = 1288834974657` (2010-11-04T01:42:54.657Z)
- `DISCORD_EPOCH_MS = 1420070400000` (2015-01-01T00:00:00.000Z)

## Quick Start & Practical Examples

### 1. Stateful Generator (`SnowflakeGenerator`)

```typescript
import { SnowflakeGenerator, TWITTER_EPOCH_MS, DISCORD_EPOCH_MS } from "snowflake";

fn generateIds(): void {
    // Use Twitter epoch, Discord epoch, or any custom epoch
    let mut twitter_gen = SnowflakeGenerator.withTwitterEpoch(1, 5);
    let mut discord_gen = SnowflakeGenerator.withDiscordEpoch(0, 3);
    let mut custom_gen = SnowflakeGenerator.new(1704067200000, 2, 7);

    let now_ms: u64 = 1705000000000;
    if (let id = twitter_gen.nextId(now_ms)) {
        let ts = twitter_gen.timestamp(id);
        let parts = twitter_gen.decompose(id);
        // parts.timestamp_ms, parts.datacenter_id, parts.worker_id, parts.sequence
    }

    // Batch-generate up to N IDs into a stack buffer (zero heap allocation)
    let mut batch: [16]u64 = undefined;
    let produced = discord_gen.nextBatch(now_ms, batch[0..16]);

    // Compose directly with generator's configured epoch/datacenter/worker
    let manual_id = custom_gen.compose(now_ms, 42);
    custom_gen.reset();
}
```

### 2. Stateless Encode, Decode & Field Extraction

```typescript
import {
    TWITTER_EPOCH_MS,
    encodeSnowflake,
    compose,
    decodeSnowflake,
    decompose,
    timestamp,
    datacenterId,
    workerId,
    sequence,
    minIdForTimestamp,
    maxIdForTimestamp,
} from "snowflake";

fn inspectSnowflake(): void {
    let ts: u64 = 1700000000000;
    let id = encodeSnowflake(TWITTER_EPOCH_MS, ts, 3, 12, 99);
    // `compose` is an alias for `encodeSnowflake`
    let same = compose(TWITTER_EPOCH_MS, ts, 3, 12, 99);

    let parts = decodeSnowflake(id, TWITTER_EPOCH_MS); // alias: decompose(id, TWITTER_EPOCH_MS)
    let extracted_ts = timestamp(id, TWITTER_EPOCH_MS);
    let dc = datacenterId(id); // 3
    let wk = workerId(id);     // 12
    let seq = sequence(id);    // 99

    // Range-query bounds for database index scans (e.g. WHERE id BETWEEN lo AND hi)
    let lo = minIdForTimestamp(TWITTER_EPOCH_MS, ts);
    let hi = maxIdForTimestamp(TWITTER_EPOCH_MS, ts);
}
```

### 3. Formatting & Parsing Decimal Strings

```typescript
import { formatSnowflake, parseSnowflake, parse, isValidSnowflakeString } from "snowflake";

fn textCodec(id: u64): void {
    let mut buf: [24]u8 = undefined;
    let n = formatSnowflake(id, buf[0..24]); // writes ASCII digits, returns byte length

    if (isValidSnowflakeString("175928847299117063")) {
        if (let parsed = parseSnowflake("175928847299117063")) {
            // `parse` is an alias for `parseSnowflake`
            let same = parse("175928847299117063");
        }
    }
}
```

## API Reference

### Constants
- `TWITTER_EPOCH_MS: u64` (`1288834974657`)
- `DISCORD_EPOCH_MS: u64` (`1420070400000`)
- `SF_MAX_DATACENTER: u16` (`31`), `SF_MAX_WORKER: u16` (`31`), `SF_MAX_SEQUENCE: u16` (`4095`)
- `SF_TIMESTAMP_SHIFT: u6` (`22`), `SF_DATACENTER_SHIFT: u6` (`17`), `SF_WORKER_SHIFT: u6` (`12`)
- `SF_MAX_TIMESTAMP: u64` (`(1 << 41) - 1`)

### Types
- `SnowflakeParts { timestamp_ms: u64, datacenter_id: u16, worker_id: u16, sequence: u16 }`
- `SnowflakeGenerator`:
  - `SnowflakeGenerator.new(epoch_ms: u64, datacenter_id: u16, worker_id: u16): SnowflakeGenerator`
  - `SnowflakeGenerator.withTwitterEpoch(datacenter_id: u16, worker_id: u16): SnowflakeGenerator`
  - `SnowflakeGenerator.withDiscordEpoch(datacenter_id: u16, worker_id: u16): SnowflakeGenerator`
  - `nextId(now_ms: u64): u64?`
  - `nextBatch(now_ms: u64, out: []u64): usize`
  - `compose(timestamp_ms: u64, seq: u16): u64`
  - `decompose(id: u64): SnowflakeParts`
  - `timestamp(id: u64): u64`
  - `reset()`

### Functions
- `encodeSnowflake(epoch_ms: u64, timestamp_ms: u64, datacenter_id: u16, worker_id: u16, seq: u16): u64` / `compose(...)`
- `decodeSnowflake(id: u64, epoch_ms: u64): SnowflakeParts` / `decompose(...)`
- `timestamp(id: u64, epoch_ms: u64): u64`, `datacenterId(id: u64): u16`, `workerId(id: u64): u16`, `sequence(id: u64): u16`
- `minIdForTimestamp(epoch_ms: u64, timestamp_ms: u64): u64`, `maxIdForTimestamp(epoch_ms: u64, timestamp_ms: u64): u64`
- `formatSnowflake(id: u64, out: []u8): usize`
- `parseSnowflake(s: string): u64?` / `parse(s: string): u64?`
- `isValidSnowflakeString(s: string): bool`
