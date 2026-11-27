# cbc — AI Agent Quick Ref

Zero-allocation pure CP AES-128-CBC and AES-256-CBC block cipher mode with PKCS#7 padding (RustCrypto cbc parity) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    cbcmReadBlock,
    cbcmWriteBlock,
    cbcmXorBlock,
    cbcmValidatePkcs7,
    aes128CbcEncryptBlocks,
    aes128CbcDecryptBlocks,
    aes128CbcEncryptPkcs7,
    aes128CbcDecryptPkcs7,
} from "cbc";
```

## API Signatures

```cp
fn cbcmReadBlock(src: &[]u8, offset: usize): [16]u8;
fn cbcmWriteBlock(dst: []u8, offset: usize, blk: [16]u8);
fn cbcmXorBlock(a: [16]u8, b: [16]u8): [16]u8;
fn cbcmValidatePkcs7(lastBlock: [16]u8): i64;
fn aes128CbcEncryptBlocks(key: [16]u8, iv: [16]u8, plaintext: &[]u8, out: []u8): usize;
fn aes128CbcDecryptBlocks(key: [16]u8, iv: [16]u8, ciphertext: &[]u8, out: []u8): usize;
fn aes128CbcEncryptPkcs7(key: [16]u8, iv: [16]u8, plaintext: &[]u8, out: []u8): usize;
fn aes128CbcDecryptPkcs7(key: [16]u8, iv: [16]u8, ciphertext: &[]u8, out: []u8): i64;
fn aes256CbcEncryptBlocks(key: [32]u8, iv: [16]u8, plaintext: &[]u8, out: []u8): usize;
fn aes256CbcDecryptBlocks(key: [32]u8, iv: [16]u8, ciphertext: &[]u8, out: []u8): usize;
fn aes256CbcEncryptPkcs7(key: [32]u8, iv: [16]u8, plaintext: &[]u8, out: []u8): usize;
fn aes256CbcDecryptPkcs7(key: [32]u8, iv: [16]u8, ciphertext: &[]u8, out: []u8): i64;
```

## Minimal Usage

```cp
import { cbcmReadBlock } from "cbc";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = cbcmReadBlock(out_buf[0..256], 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"cbc"`; install via `cap add cbc`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
