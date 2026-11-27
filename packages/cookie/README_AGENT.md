# cookie — AI Agent Quick Ref

Zero-allocation Rust-style CookieBuilder, percent-encoded Set-Cookie helpers, and removal cookie generator for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    EncodedCookieMeta,
    CookieBuilder,
    ckcDefaultCookie,
    parseEncodedCookie,
    formatEncodedCookie,
    makeRemovalCookie,
} from "cookie";
```

## Core Types & Constants

```cp
class EncodedCookieMeta { name_len: usize, value_len: usize, domain: string, path: string, /* +6 fields */ }
class CookieBuilder { cookie: Cookie } // methods: new(...) +7
```

## API Signatures

```cp
fn ckcDefaultCookie(name: string, value: string): Cookie;
fn parseEncodedCookie(header: string, name_out: []u8, val_out: []u8): EncodedCookieMeta;
fn formatEncodedCookie(c: Cookie, out: []u8): usize;
fn makeRemovalCookie(name: string, path: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseEncodedCookie } from "cookie";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = parseEncodedCookie("example", out_buf[0..256], buf_1[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"cookie"`; install via `cap add cookie`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
