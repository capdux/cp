[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# indicatif

Zero-heap terminal progress bar, throughput, and ETA renderer composing pretty-bytes and pretty-ms in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:indicatif` ([https://github.com/console-rs/indicatif](https://github.com/console-rs/indicatif))

| Metadata | Value |
| --- | --- |
| Package | `indicatif` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add indicatif
```

```cp
import {
    ProgressState,
    indWriteByte,
    indWriteStr,
    indWriteU64,
    percent,
    etaMs,
    bytesPerSec,
    renderBar,
} from "indicatif";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ProgressState` | `class` | Fields: `pos: u64, total: u64, elapsed_ms: u64, width: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `indWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `indWriteByte` and returns `usize`. |
| `indWriteStr` | `(out: []u8, pos: usize, s: &[]u8): usize` | Invokes `indWriteStr` and returns `usize`. |
| `indWriteU64` | `(out: []u8, pos: usize, val: u64): usize` | Invokes `indWriteU64` and returns `usize`. |
| `percent` | `(s: ProgressState): f64` | Invokes `percent` and returns `f64`. |
| `etaMs` | `(s: ProgressState): u64` | Invokes `etaMs` and returns `u64`. |
| `bytesPerSec` | `(s: ProgressState): f64` | Invokes `bytesPerSec` and returns `f64`. |
| `renderBar` | `(s: ProgressState, fill_ch: u8, head_ch: u8, empty_ch: u8, out: []u8): usize` | Invokes `renderBar` and returns `usize`. |
| `renderLine` | `(s: ProgressState, prefix: string, out: []u8): usize` | Invokes `renderLine` and returns `usize`. |

## Minimal Example

```cp
import { renderLine } from "indicatif";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = renderLine(/* s: ProgressState */, "example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
