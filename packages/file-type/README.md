[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# file-type

Zero-allocation binary file magic bytes signature sniffer (25+ image, archive, document, executable, audio, video, and database formats) for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:file-type` ([https://github.com/sindresorhus/file-type](https://github.com/sindresorhus/file-type)) / `crates.io:infer` ([https://github.com/bojand/infer](https://github.com/bojand/infer))（原库名 `file-type` → CP 包名 `file-type`）

| Metadata | Value |
| --- | --- |
| Package | `file-type` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add file-type
```

```cp
import {
    FileType,
    mbMatch,
    mbEqAt,
    detect,
    isImage,
    isArchive,
    isExecutable,
} from "file-type";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `FileType` | `class` | Fields: `ext: string, mime: string, kind: u8, matched: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `mbMatch` | `(ext: string, mime: string, kind: u8): FileType` | Invokes `mbMatch` and returns `FileType`. |
| `mbEqAt` | `(data: &[]u8, offset: usize, lit: &[]u8): bool` | Invokes `mbEqAt` and returns `bool`. |
| `detect` | `(data: &[]u8): FileType` | Invokes `detect` and returns `FileType`. |
| `isImage` | `(data: &[]u8): bool` | Checks predicate `isImage` and returns `bool`. |
| `isArchive` | `(data: &[]u8): bool` | Checks predicate `isArchive` and returns `bool`. |
| `isExecutable` | `(data: &[]u8): bool` | Checks predicate `isExecutable` and returns `bool`. |

## Minimal Example

```cp
import { mbMatch } from "file-type";

pub fn main() {
    let result = mbMatch("example", "example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
