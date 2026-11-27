# universal-hash — AI Agent Quick Ref

Zero-allocation NIST SP 800-38D GF(2^128) universal hash (GHASH for AES-GCM) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Block128,
    Ghash,
    bitToMask64,
    readBeU64,
    writeBeU64,
    gf128Mul,
} from "universal-hash";
```

## Core Types & Constants

```cp
class Block128 { hi: u64, lo: u64 } // methods: zero(...) +5
class Ghash { h: Block128, y: Block128 } // methods: new(...) +5
```

## API Signatures

```cp
fn bitToMask64(bit: u64): u64;
fn readBeU64(buf: &[]u8, off: usize): u64;
fn writeBeU64(val: u64, out: []u8, off: usize);
fn gf128Mul(x: Block128, y: Block128): Block128;
```

## Minimal Usage

```cp
import { bitToMask64 } from "universal-hash";

pub fn main() {
    let result = bitToMask64(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"universal-hash"`; install via `cap add universal-hash`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
