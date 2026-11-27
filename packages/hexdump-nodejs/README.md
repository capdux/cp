[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# hexdump-nodejs

Zero-allocation canonical hexdump -C 16-byte-per-line binary debugger, spaced hex encoder, and first-diff locator in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:hexdump-nodejs` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:pretty-hex` ([https://github.com/wolandr/pretty-hex](https://github.com/wolandr/pretty-hex))（原库名 `hexdump-nodejs` → CP 包名 `hexdump-nodejs`）

| Metadata | Value |
| --- | --- |
| Package | `hexdump-nodejs` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add hexdump-nodejs
```

```cp
import {
    buildPrintableLut,
    formatLine,
    formatDump,
    encodeHexSpaced,
    firstDiffOffset,
} from "hexdump-nodejs";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `buildPrintableLut` | `(): [256]u8` | Invokes `buildPrintableLut` and returns `[256]u8`. |
| `formatLine` | `(data: &[]u8, base_offset: usize, out: []u8): usize` | Serializes/formats data via `formatLine` returning `usize`. |
| `formatDump` | `(data: &[]u8, out: []u8): usize` | Serializes/formats data via `formatDump` returning `usize`. |
| `encodeHexSpaced` | `(data: &[]u8, uppercase: bool, out: []u8): usize` | Serializes/formats data via `encodeHexSpaced` returning `usize`. |
| `firstDiffOffset` | `(a: &[]u8, b: &[]u8): i64` | Invokes `firstDiffOffset` and returns `i64`. |

## Minimal Example

```cp
import { formatLine } from "hexdump-nodejs";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = formatLine(out_buf[0..256], 0, buf_1[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
