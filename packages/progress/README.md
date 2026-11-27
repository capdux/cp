[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# progress

Zero-heap terminal progress bar renderer supporting :bar, :percent, :current, :total, :eta, and :rate tokens

| Metadata | Value |
| --- | --- |
| Package | `progress` |
| Version | `2.0.3` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add progress
```

```cp
import { ProgressBar, newProgressBar, prgWriteBar, renderProgress } from "progress";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ProgressBar` | `class` | Fields: `total: u64, current: u64, width: usize, complete_char: u8, incomplete_char: u8, head_char: u8, +1 more` · Methods: `ratio, percent, isComplete, tick, etaSeconds` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `newProgressBar` | `(total: u64, width: usize): ProgressBar` | Constructs/initializes an instance via `newProgressBar`. |
| `prgWriteBar` | `(bar: ProgressBar, out: []u8, pos: usize): usize` | Invokes `prgWriteBar` and returns `usize`. |
| `renderProgress` | `(bar: ProgressBar, template: string, out: []u8): usize` | Invokes `renderProgress` and returns `usize`. |

## Minimal Example

```cp
import { renderProgress } from "progress";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = renderProgress(/* bar: ProgressBar */, "example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
