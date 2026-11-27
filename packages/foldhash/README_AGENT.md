# foldhash@1.0.0 (Agent Quickref)

> Zero-allocation 64x64->128-bit folded-multiply non-cryptographic hasher with Fast and Quality modes (Rust foldhash parity) for CP | License: MIT
> Import: `import { ... } from "foldhash";` | Full types: `src/index.d.cp`

## Functions
- `export fn foldedMultiply(x: u64, y: u64): u64;`
- `export fn hashBytesFast(data: &[]u8, seed: u64): u64;`
- `export fn hashBytesQuality(data: &[]u8, seed: u64): u64;`
- `export fn hashU64(v: u64, seed: u64): u64;`
