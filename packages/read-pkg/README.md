[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# read-pkg

Zero-allocation package.json / cap.json parser and normalizer for CP, reusing normalize-package-data.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:read-pkg` ([https://github.com/sindresorhus/read-pkg](https://github.com/sindresorhus/read-pkg))

| Metadata | Value |
| --- | --- |
| Package | `read-pkg` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add read-pkg
```

```cp
import {
    ReadPkgResult,
    rpkgIsSpace,
    rpkgStrEq,
    rpkgStartsWithAt,
    rpkgIsValueBoundary,
    rpkgSkipWs,
    rpkgFindStringEnd,
    rpkgSkipValue,
} from "read-pkg";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ReadPkgResult` | `class` | Fields: `meta: PackageMetadata, normalized: NormalizedPackageResult, parsed: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `rpkgIsSpace` | `(ch: u8): bool` | Invokes `rpkgIsSpace` and returns `bool`. |
| `rpkgStrEq` | `(a: string, b: string): bool` | Invokes `rpkgStrEq` and returns `bool`. |
| `rpkgStartsWithAt` | `(s: string, pos: usize, kw: string): bool` | Invokes `rpkgStartsWithAt` and returns `bool`. |
| `rpkgIsValueBoundary` | `(s: string, pos: usize): bool` | Invokes `rpkgIsValueBoundary` and returns `bool`. |
| `rpkgSkipWs` | `(s: string, pos: usize): usize` | Invokes `rpkgSkipWs` and returns `usize`. |
| `rpkgFindStringEnd` | `(s: string, open_quote: usize): usize?` | Invokes `rpkgFindStringEnd` and returns `usize?`. |
| `rpkgSkipValue` | `(s: string, pos: usize): usize?` | Invokes `rpkgSkipValue` and returns `usize?`. |
| `rpkgValidateTopObject` | `(json_text: string): bool` | Invokes `rpkgValidateTopObject` and returns `bool`. |
| `rpkgFindTopLevelValue` | `(json_text: string, key: string): usize?` | Invokes `rpkgFindTopLevelValue` and returns `usize?`. |
| `extractJsonStringField` | `(json_text: string, key: string): string?` | Invokes `extractJsonStringField` and returns `string?`. |
| `extractJsonBoolField` | `(json_text: string, key: string): bool?` | Invokes `extractJsonBoolField` and returns `bool?`. |
| `rpkgExtractRepository` | `(json_text: string): string` | Invokes `rpkgExtractRepository` and returns `string`. |
| `parsePackageJson` | `(json_text: string, normalize: bool): ReadPkgResult` | Parses input via `parsePackageJson` and returns `ReadPkgResult`. |

## Minimal Example

```cp
import { parsePackageJson } from "read-pkg";

pub fn main() {
    let result = parsePackageJson("example", false);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
