# did-resolver — AI Agent Quick Ref

Zero-heap W3C DID Core 1.0 Decentralized Identifier & DID URL parser, strict character validator, and formatter. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    DidUrl,
    isValidMethod,
    isValidMethodId,
    parseDid,
    appendSlice,
    formatDid,
} from "did-resolver";
```

## Core Types & Constants

```cp
class DidUrl { did: string, method: string, id: string, path: string, /* +3 fields */ }
```

## API Signatures

```cp
fn isValidMethod(method: string): bool;
fn isValidMethodId(id: string): bool;
fn parseDid(input: string): DidUrl;
fn appendSlice(out: []u8, pos: usize, s: string): i64;
fn formatDid(method: string, id: string, fragment: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { isValidMethod } from "did-resolver";

pub fn main() {
    let result = isValidMethod("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"did-resolver"`; install via `cap add did-resolver`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
