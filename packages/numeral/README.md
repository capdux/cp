[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# numeral

Zero-allocation human-readable number formatting (grouped thousands, EN/ZH compact suffixes, percentages, strict Roman numerals, and English words) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:numeral` ([https://github.com/adamwdraper/Numeral-js](https://github.com/adamwdraper/Numeral-js)) / `npm:filesize` ([https://github.com/avoidwork/filesize.js](https://github.com/avoidwork/filesize.js))（原库名 `numeral` → CP 包名 `numeral`）

| Metadata | Value |
| --- | --- |
| Package | `numeral` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add numeral
```

```cp
import {
    appendByte,
    absU64,
    formatGrouped,
    writeCompactScaled,
    formatCompactEn,
    formatCompactZh,
    formatPercent,
    toRoman,
} from "numeral";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | Invokes `appendByte` and returns `usize`. |
| `absU64` | `(n: i64): u64` | Invokes `absU64` and returns `u64`. |
| `formatGrouped` | `(n: i64, sep: u8, out: []u8): string` | Serializes/formats data via `formatGrouped` returning `string`. |
| `writeCompactScaled` | `(abs: u64, unit: u64, suffix: string, next_threshold: u64, next_suffix: string, negative: bool, out: []u8): string` | Invokes `writeCompactScaled` and returns `string`. |
| `formatCompactEn` | `(n: i64, out: []u8): string` | Serializes/formats data via `formatCompactEn` returning `string`. |
| `formatCompactZh` | `(n: i64, out: []u8): string` | Serializes/formats data via `formatCompactZh` returning `string`. |
| `formatPercent` | `(ratio: f64, decimals: u8, out: []u8): string` | Serializes/formats data via `formatPercent` returning `string`. |
| `toRoman` | `(n: u16, out: []u8): string` | Invokes `toRoman` and returns `string`. |
| `romanCharVal` | `(c: u8): u16` | Invokes `romanCharVal` and returns `u16`. |
| `fromRoman` | `(s: string): u16?` | Invokes `fromRoman` and returns `u16?`. |
| `onesWord` | `(n: u64): string` | Invokes `onesWord` and returns `string`. |
| `tensWord` | `(n: u64): string` | Invokes `tensWord` and returns `string`. |
| `appendUnder1000` | `(v: u64, out: []u8, start_pos: usize): usize` | Invokes `appendUnder1000` and returns `usize`. |
| `appendScaleGroup` | `(chunk: u64, label: string, out: []u8, start_pos: usize, has_prev: bool): usize` | Invokes `appendScaleGroup` and returns `usize`. |
| `toEnglishWords` | `(n: i64, out: []u8): string` | Invokes `toEnglishWords` and returns `string`. |

## Minimal Example

```cp
import { fromRoman } from "numeral";

pub fn main() {
    let result = fromRoman("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
