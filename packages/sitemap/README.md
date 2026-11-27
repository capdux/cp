[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# sitemap

Zero-heap Sitemaps.org 0.9 XML sitemap & sitemapindex generator and validator in pure CP (modeled after npm sitemap)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:sitemap` ([https://github.com/ekalinin/sitemap.js](https://github.com/ekalinin/sitemap.js))

| Metadata | Value |
| --- | --- |
| Package | `sitemap` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add sitemap
```

```cp
import {
    SitemapUrl,
    SitemapWriter,
    SitemapIndexWriter,
    freqName,
    isValidPriorityTenths,
} from "sitemap";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SitemapUrl` | `class` | Fields: `loc: &[]u8, lastmod: &[]u8, changefreq: u8, priority_tenths: i8` |
| `SitemapWriter` | `class` | Fields: `buf: []u8, pos: usize, url_count: usize, closed: bool, overflow: bool` · Methods: `init, writeRaw, writeXmlEscaped, addUrl, finish, urlCount, +2 more` |
| `SitemapIndexWriter` | `class` | Fields: `buf: []u8, pos: usize, entry_count: usize, closed: bool, overflow: bool` · Methods: `init, writeRaw, writeXmlEscaped, addSitemap, finish, entryCount, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `freqName` | `(f: u8): &[]u8` | Invokes `freqName` and returns `&[]u8`. |
| `isValidPriorityTenths` | `(p: i8): bool` | Checks predicate `isValidPriorityTenths` and returns `bool`. |

## Minimal Example

```cp
import { isValidPriorityTenths } from "sitemap";

pub fn main() {
    let result = isValidPriorityTenths(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
