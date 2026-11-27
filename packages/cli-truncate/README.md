[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# cli-truncate

Zero-allocation ANSI-aware CLI string truncator (end/start/middle, CJK double-width, space boundary preference) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cli-truncate` ([https://github.com/sindresorhus/cli-truncate](https://github.com/sindresorhus/cli-truncate))

| Metadata | Value |
| --- | --- |
| Package | `cli-truncate` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add cli-truncate
```

```cp
import {
    TruncatePosition,
    TruncateOptions,
    ctWriteBytes,
    ctSkipAnsi,
    ctUtf8Len,
    ctFindLastSpaceLe,
    ctIsSpaceAtCol,
    ctFindFirstSpaceGe,
} from "cli-truncate";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `TruncatePosition` | `END, START, MIDDLE` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `TruncateOptions` | `class` | Fields: `position: TruncatePosition, prefer_truncation_on_space: bool, ellipsis: string` · Methods: `defaults` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ctWriteBytes` | `(out: []u8, pos: usize, src: string): usize` | Invokes `ctWriteBytes` and returns `usize`. |
| `ctSkipAnsi` | `(text: string, pos: usize): usize` | Invokes `ctSkipAnsi` and returns `usize`. |
| `ctUtf8Len` | `(text: string, pos: usize): usize` | Invokes `ctUtf8Len` and returns `usize`. |
| `ctFindLastSpaceLe` | `(text: string, max_col: usize): usize` | Invokes `ctFindLastSpaceLe` and returns `usize`. |
| `ctIsSpaceAtCol` | `(text: string, target_col: usize): bool` | Invokes `ctIsSpaceAtCol` and returns `bool`. |
| `ctFindFirstSpaceGe` | `(text: string, min_col: usize, limit_col: usize): usize` | Invokes `ctFindFirstSpaceGe` and returns `usize`. |
| `cliTruncateInto` | `(text: string, columns: usize, opts: TruncateOptions, out: []u8): usize` | Invokes `cliTruncateInto` and returns `usize`. |
| `cliTruncateEndInto` | `(text: string, columns: usize, out: []u8): usize` | Invokes `cliTruncateEndInto` and returns `usize`. |
| `cliTruncateStartInto` | `(text: string, columns: usize, out: []u8): usize` | Invokes `cliTruncateStartInto` and returns `usize`. |
| `cliTruncateMiddleInto` | `(text: string, columns: usize, out: []u8): usize` | Invokes `cliTruncateMiddleInto` and returns `usize`. |

## Minimal Example

```cp
import { ctWriteBytes } from "cli-truncate";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = ctWriteBytes(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
