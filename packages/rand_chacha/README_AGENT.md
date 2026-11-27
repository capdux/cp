# rand_chacha — AI Agent Quick Ref

Zero-allocation RFC 8439 ChaCha8 / ChaCha12 / ChaCha20 block function and CSPRNG for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ChaCha20Rng,
    crAdd32,
    crRotl32,
    crReadLe32,
    crWriteLe32,
    crQuarterRound,
    chachaBlock,
} from "rand_chacha";
```

## Core Types & Constants

```cp
class ChaCha20Rng { key: [8]u32, nonce: [3]u32, counter: u32, buf: [64]u8, /* +1 fields */ } // methods: fromSeed(...) +6
```

## API Signatures

```cp
fn crAdd32(a: u32, b: u32): u32;
fn crRotl32(x: u32, bits: u32): u32;
fn crReadLe32(data: &[]u8, offset: usize): u32;
fn crWriteLe32(out: []u8, offset: usize, v: u32);
fn crQuarterRound(st: []u32, a: usize, b: usize, c: usize, d: usize);
fn chachaBlock(key: [8]u32, counter: u32, nonce: [3]u32, rounds: u32, out: []u8);
```

## Minimal Usage

```cp
import { crAdd32 } from "rand_chacha";

pub fn main() {
    let result = crAdd32(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"rand_chacha"`; install via `cap add rand_chacha`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
