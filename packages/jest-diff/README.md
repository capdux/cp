[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# jest-diff

Zero-heap pure CP Jest-style unified diff engine with ANSI coloring (clean-room MIT implementation of npm jest-diff)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:jest-diff` ([https://github.com/jestjs/jest](https://github.com/jestjs/jest))

| Metadata | Value |
| --- | --- |
| Package | `jest-diff` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add jest-diff
```

```cp
import {
    JdfLine,
    DiffOptions,
    DiffSummary,
    jdfSplitLines,
    jdfWriteBytes,
    jdfWriteStr,
    jdfWriteByte,
    jdfWriteUsize,
} from "jest-diff";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `JdfLine` | `class` | Fields: `start: usize, end: usize` |
| `DiffOptions` | `class` | Fields: `a_annotation: string, b_annotation: string, context_lines: usize, expand: bool, include_change_counts: bool, use_color: bool` · Methods: `defaults, colored` |
| `DiffSummary` | `class` | Fields: `insertions: usize, deletions: usize, equal: usize, has_diff: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `jdfSplitLines` | `(text: string, out_lines: []JdfLine): usize` | Invokes `jdfSplitLines` and returns `usize`. |
| `jdfWriteBytes` | `(out: []u8, pos: usize, src: string, start: usize, end: usize): usize` | Invokes `jdfWriteBytes` and returns `usize`. |
| `jdfWriteStr` | `(out: []u8, pos: usize, s: string): usize` | Invokes `jdfWriteStr` and returns `usize`. |
| `jdfWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `jdfWriteByte` and returns `usize`. |
| `jdfWriteUsize` | `(out: []u8, pos: usize, val: usize): usize` | Invokes `jdfWriteUsize` and returns `usize`. |
| `jdfEmitStyledLine` | `(fmt: PicoFormatter, color_kind: u8, line_text: string, out: []u8, pos: usize): usize` | Invokes `jdfEmitStyledLine` and returns `usize`. |
| `diffStats` | `(expected: string, received: string): DiffSummary` | Invokes `diffStats` and returns `DiffSummary`. |
| `diffLinesUnified` | `(expected: string, received: string, opts: DiffOptions, out: []u8): usize` | Invokes `diffLinesUnified` and returns `usize`. |

## Minimal Example

```cp
import { jdfSplitLines } from "jest-diff";

pub fn main() {
    let result = jdfSplitLines("example", /* out_lines: []JdfLine */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
