# @cap/cmark — AI Agent Quick Ref

CommonMark 0.31.1 official C reference Markdown to HTML/XML/LaTeX renderer for CP (`v0.31.1`, `BSD-2-Clause`)

## Import

```cp
import {
    CmarkError,
    HtmlString,
    cmark_markdown_to_html,
    cmark_version,
    cmark_version_string,
    free,
    versionString,
    versionNumber,
} from "@cap/cmark";
```

## Core Types & Constants

```cp
error CmarkError { RenderFailed }
class HtmlString { raw: [*:0]u8!?, len: usize } // methods: slice(...) +1
```

## API Signatures

```cp
fn cmark_markdown_to_html(text: [*]&u8!, len: usize, options: i32): [*:0]u8!?;
fn cmark_version(): i32;
fn cmark_version_string(): [*:0]&u8!?;
fn free(ptr: *auto!?);
fn versionString(): string;
fn versionNumber(): i32;
fn markdownToHtml(md: string, opts: i32): !HtmlString;
```

## Minimal Usage

```cp
import { markdownToHtml } from "@cap/cmark";

pub fn main() {
    let result = markdownToHtml("example", 0) catch return;
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/cmark"`; install via `cap add @cap/cmark`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.
