# pcg-random — AI Agent Quick Ref

Zero-heap Melissa O'Neill PCG-XSH-RR 32-bit deterministic PRNG with stream separation, unbiased bounded sampling, O(log n) advance, and byte filling. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Pcg32,
    pcgMult,
    defaultStream,
    add64,
    mul64,
    rotr32,
} from "pcg-random";
```

## Core Types & Constants

```cp
class Pcg32 { state: u64, inc: u64 } // methods: new(...) +7
```

## API Signatures

```cp
fn pcgMult(): u64;
fn defaultStream(): u64;
fn add64(a: u64, b: u64): u64;
fn mul64(a: u64, b: u64): u64;
fn rotr32(x: u32, rot: u32): u32;
```

## Minimal Usage

```cp
import { pcgMult } from "pcg-random";

pub fn main() {
    let result = pcgMult();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"pcg-random"`; install via `cap add pcg-random`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
