# block-padding — AI Agent Quick Ref

Zero-allocation PKCS#7 (constant-time padding-oracle-resistant unpad), ISO 7816-4, ANSI X9.23, ZeroPadding, 128-bit BE/LE CTR counter increment, and XOR block primitives reusing subtle-ct in pure CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    bpCopyBytes,
    pkcs7Pad,
    pkcs7UnpadCt,
    iso7816Pad,
    iso7816Unpad,
    ansiX923Pad,
    ansiX923Unpad,
    zeroPad,
} from "block-padding";
```

## API Signatures

```cp
fn bpCopyBytes(dst: []u8, src: &[]u8);
fn pkcs7Pad(input: &[]u8, block_size: usize, out: []u8): i64;
fn pkcs7UnpadCt(padded: &[]u8, block_size: usize): i64;
fn iso7816Pad(input: &[]u8, block_size: usize, out: []u8): i64;
fn iso7816Unpad(padded: &[]u8, block_size: usize): i64;
fn ansiX923Pad(input: &[]u8, block_size: usize, out: []u8): i64;
fn ansiX923Unpad(padded: &[]u8, block_size: usize): i64;
fn zeroPad(input: &[]u8, block_size: usize, out: []u8): i64;
fn zeroUnpad(padded: &[]u8, block_size: usize): i64;
fn ctrIncBe128(counter: []u8);
fn ctrIncLe128(counter: []u8);
fn xorBlock(dst: []u8, a: &[]u8, b: &[]u8): bool;
fn wipeBlock(buf: []u8);
```

## Minimal Usage

```cp
import { iso7816Pad } from "block-padding";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = iso7816Pad(out_buf[0..256], 0, buf_1[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"block-padding"`; install via `cap add block-padding`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
