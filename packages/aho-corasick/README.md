[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# aho-corasick

Zero-allocation Aho-Corasick multi-pattern O(N + Z) automaton with LCRS trie, BFS failure links, and dictionary suffix links (Rust aho-corasick parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:aho-corasick` ([https://github.com/BurntSushi/aho-corasick](https://github.com/BurntSushi/aho-corasick))

| Metadata | Value |
| --- | --- |
| Package | `aho-corasick` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add aho-corasick
```

```cp
import { AcMatch, AhoCorasick, noMatch } from "aho-corasick";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `AcMatch` | `class` | Fields: `pattern_id: u16, start: usize, end: usize, found: bool` |
| `AhoCorasick` | `class` | Fields: `edge_char: [MAX_NODES]u8, first_child: [MAX_NODES]u16, next_sibling: [MAX_NODES]u16, fail: [MAX_NODES]u16, dict_link: [MAX_NODES]u16, out_pat: [MAX_NODES]i16, +4 more` · Methods: `init, findChild, addPattern, build, patternCount, stepState, +4 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `noMatch` | `(): AcMatch` | Invokes `noMatch` and returns `AcMatch`. |

## Minimal Example

```cp
import { noMatch } from "aho-corasick";

pub fn main() {
    let result = noMatch();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
