[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# highlight.js

Zero-allocation multi-language lexer and syntax highlighter for Code (CP/TS/Rust/C), JSON, and SQL with ANSI terminal coloring and XSS-safe HTML `<span class="hl-...">` output for CP — clean-room MIT parity with npm `highlight.js` and `prismjs`.

Tokenizes source code into zero-copy `Token` spans (`{ kind, start, len }`) or renders directly into caller-provided stack buffers as ANSI escape sequences or HTML-escaped `<span>` markup, with deterministic `-1` overflow signaling and zero heap allocations.

## Provenance & Zero-Cost Migration

| Attribute | Value |
| --- | --- |
| Source Platform | **npm** |
| Original Package | `highlight.js` (CP `v1.0.0`) |
| Upstream Repo | `https://github.com/highlightjs/highlight.js` |
| Upstream License | `BSD-3-Clause` (CP clean-room: `MIT`) |

Keep the exact package name when migrating from TypeScript/Node.js to CP:

```ts
// TypeScript / Node.js (npm: highlight.js)
import hljs from "highlight.js";
const html = hljs.highlight("const x: i32 = 42;", { language: "typescript" }).value;
```

```cp
// CP (cap: highlight.js) — zero-allocation XSS-safe HTML & ANSI highlighting
import { LANG_CODE, highlightHtml } from "highlight.js";

const src: []const u8 = "const x: i32 = 42;";
let out: [512]u8 = undefined;
const n = highlightHtml(&src, LANG_CODE, out[0..512]);
```

## Installation

```bash
cap add highlight.js
```

## Usage

### Basic Usage: HTML & ANSI Highlighting (Code, JSON, SQL)

```cp
import {
    LANG_CODE,
    LANG_JSON,
    LANG_SQL,
    highlightHtml,
    highlightAnsi,
} from "highlight.js";

fn demoHighlight() {
    let html_buf: [512]u8 = undefined;
    let ansi_buf: [512]u8 = undefined;

    // 1. XSS-safe HTML highlighting (<, >, &, " are automatically escaped)
    const code: []const u8 = "fn add(a: i32, b: i32): i32 { return a + b; }";
    const hlen = highlightHtml(&code, LANG_CODE, html_buf[0..512]);
    // hlen >= 0 -> html_buf[0..(hlen as usize)]

    // 2. ANSI terminal highlighting for SQL & JSON
    const sql: []const u8 = "SELECT id, name FROM users WHERE id = 1;";
    const alen = highlightAnsi(&sql, LANG_SQL, ansi_buf[0..512]);

    const json: []const u8 = "{\"ok\": true, \"count\": 42}";
    const jlen = highlightHtml(&json, LANG_JSON, html_buf[0..512]);
    _ = hlen;
    _ = alen;
    _ = jlen;
}
```

### Edge Cases: Zero-Copy Tokenization & Overflow Detection (`-1`)

```cp
import {
    Token,
    LANG_CODE,
    TOK_KEYWORD,
    tokenize,
    highlightHtml,
    htmlClassFor,
    ansiCodeFor,
} from "highlight.js";

fn demoTokensAndOverflow() {
    // 1. Direct zero-copy tokenization into stack Token array
    const src: []const u8 = "let count: usize = 10; // comment";
    let tokens: [32]Token = undefined;
    const count = tokenize(&src, LANG_CODE, tokens[0..32]);
    // Each tokens[i] has { kind, start, len } referencing src[start..(start+len)]

    // 2. Overflow returns -1 cleanly (never panics or writes out of bounds)
    let tiny: [8]u8 = undefined;
    const err_n = highlightHtml(&src, LANG_CODE, tiny[0..8]);
    // err_n == -1

    const cls = htmlClassFor(TOK_KEYWORD); // "hl-kw"
    const esc = ansiCodeFor(TOK_KEYWORD);
    _ = count;
    _ = err_n;
    _ = cls;
    _ = esc;
}
```

## API Reference

| Symbol | Signature | Description |
| --- | --- | --- |
| `LANG_*` | `LANG_CODE(0)`, `LANG_JSON(1)`, `LANG_SQL(2)` | Language mode constants (`u8`). |
| `TOK_*` | `TOK_PLAIN(0)`, `TOK_KEYWORD(1)`, `TOK_STRING(2)`, `TOK_NUMBER(3)`, `TOK_COMMENT(4)`, `TOK_OPERATOR(5)`, `TOK_TYPE(6)`, `TOK_PUNCT(7)` | Token kind constants (`u8`). |
| `Token` | `class { kind: u8, start: usize, len: usize }` | Zero-copy source token span. |
| `tokenize` | `fn(src: &[]u8, lang: u8, out: []Token): usize` | Lexes `src` into contiguous `Token` spans; returns token count written. |
| `nextToken` | `fn(src: &[]u8, pos: usize, lang: u8): Token` | Reads a single token starting at byte offset `pos`. |
| `highlightAnsi` | `fn(src: &[]u8, lang: u8, dst: []u8): i64` | Writes ANSI-colored text to `dst`; returns bytes written or `-1` on overflow. |
| `highlightHtml` | `fn(src: &[]u8, lang: u8, dst: []u8): i64` | Writes XSS-escaped `<span class="hl-...">` HTML to `dst`; returns bytes written or `-1` on overflow. |
| `ansiCodeFor` / `htmlClassFor` | `fn(kind: u8): string` | Maps `TOK_*` kind to its ANSI escape prefix or HTML CSS class name. |

## Performance & Safety Highlights

- **16-lane SIMD Acceleration**: Uses `cap:ascii` (`isSpace`, `isDigit`, `isHexDigit`, `isAlpha`, `strEq`, `eqIgnoreCase`) and `cap:fmt` (`writeStr`) for fast single-pass lexing and output rendering.
- **Zero Heap Allocation**: Lexing and rendering run purely on stack arrays (`[]Token` / `[]u8`) with zero dynamic allocations.
- **Zero-Clear Stack Buffers (`undefined`)**: Callers can pass `undefined`-initialized `Token` and `u8` buffers; every write is bounds-checked.
- **XSS-Safe HTML & Explicit Overflow**: Escapes `<`, `>`, `&`, `"` automatically in `highlightHtml` and signals insufficient capacity with `-1`.

## License

MIT — Clean-room CP implementation inspired by npm `highlight.js`.
