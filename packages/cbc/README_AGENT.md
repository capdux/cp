# cbc@1.0.0 (Agent Quickref)

> Zero-allocation pure CP AES-128-CBC and AES-256-CBC block cipher mode with PKCS#7 padding (RustCrypto cbc parity) | License: MIT
> Import: `import { ... } from "cbc";` | Full types: `src/index.d.cp`

## Functions
- `export fn cbcmReadBlock(src: &[]u8, offset: usize): [16]u8;`
- `export fn cbcmWriteBlock(dst: []u8, offset: usize, blk: [16]u8);`
- `export fn cbcmXorBlock(a: [16]u8, b: [16]u8): [16]u8;`
- `export fn cbcmValidatePkcs7(lastBlock: [16]u8): i64;`
- `export fn aes128CbcEncryptBlocks(key: [16]u8, iv: [16]u8, plaintext: &[]u8, out: []u8): usize;`
- `export fn aes128CbcDecryptBlocks(key: [16]u8, iv: [16]u8, ciphertext: &[]u8, out: []u8): usize;`
- `export fn aes128CbcEncryptPkcs7(key: [16]u8, iv: [16]u8, plaintext: &[]u8, out: []u8): usize;`
- `export fn aes128CbcDecryptPkcs7(key: [16]u8, iv: [16]u8, ciphertext: &[]u8, out: []u8): i64;`
- `export fn aes256CbcEncryptBlocks(key: [32]u8, iv: [16]u8, plaintext: &[]u8, out: []u8): usize;`
- `export fn aes256CbcDecryptBlocks(key: [32]u8, iv: [16]u8, ciphertext: &[]u8, out: []u8): usize;`
- `export fn aes256CbcEncryptPkcs7(key: [32]u8, iv: [16]u8, plaintext: &[]u8, out: []u8): usize;`
- `export fn aes256CbcDecryptPkcs7(key: [32]u8, iv: [16]u8, ciphertext: &[]u8, out: []u8): i64;`
