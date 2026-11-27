# ordered-multimap — AI Agent Quick Ref

Zero-heap insertion-ordered multimap supporting multiple values per key (`v0.7.3`, `MIT`)

## Import

```cp
import { OrderedMultiMap, ommEmptyStrings, ommStrEq } from "ordered-multimap";
```

## Core Types & Constants

```cp
class OrderedMultiMap { keys: [OMM_CAP]string, vals: [OMM_CAP]string, count: usize } // methods: new(...) +8
```

## API Signatures

```cp
fn ommEmptyStrings(): [OMM_CAP]string;
fn ommStrEq(a: string, b: string): bool;
```

## Minimal Usage

```cp
import { ommStrEq } from "ordered-multimap";

pub fn main() {
    let result = ommStrEq("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ordered-multimap"`; install via `cap add ordered-multimap`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
