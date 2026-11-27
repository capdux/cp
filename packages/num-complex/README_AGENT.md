# num-complex — AI Agent Quick Ref

Zero-allocation Complex64 arithmetic, Smith overflow-safe division, polar coordinates, Euler exp, principal sqrt, and formatting (Rust num-complex parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Complex64,
    fabs,
    pushByte,
    pushU64,
    pushAbsFloat,
    formatComplex,
} from "num-complex";
```

## Core Types & Constants

```cp
class Complex64 { re: f64, im: f64 } // methods: new(...) +19
```

## API Signatures

```cp
fn fabs(x: f64): f64;
fn pushByte(out: []u8, pos: usize, b: u8): usize;
fn pushU64(out: []u8, pos: usize, val: u64): usize;
fn pushAbsFloat(out: []u8, pos: usize, v: f64): usize;
fn formatComplex(z: Complex64, out: []u8): string;
```

## Minimal Usage

```cp
import { formatComplex } from "num-complex";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatComplex(/* z: Complex64 */, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"num-complex"`; install via `cap add num-complex`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
