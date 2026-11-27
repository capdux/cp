# path-clean — AI Agent Quick Ref

Zero-heap lexical POSIX path cleaner following Go filepath.Clean / Rust path-clean semantics in pure CP (`v1.0.0`, `MIT OR Apache-2.0`)

## Import

```cp
import {
    pclByteAt,
    pclCleanTwo,
    cleanPath,
    cleanJoin,
    isClean,
} from "path-clean";
```

## API Signatures

```cp
fn pclByteAt(a: string, b: string, has_sep: bool, idx: usize): u8;
fn pclCleanTwo(a: string, b: string, has_sep: bool, out: []u8): usize;
fn cleanPath(path: string, out: []u8): usize;
fn cleanJoin(a: string, b: string, out: []u8): usize;
fn isClean(path: string): bool;
```

## Minimal Usage

```cp
import { isClean } from "path-clean";

pub fn main() {
    let result = isClean("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"path-clean"`; install via `cap add path-clean`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add path-clean
cpc check src/main.cp
```
