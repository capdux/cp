# ripemd@1.0.0 (Agent Quickref)

> Zero-heap pure-CP RIPEMD-160 160-bit cryptographic hash function (crates.io ripemd equivalent) | License: MIT
> Import: `import { ... } from "ripemd";` | Full types: `src/index.d.cp`

## Functions
- `export fn rmdAdd(a: u32, b: u32): u32;`
- `export fn rmdAdd3(a: u32, b: u32, c: u32): u32;`
- `export fn rmdAdd4(a: u32, b: u32, c: u32, d: u32): u32;`
- `export fn rmdRol(x: u32, n: u32): u32;`
- `export fn rmdNot(x: u32): u32;`
- `export fn rmdFunc(round: usize, x: u32, y: u32, z: u32): u32;`
- `export fn rmdKl(round: usize): u32;`
- `export fn rmdKr(round: usize): u32;`
- `export fn rmdReadU32Le(buf: []u8, off: usize): u32;`
- `export fn rmdWriteU32Le(v: u32, out: []u8, off: usize);`
- `export fn rmdHexNibble(v: u8): u8;`
- `export fn rmdCompress(state: [5]u32, block: []u8): [5]u32;`
- `export fn ripemd160Bytes(data: []u8, out20: []u8);`
- `export fn ripemd160Hex(s: string, out40: []u8): usize;`
