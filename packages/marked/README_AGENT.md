# marked@1.0.0 — AI Agent Quick Ref

- **Import**: `import { MarkedOptions, defaultMarkedOptions, parseMarkdown, parseMarkdownWithOptions, parseInlineMarkdown, stripMarkdown } from "marked";`
- **Provenance**: npm `marked` (v14) clean-room parity | Zero-heap | License: MIT
- **Deps**: `escape-html`
- **Types**: `src/index.d.cp`

## Core Types & Functions
```cp
export class MarkedOptions {
    gfm: bool,            // GFM strikethrough ~~text~~ (default: true)
    sanitize_links: bool, // block javascript:/vbscript:/data: URLs (default: true)
    breaks: bool,         // convert single '\n' to <br> (default: false)
}

export fn defaultMarkedOptions(): MarkedOptions;
export fn parseMarkdown(md: string, out: []u8): usize;
export fn parseMarkdownWithOptions(md: string, opts: MarkedOptions, out: []u8): usize;
export fn parseInlineMarkdown(src: string, out: []u8): usize;
export fn stripMarkdown(md: string, out: []u8): usize;
```

## Supported Syntax
- **Blocks**: ATX headings (`#`..`######`), paragraphs, fenced code blocks (`` ``` `` / `~~~` with lang class), blockquotes (`>`), unordered lists (`-`/`*`/`+`), ordered lists (`1.`), horizontal rules (`---`/`***`/`___`)
- **Inline**: `**bold**` / `__bold__`, `*italic*` / `_italic_`, `~~strike~~` (GFM), `` `code` ``, `[label](url)`, `![alt](url)`, backslash escapes

## Minimal Example
```cp
import { defaultMarkedOptions, parseMarkdownWithOptions, stripMarkdown } from "marked";

let html: [1024]u8 = undefined;
let opts = defaultMarkedOptions();
opts.breaks = true;
const n = parseMarkdownWithOptions("# Hi\n**CP**", opts, html[0..]);

let plain: [256]u8 = undefined;
const pn = stripMarkdown("# Hi\n**CP**", plain[0..]); // "Hi\nCP\n"
```

## Gotchas
1. All rendering functions write into caller-provided `out: []u8` and return bytes written (clamped safely to `out.len`); allocate `2x–4x` input length for HTML output.
2. `parseInlineMarkdown` skips block-level `<p>` wrapping; use it for titles, labels, or single-line snippets.
3. Keep `sanitize_links: true` (from `defaultMarkedOptions()`) whenever rendering untrusted user Markdown.
4. Internal helpers prefixed with `mkd*` are low-level parser internals; use the 5 public entrypoints above.
