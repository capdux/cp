# escape-html — AI Agent Quick Ref

Zero-heap single-pass HTML entity escaper and unescaper (&, <, >, ", ') in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import { needsHtmlEscape, escapedHtmlLen, escapeHtml, unescapeHtml } from "escape-html";
```

## API Signatures

```cp
fn needsHtmlEscape(input: string): bool;
fn escapedHtmlLen(input: string): usize;
fn escapeHtml(input: string, out: []u8): usize;
fn unescapeHtml(input: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { needsHtmlEscape } from "escape-html";

pub fn main() {
    let result = needsHtmlEscape("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"escape-html"`; install via `cap add escape-html`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add escape-html
cpc check src/main.cp
```
