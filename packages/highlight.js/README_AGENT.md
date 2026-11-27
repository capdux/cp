# highlight.js@1.0.0 — AI Agent Quick Ref

> Zero-alloc multi-language lexer & ANSI/XSS-safe HTML syntax highlighter (Code/JSON/SQL) | Source: `npm:highlight.js` | License: MIT
> Import: `import { LANG_CODE, LANG_JSON, LANG_SQL, Token, tokenize, highlightAnsi, highlightHtml } from "highlight.js";` | Types: `src/index.d.cp`

## Core Constants & Types

```cp
// Language modes (pub const u8):
// LANG_CODE = 0, LANG_JSON = 1, LANG_SQL = 2

// Token kinds (pub const u8):
// TOK_PLAIN = 0, TOK_KEYWORD = 1, TOK_STRING = 2, TOK_NUMBER = 3,
// TOK_COMMENT = 4, TOK_OPERATOR = 5, TOK_TYPE = 6, TOK_PUNCT = 7

export class Token {
    kind: u8,
    start: usize,
    len: usize,
}
```

## Core Functions

- `tokenize(src: &[]u8, lang: u8, out: []Token): usize` — Lexes `src` into `out`; returns count of tokens written.
- `nextToken(src: &[]u8, pos: usize, lang: u8): Token` — Reads single token at byte offset `pos`.
- `highlightAnsi(src: &[]u8, lang: u8, dst: []u8): i64` — Renders ANSI colored output; returns bytes written or `-1` on overflow.
- `highlightHtml(src: &[]u8, lang: u8, dst: []u8): i64` — Renders XSS-escaped `<span class="hl-...">` HTML; returns bytes written or `-1` on overflow.
- `ansiCodeFor(kind: u8): string` | `htmlClassFor(kind: u8): string`

## Minimal Usage

```cp
import { LANG_CODE, highlightHtml, highlightAnsi } from "highlight.js";

const src: []const u8 = "const x: i32 = 42;";
let out: [512]u8 = undefined;
const n = highlightHtml(&src, LANG_CODE, out[0..512]);
if (n >= 0) {
    const html = out[0..(n as usize)];
    _ = html;
}
```

## Pitfalls & Rules

1. **Byte-slice borrow input**: `src` is `&[]u8` (`const src: []const u8 = "..."; highlightHtml(&src, ...)`).
2. **Signed `i64` return value**: `highlightAnsi` and `highlightHtml` return `-1` on buffer overflow — always check `n >= 0` before casting `n as usize` to slice `dst`.
3. **Buffer sizing**: HTML wrapping (`<span class="hl-...">...</span>` + HTML escaping) expands size significantly; allocate `dst` to at least `4x–6x` of `src.len`.
4. **3 built-in languages**: Use `LANG_CODE` (0) for CP/TS/Rust/C-like code, `LANG_JSON` (1) for JSON, `LANG_SQL` (2) for SQL.
5. **XSS safety**: `highlightHtml` escapes `<`, `>`, `&`, `"` automatically via `writeHtmlEscaped`.
