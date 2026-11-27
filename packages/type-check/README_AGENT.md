# type-check — AI Agent Quick Ref

Zero-allocation runtime type expression parser and validator supporting primitives, unions (A | B), Maybe T, arrays ([T]), tuples ((A, B)), and objects (clean-room MIT implementation of npm type-check, #241 / 195M wk) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    DynValueKind,
    DynValue,
    tckMatchAtom,
    typeCheck,
    isValidTypeExpr,
} from "type-check";
```

## Core Types & Constants

```cp
enum DynValueKind { Undefined, Null, Boolean, Number, Int, Float, ... }
class DynValue { kind: DynValueKind, elem_kind: DynValueKind, array_len: usize } // methods: of(...) +1
```

## API Signatures

```cp
fn tckMatchAtom(atom: string, val: DynValue): bool;
fn typeCheck(type_expr: string, val: DynValue): bool;
fn isValidTypeExpr(type_expr: string): bool;
```

## Minimal Usage

```cp
import { isValidTypeExpr } from "type-check";

pub fn main() {
    let result = isValidTypeExpr("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"type-check"`; install via `cap add type-check`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
