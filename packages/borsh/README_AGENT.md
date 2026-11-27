# borsh — AI Agent Quick Ref

Zero-allocation canonical little-endian Borsh binary serializer and deserializer for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    BytesSpan,
    BorshWriter,
    BorshReader,
    i32ToBits,
    bitsToI32,
    i64ToBits,
    bitsToI64,
} from "borsh";
```

## Core Types & Constants

```cp
class BytesSpan { start: usize, len: usize, ok: bool }
class BorshWriter { pos: usize } // methods: new(...) +10
class BorshReader { pos: usize, ok: bool } // methods: new(...) +10
```

## API Signatures

```cp
fn i32ToBits(v: i32): u32;
fn bitsToI32(u: u32): i32;
fn i64ToBits(v: i64): u64;
fn bitsToI64(u: u64): i64;
```

## Minimal Usage

```cp
import { i32ToBits } from "borsh";

pub fn main() {
    let result = i32ToBits(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"borsh"`; install via `cap add borsh`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
