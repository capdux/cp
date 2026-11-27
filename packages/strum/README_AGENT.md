# strum — AI Agent Quick Ref

Zero-heap enum variant string registry, case-insensitive parser, discriminant lookup, and iteration (clean-room port of Rust strum) (`v1.0.0`, `MIT`)

## Import

```cp
import { EnumVariant, EnumRegistry, stmStrEq } from "strum";
```

## Core Types & Constants

```cp
class EnumVariant { id: i32, name: string, alias: string, message: string }
class EnumRegistry { variants: [32]EnumVariant, count: usize } // methods: new(...) +8
```

## API Signatures

```cp
fn stmStrEq(a: string, b: string): bool;
```

## Minimal Usage

```cp
import { stmStrEq } from "strum";

pub fn main() {
    let result = stmStrEq("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"strum"`; install via `cap add strum`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
