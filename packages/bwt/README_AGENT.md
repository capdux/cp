# bwt — AI Agent Quick Ref

Zero-allocation Burrows-Wheeler Transform (BWT) and Move-To-Front (MTF) reversible compression preprocessing in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    BwtResult,
    cmpRotations,
    bwtForward,
    bwtInverse,
    mtfEncode,
    mtfDecode,
} from "bwt";
```

## Core Types & Constants

```cp
class BwtResult { primary_index: usize, len: usize, ok: bool }
```

## API Signatures

```cp
fn cmpRotations(src: &[]u8, a: usize, b: usize): i32;
fn bwtForward(src: &[]u8, dst: []u8): BwtResult;
fn bwtInverse(src: &[]u8, primary_index: usize, dst: []u8): bool;
fn mtfEncode(src: &[]u8, dst: []u8): bool;
fn mtfDecode(src: &[]u8, dst: []u8): bool;
```

## Minimal Usage

```cp
import { cmpRotations } from "bwt";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = cmpRotations(out_buf[0..256], 0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"bwt"`; install via `cap add bwt`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
