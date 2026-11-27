# h3o@1.0.0 (Agent Quickref)

> Zero-heap Uber H3 64-bit hexagonal hierarchical spatial index cell parser, constructor, hierarchy traversal, and area table (clean-room port of Rust h3o) | License: MIT
> Import: `import { ... } from "h3o";` | Full types: `src/index.d.cp`

## Functions
- `export fn h3AllLow63(): u64;`
- `export fn h3DigitShift(r: u8): u6;`
- `export fn h3IsPentagonBaseCell(bc: u8): bool;`
- `export fn h3HexDigitVal(c: u8): u8;`
- `export fn h3HexChar(nibble: u8): u8;`
- `export fn h3ClearBits(val: u64, mask: u64): u64;`
- `export fn parseH3Hex(hex: string): H3Cell?;`
- `export fn avgHexAreaKm2(res: u8): f64;`
