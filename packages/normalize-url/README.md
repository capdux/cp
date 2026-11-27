[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# normalize-url

Zero-allocation URL normalization utility (npm normalize-url parity) composing prepend-http for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:normalize-url` ([https://github.com/sindresorhus/normalize-url](https://github.com/sindresorhus/normalize-url))

| Metadata | Value |
| --- | --- |
| Package | `normalize-url` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add normalize-url
```

```cp
import {
    NormalizeUrlOptions,
    NurlSegment,
    defaultNormalizeUrlOptions,
    nurlWriteByte,
    nurlWriteLower,
    nurlFindSchemeSep,
    nurlIsDefaultPort,
    nurlNormalizePath,
} from "normalize-url";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `NormalizeUrlOptions` | `class` | Fields: `default_https: bool, strip_www: bool, strip_hash: bool, remove_trailing_slash: bool, remove_default_port: bool, normalize_protocol: bool` |
| `NurlSegment` | `class` | Fields: `start: usize, end: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `defaultNormalizeUrlOptions` | `(): NormalizeUrlOptions` | Invokes `defaultNormalizeUrlOptions` and returns `NormalizeUrlOptions`. |
| `nurlWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `nurlWriteByte` and returns `usize`. |
| `nurlWriteLower` | `(out: []u8, pos: usize, s: string): usize` | Invokes `nurlWriteLower` and returns `usize`. |
| `nurlFindSchemeSep` | `(s: string): usize` | Invokes `nurlFindSchemeSep` and returns `usize`. |
| `nurlIsDefaultPort` | `(scheme: string, port: string): bool` | Invokes `nurlIsDefaultPort` and returns `bool`. |
| `nurlNormalizePath` | `(raw_path: string, remove_trailing_slash: bool, has_query_or_hash: bool, out: []u8, start_pos: usize): usize` | Invokes `nurlNormalizePath` and returns `usize`. |
| `normalizeUrlWithOptions` | `(url: string, opts: NormalizeUrlOptions, out: []u8): usize` | Invokes `normalizeUrlWithOptions` and returns `usize`. |
| `normalizeUrl` | `(url: string, out: []u8): usize` | Invokes `normalizeUrl` and returns `usize`. |

## Minimal Example

```cp
import { nurlWriteLower } from "normalize-url";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = nurlWriteLower(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
