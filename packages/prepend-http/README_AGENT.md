# prepend-http — AI Agent Quick Reference

Zero-allocation URL protocol prepender (`https://` / `http://`) matching npm `prepend-http` v4.

## Import

```cp
import {
    isProtocolRelative,
    hasScheme,
    prependHttp,
} from "prepend-http";
```

## API Signatures

```cp
fn isProtocolRelative(url: string): bool;
fn hasScheme(url: string): bool;
fn prependHttp(url: string, https: bool, out: []u8): usize;
```

## Minimal Example

```cp
import { prependHttp, hasScheme } from "prepend-http";

let buf: [128]u8 = [0; 128];
const n1 = prependHttp("example.com/docs", true, buf[0..128]);  // "https://example.com/docs"
const n2 = prependHttp("localhost:8080", false, buf[0..128]);   // "http://localhost:8080"
const has = hasScheme("mailto:dev@cp-lang.org");                // true
```

## Preserved Prefixes (Unmodified, Only Trimmed)

- Explicit RFC 3986 schemes: `http://`, `https://`, `ftp://`, `custom+v1://`
- Case-insensitive `mailto:`
- Protocol-relative `//example.com`
- Relative paths: `./`, `../`, `/`

## Gotchas & Borrow Rules

- Always ensure `out.len >= url.len + 8` (8 bytes for `"https://"`) to avoid truncation when prepending.
- Empty or whitespace-only `url` returns `0` without writing any scheme prefix.
- `url` is a read-only `string` borrow; `out: []u8` is caller-owned mutable output.
- Leading and trailing ASCII whitespace is always trimmed in the output written to `out`.
- Do not pass `https = false` if you want npm `prepend-http` v4 default behavior (`https: true`).
