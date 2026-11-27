[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# mime_guess

Zero-allocation file path and extension MIME type guesser for CP (clean-room MIT, inspired by Rust mime_guess v2, powered by mime-types).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:mime_guess` ([https://github.com/abonander/mime_guess](https://github.com/abonander/mime_guess))（原库名 `mime_guess` → CP 包名 `mime_guess`）

| Metadata | Value |
| --- | --- |
| Package | `mime_guess` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add mime_guess
```

```cp
import {
    MimeGuess,
    mgsStartsWith,
    mgsHasPathSep,
    mgsIsCompressibleMime,
    extractExt,
    getExtForMime,
    getCharsetForMime,
} from "mime_guess";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `MimeGuess` | `class` | Fields: `mime: string, found: bool` · Methods: `fromExt, fromPath, first, firstOrOctetStream, firstOrTextPlain, isText, +4 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `mgsStartsWith` | `(s: string, prefix: string): bool` | Invokes `mgsStartsWith` and returns `bool`. |
| `mgsHasPathSep` | `(s: string): bool` | Invokes `mgsHasPathSep` and returns `bool`. |
| `mgsIsCompressibleMime` | `(mime_type: string): bool` | Invokes `mgsIsCompressibleMime` and returns `bool`. |
| `extractExt` | `(path: string): string?` | Invokes `extractExt` and returns `string?`. |
| `getExtForMime` | `(mime_type: string): string?` | Queries/retrieves data via `getExtForMime` returning `string?`. |
| `getCharsetForMime` | `(mime_type: string): string?` | Queries/retrieves data via `getCharsetForMime` returning `string?`. |

## Minimal Example

```cp
import { getExtForMime } from "mime_guess";

pub fn main() {
    let result = getExtForMime("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
