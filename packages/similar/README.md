[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# similar

Zero-allocation line-level LCS diff, unified diff formatter, and similarity ratio engine for CP (clean-room MIT, inspired by Rust similar v2).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:similar` ([https://github.com/mitsuhiko/similar](https://github.com/mitsuhiko/similar))（原库名 `similar` → CP 包名 `similar`）

| Metadata | Value |
| --- | --- |
| Package | `similar` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add similar
```

```cp
import {
    DiffOp,
    DiffStats,
    SmdLine,
    smdSplitLines,
    smdLineEq,
    smdEmitOp,
    smdWriteBytes,
    smdWriteStr,
} from "similar";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `DiffOp` | `class` | Fields: `tag: u8, old_idx: usize, new_idx: usize` |
| `DiffStats` | `class` | Fields: `equal_lines: usize, deleted_lines: usize, inserted_lines: usize, total_ops: usize` |
| `SmdLine` | `class` | Fields: `start: usize, end: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `smdSplitLines` | `(text: string, out_lines: []SmdLine): usize` | Invokes `smdSplitLines` and returns `usize`. |
| `smdLineEq` | `(a: string, la: SmdLine, b: string, lb: SmdLine): bool` | Invokes `smdLineEq` and returns `bool`. |
| `smdEmitOp` | `(ops_out: []DiffOp, stats: &mut DiffStats, tag: u8, old_idx: usize, new_idx: usize)` | Executes `smdEmitOp` in-place. |
| `smdWriteBytes` | `(out: []u8, pos: usize, src: string, start: usize, end: usize): usize` | Invokes `smdWriteBytes` and returns `usize`. |
| `smdWriteStr` | `(out: []u8, pos: usize, s: string): usize` | Invokes `smdWriteStr` and returns `usize`. |
| `smdWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `smdWriteByte` and returns `usize`. |
| `smdWriteUsize` | `(out: []u8, pos: usize, val: usize): usize` | Invokes `smdWriteUsize` and returns `usize`. |
| `diffLines` | `(old_text: string, new_text: string, ops_out: []DiffOp): DiffStats` | Invokes `diffLines` and returns `DiffStats`. |
| `hasChanges` | `(old_text: string, new_text: string): bool` | Checks predicate `hasChanges` and returns `bool`. |
| `similarityRatio` | `(old_text: string, new_text: string): u32` | Invokes `similarityRatio` and returns `u32`. |
| `formatUnifiedDiff` | `(old_name: string, new_name: string, old_text: string, new_text: string, out: []u8): usize` | Serializes/formats data via `formatUnifiedDiff` returning `usize`. |

## Minimal Example

```cp
import { formatUnifiedDiff } from "similar";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatUnifiedDiff("example", "example", "example", "example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
