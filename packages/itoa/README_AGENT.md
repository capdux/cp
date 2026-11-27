# itoa — AI Agent Quick Ref

Fast zero-heap two-digits-per-step integer-to-ASCII formatter (clean-room CP implementation of crates.io itoa) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    formatU64,
    formatI64,
    formatU32,
    formatI32,
    formatUsize,
    formatHexU64,
    formatRadixU64,
} from "itoa";
```

## API Signatures

```cp
fn formatU64(val: u64, out: []u8): usize;
fn formatI64(val: i64, out: []u8): usize;
fn formatU32(val: u32, out: []u8): usize;
fn formatI32(val: i32, out: []u8): usize;
fn formatUsize(val: usize, out: []u8): usize;
fn formatHexU64(val: u64, uppercase: bool, out: []u8): usize;
fn formatRadixU64(val: u64, radix: u8, out: []u8): usize;
```

## Minimal Usage

```cp
import { formatU64 } from "itoa";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatU64(0, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"itoa"`; install via `cap add itoa`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add itoa
cpc check src/main.cp
```
