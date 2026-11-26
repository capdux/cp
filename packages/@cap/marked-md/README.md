# `@cap/marked-md`

Zero-heap CommonMark/GFM Markdown to HTML compiler and plain-text stripper in pure CP (clean-room MIT counterpart to npm `marked` v14).

## Features

- **100% Zero Heap Allocation**: Compiles Markdown directly into caller-provided `out: []u8`, returning the exact byte count written.
- **Cross-Package Reuse**: Reuses `escapeHtml` from `@cap/escape-html` for XSS-safe HTML text and attribute escaping.
- **Block-Level Syntax**:
  - ATX Headings (`#` .. `######` -> `<h1>` .. `<h6>`, with optional closing `#` stripping)
  - Fenced Code Blocks (```` ```lang ```` / `~~~lang` -> `<pre><code class="language-...">`)
  - Blockquotes (`> ...` -> `<blockquote>`, with recursive inner block parsing)
  - Unordered Lists (`-`, `*`, `+` -> `<ul><li>`, plus GFM task list checkboxes `- [ ]` / `- [x]`)
  - Ordered Lists (`1. `, `2) ` -> `<ol><li>`, preserving `start="N"` when `N != 1`)
  - Horizontal Rules (`---`, `***`, `___` -> `<hr>`)
  - Paragraphs (`<p>...</p>`)
- **Inline Syntax**:
  - Inline code spans (`` `code` `` and multi-backtick `` ``code`` ``)
  - Bold (`**...**`, `__...__`), Italic (`*...*`, `_..._`), Combined (`***...***`)
  - GFM Strikethrough (`~~...~~` -> `<del>`)
  - Links (`[text](url)` -> `<a href="...">`, with XSS protocol filtering for `javascript:`, `vbscript:`, `data:text/html`)
  - Images (`![alt](src)` -> `<img src="..." alt="...">`)
  - Backslash escapes (`\*`, `\_`, `` \` ``, etc.) and hard line breaks (trailing two spaces or `\`, plus `opts.breaks`)
- **Plain-Text Extraction**: `stripMarkdown(md, out)` strips block & inline Markdown syntax to clean plain text.

## Dependencies

- `@cap/escape-html` (`1.0.0`)

## Quick Start

```cp
import { parseMarkdown, parseInlineMarkdown, stripMarkdown, defaultMarkedOptions } from "@cap/marked-md";

let out: [1024]u8 = undefined;
const n = parseMarkdown("# Hello **World**\n\n- [x] Zero heap\n- `marked-md`", &out);
// <h1>Hello <strong>World</strong></h1>
// <ul>
// <li><input checked="" disabled="" type="checkbox"> Zero heap</li>
// <li><code>marked-md</code></li>
// </ul>
```

## API

- `MarkedOptions { gfm: bool, sanitize_links: bool, breaks: bool }`
- `defaultMarkedOptions(): MarkedOptions`
- `parseMarkdown(md: string, out: []u8): usize`
- `parseMarkdownWithOptions(md: string, opts: MarkedOptions, out: []u8): usize`
- `parseInlineMarkdown(src: string, out: []u8): usize`
- `stripMarkdown(md: string, out: []u8): usize`

## License

MIT
