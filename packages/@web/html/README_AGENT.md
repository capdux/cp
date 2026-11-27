# @web/html — AI Agent Quick Ref

Official HTML SSR, zero-heap XSS escaping, and HTMX helpers plugin for cap:web (`v1.0.0`, `MIT`)

## Import

```cp
import {
    HtmlConfig,
    HtmlBuilder,
    htmAsciiLower,
    htmAsciiEqlIgnoreCase,
    htmWriteByte,
    htmWriteRaw,
    htmEscapeInto,
    htmlEscape,
} from "@web/html";
```

## Core Types & Constants

```cp
class HtmlConfig { default_charset: string }
class HtmlBuilder { buf: []u8, pos: usize, overflow: bool } // methods: init(...) +8
```

## API Signatures

```cp
fn htmAsciiLower(c: u8): u8;
fn htmAsciiEqlIgnoreCase(a: string, b: string): bool;
fn htmWriteByte(out_buf: []u8, pos: &mut usize, b: u8): bool;
fn htmWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;
fn htmEscapeInto(out_buf: []u8, pos: &mut usize, input: string): bool;
fn htmlEscape(input: string, out_buf: []u8): usize?;
fn htmlIsHtmx(ctx: &Context): bool;
fn htmlHxRedirect(url: string): Response;
fn htmlHxTrigger(body_html: string, event_name: string): Response;
fn htmAfterHook(ctx: &mut Context, resp: &mut Response);
fn registerHtml(app: &mut App, cfg: HtmlConfig): &mut App;
```

## Minimal Usage

```cp
import { htmAsciiEqlIgnoreCase } from "@web/html";

pub fn main() {
    let result = htmAsciiEqlIgnoreCase("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/html"`; install via `cap add @web/html`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
