# tiny-keccak — AI Agent Quick Ref

Zero-allocation Keccak-f[1600] sponge with SHA3-256 (FIPS 202) and Keccak-256 (Ethereum) (Rust tiny-keccak parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Sha3_256,
    Keccak256,
    kckU64,
    kckRotl64,
    kckReadLe64,
    kckWriteLe64,
    kckHexDigit,
    kckRoundConst,
} from "tiny-keccak";
```

## Core Types & Constants

```cp
class Sha3_256 { st: [25]u64, buf: [136]u8, buf_len: usize } // methods: new(...) +4
class Keccak256 { st: [25]u64, buf: [136]u8, buf_len: usize } // methods: new(...) +4
```

## API Signatures

```cp
fn kckU64(hi: u32, lo: u32): u64;
fn kckRotl64(x: u64, n: u32): u64;
fn kckReadLe64(b: []u8, off: usize): u64;
fn kckWriteLe64(out: []u8, off: usize, v: u64);
fn kckHexDigit(nibble: u8): u8;
fn kckRoundConst(round: usize): u64;
fn kckPermute(st: []u64);
fn kckAbsorbBlock(st: []u64, block: []u8);
fn kckFinalizeSponge(st: []u64, buf: []u8, buf_len: usize, domain_sep: u8, out32: []u8);
fn kckEncodeHex32(digest: []u8, out64: []u8): usize;
fn sha3_256Hex(s: string, out64: []u8): usize;
fn keccak256Hex(s: string, out64: []u8): usize;
```

## Minimal Usage

```cp
import { sha3_256Hex } from "tiny-keccak";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = sha3_256Hex("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"tiny-keccak"`; install via `cap add tiny-keccak`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
