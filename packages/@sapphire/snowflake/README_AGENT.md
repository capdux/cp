# @sapphire/snowflake@1.0.0 (Agent Quickref)

> Zero-heap 64-bit Twitter/Discord Snowflake distributed ID generator, decomposer, clock-rollback guard, and decimal formatter. | License: MIT
> Import: `import { ... } from "@sapphire/snowflake";` | Full types: `src/index.d.cp`

## Functions
- `export fn compose(epoch_ms: u64, timestamp_ms: u64, datacenter_id: u8, worker_id: u8, sequence: u16): u64;`
- `export fn decompose(id: u64, epoch_ms: u64): SnowflakeParts;`
- `export fn initGenerator(epoch_ms: u64, datacenter_id: u8, worker_id: u8): SnowflakeGenerator;`
- `export fn nextId(gen: &mut SnowflakeGenerator, now_ms: u64): i64;`
- `export fn formatId(id: u64, out: []u8): usize;`
- `export fn parseId(s: string): i64;`
