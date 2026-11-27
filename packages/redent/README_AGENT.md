# redent — AI Agent Quick Ref

Zero-allocation strip-and-reindent multiline string utility (npm redent parity) composing strip-indent and indent-string for CP (`v1.0.0`, `MIT`)

## Import

```cp
import { redentWithIndent, redent } from "redent";
```

## API Signatures

```cp
fn redentWithIndent(s: string, count: usize, indent: string, include_empty_lines: bool, out: []u8): usize;
fn redent(s: string, count: usize, out: []u8): usize;
```

## Minimal Usage

```cp
import { redent } from "redent";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = redent("example", 0, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"redent"`; install via `cap add redent`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add redent
cpc check src/main.cp
```
