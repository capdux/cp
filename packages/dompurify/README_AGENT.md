# dompurify — AI Agent Quick Ref

Zero-allocation HTML sanitizer stripping blocked tags, event attributes, and unsafe URI schemes (npm DOMPurify parity) in pure CP (`v1.0.0`, `Apache-2.0`)

## Import

```cp
import {
    SanitizeHtmlOptions,
    defaultSanitizeOptions,
    dmpIsNameChar,
    dmpHasPrefixCI,
    dmpWriteByte,
    dmpIsBlockedContentTag,
    dmpIsBlockedVoidTag,
    dmpIsAllowedTag,
} from "dompurify";
```

## Core Types & Constants

```cp
class SanitizeHtmlOptions { allow_links: bool, allow_images: bool, strip_comments: bool }
```

## API Signatures

```cp
fn defaultSanitizeOptions(): SanitizeHtmlOptions;
fn dmpIsNameChar(c: u8): bool;
fn dmpHasPrefixCI(s: string, prefix: string): bool;
fn dmpWriteByte(out: []u8, pos: usize, b: u8): usize;
fn dmpIsBlockedContentTag(tag: string): bool;
fn dmpIsBlockedVoidTag(tag: string): bool;
fn dmpIsAllowedTag(tag: string, opts: SanitizeHtmlOptions): bool;
fn dmpIsEventAttr(attr: string): bool;
fn dmpIsSafeUrl(url: string): bool;
fn dmpIsSafeAttr(tag: string, attr: string, val: string, opts: SanitizeHtmlOptions): bool;
fn dmpFindTagClose(html: string, start: usize): usize;
fn dmpFindClosingTagEnd(html: string, start: usize, tag: string): usize;
fn sanitizeHtml(html: string, opts: SanitizeHtmlOptions, out: []u8): usize;
fn isSafeHtml(html: string): bool;
```

## Minimal Usage

```cp
import { isSafeHtml } from "dompurify";

pub fn main() {
    let result = isSafeHtml("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"dompurify"`; install via `cap add dompurify`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
