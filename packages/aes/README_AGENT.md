# aes — AI Agent Quick Ref

Zero-allocation pure CP FIPS-197 AES-128, AES-192, and AES-256 block cipher (RustCrypto aes parity) (`v1.0.0`, `MIT OR Apache-2.0`)

## Import

```cp
import {
    Aes128,
    Aes192,
    Aes256,
    aesXtime,
    aesMul,
    aesSubBytes,
    aesInvSubBytes,
    aesShiftRows,
} from "aes";
```

## Core Types & Constants

```cp
class Aes128 { rk: [176]u8 } // methods: new(...) +2
class Aes192 { rk: [208]u8 } // methods: new(...) +2
class Aes256 { rk: [240]u8 } // methods: new(...) +2
```

## API Signatures

```cp
fn aesXtime(x: u8): u8;
fn aesMul(a_in: u8, b_in: u8): u8;
fn aesSubBytes(state: []u8);
fn aesInvSubBytes(state: []u8);
fn aesShiftRows(s: []u8);
fn aesInvShiftRows(s: []u8);
fn aesMixColumns(s: []u8);
fn aesInvMixColumns(s: []u8);
fn aesAddRoundKey(s: []u8, rk: &[]u8, round: usize);
fn aesExpandKey(key: &[]u8, nk: usize, nr: usize, rk: []u8);
fn aesEncryptCore(block: [16]u8, rk: &[]u8, nr: usize): [16]u8;
fn aesDecryptCore(block: [16]u8, rk: &[]u8, nr: usize): [16]u8;
```

## Minimal Usage

```cp
import { aesXtime } from "aes";

pub fn main() {
    let result = aesXtime(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"aes"`; install via `cap add aes`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
