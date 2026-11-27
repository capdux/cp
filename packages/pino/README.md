[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# pino

Zero-heap structured NDJSON logger with numeric levels (10..60), safe JSON escaping, and child fields (clean-room port of npm pino)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pino` ([https://github.com/pinojs/pino](https://github.com/pinojs/pino))（原库名 `pino` → CP 包名 `pino`）

| Metadata | Value |
| --- | --- |
| Package | `pino` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add pino
```

```cp
import {
    PinoField,
    PinoLogger,
    pinoPush,
    pinoPushJsonStr,
    pinoPushU64,
    pinoPushI64,
    levelLabel,
} from "pino";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PinoField` | `class` | Fields: `key: string, kind: u8, str_val: string, int_val: i64, bool_val: bool` |
| `PinoLogger` | `class` | Fields: `min_level: u8, name: string, pid: u32, hostname: string, bindings: [16]PinoField, binding_count: usize` · Methods: `new, isLevelEnabled, bindStr, bindInt, bindBool, formatLine` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pinoPush` | `(out: []u8, pos: usize, s: string): usize` | Invokes `pinoPush` and returns `usize`. |
| `pinoPushJsonStr` | `(out: []u8, pos: usize, s: string): usize` | Invokes `pinoPushJsonStr` and returns `usize`. |
| `pinoPushU64` | `(out: []u8, pos: usize, v: u64): usize` | Invokes `pinoPushU64` and returns `usize`. |
| `pinoPushI64` | `(out: []u8, pos: usize, v: i64): usize` | Invokes `pinoPushI64` and returns `usize`. |
| `levelLabel` | `(level: u8): string` | Invokes `levelLabel` and returns `string`. |

## Minimal Example

```cp
import { pinoPush } from "pino";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = pinoPush(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
