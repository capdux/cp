# ucd-trie — AI Agent Quick Ref

Pure CP zero-allocation compressed two-level bitmap Trie for O(1) Unicode codepoint membership queries (ucd-trie parity) (`v1.0.0`, `MIT`)

## Import

```cp
import { UcdTrie, utPopcount64, utAllOnes64 } from "ucd-trie";
```

## Core Types & Constants

```cp
class UcdTrie { max_cp: u32, chunk_count: usize } // methods: new(...) +6
```

## API Signatures

```cp
fn utPopcount64(x: u64): u64;
fn utAllOnes64(): u64;
```

## Minimal Usage

```cp
import { utPopcount64 } from "ucd-trie";

pub fn main() {
    let result = utPopcount64(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ucd-trie"`; install via `cap add ucd-trie`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add ucd-trie
cpc check src/main.cp
```
