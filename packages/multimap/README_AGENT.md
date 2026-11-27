# multimap — AI Agent Quick Ref

Zero-allocation insertion-ordered one-to-many string multimap (capacity 128) for HTTP headers, URL query params, tag indexes, and event routing (Rust multimap parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import { MultiEntry, MultiMapStr, strEq } from "multimap";
```

## Core Types & Constants

```cp
class MultiEntry { key: string, value: string }
class MultiMapStr { entries: [MULTIMAP_CAP]MultiEntry, count: usize } // methods: init(...) +14
```

## API Signatures

```cp
fn strEq(a: string, b: string): bool;
```

## Minimal Usage

```cp
import { strEq } from "multimap";

pub fn main() {
    let result = strEq("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"multimap"`; install via `cap add multimap`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
