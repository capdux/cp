[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# querystring

Zero-allocation URL query string parser, stringifier & parameter extractor (Node.js querystring & npm query-string v9 parity) composing percent-encoding for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:querystring` ([https://github.com/sindresorhus/query-string](https://github.com/sindresorhus/query-string))

| Metadata | Value |
| --- | --- |
| Package | `querystring` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add querystring
```

```cp
import {
    QueryEntry,
    qstFindByte,
    qstNeedsDecode,
    qstHasSlashBefore,
    escapeQuery,
    unescapeQuery,
    extractUrlQuery,
    parseQueryString,
} from "querystring";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `QueryEntry` | `class` | Fields: `key: string, value: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `qstFindByte` | `(s: string, start: usize, end: usize, target: u8): usize` | Invokes `qstFindByte` and returns `usize`. |
| `qstNeedsDecode` | `(s: string): bool` | Invokes `qstNeedsDecode` and returns `bool`. |
| `qstHasSlashBefore` | `(s: string, end: usize): bool` | Invokes `qstHasSlashBefore` and returns `bool`. |
| `escapeQuery` | `(raw: string, out: []u8): usize` | Invokes `escapeQuery` and returns `usize`. |
| `unescapeQuery` | `(raw: string, out: []u8): usize` | Invokes `unescapeQuery` and returns `usize`. |
| `extractUrlQuery` | `(url: string): string` | Invokes `extractUrlQuery` and returns `string`. |
| `parseQueryString` | `(input: string, sep: u8, eq: u8, out_entries: []QueryEntry): usize` | Parses input via `parseQueryString` and returns `usize`. |
| `stringifyQueryString` | `(entries: &[]QueryEntry, sep: u8, eq: u8, out: []u8): usize` | Serializes/formats data via `stringifyQueryString` returning `usize`. |
| `pickQueryParam` | `(url_or_query: string, target_key: string, out: []u8): usize?` | Invokes `pickQueryParam` and returns `usize?`. |

## Minimal Example

```cp
import { parseQueryString } from "querystring";

pub fn main() {
    let result = parseQueryString("example", 0, 0, /* out_entries: []QueryEntry */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
