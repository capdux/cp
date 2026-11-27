# stylis — AI Agent Quick Ref

Zero-heap CSS minifier and nested selector preprocessor in pure CP (`v4.3.6`, `MIT`)

## Import

```cp
import {
    stylIsWs,
    stylPushByte,
    stylIsStripAfter,
    stylIsStripBefore,
    minifyCss,
    stylExpandSingle,
    expandNestedSelector,
    stylSkipCommentOrQuote,
} from "stylis";
```

## API Signatures

```cp
fn stylIsWs(c: u8): bool;
fn stylPushByte(out: []u8, pos: usize, b: u8): usize;
fn stylIsStripAfter(c: u8): bool;
fn stylIsStripBefore(c: u8): bool;
fn minifyCss(css: string, out: []u8): usize;
fn stylExpandSingle(parent: string, child: string, out: []u8, start_pos: usize): usize;
fn expandNestedSelector(parent: string, child: string, out: []u8): usize;
fn stylSkipCommentOrQuote(s: string, idx: usize): usize;
fn compileNestedCss(parent_sel: string, body_css: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { minifyCss } from "stylis";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = minifyCss("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"stylis"`; install via `cap add stylis`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
