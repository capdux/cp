[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# textwrap

Zero-allocation text wrapping, ANSI-aware display width, CJK wide-char breaking, indent/dedent, and truncation engine for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:wrap-ansi` ([https://github.com/chalk/wrap-ansi](https://github.com/chalk/wrap-ansi))（原库名 `wrap-ansi` → CP 包名 `textwrap`）

| Metadata | Value |
| --- | --- |
| Package | `textwrap` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add textwrap
```

```cp
import {
    WrapOptions,
    DecodedChar,
    WrapCursor,
    appendByte,
    decodeUtf8,
    ansiSeqLen,
    isAnsiReset,
    codepointWidth,
} from "textwrap";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `WrapOptions` | `class` | Fields: `width: usize, break_words: bool, preserve_newlines: bool` · Methods: `defaults, withWidth` |
| `DecodedChar` | `class` | Fields: `cp: u32, len: usize` |
| `WrapCursor` | `class` | Fields: `pos: usize, col: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | Invokes `appendByte` and returns `usize`. |
| `decodeUtf8` | `(s: string, pos: usize): DecodedChar` | Parses input via `decodeUtf8` and returns `DecodedChar`. |
| `ansiSeqLen` | `(s: string, pos: usize): usize` | Invokes `ansiSeqLen` and returns `usize`. |
| `isAnsiReset` | `(s: string, pos: usize, seq_len: usize): bool` | Checks predicate `isAnsiReset` and returns `bool`. |
| `codepointWidth` | `(cp: u32): usize` | Invokes `codepointWidth` and returns `usize`. |
| `isSpaceByte` | `(c: u8): bool` | Checks predicate `isSpaceByte` and returns `bool`. |
| `displayWidth` | `(s: string): usize` | Invokes `displayWidth` and returns `usize`. |
| `stripAnsi` | `(s: string, out: []u8): string` | Invokes `stripAnsi` and returns `string`. |
| `writeHardBrokenToken` | `(text: string, tok_start: usize, tok_end: usize, width: usize, out: []u8, start_pos: usize, start_col: usize): WrapCursor` | Invokes `writeHardBrokenToken` and returns `WrapCursor`. |
| `wrapSegment` | `(text: string, seg_start: usize, seg_end: usize, opts: WrapOptions, out: []u8, start_pos: usize): usize` | Invokes `wrapSegment` and returns `usize`. |
| `wrapTextWith` | `(text: string, opts: WrapOptions, out: []u8): string` | Invokes `wrapTextWith` and returns `string`. |
| `wrapText` | `(text: string, width: usize, out: []u8): string` | Invokes `wrapText` and returns `string`. |
| `lineHasContent` | `(text: string, start: usize, end: usize): bool` | Invokes `lineHasContent` and returns `bool`. |
| `indent` | `(text: string, prefix: string, out: []u8): string` | Invokes `indent` and returns `string`. |
| `dedent` | `(text: string, out: []u8): string` | Invokes `dedent` and returns `string`. |
| `truncate` | `(text: string, max_width: usize, ellipsis: string, out: []u8): string` | Invokes `truncate` and returns `string`. |

## Minimal Example

```cp
import { decodeUtf8 } from "textwrap";

pub fn main() {
    let result = decodeUtf8("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
