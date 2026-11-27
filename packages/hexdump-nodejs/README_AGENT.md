# hexdump-nodejs@1.0.0 (Agent Quickref)

> Zero-allocation canonical hexdump -C 16-byte-per-line binary debugger, spaced hex encoder, and first-diff locator in pure CP | License: MIT
> Import: `import { ... } from "hexdump-nodejs";` | Full types: `src/index.d.cp`

## Functions
- `export fn hexDigit(nibble: u8, uppercase: bool): u8;`
- `export fn formatLine(data: &[]u8, base_offset: usize, out: []u8): usize;`
- `export fn formatDump(data: &[]u8, out: []u8): usize;`
- `export fn encodeHexSpaced(data: &[]u8, uppercase: bool, out: []u8): usize;`
- `export fn firstDiffOffset(a: &[]u8, b: &[]u8): i64;`
