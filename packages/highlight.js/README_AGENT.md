# highlight.js@1.0.0 (Agent Quickref)

> Zero-allocation multi-language lexer and syntax highlighter for Code, JSON, and SQL with ANSI and XSS-safe HTML output (highlight.js / prismjs parity) for CP | License: MIT
> Import: `import { ... } from "highlight.js";` | Full types: `src/index.d.cp`

## Functions
- `export fn tokenize(src: &[]u8, lang: u8, out: []Token): usize;`
- `export fn highlightAnsi(src: &[]u8, lang: u8, dst: []u8): i64;`
- `export fn highlightHtml(src: &[]u8, lang: u8, dst: []u8): i64;`
