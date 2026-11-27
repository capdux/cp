# aes@1.0.0 (Agent Quickref)

> Zero-allocation pure CP FIPS-197 AES-128, AES-192, and AES-256 block cipher (RustCrypto aes parity) | License: MIT OR Apache-2.0
> Import: `import { ... } from "aes";` | Full types: `src/index.d.cp`

## Functions
- `export fn aesXtime(x: u8): u8;`
- `export fn aesMul(a_in: u8, b_in: u8): u8;`
- `export fn aesSubBytes(state: []u8);`
- `export fn aesInvSubBytes(state: []u8);`
- `export fn aesShiftRows(s: []u8);`
- `export fn aesInvShiftRows(s: []u8);`
- `export fn aesMixColumns(s: []u8);`
- `export fn aesInvMixColumns(s: []u8);`
- `export fn aesAddRoundKey(s: []u8, rk: &[]u8, round: usize);`
- `export fn aesExpandKey(key: &[]u8, nk: usize, nr: usize, rk: []u8);`
- `export fn aesEncryptCore(block: [16]u8, rk: &[]u8, nr: usize): [16]u8;`
- `export fn aesDecryptCore(block: [16]u8, rk: &[]u8, nr: usize): [16]u8;`
