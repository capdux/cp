# marked — AI Agent Quick Ref

Zero-heap CommonMark/GFM Markdown to HTML compiler and plain-text stripper in pure CP with XSS-safe HTML escaping via escape-html (clean-room MIT counterpart to npm marked v14). (`v1.0.0`, `MIT`)

## Import

```cp
import {
    MarkedOptions,
    defaultMarkedOptions,
    mkdWriteByte,
    mkdWriteStr,
    mkdWriteEscaped,
    mkdWriteEscapedByte,
    mkdIsSpace,
    mkdIsPunctEscape,
} from "marked";
```

## Core Types & Constants

```cp
class MarkedOptions { gfm: bool, sanitize_links: bool, breaks: bool }
```

## API Signatures

```cp
fn defaultMarkedOptions(): MarkedOptions;
fn mkdWriteByte(out: []u8, pos: &mut usize, b: u8);
fn mkdWriteStr(out: []u8, pos: &mut usize, s: string);
fn mkdWriteEscaped(out: []u8, pos: &mut usize, s: string);
fn mkdWriteEscapedByte(out: []u8, pos: &mut usize, b: u8);
fn mkdIsSpace(c: u8): bool;
fn mkdIsPunctEscape(c: u8): bool;
fn mkdStartsWithIgnoreCaseClean(url: string, prefix: string): bool;
fn mkdIsSafeUrl(url: string): bool;
fn mkdFindLineEnd(s: string, start: usize): usize;
fn mkdTrimCr(s: string, start: usize, end: usize): usize;
fn mkdSkipIndent(s: string, start: usize, end: usize, max_spaces: usize): usize;
fn mkdIsBlankLine(s: string, start: usize, end: usize): bool;
fn mkdMatchHeading(s: string, line_start: usize, line_end: usize, content_start: &mut usize, content_end: &mut usize): usize;
fn mkdIsHorizontalRule(s: string, line_start: usize, line_end: usize): bool;
fn mkdMatchFenceOpen(s: string, line_start: usize, line_end: usize, fence_char: &mut u8, lang_start: &mut usize, lang_end: &mut usize): usize;
fn mkdIsFenceClose(s: string, line_start: usize, line_end: usize, fence_char: u8, min_len: usize): bool;
fn mkdIsBlockquoteLine(s: string, line_start: usize, line_end: usize): bool;
fn mkdMatchUnorderedItem(s: string, line_start: usize, line_end: usize): usize;
fn mkdMatchOrderedItem(s: string, line_start: usize, line_end: usize, num_out: &mut usize): usize;
fn mkdStartsNewBlock(s: string, line_start: usize, line_end: usize): bool;
fn mkdFindClosingDouble(s: string, start: usize, ch: u8): i64;
fn mkdFindClosingSingle(s: string, start: usize, ch: u8): i64;
fn mkdMatchLinkOrImage(s: string, open_bracket: usize, label_start: &mut usize, label_end: &mut usize, url_start: &mut usize, url_end: &mut usize): usize;
fn mkdIsInlineSpecial(c: u8): bool;
fn mkdParseInlineRec(src: string, opts: MarkedOptions, out: []u8, pos: &mut usize, depth: usize);
fn mkdStripBlockquoteLines(md: string, start: usize, end: usize, dst: []u8): usize;
fn mkdParseBlocksRec(md: string, opts: MarkedOptions, out: []u8, pos: &mut usize, depth: usize);
fn mkdStripInlineRec(src: string, out: []u8, pos: &mut usize, depth: usize);
fn parseMarkdown(md: string, out: []u8): usize;
fn parseMarkdownWithOptions(md: string, opts: MarkedOptions, out: []u8): usize;
fn parseInlineMarkdown(src: string, out: []u8): usize;
fn stripMarkdown(md: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseMarkdown } from "marked";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = parseMarkdown("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"marked"`; install via `cap add marked`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
