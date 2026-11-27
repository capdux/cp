# hot-shots@1.0.0 (Agent Quickref)

> Zero-heap StatsD and DogStatsD UDP metric packet encoder/decoder supporting Counter, Timing, Gauge, Set, sample rates, and tags. | License: MIT
> Import: `import { ... } from "hot-shots";` | Full types: `src/index.d.cp`

## Functions
- `export fn isValidMetricName(name: &[]u8): bool;`
- `export fn isValidTags(tags: &[]u8): bool;`
- `export fn appendByte(dst: []u8, pos: usize, b: u8): i64;`
- `export fn appendSlice(dst: []u8, pos: usize, s: &[]u8): i64;`
- `export fn appendI64(dst: []u8, pos: usize, v: i64): i64;`
- `export fn formatMetric(name: &[]u8, value: i64, kind: u8, sample_rate_milli: u16, tags: &[]u8, dst: []u8): i64;`
- `export fn parseSampleRateMilli(s: &[]u8): i32;`
- `export fn parseMetric(packet: &[]u8): StatsdMetric;`
