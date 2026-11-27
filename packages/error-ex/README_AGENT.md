# error-ex — AI Agent Quick Ref

Zero-heap structured error and context formatter with template interpolation (`v1.3.2`, `MIT`)

## Import

```cp
import {
    ErrorField,
    StructuredError,
    erexWriteByte,
    erexWriteStr,
    erexWriteI32,
    formatStructuredError,
    erexMatchesPlaceholder,
    formatErrorTemplate,
} from "error-ex";
```

## Core Types & Constants

```cp
class ErrorField { key: string, value: string, line_mode: bool }
class StructuredError { name: string, code: i32, message: string }
```

## API Signatures

```cp
fn erexWriteByte(out: []u8, pos: usize, b: u8): usize;
fn erexWriteStr(out: []u8, pos: usize, s: string): usize;
fn erexWriteI32(out: []u8, pos: usize, val: i32): usize;
fn formatStructuredError(e: StructuredError, fields: &[]ErrorField, out: []u8): usize;
fn erexMatchesPlaceholder(tmpl: string, start: usize, key: string): bool;
fn formatErrorTemplate(tmpl: string, key: string, val: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { formatErrorTemplate } from "error-ex";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatErrorTemplate("example", "example", "example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"error-ex"`; install via `cap add error-ex`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
