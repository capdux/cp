# ulid@1.0.0 (Agent Quickref)

> Zero-allocation ULID (48-bit Unix ms timestamp + 80-bit entropy, Crockford Base32) encoder, decoder, validator, and monotonic incrementer for CP. | License: MIT
> Import: `import { ... } from "ulid";` | Full types: `src/index.d.cp`

## Functions
- `export fn fromParts(timestamp_ms: u64, rand_bytes: [10]u8): Ulid;`
- `export fn timestampMs(u: Ulid): u64;`
- `export fn encode(u: Ulid, out: []u8): usize;`
- `export fn decode(s: string, out: &mut Ulid): bool;`
- `export fn isValidUlid(s: string): bool;`
- `export fn increment(u: Ulid, out: &mut Ulid): bool;`
- `export fn compare(a: Ulid, b: Ulid): i32;`
