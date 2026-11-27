[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# cli-spinners

Zero-allocation terminal spinner frame presets and Unicode/ASCII status symbols (cli-spinners / ora / log-symbols parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cli-spinners` ([https://github.com/sindresorhus/cli-spinners](https://github.com/sindresorhus/cli-spinners))

| Metadata | Value |
| --- | --- |
| Package | `cli-spinners` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add cli-spinners
```

```cp
import {
    SpinnerKind,
    SymbolKind,
    intervalMs,
    frameCount,
    frameAt,
    frameAtTime,
    findSpinnerByName,
    logSymbol,
} from "cli-spinners";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `SpinnerKind` | `Dots, Dots2, Line, Pipe, Arc, Circle, SquareCorners, Bounce ...` |
| `enum` | `SymbolKind` | `Success, Error, Warning, Info` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `intervalMs` | `(kind: SpinnerKind): u32` | Invokes `intervalMs` and returns `u32`. |
| `frameCount` | `(kind: SpinnerKind): usize` | Invokes `frameCount` and returns `usize`. |
| `frameAt` | `(kind: SpinnerKind, tick: usize): string` | Invokes `frameAt` and returns `string`. |
| `frameAtTime` | `(kind: SpinnerKind, elapsed_ms: u64): string` | Invokes `frameAtTime` and returns `string`. |
| `findSpinnerByName` | `(name: string): SpinnerKind?` | Queries/retrieves data via `findSpinnerByName` returning `SpinnerKind?`. |
| `logSymbol` | `(kind: SymbolKind, ascii_only: bool): string` | Invokes `logSymbol` and returns `string`. |
| `renderSpinnerLine` | `(kind: SpinnerKind, tick: usize, text: string, out: []u8): string` | Invokes `renderSpinnerLine` and returns `string`. |
| `renderStatusLine` | `(sym: SymbolKind, ascii_only: bool, text: string, out: []u8): string` | Invokes `renderStatusLine` and returns `string`. |

## Minimal Example

```cp
import { findSpinnerByName } from "cli-spinners";

pub fn main() {
    let result = findSpinnerByName("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
