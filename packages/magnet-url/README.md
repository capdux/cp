[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# magnet-url

Zero-heap BitTorrent Magnet URI (BEP-9 / BEP-53 btih & btmh) parser, tracker extractor, and builder for CP (compatible with npm magnet-uri & Rust magnet-url)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:magnet-url` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `magnet-url` → CP 包名 `magnet-url`）

| Metadata | Value |
| --- | --- |
| Package | `magnet-url` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add magnet-url
```

```cp
import {
    MagnetLink,
    TopicMatch,
    startsWithIgnoreCase,
    isHexStr,
    isBase32Str,
    parsePositiveI64,
    classifyExactTopic,
    parseMagnet,
} from "magnet-url";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `MagnetLink` | `class` | Fields: `hash_kind: u8, info_hash: string, display_name: string, exact_length: i64, trackers: [8]string, tracker_count: usize, +1 more` |
| `TopicMatch` | `class` | Fields: `kind: u8, hash: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `startsWithIgnoreCase` | `(s: string, prefix: string): bool` | Invokes `startsWithIgnoreCase` and returns `bool`. |
| `isHexStr` | `(s: string): bool` | Checks predicate `isHexStr` and returns `bool`. |
| `isBase32Str` | `(s: string): bool` | Checks predicate `isBase32Str` and returns `bool`. |
| `parsePositiveI64` | `(s: string): i64` | Parses input via `parsePositiveI64` and returns `i64`. |
| `classifyExactTopic` | `(xt: string): TopicMatch` | Invokes `classifyExactTopic` and returns `TopicMatch`. |
| `parseMagnet` | `(uri: string): MagnetLink` | Parses input via `parseMagnet` and returns `MagnetLink`. |
| `appendBytes` | `(dst: []u8, pos: usize, src: string): usize` | Invokes `appendBytes` and returns `usize`. |
| `formatMagnetV1` | `(info_hash: string, display_name: string, trackers: &[]string, out: []u8): usize` | Serializes/formats data via `formatMagnetV1` returning `usize`. |

## Minimal Example

```cp
import { isHexStr } from "magnet-url";

pub fn main() {
    let result = isHexStr("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
