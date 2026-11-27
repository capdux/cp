# snowflake@1.0.0 (Agent Quickref)

> Zero-heap 64-bit Snowflake distributed ID generator, decoder, and comparator (Twitter/Discord epoch compatible) | License: MIT
> Import: `import { ... } from "snowflake";` | Full types: `src/index.d.cp`
> Deps: `itoa`

## Functions
- `export fn encodeSnowflake(epoch_ms: u64, timestamp_ms: u64, datacenter_id: u16, worker_id: u16, seq: u16): u64;`
- `export fn compose(epoch_ms: u64, timestamp_ms: u64, datacenter_id: u16, worker_id: u16, seq: u16): u64;`
- `export fn decodeSnowflake(id: u64, epoch_ms: u64): SnowflakeParts;`
- `export fn decompose(id: u64, epoch_ms: u64): SnowflakeParts;`
- `export fn timestamp(id: u64, epoch_ms: u64): u64;`
- `export fn datacenterId(id: u64): u16;`
- `export fn workerId(id: u64): u16;`
- `export fn sequence(id: u64): u16;`
- `export fn minIdForTimestamp(epoch_ms: u64, timestamp_ms: u64): u64;`
- `export fn maxIdForTimestamp(epoch_ms: u64, timestamp_ms: u64): u64;`
- `export fn formatSnowflake(id: u64, out: []u8): usize;`
- `export fn parseSnowflake(s: string): u64?;`
- `export fn parse(s: string): u64?;`
- `export fn isValidSnowflakeString(s: string): bool;`
