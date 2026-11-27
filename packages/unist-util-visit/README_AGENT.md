# unist-util-visit — AI Agent Quick Ref

Zero-heap unist AST tree pre-order/post-order visitor and node matcher (covers unist-util-visit-parents and unist-util-is) (`v5.0.0`, `MIT`)

## Import

```cp
import { UnistNode, UnistTree, uuvStrEq, newUnistTree } from "unist-util-visit";
```

## Core Types & Constants

```cp
class UnistNode { kind: string, value: string, parent: u32, first_child: u32, /* +1 fields */ }
class UnistTree { nodes: [128]UnistNode, len: usize } // methods: addNode(...) +3
```

## API Signatures

```cp
fn uuvStrEq(a: string, b: string): bool;
fn newUnistTree(): UnistTree;
```

## Minimal Usage

```cp
import { uuvStrEq } from "unist-util-visit";

pub fn main() {
    let result = uuvStrEq("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"unist-util-visit"`; install via `cap add unist-util-visit`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
