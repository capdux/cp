# fastrand — AI Agent Quick Ref

Zero-allocation WyRand 64-bit PRNG with unbiased range sampling, 53-bit unit floats, Fisher-Yates shuffle, and alphanumeric generation for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Rng,
    addWrap64,
    mulWide64Xor,
    i64ToOrderedU64,
    orderedU64ToI64,
} from "fastrand";
```

## Core Types & Constants

```cp
class Rng { state: u64 } // methods: withSeed(...) +13
```

## API Signatures

```cp
fn addWrap64(a: u64, b: u64): u64;
fn mulWide64Xor(a: u64, b: u64): u64;
fn i64ToOrderedU64(v: i64): u64;
fn orderedU64ToI64(u: u64): i64;
```

## Minimal Usage

```cp
import { addWrap64 } from "fastrand";

pub fn main() {
    let result = addWrap64(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"fastrand"`; install via `cap add fastrand`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
