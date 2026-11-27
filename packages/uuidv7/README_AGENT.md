# uuidv7@1.0.0 (Agent Quickref)

> Zero-allocation RFC 9562 UUIDv7 constructor, 36-byte formatter, parser, timestamp extractor, and comparator in pure CP | License: MIT
> Import: `import { ... } from "uuidv7";` | Full types: `src/index.d.cp`

## Functions
- `export fn fromParts(timestamp_ms: u64, rand_bytes: [10]u8): UuidV7;`
- `export fn timestampMs(u: UuidV7): u64;`
- `export fn version(u: UuidV7): u8;`
- `export fn isRfcVariant(u: UuidV7): bool;`
- `export fn hexLower(nibble: u8): u8;`
- `export fn hexVal(b: u8): i32;`
- `export fn formatHyphenated(u: UuidV7, out: []u8): usize;`
- `export fn parse(s: string, out: &mut UuidV7): bool;`
- `export fn isValidUuidV7(s: string): bool;`
- `export fn compare(a: UuidV7, b: UuidV7): i32;`
