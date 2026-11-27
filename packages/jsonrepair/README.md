[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# jsonrepair

Zero-allocation malformed & LLM-generated JSON repair engine (jsonrepair / dirty-json / Rust llm_json parity: strips markdown fences, fixes single quotes, unquoted keys, trailing commas, Python True/False/None & truncated brackets) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:jsonrepair` ([https://github.com/josdejong/jsonrepair](https://github.com/josdejong/jsonrepair))（原库名 `jsonrepair` → CP 包名 `jsonrepair`）

| Metadata | Value |
| --- | --- |
| Package | `jsonrepair` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add jsonrepair
```

```cp
import {
    isIdentStart,
    isIdentChar,
    appendByte,
    stripMarkdownFence,
    skipWsAndComments,
    repairJson,
} from "jsonrepair";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isIdentStart` | `(c: u8): bool` | Checks predicate `isIdentStart` and returns `bool`. |
| `isIdentChar` | `(c: u8): bool` | Checks predicate `isIdentChar` and returns `bool`. |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | Invokes `appendByte` and returns `usize`. |
| `stripMarkdownFence` | `(raw: string): string` | Invokes `stripMarkdownFence` and returns `string`. |
| `skipWsAndComments` | `(s: string, start: usize): usize` | Invokes `skipWsAndComments` and returns `usize`. |
| `repairJson` | `(raw: string, out: []u8): string` | Invokes `repairJson` and returns `string`. |

## Minimal Example

```cp
import { stripMarkdownFence } from "jsonrepair";

pub fn main() {
    let result = stripMarkdownFence("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
