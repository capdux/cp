# ctr@1.0.0 (Agent Quickref)

> Zero-allocation NIST SP 800-38A AES-CTR stream cipher modes (Ctr128BE / Ctr128LE) with O(1) seek in pure CP | License: MIT OR Apache-2.0
> Import: `import { ... } from "ctr";` | Full types: `src/index.d.cp`

## Functions
- `export fn ctrZeroBlock(): [16]u8;`
- `export fn ctrAddBe128(iv: [16]u8, delta: u64): [16]u8;`
- `export fn ctrAddLe128(iv: [16]u8, delta: u64): [16]u8;`
