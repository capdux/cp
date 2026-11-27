# phonenumber — AI Agent Quick Ref

Zero-heap RFC 3966 tel: URI parser, E.164 international phone number normalizer/validator, and URI formatter. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    TelUri,
    isVisualSep,
    isWs,
    isValidE164,
    normalizeE164,
    parseTelUri,
    appendSlice,
    formatTelUri,
} from "phonenumber";
```

## Core Types & Constants

```cp
class TelUri { number: string, is_global: bool, ext: string, phone_context: string, /* +1 fields */ }
```

## API Signatures

```cp
fn isVisualSep(c: u8): bool;
fn isWs(c: u8): bool;
fn isValidE164(number: string): bool;
fn normalizeE164(input: string, out: []u8): usize;
fn parseTelUri(uri: string): TelUri;
fn appendSlice(out: []u8, pos: usize, s: string): i64;
fn formatTelUri(e164_number: string, ext: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { isValidE164 } from "phonenumber";

pub fn main() {
    let result = isValidE164("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"phonenumber"`; install via `cap add phonenumber`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
