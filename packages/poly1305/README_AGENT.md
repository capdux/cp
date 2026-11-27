# poly1305 — AI Agent Quick Ref

Pure CP zero-allocation RFC 8439 Poly1305 one-time authenticator (Donna 26-bit limbs + constant-time verification via subtle-ct) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Poly1305,
    p1305ReadU32Le,
    p1305WriteU32Le,
    poly1305Mac,
    poly1305Verify,
} from "poly1305";
```

## Core Types & Constants

```cp
class Poly1305 { r0: u32, r1: u32, r2: u32, r3: u32, /* +12 fields */ } // methods: init(...) +3
```

## API Signatures

```cp
fn p1305ReadU32Le(b: []u8, off: usize): u32;
fn p1305WriteU32Le(out: []u8, off: usize, v: u32);
fn poly1305Mac(key32: []u8, msg: []u8, out_tag16: []u8): bool;
fn poly1305Verify(key32: []u8, msg: []u8, expected_tag16: []u8): bool;
```

## Minimal Usage

```cp
import { p1305ReadU32Le } from "poly1305";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = p1305ReadU32Le(out_buf[0..256], 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"poly1305"`; install via `cap add poly1305`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
