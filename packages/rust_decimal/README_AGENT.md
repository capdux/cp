# rust_decimal — AI Agent Quick Ref

Zero-heap fixed-point financial decimal arithmetic (scale 0..12) with banker's rounding in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ParseDecResult,
    Decimal,
    absToU64,
    clampScale,
    pow10F64,
} from "rust_decimal";
```

## Core Types & Constants

```cp
class ParseDecResult { value: Decimal, valid: bool }
class Decimal { mantissa: i64, scale: u8 } // methods: new(...) +17
```

## API Signatures

```cp
fn absToU64(v: i64): u64;
fn clampScale(s: u8): u8;
fn pow10F64(s: u8): f64;
```

## Minimal Usage

```cp
import { absToU64 } from "rust_decimal";

pub fn main() {
    let result = absToU64(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"rust_decimal"`; install via `cap add rust_decimal`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
