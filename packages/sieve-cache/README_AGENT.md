# sieve-cache — AI Agent Quick Ref

Zero-allocation NSDI'24 SIEVE cache eviction engine with static doubly-linked pool and open-addressing index for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SieveNode,
    SieveCache,
    emptyNode,
    mulWrap64,
    hashSlot,
} from "sieve-cache";
```

## Core Types & Constants

```cp
class SieveNode { key: u64, value: i64, visited: bool, in_use: bool, /* +2 fields */ }
class SieveCache { nodes: [128]SieveNode, buckets: [256]u16, free_stack: [128]u16, free_top: usize, /* +9 fields */ } // methods: init(...) +19
```

## API Signatures

```cp
fn emptyNode(): SieveNode;
fn mulWrap64(a: u64, b: u64): u64;
fn hashSlot(key: u64): usize;
```

## Minimal Usage

```cp
import { hashSlot } from "sieve-cache";

pub fn main() {
    let result = hashSlot(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"sieve-cache"`; install via `cap add sieve-cache`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
