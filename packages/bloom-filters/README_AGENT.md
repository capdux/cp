# bloom-filters@1.0.0 (Agent Quickref)

> CP package bloom-filters | License: MIT
> Import: `import { ... } from "bloom-filters";` | Full types: `src/index.d.cp`

## Functions
- `export fn fnv1a64(data: &[]u8): u64;`
- `export fn djb2Mix64(data: &[]u8): u64;`
- `export fn popcount(bits: &[]u8): u64;`
- `export fn fillRatio(bits: &[]u8): f64;`
- `export fn optimalNumHashes(bit_count: usize, expected_items: usize): u32;`
