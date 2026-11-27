[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# version-compare

Zero-heap flexible version string comparison (multi-segment, v-prefix, prerelease ordering, operator matching) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:version-compare` ([https://github.com/timvisee/version-compare](https://github.com/timvisee/version-compare))

| Metadata | Value |
| --- | --- |
| Package | `version-compare` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add version-compare
```

```cp
import {
    VersionCmp,
    VcSeg,
    VcParsed,
    vcIsSep,
    vcTagRank,
    vcCmpLexLower,
    vcParse,
    compareVersions,
} from "version-compare";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `VersionCmp` | `Lt, Eq, Gt, Invalid` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `VcSeg` | `class` | Fields: `isNum: bool, numVal: u64, start: usize, len: usize` |
| `VcParsed` | `class` | Fields: `segs: [32]VcSeg, count: usize, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `vcIsSep` | `(b: u8): bool` | Invokes `vcIsSep` and returns `bool`. |
| `vcTagRank` | `(s: string, start: usize, len: usize): i32` | Invokes `vcTagRank` and returns `i32`. |
| `vcCmpLexLower` | `(a: string, aStart: usize, aLen: usize, b: string, bStart: usize, bLen: usize): i32` | Invokes `vcCmpLexLower` and returns `i32`. |
| `vcParse` | `(raw: string): VcParsed` | Invokes `vcParse` and returns `VcParsed`. |
| `compareVersions` | `(a: string, b: string): VersionCmp` | Invokes `compareVersions` and returns `VersionCmp`. |
| `isLess` | `(a: string, b: string): bool` | Checks predicate `isLess` and returns `bool`. |
| `isEqual` | `(a: string, b: string): bool` | Checks predicate `isEqual` and returns `bool`. |
| `isGreater` | `(a: string, b: string): bool` | Checks predicate `isGreater` and returns `bool`. |
| `satisfies` | `(ver: string, op: string, target: string): bool` | Invokes `satisfies` and returns `bool`. |

## Minimal Example

```cp
import { isLess } from "version-compare";

pub fn main() {
    let result = isLess("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
