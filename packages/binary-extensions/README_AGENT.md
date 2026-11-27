# binary-extensions — AI Agent Quick Ref

Zero-heap O(log n) lookup of 127 common binary file extensions and categories in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import { cmpLowerToAscii, extensionCategory, isBinaryExtension, isBinaryPath } from "binary-extensions";
```

## API Signatures

```cp
fn cmpLowerToAscii(a: string, b: string): i32;
fn extensionCategory(ext: string): u8;
fn isBinaryExtension(ext: string): bool;
fn isBinaryPath(path: string): bool;
```

## Minimal Usage

```cp
import { isBinaryExtension } from "binary-extensions";

pub fn main() {
    let result = isBinaryExtension("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"binary-extensions"`; install via `cap add binary-extensions`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add binary-extensions
cpc check src/main.cp
```
