# seahash — AI Agent Quick Ref

Zero-heap pure-CP 4-lane parallel SeaHash 64-bit fast non-cryptographic hash function (crates.io seahash equivalent) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SeaHasher,
    seaU64,
    seaMul64,
    seahashDiffuse,
    seahashBytes,
    seahashSeeded,
} from "seahash";
```

## Core Types & Constants

```cp
class SeaHasher { a: u64, b: u64, c: u64, d: u64, /* +3 fields */ } // methods: new(...) +5
```

## API Signatures

```cp
fn seaU64(hi: u32, lo: u32): u64;
fn seaMul64(a: u64, b: u64): u64;
fn seahashDiffuse(x_in: u64): u64;
fn seahashBytes(buf: []u8): u64;
fn seahashSeeded(buf: []u8, k1: u64, k2: u64, k3: u64, k4: u64): u64;
```

## Minimal Usage

```cp
import { seaU64 } from "seahash";

pub fn main() {
    let result = seaU64(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"seahash"`; install via `cap add seahash`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
