[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `marked`

Zero-heap CommonMark / GFM Markdown-to-HTML compiler and plain-text stripper in pure CP — clean-room parity with **npm `marked` v14**, with built-in safe HTML escaping via `escape-html` and link scheme sanitization.

## Provenance & Zero-Cost Migration

| Field | Details |
| :--- | :--- |
| **Source Registry** | `npm:marked` ([markedjs/marked](https://github.com/markedjs/marked), v14 parity) |
| **CP Package** | `marked@1.0.0` (`MIT`) |
| **Dependencies** | `escape-html@1.0.0` |

Keep the exact original package name — no renaming required:

```ts
// TypeScript (npm marked)
import { marked } from "marked";
const html = marked.parse("# Hello **CP**");
```

```cp
// CP — identical package name, zero-heap caller buffer
import { parseMarkdown } from "marked";

let out: [512]u8 = undefined;
const n = parseMarkdown("# Hello **CP**\n", out[0..]);
// out[0..n] == "<h1>Hello <strong>CP</strong></h1>\n"
```

## Architecture & Zero-Heap Design

- **Zero dynamic allocations**: Single-pass recursive block & inline compiler writing directly into caller-provided `out: []u8`, with bounded recursion depth and compile-time 256-entry ASCII lookup tables (`MKD_PUNCT_ESCAPE_LUT`, `MKD_INLINE_SPECIAL_LUT`).
- **Safe by default**: Escapes raw HTML entities via `escape-html` and neutralizes unsafe link schemes (`javascript:`, `vbscript:`, `data:`) when `sanitize_links: true` (default).
- **Rich CommonMark + GFM support**: ATX headings (`#`..`######`), fenced code blocks with language class, blockquotes, ordered/unordered lists, horizontal rules, emphasis (`*`/`_`), strong (`**`/`__`), GFM strikethrough (`~~`), inline code, links/images, and plain-text stripping.

## Examples

### 1. Basic Markdown-to-HTML Compilation (`parseMarkdown` & `parseInlineMarkdown`)

```cp
import { parseMarkdown, parseInlineMarkdown } from "marked";

fn demoBasic() {
    const md = "# Title\n\nHello **bold**, *italic*, ~~old~~, and `code`.\n\n- item 1\n- item 2\n";
    let html_buf: [1024]u8 = undefined;
    const n = parseMarkdown(md, html_buf[0..]);
    // html_buf[0..n] contains rendered HTML

    // Inline-only rendering (no wrapping <p>...</p>)
    let inline_buf: [256]u8 = undefined;
    const in_len = parseInlineMarkdown("Visit [CP](https://cp-lang.org)", inline_buf[0..]);
    _ = n; _ = in_len;
}
```

### 2. Custom `MarkedOptions` (`gfm`, `sanitize_links`, `breaks`)

```cp
import { MarkedOptions, defaultMarkedOptions, parseMarkdownWithOptions } from "marked";

fn demoOptions() {
    let opts = defaultMarkedOptions();
    opts.breaks = true;         // render single newlines as <br>
    opts.gfm = true;            // enable GFM strikethrough ~~text~~
    opts.sanitize_links = true; // block unsafe link schemes

    let out: [512]u8 = undefined;
    const n = parseMarkdownWithOptions("line1\nline2\n\n[link](javascript:void(0))", opts, out[0..]);
    _ = n;
}
```

### 3. Plain-Text Stripping (`stripMarkdown`) & Buffer Sizing

```cp
import { stripMarkdown, parseMarkdown } from "marked";

fn demoStripAndSizing() {
    const doc = "## Release Notes\n\nSee **[docs](https://cp-lang.org)** and `cap build`.\n";

    // Extract clean plain text (ideal for search indexing / LLM context / previews)
    let text_buf: [512]u8 = undefined;
    const tn = stripMarkdown(doc, text_buf[0..]);
    // text_buf[0..tn] == "Release Notes\nSee docs and cap build.\n"

    // Buffer sizing rule: allocate ~2x–4x input length for HTML output;
    // writes clamp safely to out.len without memory overrun.
    let small_buf: [32]u8 = undefined;
    const clamped = parseMarkdown(doc, small_buf[0..]);
    _ = tn; _ = clamped;
}
```

## API Overview

- `MarkedOptions` (`gfm: bool`, `sanitize_links: bool`, `breaks: bool`)
- `defaultMarkedOptions(): MarkedOptions` (`gfm: true, sanitize_links: true, breaks: false`)
- `parseMarkdown(md: string, out: []u8): usize`
- `parseMarkdownWithOptions(md: string, opts: MarkedOptions, out: []u8): usize`
- `parseInlineMarkdown(src: string, out: []u8): usize`
- `stripMarkdown(md: string, out: []u8): usize`

See [`src/index.d.cp`](src/index.d.cp) for full signatures.
