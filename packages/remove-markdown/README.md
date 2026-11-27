[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# remove-markdown

Zero-allocation Markdown to plain-text stripper and UTF-8 safe excerpt generator for CP — clean-room MIT parity with npm `remove-markdown` and `strip-markdown`.

Strips ATX/Setext headings, horizontal rules, emphasis (`**bold**`, `_italic_`, `~~strike~~`), inline code, links/images, fenced code blocks, blockquotes, lists/task lists, and HTML tags while preserving `snake_case` identifiers and math operators (`a < b`, `2 * 3`) — ideal for full-text search indexing, preview snippets, and LLM context trimming.

## Provenance & Zero-Cost Migration

| Attribute | Value |
| --- | --- |
| Source Platform | **npm** |
| Original Package | `remove-markdown` (CP `v1.0.0`) |
| Upstream Repo | `https://github.com/stiang/remove-markdown` |
| License | MIT |

Keep the exact package name when migrating from TypeScript/Node.js to CP:

```ts
// TypeScript / Node.js (npm: remove-markdown)
import removeMd from "remove-markdown";
const plain = removeMd("# Hello **CP**\n\nVisit [CapDux](https://capdux.org)!");
```

```cp
// CP (cap: remove-markdown) — zero-allocation plain-text stripper & excerpt
import { stripMarkdown } from "remove-markdown";

let buf: [1024]u8 = undefined;
const plain = stripMarkdown("# Hello **CP**\n\nVisit [CapDux](https://capdux.org)!", buf[0..1024]);
```

## Installation

```bash
cap add remove-markdown
```

## Usage

### Basic Usage: Plain Text, Single-Line Index & UTF-8 Safe Excerpt

```cp
import { stripMarkdown, stripToSingleLine, excerpt } from "remove-markdown";

fn demoStrip() {
    let b1: [1024]u8 = undefined;
    let b2: [1024]u8 = undefined;
    let b3: [128]u8 = undefined;

    const plain = stripMarkdown("# Hello **CP**\n\nVisit [CapDux](https://capdux.org)!", b1[0..1024]);
    // => "Hello CP\n\nVisit CapDux!"

    // Compact single-line text for search indexes or embeddings
    const line = stripToSingleLine("## Title\n- [x] **Fast** & `zero-alloc`", b2[0..1024]);
    // => "Title Fast & zero-alloc"

    // UTF-8 boundary-safe excerpt (never splits multi-byte CJK/emoji sequences)
    const summary = excerpt("# Long Article\n\nThis is a **very long** markdown document.", 24 as usize, b3[0..128]);
    // => "Long Article This is..."
    _ = plain;
    _ = line;
    _ = summary;
}
```

### Edge Cases: Custom `StripOptions`, Code Blocks, Images & `snake_case` Preservation

```cp
import { StripOptions, stripMarkdownWith } from "remove-markdown";

fn demoOptions() {
    let buf: [1024]u8 = undefined;

    let opts = StripOptions.defaults();
    opts.keep_img_alt = false;          // drop ![alt](url) completely
    opts.keep_code_block_text = false;  // drop fenced code block bodies
    opts.strip_html = true;             // strip <tag>...</tag>
    opts.collapse_whitespace = true;    // collapse runs of spaces

    const md = "Check `my_var_name` and ![logo](img.png) <span>ok</span>\n```cp\nlet x = 1;\n```";
    const out = stripMarkdownWith(md, opts, buf[0..1024]);
    // Preserves "my_var_name" underscores while removing image alt and code fence body
    _ = out;
}
```

## API Reference

| Symbol | Signature | Description |
| --- | --- | --- |
| `StripOptions` | `class { keep_img_alt: bool, keep_code_block_text: bool, strip_html: bool, collapse_whitespace: bool }` | Stripping behavior options with `StripOptions.defaults()`. |
| `stripMarkdown` | `fn(md: string, out: []u8): string` | Strips Markdown to clean multiline plain text using default options. |
| `stripMarkdownWith` | `fn(md: string, opts: StripOptions, out: []u8): string` | Strips Markdown using custom `StripOptions`. |
| `stripToSingleLine` | `fn(md: string, out: []u8): string` | Strips Markdown and collapses line breaks into a compact single line. |
| `excerpt` | `fn(md: string, max_bytes: usize, out: []u8): string` | Produces a UTF-8 safe single-line excerpt truncated to `<= max_bytes` (appending `...` when truncated). |

## Performance & Safety Highlights

- **16-lane SIMD Acceleration**: Uses `cap:ascii` (`isSpace`, `isAlpha`, `isAlnum`, `isDigit`, `trim`, `findByte`) for fast line scanning and bracket/delimiter lookup.
- **Zero Heap Allocation**: Operates in a single pass directly into caller-provided `out: []u8` stack buffers without regex engines or heap allocations.
- **Zero-Clear Caller Buffers (`undefined`)**: Accepts stack buffers initialized with `undefined`; every write is strictly bounds-checked against `out.len`.
- **UTF-8 & Recursion Safe**: `excerpt` backs off to valid UTF-8 codepoint boundaries, and inline stripping enforces a recursion depth guard.

## License

MIT — Clean-room CP implementation inspired by npm `remove-markdown`.
