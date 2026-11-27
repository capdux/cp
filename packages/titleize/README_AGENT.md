# titleize — AI Agent Quick Ref

Zero-allocation string title-case formatter (npm titleize v3 parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import { titIsWordChar, upperCaseFirst, titleize } from "titleize";
```

## API Signatures

```cp
fn titIsWordChar(c: u8): bool;
fn upperCaseFirst(input: string, out: []u8): usize;
fn titleize(input: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { upperCaseFirst } from "titleize";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = upperCaseFirst("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"titleize"`; install via `cap add titleize`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add titleize
cpc check src/main.cp
```
