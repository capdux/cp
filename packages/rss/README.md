[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# rss

Zero-allocation RSS 2.0 XML feed generator and CDATA-aware tag extractor in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rss` ([https://github.com/rust-syndication/rss](https://github.com/rust-syndication/rss))

| Metadata | Value |
| --- | --- |
| Package | `rss` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add rss
```

```cp
import {
    RssChannel,
    RssItem,
    RssWriter,
    pushSlice,
    pushXmlEscaped,
    pushTag,
    extractTagValue,
    countItems,
} from "rss";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RssChannel` | `class` | Fields: `title: &[]u8, link: &[]u8, description: &[]u8, language: &[]u8` |
| `RssItem` | `class` | Fields: `title: &[]u8, link: &[]u8, description: &[]u8, guid: &[]u8, pub_date: &[]u8` |
| `RssWriter` | `class` | Fields: `buf: []u8, pos: usize, item_count: usize, failed: bool, finished: bool` · Methods: `init, addItem, finish, itemCount, written` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pushSlice` | `(dst: []u8, pos: usize, src: &[]u8): i64` | Mutates or appends state via `pushSlice`. |
| `pushXmlEscaped` | `(dst: []u8, pos: usize, src: &[]u8): i64` | Mutates or appends state via `pushXmlEscaped`. |
| `pushTag` | `(dst: []u8, pos: usize, tag: &[]u8, val: &[]u8): i64` | Mutates or appends state via `pushTag`. |
| `extractTagValue` | `(xml: &[]u8, tag: &[]u8, start_pos: usize): &[]u8` | Invokes `extractTagValue` and returns `&[]u8`. |
| `countItems` | `(xml: &[]u8): usize` | Invokes `countItems` and returns `usize`. |

## Minimal Example

```cp
import { pushSlice } from "rss";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = pushSlice(out_buf[0..256], 0, buf_1[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
