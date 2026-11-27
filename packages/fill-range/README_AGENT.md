# fill-range@1.0.0 (Agent Quickref)

> Zero-heap numeric, alphabetic, stepped, and zero-padded range expansion engine (npm fill-range parity). | License: MIT
> Import: `import { ... } from "fill-range";` | Full types: `src/index.d.cp`

## Functions
- `export fn isAsciiAlpha(c: u8): bool;`
- `export fn absStepU64(step: i64): u64;`
- `export fn absDiffI64(a: i64, b: i64): u64;`
- `export fn countRange(start: i64, end: i64, step: i64): usize;`
- `export fn expandNumbers(start: i64, end: i64, step: i64, out: []i64): i64;`
- `export fn expandChars(start: u8, end: u8, step: usize, out: []u8): i64;`
- `export fn parseRangeInt(s: &[]u8): ParsedInt;`
- `export fn writeSlice(dst: []u8, pos: usize, src: &[]u8): i64;`
- `export fn writePaddedInt(dst: []u8, start_pos: usize, val: i64, pad_width: usize): i64;`
- `export fn formatRange(start: &[]u8, end: &[]u8, step: i64, sep: &[]u8, dst: []u8): i64;`
