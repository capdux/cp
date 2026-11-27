# decimal.js — AI Agent Quick Ref

Zero-allocation 128-bit exact decimal & financial arithmetic engine (decimal.js / bignumber.js / rust_decimal parity: exact add/sub/mul/div, 6 rounding modes including Banker's HalfEven, scientific notation parsing) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    RoundingMode,
    Decimal,
    pow10,
    absI128,
    divRoundI128,
} from "decimal.js";
```

## Core Types & Constants

```cp
enum RoundingMode { HalfEven, HalfUp, HalfDown, Floor, Ceil, Trunc }
class Decimal { mantissa: i128, scale: u8 } // methods: zero(...) +26
```

## API Signatures

```cp
fn pow10(exp: u8): i128;
fn absI128(v: i128): i128;
fn divRoundI128(num: i128, den: i128, mode: RoundingMode): i128;
```

## Minimal Usage

```cp
import { pow10 } from "decimal.js";

pub fn main() {
    let result = pow10(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"decimal.js"`; install via `cap add decimal.js`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
