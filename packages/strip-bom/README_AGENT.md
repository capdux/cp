# strip-bom — AI Agent Quick Ref

Pure CP zero-allocation UTF-8/UTF-16/UTF-32 BOM detection, zero-copy stripping and idempotent prepending (npm strip-bom parity) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    BomInfo,
    detectBom,
    hasUtf8Bom,
    stripUtf8Bom,
    stripAnyBomOffset,
    prependUtf8Bom,
} from "strip-bom";
```

## Core Types & Constants

```cp
class BomInfo { kind: u8, byte_len: usize }
```

## API Signatures

```cp
fn detectBom(data: []u8): BomInfo;
fn hasUtf8Bom(s: string): bool;
fn stripUtf8Bom(s: string): string;
fn stripAnyBomOffset(data: []u8): usize;
fn prependUtf8Bom(s: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { hasUtf8Bom } from "strip-bom";

pub fn main() {
    let result = hasUtf8Bom("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"strip-bom"`; install via `cap add strip-bom`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
