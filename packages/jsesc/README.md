[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# jsesc

Zero-allocation JavaScript & JSON string escaper with ES6 code point, script-context, and minimal modes (clean-room MIT implementation of npm jsesc v3, #148 / 250M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:jsesc` ([https://github.com/mathiasbynens/jsesc](https://github.com/mathiasbynens/jsesc)) | 许可：`MIT`

| Metadata | Value |
| --- | --- |
| Package | `jsesc` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add jsesc
```

```cp
import {
    JsescOptions,
    JescDecoded,
    jescHexChar,
    jescPush,
    jescPush2,
    jescPushHex2,
    jescPushUnicode4,
    jescPushEs6CodePoint,
} from "jsesc";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `JsescOptions` | `class` | Fields: `quote: u8, wrap: bool, escape_everything: bool, minimal: bool, is_script_context: bool, json: bool, +2 more` · Methods: `default, jsonDefault` |
| `JescDecoded` | `class` | Fields: `cp: u32, len: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `jescHexChar` | `(nibble: u32, lowercase: bool): u8` | Invokes `jescHexChar` and returns `u8`. |
| `jescPush` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `jescPush` and returns `usize`. |
| `jescPush2` | `(out: []u8, pos: usize, b0: u8, b1: u8): usize` | Invokes `jescPush2` and returns `usize`. |
| `jescPushHex2` | `(out: []u8, pos: usize, val: u32, lowercase: bool): usize` | Invokes `jescPushHex2` and returns `usize`. |
| `jescPushUnicode4` | `(out: []u8, pos: usize, val: u32, lowercase: bool): usize` | Invokes `jescPushUnicode4` and returns `usize`. |
| `jescPushEs6CodePoint` | `(out: []u8, pos: usize, cp: u32, lowercase: bool): usize` | Invokes `jescPushEs6CodePoint` and returns `usize`. |
| `jescDecodeUtf8` | `(input: string, idx: usize): JescDecoded` | Invokes `jescDecodeUtf8` and returns `JescDecoded`. |
| `jescAsciiLower` | `(b: u8): u8` | Invokes `jescAsciiLower` and returns `u8`. |
| `jescMatchesHtmlComment` | `(input: string, idx: usize): bool` | Invokes `jescMatchesHtmlComment` and returns `bool`. |
| `jescMatchesCloseScript` | `(input: string, idx: usize): bool` | Invokes `jescMatchesCloseScript` and returns `bool`. |
| `jescEscapeCodePoint` | `(out: []u8, pos: usize, cp: u32, is_json: bool, use_es6: bool, lowercase: bool): usize` | Invokes `jescEscapeCodePoint` and returns `usize`. |
| `jsescWithOptions` | `(input: string, opts: JsescOptions, out: []u8): usize` | Invokes `jsescWithOptions` and returns `usize`. |
| `jsesc` | `(input: string, out: []u8): usize` | Invokes `jsesc` and returns `usize`. |

## Minimal Example

```cp
import { jescDecodeUtf8 } from "jsesc";

pub fn main() {
    let result = jescDecodeUtf8("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
