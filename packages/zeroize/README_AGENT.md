# zeroize — AI Agent Quick Ref

Zero-heap secure secret memory wiping and ZeroizingBuf32/ZeroizingBuf64 key containers (crates.io zeroize equivalent) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ZeroizingKey32,
    ZeroizingBuf64,
    zeroizeBytes,
    zeroizeU32,
    zeroizeU64,
    isAllZeroBytes,
} from "zeroize";
```

## Core Types & Constants

```cp
class ZeroizingKey32 { bytes: [32]u8, active: bool } // methods: fromSlice(...) +2
class ZeroizingBuf64 { bytes: [64]u8, len: usize, active: bool } // methods: fromSlice(...) +2
```

## API Signatures

```cp
fn zeroizeBytes(buf: []u8): usize;
fn zeroizeU32(buf: []u32): usize;
fn zeroizeU64(buf: []u64): usize;
fn isAllZeroBytes(buf: []u8): bool;
```

## Minimal Usage

```cp
import { isAllZeroBytes } from "zeroize";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = isAllZeroBytes(out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"zeroize"`; install via `cap add zeroize`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
