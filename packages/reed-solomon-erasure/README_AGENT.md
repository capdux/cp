# reed-solomon-erasure — AI Agent Quick Ref

Zero-allocation GF(2^8) Cauchy Reed-Solomon erasure coding and FEC reconstruction engine for CP (`v1.0.0`, `MIT`)

## Import

```cp
import { RsCodec, gfMul, gfPow, gfInv } from "reed-solomon-erasure";
```

## Core Types & Constants

```cp
class RsCodec { data_count: usize, parity_count: usize, parity_matrix: [128]u8 } // methods: init(...) +6
```

## API Signatures

```cp
fn gfMul(a: u8, b: u8): u8;
fn gfPow(a: u8, n: u32): u8;
fn gfInv(a: u8): u8;
```

## Minimal Usage

```cp
import { gfMul } from "reed-solomon-erasure";

pub fn main() {
    let result = gfMul(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"reed-solomon-erasure"`; install via `cap add reed-solomon-erasure`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
