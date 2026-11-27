[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# dompurify

Zero-allocation HTML sanitizer stripping blocked tags, event attributes, and unsafe URI schemes (npm DOMPurify parity) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`dompurify`](https://github.com/cure53/DOMPurify)（`Apache-2.0`）（包名 `dompurify`）。

| Metadata | Value |
| --- | --- |
| Package | `dompurify` |
| Version | `1.0.0` |
| License | `Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add dompurify
```

```cp
import {
    SanitizeHtmlOptions,
    defaultSanitizeOptions,
    dmpIsNameChar,
    dmpHasPrefixCI,
    dmpWriteByte,
    dmpIsBlockedContentTag,
    dmpIsBlockedVoidTag,
    dmpIsAllowedTag,
} from "dompurify";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SanitizeHtmlOptions` | `class` | Fields: `allow_links: bool, allow_images: bool, strip_comments: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `defaultSanitizeOptions` | `(): SanitizeHtmlOptions` | Invokes `defaultSanitizeOptions` and returns `SanitizeHtmlOptions`. |
| `dmpIsNameChar` | `(c: u8): bool` | Invokes `dmpIsNameChar` and returns `bool`. |
| `dmpHasPrefixCI` | `(s: string, prefix: string): bool` | Invokes `dmpHasPrefixCI` and returns `bool`. |
| `dmpWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `dmpWriteByte` and returns `usize`. |
| `dmpIsBlockedContentTag` | `(tag: string): bool` | Invokes `dmpIsBlockedContentTag` and returns `bool`. |
| `dmpIsBlockedVoidTag` | `(tag: string): bool` | Invokes `dmpIsBlockedVoidTag` and returns `bool`. |
| `dmpIsAllowedTag` | `(tag: string, opts: SanitizeHtmlOptions): bool` | Invokes `dmpIsAllowedTag` and returns `bool`. |
| `dmpIsEventAttr` | `(attr: string): bool` | Invokes `dmpIsEventAttr` and returns `bool`. |
| `dmpIsSafeUrl` | `(url: string): bool` | Invokes `dmpIsSafeUrl` and returns `bool`. |
| `dmpIsSafeAttr` | `(tag: string, attr: string, val: string, opts: SanitizeHtmlOptions): bool` | Invokes `dmpIsSafeAttr` and returns `bool`. |
| `dmpFindTagClose` | `(html: string, start: usize): usize` | Invokes `dmpFindTagClose` and returns `usize`. |
| `dmpFindClosingTagEnd` | `(html: string, start: usize, tag: string): usize` | Invokes `dmpFindClosingTagEnd` and returns `usize`. |
| `sanitizeHtml` | `(html: string, opts: SanitizeHtmlOptions, out: []u8): usize` | Invokes `sanitizeHtml` and returns `usize`. |
| `isSafeHtml` | `(html: string): bool` | Checks predicate `isSafeHtml` and returns `bool`. |

## Minimal Example

```cp
import { isSafeHtml } from "dompurify";

pub fn main() {
    let result = isSafeHtml("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
