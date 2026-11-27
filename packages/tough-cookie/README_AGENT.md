# tough-cookie — AI Agent Quick Ref

Zero-heap RFC 6265 HTTP Cookie, Set-Cookie parser, serializer, and domain/path matching jar (clean-room port of npm cookie + tough-cookie) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Cookie,
    CookieJar,
    parseSetCookie,
    serializeSetCookie,
    getCookieHeaderValue,
    domainMatches,
    pathMatches,
} from "tough-cookie";
```

## Core Types & Constants

```cp
class Cookie { name: string, value: string, domain: string, path: string, /* +5 fields */ }
class CookieJar { items: [32]Cookie, count: usize } // methods: new(...) +3
```

## API Signatures

```cp
fn parseSetCookie(header: string): Cookie;
fn serializeSetCookie(c: Cookie, out: []u8): usize;
fn getCookieHeaderValue(header: string, name: string): string?;
fn domainMatches(host: string, cookie_domain: string): bool;
fn pathMatches(req_path: string, cookie_path: string): bool;
```

## Minimal Usage

```cp
import { parseSetCookie } from "tough-cookie";

pub fn main() {
    let result = parseSetCookie("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"tough-cookie"`; install via `cap add tough-cookie`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
