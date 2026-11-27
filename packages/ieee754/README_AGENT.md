# ieee754 — AI Agent Quick Ref

Zero-allocation IEEE-754 32-bit and 64-bit floating-point binary encoder/decoder (little-endian & big-endian) with NaN, Infinity, and subnormal fidelity (clean-room MIT implementation of npm ieee754, #293 / 173M wk) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    readF32Le,
    readF32Be,
    writeF32Le,
    writeF32Be,
    readF64Le,
    readF64Be,
    writeF64Le,
    writeF64Be,
} from "ieee754";
```

## API Signatures

```cp
fn readF32Le(buf: &[]u8, offset: usize): f32;
fn readF32Be(buf: &[]u8, offset: usize): f32;
fn writeF32Le(buf: []u8, offset: usize, value: f32): bool;
fn writeF32Be(buf: []u8, offset: usize, value: f32): bool;
fn readF64Le(buf: &[]u8, offset: usize): f64;
fn readF64Be(buf: &[]u8, offset: usize): f64;
fn writeF64Le(buf: []u8, offset: usize, value: f64): bool;
fn writeF64Be(buf: []u8, offset: usize, value: f64): bool;
fn readIeee754(buf: &[]u8, offset: usize, is_le: bool, n_bytes: usize): f64;
fn writeIeee754(buf: []u8, value: f64, offset: usize, is_le: bool, n_bytes: usize): bool;
```

## Minimal Usage

```cp
import { readF32Le } from "ieee754";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = readF32Le(out_buf[0..256], 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ieee754"`; install via `cap add ieee754`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
