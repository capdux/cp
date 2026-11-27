# lexical-write-float — AI Agent Quick Ref

Zero-allocation pure CP integer and floating-point formatter (rust-lexical lexical-write-float / lexical-write-integer parity) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    lexwCopyStr,
    lexwWritePaddedU64,
    lexwWriteSpecialFloat,
    writeUintRadix,
    writeIntRadix,
    writeFloatFixed,
    writeFloatSci,
} from "lexical-write-float";
```

## API Signatures

```cp
fn lexwCopyStr(out: []u8, pos: usize, src: string): usize;
fn lexwWritePaddedU64(out: []u8, pos: usize, val: u64, width: usize): usize;
fn lexwWriteSpecialFloat(val: f64, out: []u8): usize;
fn writeUintRadix(val: u64, radix: u32, uppercase: bool, out: []u8): usize;
fn writeIntRadix(val: i64, radix: u32, uppercase: bool, out: []u8): usize;
fn writeFloatFixed(val: f64, decimals: u8, out: []u8): usize;
fn writeFloatSci(val: f64, precision: u8, out: []u8): usize;
```

## Minimal Usage

```cp
import { lexwCopyStr } from "lexical-write-float";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = lexwCopyStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"lexical-write-float"`; install via `cap add lexical-write-float`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
