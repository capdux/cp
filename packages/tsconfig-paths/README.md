[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# tsconfig-paths

Zero-allocation TypeScript tsconfig.json baseUrl + paths alias wildcard matcher and path resolver for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:tsconfig-paths` ([https://github.com/dividab/tsconfig-paths](https://github.com/dividab/tsconfig-paths))

| Metadata | Value |
| --- | --- |
| Package | `tsconfig-paths` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add tsconfig-paths
```

```cp
import {
    PathRule,
    PatternMatch,
    CandidateList,
    PathsMatcher,
    SegSpan,
    BestRuleHit,
    matchesPattern,
    appendByte,
} from "tsconfig-paths";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PathRule` | `class` | Fields: `pattern: string, targets: [4]string, target_count: usize` · Methods: `empty` |
| `PatternMatch` | `class` | Fields: `matched: bool, is_exact: bool, prefix_len: usize, captured: string` · Methods: `none` |
| `CandidateList` | `class` | Fields: `items: [4]string, count: usize` · Methods: `empty, itemAt` |
| `PathsMatcher` | `class` | Fields: `base_url: string, rules: [32]PathRule, rule_count: usize` · Methods: `init, addRule, addRuleMulti` |
| `SegSpan` | `class` | Fields: `start: usize, end: usize` |
| `BestRuleHit` | `class` | Fields: `found: bool, rule_idx: usize, match_info: PatternMatch` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `matchesPattern` | `(pattern: string, specifier: string): PatternMatch` | Invokes `matchesPattern` and returns `PatternMatch`. |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | Invokes `appendByte` and returns `usize`. |
| `writeNormalizedPath` | `(raw: string, out: []u8, start_pos: usize): usize` | Invokes `writeNormalizedPath` and returns `usize`. |
| `expandTargetInto` | `(base_url: string, target: string, captured: string, out: []u8, start_pos: usize): usize` | Invokes `expandTargetInto` and returns `usize`. |
| `findBestRule` | `(matcher: PathsMatcher, specifier: string): BestRuleHit` | Queries/retrieves data via `findBestRule` returning `BestRuleHit`. |
| `resolvePath` | `(matcher: PathsMatcher, specifier: string, out: []u8): string?` | Invokes `resolvePath` and returns `string?`. |
| `resolveAllCandidates` | `(matcher: PathsMatcher, specifier: string, out_buf: []u8): CandidateList` | Invokes `resolveAllCandidates` and returns `CandidateList`. |

## Minimal Example

```cp
import { matchesPattern } from "tsconfig-paths";

pub fn main() {
    let result = matchesPattern("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
