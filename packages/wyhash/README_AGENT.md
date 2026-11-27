# wyhash@1.0.0 (Agent Quickref)

> Zero-allocation Wang Yi wyhash v4 64-bit hash and WyRng PRNG with overflow-free 128-bit multiply in pure CP | License: MIT
> Import: `import { ... } from "wyhash";` | Full types: `src/index.d.cp`

## Functions
- `export fn makeU64(hi: u32, lo: u32): u64;`
- `export fn mul128(a: u64, b: u64): Mul128;`
- `export fn wymum(a: u64, b: u64): u64;`
- `export fn read4(data: &[]u8, off: usize): u64;`
- `export fn read8(data: &[]u8, off: usize): u64;`
- `export fn wyhashBytes(data: &[]u8, seed: u64): u64;`
- `export fn wyhash(data: string, seed: u64): u64;`
