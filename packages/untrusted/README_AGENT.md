# untrusted@1.0.1 (Agent Quickref)

> Zero-heap big-endian/little-endian binary codecs & bounds-checked untrusted ByteReader/ByteWriter (inspired by Rust byteorder + untrusted) | License: MIT
> Import: `import { ... } from "untrusted";` | Full types: `src/index.d.cp`

## Functions
- `export fn readU16Be(buf: string, off: usize): u16?;`
- `export fn readU16Le(buf: string, off: usize): u16?;`
- `export fn readU32Be(buf: string, off: usize): u32?;`
- `export fn readU32Le(buf: string, off: usize): u32?;`
- `export fn readU64Be(buf: string, off: usize): u64?;`
- `export fn readU64Le(buf: string, off: usize): u64?;`
- `export fn writeU16Be(out: []u8, off: usize, v: u16): bool;`
- `export fn writeU16Le(out: []u8, off: usize, v: u16): bool;`
- `export fn writeU32Be(out: []u8, off: usize, v: u32): bool;`
- `export fn writeU32Le(out: []u8, off: usize, v: u32): bool;`
- `export fn writeU64Be(out: []u8, off: usize, v: u64): bool;`
- `export fn writeU64Le(out: []u8, off: usize, v: u64): bool;`
