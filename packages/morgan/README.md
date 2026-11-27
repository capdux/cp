[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# morgan

Zero-heap HTTP access log template formatter supporting Morgan tiny/dev/short/common/combined presets and custom tokens (clean-room port of morgan v1.10)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:morgan` ([https://github.com/expressjs/morgan](https://github.com/expressjs/morgan))（原库名 `morgan` → CP 包名 `morgan`）

| Metadata | Value |
| --- | --- |
| Package | `morgan` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add morgan
```

```cp
import {
    HttpLogEvent,
    mrgIsTokenChar,
    mrgWriteByte,
    mrgWriteOrDash,
    mrgWriteResponseTime,
    formatMorganLog,
    formatPreset,
} from "morgan";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `HttpLogEvent` | `class` | Fields: `remote_addr: string, remote_user: string, date: string, method: string, url: string, http_version: string, +6 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `mrgIsTokenChar` | `(c: u8): bool` | Invokes `mrgIsTokenChar` and returns `bool`. |
| `mrgWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `mrgWriteByte` and returns `usize`. |
| `mrgWriteOrDash` | `(out: []u8, pos: usize, s: string): usize` | Invokes `mrgWriteOrDash` and returns `usize`. |
| `mrgWriteResponseTime` | `(out: []u8, pos: usize, us: u64): usize` | Invokes `mrgWriteResponseTime` and returns `usize`. |
| `formatMorganLog` | `(format_str: string, ev: HttpLogEvent, out: []u8): usize` | Serializes/formats data via `formatMorganLog` returning `usize`. |
| `formatPreset` | `(preset: string, ev: HttpLogEvent, out: []u8): usize` | Serializes/formats data via `formatPreset` returning `usize`. |

## Minimal Example

```cp
import { formatMorganLog } from "morgan";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatMorganLog("example", /* ev: HttpLogEvent */, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
