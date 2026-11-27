[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# csv-parse

Zero-heap RFC 4180 CSV/TSV record parser and serializer with quoted fields, escaped quotes, embedded newlines, custom delimiters, and spreadsheet formula guard.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:csv-parse` ([https://github.com/adaltas/node-csv](https://github.com/adaltas/node-csv))（原库名 `csv-parse` → CP 包名 `csv-parse`）

| Metadata | Value |
| --- | --- |
| Package | `csv-parse` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add csv-parse
```

```cp
import {
    CsvFieldSpan,
    CsvParseResult,
    parseRecord,
    formatField,
    formatFieldSafe,
    formatRecord,
} from "csv-parse";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CsvFieldSpan` | `class` | Fields: `start: usize, len: usize` |
| `CsvParseResult` | `class` | Fields: `field_count: usize, next_pos: usize, ok: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `parseRecord` | `(src: &[]u8, start_pos: usize, delim: u8, out_buf: []u8, out_spans: []CsvFieldSpan): CsvParseResult` | Parses input via `parseRecord` and returns `CsvParseResult`. |
| `formatField` | `(field: &[]u8, delim: u8, dst: []u8): i64` | Serializes/formats data via `formatField` returning `i64`. |
| `formatFieldSafe` | `(field: &[]u8, delim: u8, dst: []u8): i64` | Serializes/formats data via `formatFieldSafe` returning `i64`. |
| `formatRecord` | `(fields: &[]&[]u8, delim: u8, dst: []u8): i64` | Serializes/formats data via `formatRecord` returning `i64`. |

## Minimal Example

```cp
import { formatField } from "csv-parse";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = formatField(out_buf[0..256], 0, buf_1[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
