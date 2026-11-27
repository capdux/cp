# skiplist — AI Agent Quick Ref

Zero-allocation static-pool deterministic skip list (O(log N) ordered map, lowerBound, rangeCount, freelist reuse) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import { SkipEntry, SkipNode, SkipList, emptyNode } from "skiplist";
```

## Core Types & Constants

```cp
class SkipEntry { key: i64, value: i64, found: bool } // methods: none(...)
class SkipNode { key: i64, value: i64, level: u8, next: [8]u16 }
class SkipList { nodes: [129]SkipNode, free_stack: [128]u16, free_top: usize, cur_level: u8, /* +2 fields */ } // methods: init(...) +13
```

## API Signatures

```cp
fn emptyNode(): SkipNode;
```

## Minimal Usage

```cp
import { emptyNode } from "skiplist";

pub fn main() {
    let result = emptyNode();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"skiplist"`; install via `cap add skiplist`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
