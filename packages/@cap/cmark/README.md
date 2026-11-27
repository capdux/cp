[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/cmark

CommonMark 0.31.1 official C reference Markdown to HTML/XML/LaTeX renderer for CP

| Metadata | Value |
| --- | --- |
| Package | `@cap/cmark` |
| Version | `0.31.1` |
| License | `BSD-2-Clause` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/cmark
```

```cp
import {
    CmarkError,
    HtmlString,
    cmark_markdown_to_html,
    cmark_version,
    cmark_version_string,
    free,
    versionString,
    versionNumber,
} from "@cap/cmark";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `CmarkError` | `RenderFailed` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `HtmlString` | `class` | Fields: `raw: [*:0]u8!?, len: usize` · Methods: `slice, __cp_deinit` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cmark_markdown_to_html` | `(text: [*]&u8!, len: usize, options: i32): [*:0]u8!?` | Invokes `cmark_markdown_to_html` and returns `[*:0]u8!?`. |
| `cmark_version` | `(): i32` | Invokes `cmark_version` and returns `i32`. |
| `cmark_version_string` | `(): [*:0]&u8!?` | Invokes `cmark_version_string` and returns `[*:0]&u8!?`. |
| `free` | `(ptr: *auto!?)` | Releases native resources allocated by this module. |
| `versionString` | `(): string` | Invokes `versionString` and returns `string`. |
| `versionNumber` | `(): i32` | Invokes `versionNumber` and returns `i32`. |
| `markdownToHtml` | `(md: string, opts: i32): !HtmlString` | Invokes `markdownToHtml` and returns `!HtmlString`. |

## Minimal Example

```cp
import { markdownToHtml } from "@cap/cmark";

pub fn main() {
    let result = markdownToHtml("example", 0) catch return;
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Deterministic resource cleanup**: Native handles provide RAII `__cp_deinit` or explicit `defer` release functions to prevent leaks.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
