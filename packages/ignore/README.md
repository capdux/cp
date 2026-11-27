[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ignore

Zero-heap .gitignore specification rule engine (clean-room CP implementation of npm ignore)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ignore` ([https://github.com/kaelzhang/node-ignore](https://github.com/kaelzhang/node-ignore))

| Metadata | Value |
| --- | --- |
| Package | `ignore` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ignore
```

```cp
import {
    IgnoreRule,
    IgnoreMatcher,
    igTrimRightSpaces,
    igHasInnerSlash,
    igRuleMatches,
    igMatchSingle,
} from "ignore";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `IgnoreRule` | `class` | Fields: `pat: [128]u8, pat_len: usize, negated: bool, dir_only: bool, anchored: bool` |
| `IgnoreMatcher` | `class` | Fields: `rules: [64]IgnoreRule, count: usize, ignore_case: bool` · Methods: `new, newCaseInsensitive, ruleCount, addRule, addLines, ignores, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `igTrimRightSpaces` | `(s: string): usize` | Invokes `igTrimRightSpaces` and returns `usize`. |
| `igHasInnerSlash` | `(s: string, start: usize, end: usize): bool` | Invokes `igHasInnerSlash` and returns `bool`. |
| `igRuleMatches` | `(rule: IgnoreRule, rel_path: string, is_dir: bool, case_insensitive: bool): bool` | Invokes `igRuleMatches` and returns `bool`. |
| `igMatchSingle` | `(pat: &[]u8, anchored: bool, path: string, case_insensitive: bool): bool` | Invokes `igMatchSingle` and returns `bool`. |

## Minimal Example

```cp
import { igTrimRightSpaces } from "ignore";

pub fn main() {
    let result = igTrimRightSpaces("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
