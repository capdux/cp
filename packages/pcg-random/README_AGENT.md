# pcg-random@1.0.0 (Agent Quickref)

> Zero-heap Melissa O'Neill PCG-XSH-RR 32-bit deterministic PRNG with stream separation, unbiased bounded sampling, O(log n) advance, and byte filling. | License: MIT
> Import: `import { ... } from "pcg-random";` | Full types: `src/index.d.cp`

## Functions
- `export fn pcgMult(): u64;`
- `export fn defaultStream(): u64;`
- `export fn add64(a: u64, b: u64): u64;`
- `export fn mul64(a: u64, b: u64): u64;`
- `export fn rotr32(x: u32, rot: u32): u32;`
