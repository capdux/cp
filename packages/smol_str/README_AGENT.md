# smol_str@1.0.0 (Agent Quickref)

> Zero-heap pure-CP O(1)-clone small string optimization (24-byte inline SSO + read-only slice fallback, crates.io smol_str equivalent) | License: MIT
> Import: `import { ... } from "smol_str";` | Full types: `src/index.d.cp`

## Functions
- `export fn smolGetByte(s: SmolStr, idx: usize): u8;`
- `export fn smolMinUsize(a: usize, b: usize): usize;`
- `export fn smolStartsWith(s: SmolStr, prefix: string): bool;`
- `export fn smolEndsWith(s: SmolStr, suffix: string): bool;`
