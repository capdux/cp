[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# vte

Zero-allocation ANSI/VT100 terminal escape sequence state machine parser (Paul Williams model) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:vte` ([https://github.com/alacritty/vte](https://github.com/alacritty/vte))

| Metadata | Value |
| --- | --- |
| Package | `vte` |
| Version | `1.0.0` |
| License | `Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add vte
```

```cp
import {
    VteActionKind,
    VteState,
    VteEvent,
    VteParser,
    vtepZeroParams,
    vtepZeroOsc,
    vtepIsExecute,
    vtepAddDigit,
} from "vte";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `VteActionKind` | `Print, Execute, CsiDispatch, OscDispatch, EscDispatch` |
| `enum` | `VteState` | `Ground, Escape, CsiEntry, CsiParam, OscString, CsiIgnore, OscEsc` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `VteEvent` | `class` | Fields: `kind: VteActionKind, ch: u8, final_byte: u8, param_count: usize, params: [8]u16, intermediate: u8` |
| `VteParser` | `class` | Fields: `state: VteState, params: [8]u16, param_count: usize, cur_param: u16, has_param: bool, intermediate: u8, +2 more` · Methods: `new, reset, oscLen, oscByte, clearCsi, pushParam, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `vtepZeroParams` | `(): [8]u16` | Invokes `vtepZeroParams` and returns `[8]u16`. |
| `vtepZeroOsc` | `(): [64]u8` | Invokes `vtepZeroOsc` and returns `[64]u8`. |
| `vtepIsExecute` | `(b: u8): bool` | Invokes `vtepIsExecute` and returns `bool`. |
| `vtepAddDigit` | `(cur: u16, digit: u8): u16` | Invokes `vtepAddDigit` and returns `u16`. |
| `vtepParseOscCmd` | `(buf: &[]u8, len: usize): u16` | Invokes `vtepParseOscCmd` and returns `u16`. |
| `vtepHasOscCmd` | `(buf: &[]u8, len: usize): bool` | Invokes `vtepHasOscCmd` and returns `bool`. |
| `vtepMakeEvent` | `(kind: VteActionKind, ch: u8, final_byte: u8, param_count: usize, params: [8]u16, intermediate: u8): VteEvent` | Invokes `vtepMakeEvent` and returns `VteEvent`. |

## Minimal Example

```cp
import { vtepZeroParams } from "vte";

pub fn main() {
    let result = vtepZeroParams();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
