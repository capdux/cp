# chacha20 — AI Agent Quick Ref

Zero-allocation RFC 8439 ChaCha20 stream cipher (Rust chacha20 crate parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ChaCha20Cipher,
    cc20Add32,
    cc20Rotl16,
    cc20Rotl12,
    cc20Rotl8,
    cc20Rotl7,
    cc20ReadLe32,
    cc20WriteLe32,
} from "chacha20";
```

## Core Types & Constants

```cp
class ChaCha20Cipher { key: [32]u8, nonce: [12]u8, counter: u32, block_buf: [64]u8, /* +1 fields */ } // methods: new(...) +3
```

## API Signatures

```cp
fn cc20Add32(a: u32, b: u32): u32;
fn cc20Rotl16(x: u32): u32;
fn cc20Rotl12(x: u32): u32;
fn cc20Rotl8(x: u32): u32;
fn cc20Rotl7(x: u32): u32;
fn cc20ReadLe32(b: []u8, off: usize): u32;
fn cc20WriteLe32(out: []u8, off: usize, v: u32);
fn cc20QuarterRound(st: []u32, ai: usize, bi: usize, ci: usize, di: usize);
fn chacha20Block(key: []u8, counter: u32, nonce: []u8, out64: []u8): bool;
```

## Minimal Usage

```cp
import { cc20Add32 } from "chacha20";

pub fn main() {
    let result = cc20Add32(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"chacha20"`; install via `cap add chacha20`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
