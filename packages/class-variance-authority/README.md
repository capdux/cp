[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# class-variance-authority

Zero-heap CSS variant class composer for CP (clean-room Apache-2.0 implementation of npm class-variance-authority / cva).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:class-variance-authority` ([https://github.com/joe-bell/cva](https://github.com/joe-bell/cva))（原库名 `class-variance-authority` → CP 包名 `class-variance-authority`）

| Metadata | Value |
| --- | --- |
| Package | `class-variance-authority` |
| Version | `1.0.0` |
| License | `Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add class-variance-authority
```

```cp
import {
    VariantRule,
    VariantSelection,
    CompoundRule,
    cvaStrEq,
    cvaIsSpace,
    cvaAppendClasses,
    cvaResolveOption,
    cvaMatchesCond,
} from "class-variance-authority";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `VariantRule` | `class` | Fields: `group: string, option: string, classes: string` |
| `VariantSelection` | `class` | Fields: `group: string, option: string` |
| `CompoundRule` | `class` | Fields: `group1: string, option1: string, group2: string, option2: string, group3: string, option3: string, +1 more` · Methods: `pair, triple` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cvaStrEq` | `(a: string, b: string): bool` | Invokes `cvaStrEq` and returns `bool`. |
| `cvaIsSpace` | `(ch: u8): bool` | Invokes `cvaIsSpace` and returns `bool`. |
| `cvaAppendClasses` | `(out: []u8, pos: usize, src: string): usize` | Invokes `cvaAppendClasses` and returns `usize`. |
| `cvaResolveOption` | `(group: string, defaults: &[]VariantSelection, selected: &[]VariantSelection): string?` | Invokes `cvaResolveOption` and returns `string?`. |
| `cvaMatchesCond` | `(group: string, option: string, defaults: &[]VariantSelection, selected: &[]VariantSelection): bool` | Invokes `cvaMatchesCond` and returns `bool`. |
| `cvaCompose` | `(base: string, rules: &[]VariantRule, defaults: &[]VariantSelection, selected: &[]VariantSelection, extra: string, out: []u8): usize` | Invokes `cvaCompose` and returns `usize`. |
| `cvaComposeWithCompounds` | `(base: string, rules: &[]VariantRule, compounds: &[]CompoundRule, defaults: &[]VariantSelection, selected: &[]VariantSelection, extra: string, out: []u8): usize` | Invokes `cvaComposeWithCompounds` and returns `usize`. |

## Minimal Example

```cp
import { cvaStrEq } from "class-variance-authority";

pub fn main() {
    let result = cvaStrEq("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
