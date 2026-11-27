# humanize-url — AI Agent Quick Reference

Zero-allocation, zero-copy URL humanizer stripping protocol, `www.`, and trailing `/` (npm `humanize-url` v3 parity).

## Import

```cp
import {
    isProtocolRelative,
    extractScheme,
    stripProtocol,
    stripWww,
    stripTrailingSlash,
    humanizeUrlSlice,
    humanizedLen,
    humanizeUrl,
} from "humanize-url";
```

## API Signatures

```cp
fn isProtocolRelative(url: string): bool;
fn extractScheme(url: string): string;
fn stripProtocol(url: string): string;
fn stripWww(host_and_rest: string): string;
fn stripTrailingSlash(url: string): string;
fn humanizeUrlSlice(url: string): string;
fn humanizedLen(url: string): usize;
fn humanizeUrl(url: string, out: []u8): usize;
```

## Minimal Example

```cp
import { humanizeUrlSlice, humanizeUrl } from "humanize-url";

// Zero-copy slice (preferred when input outlives result)
const display = humanizeUrlSlice("https://www.example.com/path/"); // "example.com/path"

// Owned buffer copy
let buf: [128]u8 = [0; 128];
const n = humanizeUrl("  http://www.cp-lang.org/  ", buf[0..128]);
// buf[0..n] == "cp-lang.org"
```

## Gotchas & Borrow Rules

- `humanizeUrlSlice`, `extractScheme`, `stripProtocol`, `stripWww`, and `stripTrailingSlash` return borrowed sub-slices of `url` — keep `url` alive while using the returned slice.
- Prefer `humanizeUrlSlice(url)` over `humanizeUrl(url, out)` whenever you do not need an owned copy.
- `stripProtocol` only strips `http://`, `https://` (case-insensitive), and `//`; non-HTTP schemes like `ftp://` or `mailto:` are preserved intact.
- `stripWww` preserves bare domains without another dot (e.g. `https://www.com/` -> `"www.com"`).
- `stripTrailingSlash` removes at most one trailing `/`; query strings (`?a=1`), ports (`:8080`), and fragments (`#top`) are preserved.
