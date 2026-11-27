# universal-hash@1.0.0 (Agent Quickref)

> Zero-allocation NIST SP 800-38D GF(2^128) universal hash (GHASH for AES-GCM) in pure CP | License: MIT
> Import: `import { ... } from "universal-hash";` | Full types: `src/index.d.cp`

## Functions
- `export fn bitToMask64(bit: u64): u64;`
- `export fn readBeU64(buf: &[]u8, off: usize): u64;`
- `export fn writeBeU64(val: u64, out: []u8, off: usize);`
- `export fn gf128Mul(x: Block128, y: Block128): Block128;`
