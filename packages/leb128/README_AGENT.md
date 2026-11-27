# leb128 — AI Agent Quick Ref

Zero-heap ULEB128 (unsigned) and SLEB128 (signed) variable-length integer encoder/decoder for WebAssembly, DWARF & binary formats (compatible with Rust leb128) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    UlebResult,
    SlebResult,
    encodeUnsigned,
    decodeUnsigned,
    encodeSigned,
    decodeSigned,
    unsignedEncodedLen,
} from "leb128";
```

## Core Types & Constants

```cp
class UlebResult { value: u64, consumed: usize, valid: bool }
class SlebResult { value: i64, consumed: usize, valid: bool }
```

## API Signatures

```cp
fn encodeUnsigned(val: u64, out: []u8): usize;
fn decodeUnsigned(data: &[]u8, start: usize): UlebResult;
fn encodeSigned(val: i64, out: []u8): usize;
fn decodeSigned(data: &[]u8, start: usize): SlebResult;
fn unsignedEncodedLen(val: u64): usize;
```

## Minimal Usage

```cp
import { encodeUnsigned } from "leb128";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeUnsigned(0, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"leb128"`; install via `cap add leb128`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
