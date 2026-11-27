[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ejs

Zero-heap EJS template engine supporting <%= escaped %>, <%- raw %>, <%# comments %>, <% if/else %>, and <% repeat %> tags (clean-room port of npm ejs)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ejs` ([https://github.com/mde/ejs](https://github.com/mde/ejs))（原库名 `ejs` → CP 包名 `ejs`）

| Metadata | Value |
| --- | --- |
| Package | `ejs` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ejs
```

```cp
import {
    EjsEntry,
    EjsContext,
    ejsWriteStr,
    ejsLookupStr,
    ejsLookupBool,
    ejsLookupCount,
    ejsFindTagOpen,
    ejsFindTagClose,
} from "ejs";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `EjsEntry` | `class` | Fields: `key: string, str_val: string, bool_val: bool, count_val: usize, is_count: bool` |
| `EjsContext` | `class` | Fields: `entries: [32]EjsEntry, count: usize` · Methods: `new, setStr, setBool, setRepeatCount, size` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ejsWriteStr` | `(s: string, out: []u8, pos: usize): usize` | Invokes `ejsWriteStr` and returns `usize`. |
| `ejsLookupStr` | `(ctx: EjsContext, key: string): string` | Invokes `ejsLookupStr` and returns `string`. |
| `ejsLookupBool` | `(ctx: EjsContext, key: string): bool` | Invokes `ejsLookupBool` and returns `bool`. |
| `ejsLookupCount` | `(ctx: EjsContext, key: string): usize` | Invokes `ejsLookupCount` and returns `usize`. |
| `ejsFindTagOpen` | `(tpl: string, start_pos: usize): usize` | Invokes `ejsFindTagOpen` and returns `usize`. |
| `ejsFindTagClose` | `(tpl: string, start_pos: usize): usize` | Invokes `ejsFindTagClose` and returns `usize`. |
| `ejsFindBlockBounds` | `(tpl: string, body_start: usize, out_pos: []usize): bool` | Invokes `ejsFindBlockBounds` and returns `bool`. |
| `ejsRenderSlice` | `(tpl: string, ctx: EjsContext, out: []u8, start_out: usize, depth: usize): usize` | Invokes `ejsRenderSlice` and returns `usize`. |
| `renderEjs` | `(tpl: string, ctx: EjsContext, out: []u8): usize` | Invokes `renderEjs` and returns `usize`. |

## Minimal Example

```cp
import { ejsWriteStr } from "ejs";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = ejsWriteStr("example", out_buf[0..256], 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
