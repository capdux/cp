# tinystr@1.0.0 (Agent Quickref)

> Zero-allocation stack-inline fixed-capacity ASCII strings (TinyAsciiStr4, TinyAsciiStr8, TinyAsciiStr16) with u32/u64 packing and BCP-47 case transforms (ICU4X tinystr parity) in pure CP. | License: MIT
> Import: `import { ... } from "tinystr";` | Full types: `src/index.d.cp`

## Functions
- `export fn tsIsLower(b: u8): bool;`
- `export fn tsIsUpper(b: u8): bool;`
- `export fn tsValidateBytes(src: &[]u8, max_len: usize): i64;`
- `export fn tsValidatePacked(buf: &[]u8): i64;`
- `export fn tsSliceLen(buf: &[]u8): usize;`
- `export fn tsCopyOut(src: &[]u8, n: usize, out: []u8): usize;`
- `export fn tsSliceCmp(a: &[]u8, b: &[]u8): i32;`
- `export fn tsAllAlpha(buf: &[]u8, n: usize): bool;`
- `export fn tsAllAlnum(buf: &[]u8, n: usize): bool;`
- `export fn tsAllDigit(buf: &[]u8, n: usize): bool;`
- `export fn tsAllLower(buf: &[]u8, n: usize): bool;`
- `export fn tsAllUpper(buf: &[]u8, n: usize): bool;`
