[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# parse-link-header

Zero-allocation RFC 8288 HTTP Link header parser and builder with GitHub/REST pagination helpers for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:parse-link-header` ([https://github.com/thlorenz/parse-link-header](https://github.com/thlorenz/parse-link-header))（原库名 `parse-link-header` → CP 包名 `parse-link-header`）

| Metadata | Value |
| --- | --- |
| Package | `parse-link-header` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add parse-link-header
```

```cp
import {
    LinkEntry,
    LinkHeader,
    relMatches,
    skipToNextEntry,
    parseLinkHeader,
    appendByte,
    appendParam,
    formatLinkHeader,
} from "parse-link-header";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `LinkEntry` | `class` | Fields: `url: string, rel: string, title: string, type_val: string, hreflang: string` · Methods: `empty` |
| `LinkHeader` | `class` | Fields: `entries: [32]LinkEntry, count: usize` · Methods: `empty, entryAt, getByRel, nextUrl, prevUrl, firstUrl, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `relMatches` | `(rel_attr: string, target: string): bool` | Invokes `relMatches` and returns `bool`. |
| `skipToNextEntry` | `(s: string, start: usize): usize` | Invokes `skipToNextEntry` and returns `usize`. |
| `parseLinkHeader` | `(header: string): LinkHeader` | Parses input via `parseLinkHeader` and returns `LinkHeader`. |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | Invokes `appendByte` and returns `usize`. |
| `appendParam` | `(out: []u8, pos: usize, name: string, val: string): usize` | Invokes `appendParam` and returns `usize`. |
| `formatLinkHeader` | `(entries: []LinkEntry, out: []u8): string` | Serializes/formats data via `formatLinkHeader` returning `string`. |
| `appendPageLink` | `(out: []u8, start_pos: usize, base_url: string, sep: u8, target_page: usize, rel: string, emitted: usize): usize` | Invokes `appendPageLink` and returns `usize`. |
| `formatPaginationLinks` | `(base_url: string, page: usize, total_pages: usize, out: []u8): string` | Serializes/formats data via `formatPaginationLinks` returning `string`. |

## Minimal Example

```cp
import { parseLinkHeader } from "parse-link-header";

pub fn main() {
    let result = parseLinkHeader("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
