[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# serialize-javascript

Zero-heap XSS-safe JSON/JavaScript literal serializer escaping HTML script breakouts and line separators

| Metadata | Value |
| --- | --- |
| Package | `serialize-javascript` |
| Version | `6.0.2` |
| License | `BSD-3-Clause` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add serialize-javascript
```

```cp
import {
    sjsWriteStr,
    sjsHex,
    escapeUnsafeJson,
    serializeString,
    hasUnsafeScriptChars,
} from "serialize-javascript";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `sjsWriteStr` | `(s: string, out: []u8, pos: usize): usize` | Invokes `sjsWriteStr` and returns `usize`. |
| `sjsHex` | `(n: u8): u8` | Invokes `sjsHex` and returns `u8`. |
| `escapeUnsafeJson` | `(json_text: string, out: []u8): usize` | Invokes `escapeUnsafeJson` and returns `usize`. |
| `serializeString` | `(s: string, out: []u8): usize` | Invokes `serializeString` and returns `usize`. |
| `hasUnsafeScriptChars` | `(s: string): bool` | Checks predicate `hasUnsafeScriptChars` and returns `bool`. |

## Minimal Example

```cp
import { sjsWriteStr } from "serialize-javascript";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = sjsWriteStr("example", out_buf[0..256], 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
