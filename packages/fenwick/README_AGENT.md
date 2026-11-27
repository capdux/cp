# fenwick — AI Agent Quick Ref

Zero-allocation Binary Indexed Tree (Fenwick Tree) with O(log N) prefix/range sums and binary-lifting lowerBound for CP (`v1.0.0`, `MIT`)

## Import

```cp
import { FenwickTree, lowbit } from "fenwick";
```

## Core Types & Constants

```cp
class FenwickTree { tree: [TREE_LEN]i64, length: usize } // methods: init(...) +10
```

## API Signatures

```cp
fn lowbit(x: usize): usize;
```

## Minimal Usage

```cp
import { lowbit } from "fenwick";

pub fn main() {
    let result = lowbit(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"fenwick"`; install via `cap add fenwick`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add fenwick
cpc check src/main.cp
```
