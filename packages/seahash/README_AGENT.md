# seahash@1.0.0 (Agent Quickref)

> Zero-heap pure-CP 4-lane parallel SeaHash 64-bit fast non-cryptographic hash function (crates.io seahash equivalent) | License: MIT
> Import: `import { ... } from "seahash";` | Full types: `src/index.d.cp`

## Functions
- `export fn seaU64(hi: u32, lo: u32): u64;`
- `export fn seaMul64(a: u64, b: u64): u64;`
- `export fn seaReadU64Le(buf: []u8, off: usize): u64;`
- `export fn seahashDiffuse(x_in: u64): u64;`
- `export fn seahashBytes(buf: []u8): u64;`
- `export fn seahashSeeded(buf: []u8, k1: u64, k2: u64, k3: u64, k4: u64): u64;`
