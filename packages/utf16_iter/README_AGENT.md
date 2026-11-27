# utf16_iter@1.0.0 (Agent Quickref)

> Zero-allocation bidirectional UTF-16 code point iterator, Latin-1 to UTF-16 expander, and UTF-16 to UTF-8 transcoder in CP | License: Apache-2.0 OR MIT
> Import: `import { ... } from "utf16_iter";` | Full types: `src/index.d.cp`

## Functions
- `export fn u16iIsHighSurrogate(unit: u16): bool;`
- `export fn u16iIsLowSurrogate(unit: u16): bool;`
- `export fn u16iCombineSurrogates(high: u16, low: u16): u32;`
- `export fn u16iEncodeUtf8(cp: u32, out: []u8, offset: usize): usize;`
- `export fn countUtf16Chars(slice: &[]u16): usize;`
- `export fn utf16ToUtf8(slice: &[]u16, out: []u8): usize;`
