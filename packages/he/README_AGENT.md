# he — AI Agent Quick Ref

Zero-allocation HTML5 named entity & numeric character reference UTF-8 encoder/decoder and XSS escaper (he / escape-html / html-entities / Rust html-escape parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    pushByte,
    writeUtf8,
    pushHexEntity,
    needsHtmlEscape,
    escapeHtml,
    encodeNonAscii,
    lookupNamedEntity,
    decodeHtmlEntities,
} from "he";
```

## API Signatures

```cp
fn pushByte(out: []u8, pos: usize, b: u8): usize;
fn writeUtf8(out: []u8, pos: usize, raw_cp: u32): usize;
fn pushHexEntity(out: []u8, pos: usize, cp: u32): usize;
fn needsHtmlEscape(s: string): bool;
fn escapeHtml(input: string, out: []u8): string;
fn encodeNonAscii(input: string, out: []u8): string;
fn lookupNamedEntity(name: string): u32;
fn decodeHtmlEntities(input: string, out: []u8): string;
```

## Minimal Usage

```cp
import { encodeNonAscii } from "he";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeNonAscii("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"he"`; install via `cap add he`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
