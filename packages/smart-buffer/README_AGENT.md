# smart-buffer@4.2.0 (Agent Quickref)

> Zero-heap cursor-based binary reader/writer buffer with big/little-endian integer and string operations | License: MIT
> Import: `import { ... } from "smart-buffer";` | Full types: `src/index.d.cp`

## Functions
- `export fn sbufMin(a: usize, b: usize): usize;`
- `export fn sbufW16BE(dst: []u8, off: usize, v: u16): bool;`
- `export fn sbufW16LE(dst: []u8, off: usize, v: u16): bool;`
- `export fn sbufW32BE(dst: []u8, off: usize, v: u32): bool;`
- `export fn sbufW32LE(dst: []u8, off: usize, v: u32): bool;`
- `export fn sbufR16BE(src: &[]u8, off: usize): u16?;`
- `export fn sbufR16LE(src: &[]u8, off: usize): u16?;`
- `export fn sbufR32BE(src: &[]u8, off: usize): u32?;`
- `export fn sbufR32LE(src: &[]u8, off: usize): u32?;`
