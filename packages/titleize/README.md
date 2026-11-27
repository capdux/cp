[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# titleize

Zero-allocation string title-case formatter (npm titleize v3 parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:titleize` ([https://github.com/sindresorhus/titleize](https://github.com/sindresorhus/titleize))

| Metadata | Value |
| --- | --- |
| Package | `titleize` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add titleize
```

```cp
import { titIsWordChar, upperCaseFirst, titleize } from "titleize";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `titIsWordChar` | `(c: u8): bool` | Invokes `titIsWordChar` and returns `bool`. |
| `upperCaseFirst` | `(input: string, out: []u8): usize` | Invokes `upperCaseFirst` and returns `usize`. |
| `titleize` | `(input: string, out: []u8): usize` | Invokes `titleize` and returns `usize`. |

## Minimal Example

```cp
import { upperCaseFirst } from "titleize";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = upperCaseFirst("example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
