# radix_trie — AI Agent Quick Ref

Zero-allocation LCRS static-pool byte prefix tree (Trie) with exact lookup, deletion, prefix count, and longest prefix match (LPM) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import { TrieNode, LongestMatch, Trie } from "radix_trie";
```

## Core Types & Constants

```cp
class TrieNode { byte: u8, is_end: bool, value: i64, first_child: u16, /* +2 fields */ } // methods: empty(...)
class LongestMatch { matched_len: usize, value: i64, found: bool } // methods: none(...)
class Trie { nodes: [512]TrieNode, used_nodes: usize, word_count: usize } // methods: init(...) +12
```

## Minimal Usage

```cp
import { TrieNode } from "radix_trie";

pub fn main() {
    // Refer to src/index.d.cp for TrieNode fields and methods
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"radix_trie"`; install via `cap add radix_trie`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add radix_trie
cpc check src/main.cp
```
