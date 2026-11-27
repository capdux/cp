# is-path-inside — AI Agent Quick Ref

Zero-allocation path traversal guard checking whether child_path is strictly inside parent_path (`v4.0.0`, `MIT`)

## Import

```cp
import { ipiIsSep, ipiNormalize, isPathInside, isPathInsideOrEqual } from "is-path-inside";
```

## API Signatures

```cp
fn ipiIsSep(b: u8): bool;
fn ipiNormalize(input: string, out: []u8): usize;
fn isPathInside(child_path: string, parent_path: string): bool;
fn isPathInsideOrEqual(child_path: string, parent_path: string): bool;
```

## Minimal Usage

```cp
import { isPathInside } from "is-path-inside";

pub fn main() {
    let result = isPathInside("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"is-path-inside"`; install via `cap add is-path-inside`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add is-path-inside
cpc check src/main.cp
```
