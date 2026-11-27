[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# humantime

Zero-heap human-readable duration parser/formatter and RFC 3339 timestamp codec (clean-room CP implementation of crates.io humantime)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:humantime` ([https://github.com/tailhook/humantime](https://github.com/tailhook/humantime))

| Metadata | Value |
| --- | --- |
| Package | `humantime` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add humantime
```

```cp
import {
    DurationParseResult,
    htIsSpace,
    htIsDigit,
    htIsAlpha,
    htEq,
    htUnitNanos,
    parseDuration,
    htPushStr,
} from "humantime";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `DurationParseResult` | `class` | Fields: `ok: bool, nanos: u64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `htIsSpace` | `(c: u8): bool` | Invokes `htIsSpace` and returns `bool`. |
| `htIsDigit` | `(c: u8): bool` | Invokes `htIsDigit` and returns `bool`. |
| `htIsAlpha` | `(c: u8): bool` | Invokes `htIsAlpha` and returns `bool`. |
| `htEq` | `(s: string, start: usize, end: usize, lit: string): bool` | Invokes `htEq` and returns `bool`. |
| `htUnitNanos` | `(s: string, start: usize, end: usize): u64` | Invokes `htUnitNanos` and returns `u64`. |
| `parseDuration` | `(input: string): DurationParseResult` | Parses input via `parseDuration` and returns `DurationParseResult`. |
| `htPushStr` | `(out: []u8, pos: usize, s: string): usize` | Invokes `htPushStr` and returns `usize`. |
| `htPushPart` | `(out: []u8, pos: usize, val: u64, suffix: string): usize` | Invokes `htPushPart` and returns `usize`. |
| `formatDuration` | `(nanos: u64, out: []u8): usize` | Serializes/formats data via `formatDuration` returning `usize`. |
| `htPush2Digits` | `(out: []u8, pos: usize, v: u64): usize` | Invokes `htPush2Digits` and returns `usize`. |
| `htPush4Digits` | `(out: []u8, pos: usize, v: u64): usize` | Invokes `htPush4Digits` and returns `usize`. |
| `formatRfc3339Seconds` | `(unix_secs: u64, out: []u8): usize` | Serializes/formats data via `formatRfc3339Seconds` returning `usize`. |

## Minimal Example

```cp
import { parseDuration } from "humantime";

pub fn main() {
    let result = parseDuration("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
