[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# enquirer

Zero-heap pure CP interactive CLI prompt state machines for Confirm, Select, and MultiSelect (clean-room MIT implementation of npm enquirer v2)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:enquirer` ([https://github.com/enquirer/enquirer](https://github.com/enquirer/enquirer))

| Metadata | Value |
| --- | --- |
| Package | `enquirer` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add enquirer
```

```cp
import { ConfirmPrompt, SelectChoice, SelectPrompt, MultiSelectPrompt } from "enquirer";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ConfirmPrompt` | `class` | Fields: `message: string, initial: bool` · Methods: `new, parseAnswer, render` |
| `SelectChoice` | `class` | Fields: `label: string, enabled: bool` |
| `SelectPrompt` | `class` | Fields: `message: string, choices: [16]SelectChoice, choice_count: usize, cursor: usize, submitted: bool` · Methods: `new, addChoice, moveDown, moveUp, submit, render` |
| `MultiSelectPrompt` | `class` | Fields: `message: string, choices: [16]string, selected: [16]bool, choice_count: usize` · Methods: `new, addChoice, toggle, selectedCount, isSelected, render` |

## Minimal Example

```cp
import { ConfirmPrompt } from "enquirer";

pub fn main() {
    // Refer to src/index.d.cp for ConfirmPrompt fields and methods
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
