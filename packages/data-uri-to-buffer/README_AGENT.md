# data-uri-to-buffer — AI Agent Quick Ref

Zero-allocation RFC 2397 Data URI parser, Base64/percent decoder, and encoder for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    DataUri,
    appendByte,
    appendStr,
    isUnreservedUriByte,
    parseDataUri,
    isDataUri,
    b64Val,
    decodeBase64Payload,
} from "data-uri-to-buffer";
```

## Core Types & Constants

```cp
class DataUri { ok: bool, media_type: string, charset: string, is_base64: bool, /* +1 fields */ } // methods: invalid(...)
```

## API Signatures

```cp
fn appendByte(out: []u8, pos: usize, c: u8): usize;
fn appendStr(out: []u8, pos: usize, s: string): usize;
fn isUnreservedUriByte(c: u8): bool;
fn parseDataUri(uri: string): DataUri;
fn isDataUri(s: string): bool;
fn b64Val(c: u8): u8?;
fn decodeBase64Payload(payload: string, out: []u8): ([]u8)?;
fn decodePercentPayload(payload: string, out: []u8): ([]u8)?;
fn decodeDataUri(uri: string, out: []u8): ([]u8)?;
fn encodeDataUriBase64(media_type: string, payload: []u8, out: []u8): string;
fn encodeDataUriText(media_type: string, text: string, out: []u8): string;
```

## Minimal Usage

```cp
import { parseDataUri } from "data-uri-to-buffer";

pub fn main() {
    let result = parseDataUri("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"data-uri-to-buffer"`; install via `cap add data-uri-to-buffer`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
