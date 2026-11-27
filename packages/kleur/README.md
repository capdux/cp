[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# kleur

Zero-allocation composable chainable ANSI terminal style builder with nested close/reset recovery for CP (clean-room parity with npm kleur v4)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:kleur` ([https://github.com/lukeed/kleur](https://github.com/lukeed/kleur))

| Metadata | Value |
| --- | --- |
| Package | `kleur` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add kleur
```

```cp
import { KleurChain } from "kleur";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `KleurChain` | `class` | Fields: `enabled: bool, stack: [8]StylePair, depth: usize` · Methods: `new, on, off, depthCount, withStyle, reset, +25 more` |

## Minimal Example

```cp
import { KleurChain } from "kleur";

pub fn main() {
    // Refer to src/index.d.cp for KleurChain fields and methods
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
