# parse_duration@1.0.0 (Agent Quickref)

> Zero-heap human-readable duration parser and formatter supporting compound expressions, fractional units, Unicode microseconds, and overflow protection (clean-room port of Rust parse_duration / duration-str) | License: MIT
> Import: `import { ... } from "parse_duration";` | Full types: `src/index.d.cp`

## Functions
- `export fn pdurCheckedAddU64(a: u64, b: u64): u64?;`
- `export fn pdurCheckedMulU64(a: u64, b: u64): u64?;`
- `export fn pdurMatchUnit(u: string): u32;`
- `export fn pdurAppendPart(out: []u8, pos: usize, val: u64, suffix: string): usize;`
- `export fn parseDuration(input: string): ParsedDuration?;`
- `export fn formatDuration(d: ParsedDuration, out: []u8): usize;`
