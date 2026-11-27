# html-escape — AI Agent Quick Ref

Zero-heap HTML entity escaper and unescaper (named entities + decimal/hex numeric character references) for CP (compatible with npm escape-html, he & Rust html-escape) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    appendStr,
    needsEscape,
    escapeHtml,
    encodeUtf8,
    decodeNamedEntity,
    decodeNumericEntity,
    unescapeHtml,
} from "html-escape";
```

## API Signatures

```cp
fn appendStr(dst: []u8, pos: usize, src: string): usize;
fn needsEscape(input: string): bool;
fn escapeHtml(input: string, out: []u8): usize;
fn encodeUtf8(cp: u32, out: []u8, pos: usize): usize;
fn decodeNamedEntity(name: string): u32;
fn decodeNumericEntity(body: string): u32;
fn unescapeHtml(input: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { decodeNamedEntity } from "html-escape";

pub fn main() {
    let result = decodeNamedEntity("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"html-escape"`; install via `cap add html-escape`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
