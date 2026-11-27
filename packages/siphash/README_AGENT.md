# siphash@1.0.0 (Agent Quickref)

> Zero-allocation SipHash-2-4 and SipHash-1-3 keyed 64-bit HashDoS-resistant PRF for CP. | License: MIT
> Import: `import { ... } from "siphash";` | Full types: `src/index.d.cp`

## Functions
- `export fn siphash24(k0: u64, k1: u64, data: &[]u8): u64;`
- `export fn siphash13(k0: u64, k1: u64, data: &[]u8): u64;`
- `export fn siphash24Key16(key: [16]u8, data: &[]u8): u64;`
- `export fn siphash13Key16(key: [16]u8, data: &[]u8): u64;`
