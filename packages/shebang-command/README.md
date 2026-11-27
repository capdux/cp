[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# shebang-command

Zero-allocation shebang executable command extractor reusing shebang-regex for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:shebang-command` ([https://github.com/kevva/shebang-command](https://github.com/kevva/shebang-command))

| Metadata | Value |
| --- | --- |
| Package | `shebang-command` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add shebang-command
```

```cp
import {
    ShebangCmd,
    Parts,
    skipSpaces,
    scanTokenEnd,
    trimRight,
    basenameStart,
    locateParts,
    shebangCommand,
} from "shebang-command";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ShebangCmd` | `class` | Fields: `ok: bool, bin_len: usize, arg_len: usize` |
| `Parts` | `class` | Fields: `ok: bool, bin_start: usize, bin_len: usize, arg_start: usize, arg_len: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `skipSpaces` | `(s: string, pos: usize, end: usize): usize` | Invokes `skipSpaces` and returns `usize`. |
| `scanTokenEnd` | `(s: string, pos: usize, end: usize): usize` | Invokes `scanTokenEnd` and returns `usize`. |
| `trimRight` | `(s: string, start: usize, end: usize): usize` | Invokes `trimRight` and returns `usize`. |
| `basenameStart` | `(s: string, start: usize, end: usize): usize` | Invokes `basenameStart` and returns `usize`. |
| `locateParts` | `(input: string): Parts` | Invokes `locateParts` and returns `Parts`. |
| `shebangCommand` | `(input: string, out_cmd: []u8): usize` | Invokes `shebangCommand` and returns `usize`. |
| `parseShebangCmd` | `(input: string, out_bin: []u8, out_arg: []u8): ShebangCmd` | Parses input via `parseShebangCmd` and returns `ShebangCmd`. |

## Minimal Example

```cp
import { parseShebangCmd } from "shebang-command";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = parseShebangCmd("example", out_buf[0..256], buf_1[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
