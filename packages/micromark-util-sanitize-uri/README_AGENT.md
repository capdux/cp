# micromark-util-sanitize-uri — AI Agent Quick Ref

Zero-allocation safe URI sanitizer and RFC 3986 percent-encoder blocking unsafe schemes (`v2.0.1`, `MIT`)

## Import

```cp
import { surIsSafeUriByte, isSafeUri, sanitizeUri } from "micromark-util-sanitize-uri";
```

## API Signatures

```cp
fn surIsSafeUriByte(b: u8): bool;
fn isSafeUri(uri: string): bool;
fn sanitizeUri(uri: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { isSafeUri } from "micromark-util-sanitize-uri";

pub fn main() {
    let result = isSafeUri("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"micromark-util-sanitize-uri"`; install via `cap add micromark-util-sanitize-uri`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add micromark-util-sanitize-uri
cpc check src/main.cp
```
