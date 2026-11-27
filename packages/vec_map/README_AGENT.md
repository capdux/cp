# vec_map@1.0.0 (Agent Quickref)

> Zero-allocation O(1) direct-indexed sparse integer-key map (0..127) with 128-bit presence bitmap in CP | License: MIT OR Apache-2.0
> Import: `import { ... } from "vec_map";` | Full types: `src/index.d.cp`

## Functions
- `export fn vmapMask(bit: usize): u64;`
- `export fn vmapHasBit(bits_lo: u64, bits_hi: u64, k: usize): bool;`
