[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ora

Zero-heap CLI terminal spinner animation and status line renderer (clean-room port of ora v8)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ora` ([https://github.com/sindresorhus/ora](https://github.com/sindresorhus/ora))（原库名 `ora` → CP 包名 `ora`）

| Metadata | Value |
| --- | --- |
| Package | `ora` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ora
```

```cp
import {
    Spinner,
    oraWriteSlice,
    oraFrameCount,
    oraFrameAt,
    oraRenderStatus,
} from "ora";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Spinner` | `class` | Fields: `style: u8, text: string, frame_index: usize` · Methods: `new, frameCount, currentFrame, tick, setText, renderFrame, +6 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `oraWriteSlice` | `(out: []u8, pos: usize, s: string): usize` | Invokes `oraWriteSlice` and returns `usize`. |
| `oraFrameCount` | `(style: u8): usize` | Invokes `oraFrameCount` and returns `usize`. |
| `oraFrameAt` | `(style: u8, index: usize): string` | Invokes `oraFrameAt` and returns `string`. |
| `oraRenderStatus` | `(symbol: string, msg: string, fallback: string, out: []u8): usize` | Invokes `oraRenderStatus` and returns `usize`. |

## Minimal Example

```cp
import { oraWriteSlice } from "ora";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = oraWriteSlice(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
