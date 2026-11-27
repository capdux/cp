[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# whatwg-mimetype

Zero-allocation WHATWG MIME Sniffing Standard MIMEType parser, parameter map, essence, and serializer (clean-room MIT implementation of npm whatwg-mimetype, #200 / 215M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:whatwg-mimetype` ([https://github.com/jsdom/whatwg-mimetype](https://github.com/jsdom/whatwg-mimetype)) | 许可：`MIT`

| Metadata | Value |
| --- | --- |
| Package | `whatwg-mimetype` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add whatwg-mimetype
```

```cp
import {
    MimeParam,
    MimeType,
    wmimeIsHttpWs,
    wmimeIsTokenChar,
    wmimeEndsWithIgnoreCase,
    wmimePushLower,
    parseMimeType,
} from "whatwg-mimetype";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `MimeParam` | `class` | Fields: `name: string, value: string` |
| `MimeType` | `class` | Fields: `valid: bool, main_type: string, subtype: string, params: [8]MimeParam, param_count: usize` · Methods: `paramCount, getParam, essence, isHTML, isXML, isJavaScript, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `wmimeIsHttpWs` | `(b: u8): bool` | Invokes `wmimeIsHttpWs` and returns `bool`. |
| `wmimeIsTokenChar` | `(b: u8): bool` | Invokes `wmimeIsTokenChar` and returns `bool`. |
| `wmimeEndsWithIgnoreCase` | `(s: string, suffix: string): bool` | Invokes `wmimeEndsWithIgnoreCase` and returns `bool`. |
| `wmimePushLower` | `(out: []u8, pos: usize, s: string): usize` | Invokes `wmimePushLower` and returns `usize`. |
| `parseMimeType` | `(input: string): MimeType` | Parses input via `parseMimeType` and returns `MimeType`. |

## Minimal Example

```cp
import { parseMimeType } from "whatwg-mimetype";

pub fn main() {
    let result = parseMimeType("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
