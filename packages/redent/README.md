[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# redent

Zero-allocation strip-and-reindent multiline string utility (npm redent parity) composing strip-indent and indent-string for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:redent` ([https://github.com/sindresorhus/redent](https://github.com/sindresorhus/redent))

| Metadata | Value |
| --- | --- |
| Package | `redent` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add redent
```

```cp
import { redentWithIndent, redent } from "redent";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `redentWithIndent` | `(s: string, count: usize, indent: string, include_empty_lines: bool, out: []u8): usize` | Invokes `redentWithIndent` and returns `usize`. |
| `redent` | `(s: string, count: usize, out: []u8): usize` | Invokes `redent` and returns `usize`. |

## Minimal Example

```cp
import { redent } from "redent";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = redent("example", 0, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
