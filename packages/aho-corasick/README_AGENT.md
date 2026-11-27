# aho-corasick — AI Agent Quick Ref

Zero-allocation Aho-Corasick multi-pattern O(N + Z) automaton with LCRS trie, BFS failure links, and dictionary suffix links (Rust aho-corasick parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import { AcMatch, AhoCorasick, noMatch } from "aho-corasick";
```

## Core Types & Constants

```cp
class AcMatch { pattern_id: u16, start: usize, end: usize, found: bool }
class AhoCorasick { edge_char: [MAX_NODES]u8, first_child: [MAX_NODES]u16, next_sibling: [MAX_NODES]u16, fail: [MAX_NODES]u16, /* +6 fields */ } // methods: init(...) +9
```

## API Signatures

```cp
fn noMatch(): AcMatch;
```

## Minimal Usage

```cp
import { noMatch } from "aho-corasick";

pub fn main() {
    let result = noMatch();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"aho-corasick"`; install via `cap add aho-corasick`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add aho-corasick
cpc check src/main.cp
```
