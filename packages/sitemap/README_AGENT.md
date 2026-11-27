# sitemap — AI Agent Quick Ref

Zero-heap Sitemaps.org 0.9 XML sitemap & sitemapindex generator and validator in pure CP (modeled after npm sitemap) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SitemapUrl,
    SitemapWriter,
    SitemapIndexWriter,
    freqName,
    isValidPriorityTenths,
} from "sitemap";
```

## Core Types & Constants

```cp
class SitemapUrl { loc: &[]u8, lastmod: &[]u8, changefreq: u8, priority_tenths: i8 }
class SitemapWriter { buf: []u8, pos: usize, url_count: usize, closed: bool, /* +1 fields */ } // methods: init(...) +7
class SitemapIndexWriter { buf: []u8, pos: usize, entry_count: usize, closed: bool, /* +1 fields */ } // methods: init(...) +6
```

## API Signatures

```cp
fn freqName(f: u8): &[]u8;
fn isValidPriorityTenths(p: i8): bool;
```

## Minimal Usage

```cp
import { isValidPriorityTenths } from "sitemap";

pub fn main() {
    let result = isValidPriorityTenths(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"sitemap"`; install via `cap add sitemap`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
