[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# braces

Zero-allocation Bash/Glob brace expansion and sequence range generator (braces + fill-range parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:braces` ([https://github.com/micromatch/braces](https://github.com/micromatch/braces))

| Metadata | Value |
| --- | --- |
| Package | `braces` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add braces
```

```cp
import {
    BraceResult,
    BrExpander,
    BrInt,
    BrRangeSpec,
    BrSpan,
    brInitExpander,
    brToResult,
    brPushItem,
} from "braces";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `BraceResult` | `class` | Fields: `buf: []u8, starts: [BR_MAX_ITEMS]usize, lens: [BR_MAX_ITEMS]usize, count: usize, truncated: bool` · Methods: `itemAt` |
| `BrExpander` | `class` | Fields: `out: []u8, pos: usize, starts: [BR_MAX_ITEMS]usize, lens: [BR_MAX_ITEMS]usize, count: usize, truncated: bool` |
| `BrInt` | `class` | Fields: `ok: bool, val: i64` |
| `BrRangeSpec` | `class` | Fields: `ok: bool, startStr: string, endStr: string, step: i32` |
| `BrSpan` | `class` | Fields: `found: bool, openIdx: usize, closeIdx: usize, isCommaSet: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `brInitExpander` | `(out: []u8): BrExpander` | Invokes `brInitExpander` and returns `BrExpander`. |
| `brToResult` | `(e: &mut BrExpander): BraceResult` | Invokes `brToResult` and returns `BraceResult`. |
| `brPushItem` | `(e: &mut BrExpander, s: string)` | Executes `brPushItem` in-place. |
| `brIsDigit` | `(c: u8): bool` | Invokes `brIsDigit` and returns `bool`. |
| `brIsUpper` | `(c: u8): bool` | Invokes `brIsUpper` and returns `bool`. |
| `brIsLower` | `(c: u8): bool` | Invokes `brIsLower` and returns `bool`. |
| `brParseInt` | `(s: string): BrInt` | Invokes `brParseInt` and returns `BrInt`. |
| `brFormatSimpleInt` | `(v: i64, buf: &mut [32]u8): string` | Invokes `brFormatSimpleInt` and returns `string`. |
| `brParseRangeSpec` | `(body: string): BrRangeSpec` | Invokes `brParseRangeSpec` and returns `BrRangeSpec`. |
| `brHasTopLevelComma` | `(body: string): bool` | Invokes `brHasTopLevelComma` and returns `bool`. |
| `brFindExpandableBrace` | `(pat: string): BrSpan` | Invokes `brFindExpandableBrace` and returns `BrSpan`. |
| `brBuildCombined` | `(prefix: string, mid: string, suffix: string, buf: &mut [BR_MAX_WORK_BUF]u8): usize` | Invokes `brBuildCombined` and returns `usize`. |
| `brEmitToken` | `(e: &mut BrExpander, prefix: string, tok: string, suffix: string, depth: usize)` | Executes `brEmitToken` in-place. |
| `brExpandRangeWithContext` | `(e: &mut BrExpander, prefix: string, startStr: string, endStr: string, stepIn: i32, suffix: string, depth: usize)` | Executes `brExpandRangeWithContext` in-place. |
| `brExpandRec` | `(e: &mut BrExpander, pat: string, depth: usize)` | Executes `brExpandRec` in-place. |
| `expandBraces` | `(pattern: string, out: []u8): BraceResult` | Invokes `expandBraces` and returns `BraceResult`. |
| `fillRange` | `(start: string, end: string, step: i32, out: []u8): BraceResult` | Invokes `fillRange` and returns `BraceResult`. |

## Minimal Example

```cp
import { brParseInt } from "braces";

pub fn main() {
    let result = brParseInt("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
