# rand_hc — AI Agent Quick Ref

Zero-allocation HC-128 eSTREAM cryptographic stream cipher PRNG (Hongjun Wu specification) in pure CP (`v1.0.0`, `MIT OR Apache-2.0`)

## Import

```cp
import {
    Hc128Rng,
    rhcRotr32,
    rhcRotl32,
    rhcF1,
    rhcF2,
    rhcG1,
    rhcG2,
    rhcReadU32Le,
} from "rand_hc";
```

## Core Types & Constants

```cp
class Hc128Rng { p: [512]u32, q: [512]u32, counter: usize } // methods: h1(...) +5
```

## API Signatures

```cp
fn rhcRotr32(x: u32, n: usize): u32;
fn rhcRotl32(x: u32, n: usize): u32;
fn rhcF1(x: u32): u32;
fn rhcF2(x: u32): u32;
fn rhcG1(x: u32, y: u32, z: u32): u32;
fn rhcG2(x: u32, y: u32, z: u32): u32;
fn rhcReadU32Le(seed: [32]u8, offset: usize): u32;
```

## Minimal Usage

```cp
import { rhcRotr32 } from "rand_hc";

pub fn main() {
    let result = rhcRotr32(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"rand_hc"`; install via `cap add rand_hc`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
