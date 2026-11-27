# merge-descriptors — AI Agent Quick Ref

Zero-allocation property descriptor table and safe descriptor merge engine for CP (clean-room parity with npm merge-descriptors v2) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    PropDescriptor,
    DescriptorTable,
    mgdStrEq,
    mgdEmptyDesc,
    mergeDescriptors,
    countEnumerable,
} from "merge-descriptors";
```

## Core Types & Constants

```cp
class PropDescriptor { key: string, value: string, writable: bool, enumerable: bool, /* +1 fields */ } // methods: data(...) +1
class DescriptorTable { entries: [32]PropDescriptor, count: usize } // methods: new(...) +5
```

## API Signatures

```cp
fn mgdStrEq(a: string, b: string): bool;
fn mgdEmptyDesc(): PropDescriptor;
fn mergeDescriptors(dst: &mut DescriptorTable, src: DescriptorTable, redefine: bool): usize;
fn countEnumerable(tbl: DescriptorTable): usize;
```

## Minimal Usage

```cp
import { mgdStrEq } from "merge-descriptors";

pub fn main() {
    let result = mgdStrEq("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"merge-descriptors"`; install via `cap add merge-descriptors`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
