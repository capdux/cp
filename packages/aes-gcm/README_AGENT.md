# aes-gcm@1.0.0 (Agent Quickref)

> Zero-allocation NIST SP 800-38D AES-128-GCM and AES-256-GCM AEAD authenticated encryption composing aes and universal-hash (RustCrypto aes-gcm parity) for CP | License: Apache-2.0 OR MIT
> Import: `import { ... } from "aes-gcm";` | Full types: `src/index.d.cp`

## Functions
- `export fn agcmZeroBlock(): [16]u8;`
- `export fn agcmMakeCounterBlock(nonce: [12]u8, counter: u32): [16]u8;`
- `export fn agcmCtEq16(a: [16]u8, b: [16]u8): bool;`
- `export fn agcmComputeTag(h: [16]u8, e_j0: [16]u8, aad: &[]u8, ct: &[]u8): [16]u8;`
