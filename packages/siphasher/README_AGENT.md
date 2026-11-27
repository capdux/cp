# siphasher@1.0.0 (Agent Quickref)

> Zero-allocation SipHash-2-4 and SipHash-1-3 keyed streaming PRF/hashers (HashDoS-resistant) for CP | License: MIT
> Import: `import { ... } from "siphasher";` | Full types: `src/index.d.cp`

## Functions
- `export fn addWrap64(a: u64, b: u64): u64;`
- `export fn rotl64(x: u64, r: u64): u64;`
- `export fn sipRound(s: SipState): SipState;`
- `export fn loadLe64(buf: []u8, offset: usize): u64;`
- `export fn loadLe64Str(s: string, offset: usize): u64;`
- `export fn siphash24(k0: u64, k1: u64, data: string): u64;`
- `export fn siphash13(k0: u64, k1: u64, data: string): u64;`
