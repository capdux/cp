[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# pkg-types

Zero-heap package.json and cap.json manifest parser and package name validator

| Metadata | Value |
| --- | --- |
| Package | `pkg-types` |
| Version | `1.3.1` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add pkg-types
```

```cp
import {
    PkgManifest,
    PkgNameParts,
    pkgtIsSpace,
    pkgtSkipSpace,
    pkgtStrEq,
    pkgtScanStringEnd,
    pkgtSkipValue,
    pkgtEmptyManifest,
} from "pkg-types";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PkgManifest` | `class` | Fields: `name: string, version: string, license: string, main: string, module: string, entry: string, +2 more` |
| `PkgNameParts` | `class` | Fields: `scope: string, pkg_name: string, has_scope: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pkgtIsSpace` | `(c: u8): bool` | Invokes `pkgtIsSpace` and returns `bool`. |
| `pkgtSkipSpace` | `(s: string, start: usize): usize` | Invokes `pkgtSkipSpace` and returns `usize`. |
| `pkgtStrEq` | `(a: string, b: string): bool` | Invokes `pkgtStrEq` and returns `bool`. |
| `pkgtScanStringEnd` | `(s: string, open_quote: usize): usize` | Invokes `pkgtScanStringEnd` and returns `usize`. |
| `pkgtSkipValue` | `(s: string, start: usize): usize` | Invokes `pkgtSkipValue` and returns `usize`. |
| `pkgtEmptyManifest` | `(): PkgManifest` | Invokes `pkgtEmptyManifest` and returns `PkgManifest`. |
| `parsePkgManifest` | `(json_text: string): PkgManifest` | Parses input via `parsePkgManifest` and returns `PkgManifest`. |
| `pkgtIsValidSegmentChar` | `(c: u8): bool` | Invokes `pkgtIsValidSegmentChar` and returns `bool`. |
| `pkgtIsValidSegment` | `(seg: string): bool` | Invokes `pkgtIsValidSegment` and returns `bool`. |
| `isValidPkgName` | `(name: string): bool` | Checks predicate `isValidPkgName` and returns `bool`. |
| `splitPkgScope` | `(name: string): PkgNameParts` | Invokes `splitPkgScope` and returns `PkgNameParts`. |

## Minimal Example

```cp
import { parsePkgManifest } from "pkg-types";

pub fn main() {
    let result = parsePkgManifest("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
