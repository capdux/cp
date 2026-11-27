# dedent — AI Agent Quick Ref

Zero-heap common leading indentation detector and dedent engine for multiline strings (compatible with npm strip-indent, dedent & Rust indoc) (`v1.0.0`, `MIT`)

## Import

```cp
import { isHorizSpace, minIndent, stripIndent, indentLines } from "dedent";
```

## API Signatures

```cp
fn isHorizSpace(c: u8): bool;
fn minIndent(input: string): usize;
fn stripIndent(input: string, out: []u8): usize;
fn indentLines(input: string, spaces: usize, out: []u8): usize;
```

## Minimal Usage

```cp
import { minIndent } from "dedent";

pub fn main() {
    let result = minIndent("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"dedent"`; install via `cap add dedent`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add dedent
cpc check src/main.cp
```
