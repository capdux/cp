# ksuid@1.0.0 (Agent Quickref)

> Zero-heap Segment KSUID (20-byte K-Sortable Unique IDentifier) 27-character Base62 encoder, decoder, validator, and comparator. | License: MIT
> Import: `import { ... } from "ksuid";` | Full types: `src/index.d.cp`

## Functions
- `export fn b62Char(v: u8): u8;`
- `export fn b62Val(c: u8): i32;`
- `export fn fromParts(unix_secs: u64, payload: [16]u8): Ksuid;`
- `export fn timestampSecs(k: Ksuid): u64;`
- `export fn encode(k: Ksuid, out: []u8): usize;`
- `export fn decode(s: string, out: &mut Ksuid): bool;`
- `export fn isValidKsuid(s: string): bool;`
- `export fn compare(a: Ksuid, b: Ksuid): i32;`
