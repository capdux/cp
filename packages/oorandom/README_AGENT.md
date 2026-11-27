# oorandom@1.0.0 (Agent Quickref)

> Zero-heap deterministic PCG32 (XSH RR 64/32) and dual-stream PCG64 PRNG in pure CP (modeled after Rust oorandom) | License: MIT
> Import: `import { ... } from "oorandom";` | Full types: `src/index.d.cp`

## Functions
- `export fn oorStep(st: u64, inc: u64): u64;`
- `export fn oorOutput(oldstate: u64): u32;`
- `export fn oorAdvanceState(st_in: u64, inc: u64, delta_in: u64): u64;`
