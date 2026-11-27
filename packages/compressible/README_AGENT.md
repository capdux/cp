# compressible — AI Agent Quick Ref

Zero-allocation HTTP Content-Type compressibility checker (`v2.0.18`, `MIT`)

## Import

```cp
import {
    cmprIsSpace,
    cmprToLower,
    cmprEqlIgnoreCase,
    cmprHasPrefixIgnoreCase,
    cmprHasSuffixIgnoreCase,
    cmprFindSlash,
    stripContentTypeParams,
    isCompressible,
} from "compressible";
```

## API Signatures

```cp
fn cmprIsSpace(b: u8): bool;
fn cmprToLower(b: u8): u8;
fn cmprEqlIgnoreCase(a: string, b: string): bool;
fn cmprHasPrefixIgnoreCase(s: string, prefix: string): bool;
fn cmprHasSuffixIgnoreCase(s: string, suffix: string): bool;
fn cmprFindSlash(s: string): usize;
fn stripContentTypeParams(ct: string): string;
fn isCompressible(content_type: string): bool;
```

## Minimal Usage

```cp
import { isCompressible } from "compressible";

pub fn main() {
    let result = isCompressible("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"compressible"`; install via `cap add compressible`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
