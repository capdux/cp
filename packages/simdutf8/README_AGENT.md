# simdutf8 — AI Agent Quick Ref

Zero-heap strict RFC 3629 UTF-8 validator with ASCII fast-path, basic/compat validation, valid_up_to & error_len reporting (crates.io simdutf8 equivalent) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Utf8Error,
    Utf8CompatResult,
    sutfIsCont,
    isAscii,
    validateUtf8Compat,
    basicIsValidUtf8,
    countCodepoints,
} from "simdutf8";
```

## Core Types & Constants

```cp
class Utf8Error { valid_up_to: usize, has_error_len: bool, error_len: usize }
class Utf8CompatResult { valid: bool, valid_up_to: usize, has_error_len: bool, error_len: usize }
```

## API Signatures

```cp
fn sutfIsCont(b: u8): bool;
fn isAscii(buf: []u8): bool;
fn validateUtf8Compat(buf: []u8): Utf8CompatResult;
fn basicIsValidUtf8(buf: []u8): bool;
fn countCodepoints(buf: []u8): usize?;
```

## Minimal Usage

```cp
import { isAscii } from "simdutf8";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = isAscii(out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"simdutf8"`; install via `cap add simdutf8`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
