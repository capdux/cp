[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# parse-svg-path

Zero-allocation SVG path `d` attribute parser, bounding box calculator, and canonical formatter for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:parse-svg-path` ([https://github.com/jkroso/parse-svg-path](https://github.com/jkroso/parse-svg-path))（原库名 `parse-svg-path` → CP 包名 `parse-svg-path`）

| Metadata | Value |
| --- | --- |
| Package | `parse-svg-path` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add parse-svg-path
```

```cp
import {
    PathCmd,
    ParsedPath,
    PathBounds,
    ParsedNum,
    BoundsAcc,
    skipWs,
    skipWsComma,
    expectedArgCount,
} from "parse-svg-path";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PathCmd` | `class` | Fields: `code: u8, relative: bool, args: [7]f64, arg_count: u8` · Methods: `empty` |
| `ParsedPath` | `class` | Fields: `cmds: [64]PathCmd, count: usize, valid: bool` · Methods: `invalid, cmdAt` |
| `PathBounds` | `class` | Fields: `valid: bool, min_x: f64, min_y: f64, max_x: f64, max_y: f64, width: f64, +1 more` · Methods: `empty` |
| `ParsedNum` | `class` | Fields: `ok: bool, val: f64, next_pos: usize` |
| `BoundsAcc` | `class` | Fields: `has_point: bool, min_x: f64, min_y: f64, max_x: f64, max_y: f64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `skipWs` | `(s: string, start: usize): usize` | Invokes `skipWs` and returns `usize`. |
| `skipWsComma` | `(s: string, start: usize): usize` | Invokes `skipWsComma` and returns `usize`. |
| `expectedArgCount` | `(code: u8): u8?` | Invokes `expectedArgCount` and returns `u8?`. |
| `isLowerCmd` | `(code: u8): bool` | Checks predicate `isLowerCmd` and returns `bool`. |
| `isNumberStart` | `(c: u8): bool` | Checks predicate `isNumberStart` and returns `bool`. |
| `pow10` | `(exp: i32): f64` | Invokes `pow10` and returns `f64`. |
| `parseSvgNumber` | `(s: string, start: usize): ParsedNum` | Parses input via `parseSvgNumber` and returns `ParsedNum`. |
| `parseSvgFlag` | `(s: string, start: usize): ParsedNum` | Parses input via `parseSvgFlag` and returns `ParsedNum`. |
| `parseSvgPath` | `(d: string): ParsedPath` | Parses input via `parseSvgPath` and returns `ParsedPath`. |
| `includePoint` | `(acc: BoundsAcc, x: f64, y: f64): BoundsAcc` | Invokes `includePoint` and returns `BoundsAcc`. |
| `computeBounds` | `(p: ParsedPath): PathBounds` | Invokes `computeBounds` and returns `PathBounds`. |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | Invokes `appendByte` and returns `usize`. |
| `appendFloatCompact` | `(out: []u8, start_pos: usize, val: f64): usize` | Invokes `appendFloatCompact` and returns `usize`. |
| `formatSvgPath` | `(p: ParsedPath, out: []u8): string` | Serializes/formats data via `formatSvgPath` returning `string`. |

## Minimal Example

```cp
import { parseSvgNumber } from "parse-svg-path";

pub fn main() {
    let result = parseSvgNumber("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
