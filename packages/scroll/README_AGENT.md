# scroll@1.0.0 (Agent Quickref)

> Zero-allocation endian-aware offset-advancing binary reader/writer cursors (PreadCursor & PwriteCursor) in pure CP | License: MIT
> Import: `import { ... } from "scroll";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum Endian`

## Functions
- `export fn scrlReadU16(buf: &[]u8, off: usize, endian: Endian): u16;`
- `export fn scrlReadU32(buf: &[]u8, off: usize, endian: Endian): u32;`
- `export fn scrlReadU64(buf: &[]u8, off: usize, endian: Endian): u64;`
- `export fn scrlWriteU16(buf: []u8, off: usize, v: u16, endian: Endian);`
- `export fn scrlWriteU32(buf: []u8, off: usize, v: u32, endian: Endian);`
- `export fn scrlWriteU64(buf: []u8, off: usize, v: u64, endian: Endian);`
