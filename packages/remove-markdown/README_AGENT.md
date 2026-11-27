# remove-markdown@1.0.0 — AI Agent Quick Ref

> Zero-alloc Markdown to plain-text stripper & UTF-8 safe excerpt generator | Source: `npm:remove-markdown` | License: MIT
> Import: `import { StripOptions, stripMarkdown, stripMarkdownWith, stripToSingleLine, excerpt } from "remove-markdown";` | Types: `src/index.d.cp`

## Core Types

```cp
export class StripOptions {
    keep_img_alt: bool,
    keep_code_block_text: bool,
    strip_html: bool,
    collapse_whitespace: bool,
    fn defaults(): StripOptions;
}
```

## Core Functions

- `stripMarkdown(md: string, out: []u8): string` — Multiline plain text with default options.
- `stripMarkdownWith(md: string, opts: StripOptions, out: []u8): string` — Multiline plain text with custom `StripOptions`.
- `stripToSingleLine(md: string, out: []u8): string` — Compact single-line plain text (search indexing / LLM context).
- `excerpt(md: string, max_bytes: usize, out: []u8): string` — UTF-8 boundary-safe single-line excerpt (`<= max_bytes`, appends `...` on truncation).

## Minimal Usage

```cp
import { StripOptions, stripMarkdown, stripToSingleLine, excerpt } from "remove-markdown";

let buf: [1024]u8 = undefined;
const plain = stripMarkdown("# Title\n\n**Bold** [link](https://example.com)", buf[0..1024]);
const line = stripToSingleLine("## Heading\n- [x] item", buf[0..1024]);
const brief = excerpt("# Doc\n\nLong markdown paragraph...", 32 as usize, buf[0..1024]);
```

## Pitfalls & Rules

1. **Buffer aliasing**: All four functions return a `string` slice backed by `out`; use separate buffers if holding multiple results at once.
2. **Buffer sizing**: Stripped output is normally `<= md.len` (except `excerpt` adding `...`); size `out` to at least `md.len` (or `max_bytes + 4` for `excerpt`).
3. **UTF-8 safety in `excerpt`**: `max_bytes` is a byte cap; multi-byte UTF-8 characters (CJK/emoji) are never split mid-sequence.
4. **Preserved syntax**: Underscores inside identifiers (`snake_case`) and standalone comparison/math operators (`a < b`, `2 * 3`) are intentionally preserved.
5. **No regex / ReDoS-safe**: Single-pass linear scanner with bounded inline recursion depth; safe on untrusted user Markdown.
