[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# radix_trie

Zero-allocation LCRS static-pool byte prefix tree (Trie) with exact lookup, deletion, prefix count, and longest prefix match (LPM) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:radix_trie` ([https://github.com/michaelsproul/rust_radix_trie](https://github.com/michaelsproul/rust_radix_trie))（原库名 `radix_trie` → CP 包名 `radix_trie`）

| Metadata | Value |
| --- | --- |
| Package | `radix_trie` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add radix_trie
```

```cp
import { TrieNode, LongestMatch, Trie } from "radix_trie";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `TrieNode` | `class` | Fields: `byte: u8, is_end: bool, value: i64, first_child: u16, next_sibling: u16, subtree_words: u16` · Methods: `empty` |
| `LongestMatch` | `class` | Fields: `matched_len: usize, value: i64, found: bool` · Methods: `none` |
| `Trie` | `class` | Fields: `nodes: [512]TrieNode, used_nodes: usize, word_count: usize` · Methods: `init, clear, len, nodeCount, findChild, walkPrefix, +7 more` |

## Minimal Example

```cp
import { TrieNode } from "radix_trie";

pub fn main() {
    // Refer to src/index.d.cp for TrieNode fields and methods
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
