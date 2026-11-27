[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# zerotrie

Zero-allocation compact left-child right-sibling prefix trie (ZeroTrie) for fast static/pooled key-to-u32 lookup and longest-prefix matching in CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:zerotrie` ([https://github.com/unicode-org/icu4x](https://github.com/unicode-org/icu4x)) / `npm:trie-prefix-tree` ([https://github.com/lyndseybrowning/trie-prefix-tree](https://github.com/lyndseybrowning/trie-prefix-tree))

| Metadata | Value |
| --- | --- |
| Package | `zerotrie` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add zerotrie
```

```cp
import { TrieNode, PrefixMatch, ZeroTrie, findChild } from "zerotrie";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `TrieNode` | `class` | Fields: `ch: u8, has_value: bool, value: u32, first_child: i32, next_sibling: i32` · Methods: `empty` |
| `PrefixMatch` | `class` | Fields: `matched_len: usize, value: u32, found: bool` |
| `ZeroTrie` | `class` | Fields: `node_count: usize, key_count: usize` · Methods: `new, init, insert, get, containsKey, hasPrefix, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `findChild` | `(nodes: &[]TrieNode, parent: usize, ch: u8): i32` | Queries/retrieves data via `findChild` returning `i32`. |

## Minimal Example

```cp
import { findChild } from "zerotrie";

pub fn main() {
    let result = findChild(/* nodes: &[]TrieNode */, 0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
