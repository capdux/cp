[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ucd-trie

Pure CP zero-allocation compressed two-level bitmap Trie for O(1) Unicode codepoint membership queries (ucd-trie parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ucd-trie` ([https://github.com/BurntSushi/ucd-generate](https://github.com/BurntSushi/ucd-generate))

| Metadata | Value |
| --- | --- |
| Package | `ucd-trie` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ucd-trie
```

```cp
import { UcdTrie, utPopcount64, utAllOnes64 } from "ucd-trie";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `UcdTrie` | `class` | Fields: `max_cp: u32, chunk_count: usize` · Methods: `new, init, insertCodepoint, insertRange, contains, countMembers, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `utPopcount64` | `(x: u64): u64` | Invokes `utPopcount64` and returns `u64`. |
| `utAllOnes64` | `(): u64` | Invokes `utAllOnes64` and returns `u64`. |

## Minimal Example

```cp
import { utPopcount64 } from "ucd-trie";

pub fn main() {
    let result = utPopcount64(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
