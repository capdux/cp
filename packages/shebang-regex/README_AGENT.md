# shebang-regex — AI Agent Quick Ref

Zero-allocation shebang (#!) line matcher and stripper for CP (`v1.0.0`, `MIT`)

## Import

```cp
import { ShebangMatch, hasShebang, matchShebang, stripShebang } from "shebang-regex";
```

## Core Types & Constants

```cp
class ShebangMatch { matched: bool, line_len: usize, cmd_start: usize, cmd_len: usize }
```

## API Signatures

```cp
fn hasShebang(input: string): bool;
fn matchShebang(input: string): ShebangMatch;
fn stripShebang(input: string): usize;
```

## Minimal Usage

```cp
import { hasShebang } from "shebang-regex";

pub fn main() {
    let result = hasShebang("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"shebang-regex"`; install via `cap add shebang-regex`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
