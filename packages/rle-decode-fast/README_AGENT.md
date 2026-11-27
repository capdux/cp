# rle-decode-fast — AI Agent Quick Ref

Zero-allocation Apple/TIFF PackBits byte run-length compression and Prometheus/Gorilla-style Delta & Delta-of-Delta time-series integer encoding for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    encodePackBits,
    decodePackBits,
    encodeDeltaI64,
    decodeDeltaI64,
    encodeDeltaOfDeltaI64,
    decodeDeltaOfDeltaI64,
} from "rle-decode-fast";
```

## API Signatures

```cp
fn encodePackBits(input: string, out: []u8): string?;
fn decodePackBits(encoded: string, out: []u8): string?;
fn encodeDeltaI64(values: &[]i64, out: []i64): usize;
fn decodeDeltaI64(deltas: &[]i64, out: []i64): usize;
fn encodeDeltaOfDeltaI64(values: &[]i64, out: []i64): usize;
fn decodeDeltaOfDeltaI64(dod: &[]i64, out: []i64): usize;
```

## Minimal Usage

```cp
import { encodePackBits } from "rle-decode-fast";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodePackBits("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"rle-decode-fast"`; install via `cap add rle-decode-fast`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
