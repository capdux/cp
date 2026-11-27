[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# optionator

Zero-heap CLI option specification and auto-aligned --help documentation generator in pure CP

| Metadata | Value |
| --- | --- |
| Package | `optionator` |
| Version | `0.9.4` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add optionator
```

```cp
import {
    OptionSpec,
    optnLeftColLen,
    formatOptionRow,
    generateHelp,
    findOptionSpec,
} from "optionator";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `OptionSpec` | `class` | Fields: `long_name: string, short_alias: string, value_type: string, description: string, default_val: string, required: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `optnLeftColLen` | `(spec: OptionSpec): usize` | Invokes `optnLeftColLen` and returns `usize`. |
| `formatOptionRow` | `(spec: OptionSpec, col_width: usize, out: []u8): usize` | Serializes/formats data via `formatOptionRow` returning `usize`. |
| `generateHelp` | `(program: string, summary: string, specs: &[]OptionSpec, out: []u8): usize` | Invokes `generateHelp` and returns `usize`. |
| `findOptionSpec` | `(specs: &[]OptionSpec, flag: string): OptionSpec?` | Queries/retrieves data via `findOptionSpec` returning `OptionSpec?`. |

## Minimal Example

```cp
import { findOptionSpec } from "optionator";

pub fn main() {
    let result = findOptionSpec(/* specs: &[]OptionSpec */, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
