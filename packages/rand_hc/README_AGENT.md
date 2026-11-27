# rand_hc@1.0.0 (Agent Quickref)

> Zero-allocation HC-128 eSTREAM cryptographic stream cipher PRNG (Hongjun Wu specification) in pure CP | License: MIT OR Apache-2.0
> Import: `import { ... } from "rand_hc";` | Full types: `src/index.d.cp`

## Functions
- `export fn rhcRotr32(x: u32, n: usize): u32;`
- `export fn rhcRotl32(x: u32, n: usize): u32;`
- `export fn rhcF1(x: u32): u32;`
- `export fn rhcF2(x: u32): u32;`
- `export fn rhcG1(x: u32, y: u32, z: u32): u32;`
- `export fn rhcG2(x: u32, y: u32, z: u32): u32;`
- `export fn rhcReadU32Le(seed: [32]u8, offset: usize): u32;`
