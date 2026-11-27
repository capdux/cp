# snowflake — AI Agent Quick Ref

- **Import**: `import { SnowflakeGenerator, SnowflakeParts, TWITTER_EPOCH_MS, DISCORD_EPOCH_MS, encodeSnowflake, compose, decodeSnowflake, decompose, timestamp, datacenterId, workerId, sequence, minIdForTimestamp, maxIdForTimestamp, formatSnowflake, parseSnowflake, parse, isValidSnowflakeString } from "snowflake";`
- **Source**: crates.io `snowflake` (`v1.0.0`, zero-heap)

## Constants
```cp
TWITTER_EPOCH_MS: u64 = 1288834974657
DISCORD_EPOCH_MS: u64 = 1420070400000
SF_MAX_DATACENTER: u16 = 31   // 5 bits
SF_MAX_WORKER: u16 = 31       // 5 bits
SF_MAX_SEQUENCE: u16 = 4095   // 12 bits
SF_TIMESTAMP_SHIFT: u6 = 22
SF_DATACENTER_SHIFT: u6 = 17
SF_WORKER_SHIFT: u6 = 12
SF_MAX_TIMESTAMP: u64 = (1 << 41) - 1
```

## Core Signatures
```cp
class SnowflakeParts { timestamp_ms: u64, datacenter_id: u16, worker_id: u16, sequence: u16 }

class SnowflakeGenerator {
    static new(epoch_ms: u64, datacenter_id: u16, worker_id: u16): SnowflakeGenerator;
    static withTwitterEpoch(datacenter_id: u16, worker_id: u16): SnowflakeGenerator;
    static withDiscordEpoch(datacenter_id: u16, worker_id: u16): SnowflakeGenerator;
    nextId(now_ms: u64): u64?;                       // null on clock rollback or seq overflow (>4095)
    nextBatch(now_ms: u64, out: []u64): usize;       // returns count written into out
    compose(timestamp_ms: u64, seq: u16): u64;
    decompose(id: u64): SnowflakeParts;
    timestamp(id: u64): u64;
    reset();
}

encodeSnowflake(epoch_ms: u64, timestamp_ms: u64, datacenter_id: u16, worker_id: u16, seq: u16): u64
compose(epoch_ms: u64, timestamp_ms: u64, datacenter_id: u16, worker_id: u16, seq: u16): u64
decodeSnowflake(id: u64, epoch_ms: u64): SnowflakeParts
decompose(id: u64, epoch_ms: u64): SnowflakeParts
timestamp(id: u64, epoch_ms: u64): u64
datacenterId(id: u64): u16
workerId(id: u64): u16
sequence(id: u64): u16
minIdForTimestamp(epoch_ms: u64, timestamp_ms: u64): u64
maxIdForTimestamp(epoch_ms: u64, timestamp_ms: u64): u64
formatSnowflake(id: u64, out: []u8): usize           // pass >= 20 bytes buffer
parseSnowflake(s: string): u64?                      // alias: parse(s)
isValidSnowflakeString(s: string): bool
```

## Pitfalls
- `SnowflakeGenerator` mutates internal state (`last_ts_ms`, `sequence`): bind with `let mut gen = ...`.
- `nextId(now_ms)` returns `null` if `now_ms < epoch_ms`, clock moves backward (`now_ms < last_ts_ms`), or same-ms sequence exceeds `4095`.
- `formatSnowflake` writes into caller-provided `[]u8` (use `[20]u8` or `[24]u8`); returns `0` if buffer is too small.
- Always pass the same `epoch_ms` to `decodeSnowflake`/`timestamp` that was used during encoding.
