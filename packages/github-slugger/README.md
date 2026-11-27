[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# github-slugger

Zero-allocation Markdown heading outline extractor, GitHub-compatible anchor slugger & automatic nested TOC generator (github-slugger / markdown-toc parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:github-slugger` ([https://github.com/Flet/github-slugger](https://github.com/Flet/github-slugger))（原库名 `github-slugger` → CP 包名 `github-slugger`）

| Metadata | Value |
| --- | --- |
| Package | `github-slugger` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add github-slugger
```

```cp
import {
    Heading,
    TocResult,
    appendByte,
    githubSlug,
    trimHeadingText,
    isFenceLine,
    slugExists,
    extractToc,
} from "github-slugger";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Heading` | `class` | Fields: `level: u8, text: string, slug_start: usize, slug_len: usize, line: usize` |
| `TocResult` | `class` | Fields: `headings: [MAX_HEADINGS]Heading, count: usize, slug_pool: [MAX_SLUG_POOL]u8, slug_pool_len: usize` · Methods: `empty, slugAt, levelAt, textAt, lineAt` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | Invokes `appendByte` and returns `usize`. |
| `githubSlug` | `(heading: string, out: []u8): string` | Invokes `githubSlug` and returns `string`. |
| `trimHeadingText` | `(raw: string): string` | Invokes `trimHeadingText` and returns `string`. |
| `isFenceLine` | `(line: string): bool` | Checks predicate `isFenceLine` and returns `bool`. |
| `slugExists` | `(res: &mut TocResult, cand: string): bool` | Invokes `slugExists` and returns `bool`. |
| `extractToc` | `(markdown: string): TocResult` | Invokes `extractToc` and returns `TocResult`. |
| `renderTocMarkdown` | `(toc: TocResult, min_level: u8, max_level: u8, out: []u8): string` | Invokes `renderTocMarkdown` and returns `string`. |

## Minimal Example

```cp
import { isFenceLine } from "github-slugger";

pub fn main() {
    let result = isFenceLine("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
