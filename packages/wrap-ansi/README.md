[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# wrap-ansi

Zero-allocation ANSI-preserving word/hard wrapper and visible-column slicer with CJK double-width awareness (wrap-ansi / slice-ansi parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:wrap-ansi` ([https://github.com/chalk/wrap-ansi](https://github.com/chalk/wrap-ansi))

| Metadata | Value |
| --- | --- |
| Package | `wrap-ansi` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add wrap-ansi
```

```cp
import {
    Utf8Step,
    AnsiStyleState,
    decodeUtf8At,
    codepointWidth,
    scanAnsiCsi,
    isResetSgr,
    startsWithZeroReset,
    pushByte,
} from "wrap-ansi";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Utf8Step` | `class` | Fields: `cp: u32, width: usize` |
| `AnsiStyleState` | `class` | Fields: `seqs: [MAX_ACTIVE_SGR]string, count: usize` · Methods: `empty, hasActive, applySeq, emitActive` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `decodeUtf8At` | `(s: string, i: usize): Utf8Step` | Parses input via `decodeUtf8At` and returns `Utf8Step`. |
| `codepointWidth` | `(cp: u32): usize` | Invokes `codepointWidth` and returns `usize`. |
| `scanAnsiCsi` | `(s: string, i: usize): usize` | Invokes `scanAnsiCsi` and returns `usize`. |
| `isResetSgr` | `(seq: string): bool` | Checks predicate `isResetSgr` and returns `bool`. |
| `startsWithZeroReset` | `(seq: string): bool` | Invokes `startsWithZeroReset` and returns `bool`. |
| `pushByte` | `(out: []u8, pos: usize, b: u8): usize` | Mutates or appends state via `pushByte`. |
| `pushStr` | `(out: []u8, pos: usize, s: string): usize` | Mutates or appends state via `pushStr`. |
| `visibleWidth` | `(s: string): usize` | Invokes `visibleWidth` and returns `usize`. |
| `emitLineBreak` | `(out: []u8, pos: usize, style: AnsiStyleState): usize` | Invokes `emitLineBreak` and returns `usize`. |
| `wrapAnsi` | `(text: string, max_cols: usize, hard_wrap: bool, out: []u8): string` | Invokes `wrapAnsi` and returns `string`. |
| `sliceAnsi` | `(text: string, start_col: usize, end_col: usize, out: []u8): string` | Invokes `sliceAnsi` and returns `string`. |

## Minimal Example

```cp
import { decodeUtf8At } from "wrap-ansi";

pub fn main() {
    let result = decodeUtf8At("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
