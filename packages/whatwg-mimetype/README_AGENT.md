# whatwg-mimetype — AI Agent Quick Ref

Zero-allocation WHATWG MIME Sniffing Standard MIMEType parser, parameter map, essence, and serializer (clean-room MIT implementation of npm whatwg-mimetype, #200 / 215M wk) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    MimeParam,
    MimeType,
    wmimeIsHttpWs,
    wmimeIsTokenChar,
    wmimeEndsWithIgnoreCase,
    wmimePushLower,
    parseMimeType,
} from "whatwg-mimetype";
```

## Core Types & Constants

```cp
class MimeParam { name: string, value: string }
class MimeType { valid: bool, main_type: string, subtype: string, params: [8]MimeParam, /* +1 fields */ } // methods: paramCount(...) +7
```

## API Signatures

```cp
fn wmimeIsHttpWs(b: u8): bool;
fn wmimeIsTokenChar(b: u8): bool;
fn wmimeEndsWithIgnoreCase(s: string, suffix: string): bool;
fn wmimePushLower(out: []u8, pos: usize, s: string): usize;
fn parseMimeType(input: string): MimeType;
```

## Minimal Usage

```cp
import { parseMimeType } from "whatwg-mimetype";

pub fn main() {
    let result = parseMimeType("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"whatwg-mimetype"`; install via `cap add whatwg-mimetype`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
