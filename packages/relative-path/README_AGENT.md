# relative-path — AI Agent Quick Ref

Portable platform-independent relative path manipulation, normalization, joining, and resolution (zero-heap pure CP) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    RelPath,
    rpIsSep,
    normalizeRelPath,
    joinRelPath,
    relativeTo,
} from "relative-path";
```

## Core Types & Constants

```cp
class RelPath { buf: [256]u8, length: usize } // methods: empty(...) +15
```

## API Signatures

```cp
fn rpIsSep(b: u8): bool;
fn normalizeRelPath(path: string): RelPath;
fn joinRelPath(base: string, rel: string): RelPath;
fn relativeTo(from_path: string, to_path: string): RelPath;
```

## Minimal Usage

```cp
import { normalizeRelPath } from "relative-path";

pub fn main() {
    let result = normalizeRelPath("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"relative-path"`; install via `cap add relative-path`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
