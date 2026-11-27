# parseurl — AI Agent Quick Ref

Zero-allocation high-speed HTTP request URL pathname/query zero-copy splitter and path safety checker for CP (clean-room parity with Express parseurl) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ParsedRequestUrl,
    purlIsAlpha,
    purlIsSchemeChar,
    purlHexVal,
    purlFindOriginEnd,
    parseRequestUrl,
    decodePathname,
    isSafeRequestPath,
} from "parseurl";
```

## Core Types & Constants

```cp
class ParsedRequestUrl { href: string, path: string, pathname: string, search: string, /* +3 fields */ } // methods: empty(...)
```

## API Signatures

```cp
fn purlIsAlpha(b: u8): bool;
fn purlIsSchemeChar(b: u8): bool;
fn purlHexVal(ch: u8): u8;
fn purlFindOriginEnd(raw_url: string): usize;
fn parseRequestUrl(raw_url: string): ParsedRequestUrl;
fn decodePathname(pathname: string, out: []u8): usize;
fn isSafeRequestPath(pathname: string): bool;
fn getRequestQueryParam(raw_url: string, key: string, out: []u8): usize?;
fn decodeQueryValue(raw: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseRequestUrl } from "parseurl";

pub fn main() {
    let result = parseRequestUrl("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"parseurl"`; install via `cap add parseurl`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
