[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# url-join

Zero-allocation smart URL joiner, slash/dot-segment path normalizer, query merger, and trailing-slash utilities (url-join / ufo parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:url-join` ([https://github.com/jfromaniello/url-join](https://github.com/jfromaniello/url-join))

| Metadata | Value |
| --- | --- |
| Package | `url-join` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add url-join
```

```cp
import {
    PartSplit,
    pushByte,
    isSchemeChar,
    schemePrefixLen,
    splitPart,
    trimLeadingSlashes,
    trimTrailingSlashes,
    urlJoin,
} from "url-join";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PartSplit` | `class` | Fields: `path: string, query: string, hash: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pushByte` | `(out: []u8, pos: usize, b: u8): usize` | Mutates or appends state via `pushByte`. |
| `isSchemeChar` | `(c: u8): bool` | Checks predicate `isSchemeChar` and returns `bool`. |
| `schemePrefixLen` | `(s: string): usize` | Invokes `schemePrefixLen` and returns `usize`. |
| `splitPart` | `(p: string): PartSplit` | Invokes `splitPart` and returns `PartSplit`. |
| `trimLeadingSlashes` | `(s: string): string` | Invokes `trimLeadingSlashes` and returns `string`. |
| `trimTrailingSlashes` | `(s: string): string` | Invokes `trimTrailingSlashes` and returns `string`. |
| `urlJoin` | `(parts: []string, out: []u8): string` | Invokes `urlJoin` and returns `string`. |
| `normalizeUrlPath` | `(path: string, out: []u8): string` | Invokes `normalizeUrlPath` and returns `string`. |
| `withQuery` | `(base_url: string, key: string, value: string, out: []u8): string` | Invokes `withQuery` and returns `string`. |
| `findQueryOrHash` | `(url: string): usize` | Queries/retrieves data via `findQueryOrHash` returning `usize`. |
| `withTrailingSlash` | `(url: string, out: []u8): string` | Invokes `withTrailingSlash` and returns `string`. |
| `withoutTrailingSlash` | `(url: string, out: []u8): string` | Invokes `withoutTrailingSlash` and returns `string`. |

## Minimal Example

```cp
import { schemePrefixLen } from "url-join";

pub fn main() {
    let result = schemePrefixLen("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
