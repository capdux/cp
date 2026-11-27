[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# sprintf-js

Zero-allocation C-style sprintf formatting engine supporting %s/%d/%i/%u/%x/%X/%o/%b/%f/%c with flags, width, and precision in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:sprintf-js` ([https://github.com/alexei/sprintf.js](https://github.com/alexei/sprintf.js), `BSD-3-Clause`)

| Metadata | Value |
| --- | --- |
| Package | `sprintf-js` |
| Version | `1.0.0` |
| License | `BSD-3-Clause` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add sprintf-js
```

```cp
import {
    FmtArgKind,
    FmtArg,
    spfPutByte,
    spfPutRepeat,
    spfEmitPadded,
    spfFormatUintRadix,
    spfArgAsSigned,
    spfArgAsUnsigned,
} from "sprintf-js";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `FmtArgKind` | `Int, Uint, Float, Str, Char, Bool` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `FmtArg` | `class` | Fields: `kind: FmtArgKind, int_val: i64, uint_val: u64, float_val: f64, str_val: string, char_val: u8, +1 more` · Methods: `int, uint, float, str, ch, boolean` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `spfPutByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `spfPutByte` and returns `usize`. |
| `spfPutRepeat` | `(out: []u8, pos: usize, b: u8, count: usize): usize` | Invokes `spfPutRepeat` and returns `usize`. |
| `spfEmitPadded` | `(out: []u8, pos: usize, sign: u8, body: &[]u8, width: usize, left_align: bool, zero_pad: bool): usize` | Invokes `spfEmitPadded` and returns `usize`. |
| `spfFormatUintRadix` | `(val: u64, base: u64, upper: bool, buf: []u8): usize` | Invokes `spfFormatUintRadix` and returns `usize`. |
| `spfArgAsSigned` | `(arg: FmtArg): i64` | Invokes `spfArgAsSigned` and returns `i64`. |
| `spfArgAsUnsigned` | `(arg: FmtArg): u64` | Invokes `spfArgAsUnsigned` and returns `u64`. |
| `spfArgAsFloat` | `(arg: FmtArg): f64` | Invokes `spfArgAsFloat` and returns `f64`. |
| `spfFormatFloat` | `(val: f64, prec: usize, plus_sign: bool, width: usize, left_align: bool, zero_pad: bool, out: []u8, pos: usize): usize` | Invokes `spfFormatFloat` and returns `usize`. |
| `sprintf` | `(fmt: string, args: &[]FmtArg, out: []u8): usize` | Invokes `sprintf` and returns `usize`. |

## Minimal Example

```cp
import { sprintf } from "sprintf-js";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = sprintf("example", /* args: &[]FmtArg */, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
