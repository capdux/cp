# snowflake — AI Agent Quick Ref

Zero-heap 64-bit Snowflake ID generator (Twitter/Discord 41-5-5-12 layout), decoder, time-range bounds, and decimal parser/formatter.

## Import
```typescript
import {
    SnowflakeGenerator, SnowflakeParts,
    TWITTER_EPOCH_MS, DISCORD_EPOCH_MS, SF_MAX_DATACENTER, SF_MAX_WORKER, SF_MAX_SEQUENCE,
    encodeSnowflake, compose, decodeSnowflake, decompose,
    timestamp, datacenterId, workerId, sequence,
    minIdForTimestamp, maxIdForTimestamp,
    formatSnowflake, parseSnowflake, parse, isValidSnowflakeString
} from "snowflake";
```

## API Signatures
```typescript
export const TWITTER_EPOCH_MS: u64 = 1288834974657;
export const DISCORD_EPOCH_MS: u64 = 1420070400000;
export const SF_MAX_DATACENTER: u16 = 31;
export const SF_MAX_WORKER: u16 = 31;
export const SF_MAX_SEQUENCE: u16 = 4095;
export class SnowflakeParts { timestamp_ms: u64, datacenter_id: u16, worker_id: u16, sequence: u16 }
export class SnowflakeGenerator {
    epoch_ms: u64, datacenter_id: u16, worker_id: u16, last_ts_ms: u64, sequence: u16,
    fn new(epoch_ms: u64, datacenter_id: u16, worker_id: u16): SnowflakeGenerator;
    fn withTwitterEpoch(datacenter_id: u16, worker_id: u16): SnowflakeGenerator;
    fn withDiscordEpoch(datacenter_id: u16, worker_id: u16): SnowflakeGenerator;
    fn nextId(*_, now_ms: u64): u64?;
    fn nextBatch(*_, now_ms: u64, out: []u64): usize;
    fn compose(timestamp_ms: u64, seq: u16): u64;
    fn decompose(id: u64): SnowflakeParts;
    fn timestamp(id: u64): u64;
    fn reset(*_);
}
export fn encodeSnowflake(epoch_ms: u64, timestamp_ms: u64, datacenter_id: u16, worker_id: u16, seq: u16): u64;
export fn compose(epoch_ms: u64, timestamp_ms: u64, datacenter_id: u16, worker_id: u16, seq: u16): u64;
export fn decodeSnowflake(id: u64, epoch_ms: u64): SnowflakeParts;
export fn decompose(id: u64, epoch_ms: u64): SnowflakeParts;
export fn timestamp(id: u64, epoch_ms: u64): u64;
export fn datacenterId(id: u64): u16;
export fn workerId(id: u64): u16;
export fn sequence(id: u64): u16;
export fn minIdForTimestamp(epoch_ms: u64, timestamp_ms: u64): u64;
export fn maxIdForTimestamp(epoch_ms: u64, timestamp_ms: u64): u64;
export fn formatSnowflake(id: u64, out: []u8): usize;
export fn parseSnowflake(s: string): u64?;
export fn parse(s: string): u64?;
export fn isValidSnowflakeString(s: string): bool;
```

## Minimal Example
```typescript
import { SnowflakeGenerator, formatSnowflake } from "snowflake";

fn makeId(now_ms: u64): u64 {
    let mut gen = SnowflakeGenerator.withDiscordEpoch(0, 1);
    let id = gen.nextId(now_ms) orelse return 0;
    let mut buf: [24]u8 = undefined;
    let _ = formatSnowflake(id, &mut buf);
    return id;
}
```

## Pitfalls & Rules
- `nextId` requires `let mut gen` and returns `u64?`: `null` if `now_ms < last_ts_ms` (clock rollback) or `sequence > 4095` in the same ms.
- `datacenter_id` and `worker_id` are masked to 5 bits (`0..=31`); `sequence` is 12 bits (`0..=4095`).
- `formatSnowflake` needs at least 20 bytes in `out: []u8` for max `u64`.
- `encodeSnowflake`/`compose`, `decodeSnowflake`/`decompose`, `parseSnowflake`/`parse` are aliases.
