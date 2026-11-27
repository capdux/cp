[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# logfmt

Zero-allocation logfmt structured key=value log parser and writer (Heroku/Grafana Loki/Kubernetes) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:logfmt` ([https://github.com/csquared/node-logfmt](https://github.com/csquared/node-logfmt))

| Metadata | Value |
| --- | --- |
| Package | `logfmt` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add logfmt
```

```cp
import { LogfmtField, LogfmtRecord, LogfmtWriter, needsQuotes } from "logfmt";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `LogfmtField` | `class` | Fields: `key_start: usize, key_len: usize, val_start: usize, val_len: usize` |
| `LogfmtRecord` | `class` | Fields: `key_buf: [512]u8, key_used: usize, val_buf: [1024]u8, val_used: usize, fields: [64]LogfmtField, count: usize` · Methods: `parse, len, has, get` |
| `LogfmtWriter` | `class` | Fields: `buf: []u8, pos: usize, count: usize` · Methods: `init, writeStr, writeInt, writeBool, written, len` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `needsQuotes` | `(val: &[]u8): bool` | Invokes `needsQuotes` and returns `bool`. |

## Minimal Example

```cp
import { needsQuotes } from "logfmt";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = needsQuotes(out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
