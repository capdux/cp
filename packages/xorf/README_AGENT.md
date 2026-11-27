# xorf@1.0.0 (Agent Quickref)

> Zero-allocation Xor8 static fingerprint filter (Graf & Lemire ACM JEA 2020 / Rust xorf parity) with O(1) 3-lookup XOR queries and ~0.39% false-positive rate for CP | License: MIT
> Import: `import { ... } from "xorf";` | Full types: `src/index.d.cp`

## Functions
- `export fn makeU64(hi: u32, lo: u32): u64;`
- `export fn hashKey64(key: u64, seed: u64): u64;`
- `export fn deriveSlots(h: u64, block_len: usize): Slots3;`
