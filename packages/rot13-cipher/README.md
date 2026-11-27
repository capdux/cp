[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# rot13-cipher

Zero-heap ROT13, ROT47, arbitrary Caesar shift, and Atbash alphabet substitution utilities.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:rot13-cipher` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:rot13` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `rot13-cipher` → CP 包名 `rot13-cipher`）

| Metadata | Value |
| --- | --- |
| Package | `rot13-cipher` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add rot13-cipher
```

```cp
import {
    shiftLetter,
    rot13,
    rot13InPlace,
    rot47,
    caesar,
    atbash,
} from "rot13-cipher";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `shiftLetter` | `(c: u8, norm_shift: u8): u8` | Invokes `shiftLetter` and returns `u8`. |
| `rot13` | `(input: string, out: []u8): usize` | Invokes `rot13` and returns `usize`. |
| `rot13InPlace` | `(buf: []u8)` | Executes `rot13InPlace` in-place. |
| `rot47` | `(input: string, out: []u8): usize` | Invokes `rot47` and returns `usize`. |
| `caesar` | `(input: string, shift: i32, out: []u8): usize` | Invokes `caesar` and returns `usize`. |
| `atbash` | `(input: string, out: []u8): usize` | Invokes `atbash` and returns `usize`. |

## Minimal Example

```cp
import { rot13 } from "rot13-cipher";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = rot13("example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
