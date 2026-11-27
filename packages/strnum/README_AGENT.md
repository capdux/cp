# strnum — AI Agent Quick Ref

Zero-heap configurable string-to-number parser for CP (clean-room MIT implementation of npm strnum). (`v1.0.0`, `MIT`)

## Import

```cp
import {
    StrNumOptions,
    StrNumResult,
    snumInvalid,
    snumIsSpace,
    snumHexDigit,
    snumScalePow10,
    parseStrNum,
} from "strnum";
```

## Core Types & Constants

```cp
class StrNumOptions { hex: bool, leading_zeros: bool, e_notation: bool }
class StrNumResult { is_number: bool, value: f64, is_int: bool, int_value: i64 }
```

## API Signatures

```cp
fn snumInvalid(): StrNumResult;
fn snumIsSpace(ch: u8): bool;
fn snumHexDigit(ch: u8): i32;
fn snumScalePow10(val: f64, exp10: i32): f64;
fn parseStrNum(input: string, opts: StrNumOptions): StrNumResult;
```

## Minimal Usage

```cp
import { parseStrNum } from "strnum";

pub fn main() {
    let result = parseStrNum("example", /* opts: StrNumOptions */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"strnum"`; install via `cap add strnum`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
