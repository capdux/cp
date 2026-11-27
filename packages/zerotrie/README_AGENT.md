# zerotrie — AI Agent Quick Ref

Zero-allocation compact left-child right-sibling prefix trie (ZeroTrie) for fast static/pooled key-to-u32 lookup and longest-prefix matching in CP (`v1.0.0`, `MIT`)

## Import

```cp
import { TrieNode, PrefixMatch, ZeroTrie, findChild } from "zerotrie";
```

## Core Types & Constants

```cp
class TrieNode { ch: u8, has_value: bool, value: u32, first_child: i32, /* +1 fields */ } // methods: empty(...)
class PrefixMatch { matched_len: usize, value: u32, found: bool }
class ZeroTrie { node_count: usize, key_count: usize } // methods: new(...) +6
```

## API Signatures

```cp
fn findChild(nodes: &[]TrieNode, parent: usize, ch: u8): i32;
```

## Minimal Usage

```cp
import { findChild } from "zerotrie";

pub fn main() {
    let result = findChild(/* nodes: &[]TrieNode */, 0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"zerotrie"`; install via `cap add zerotrie`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
