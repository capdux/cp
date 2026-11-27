# line-column@1.0.0 (Agent Quickref)

> Zero-allocation byte offset <-> 1-based (line, col) locator, zero-copy line extractor, and LF/CRLF line counter in pure CP | License: MIT
> Import: `import { ... } from "line-column";` | Full types: `src/index.d.cp`

## Functions
- `export fn offsetToLineCol(text: string, offset: usize): LineCol;`
- `export fn lineColToOffset(text: string, line: usize, col: usize): i64;`
- `export fn getLine(text: string, line_1based: usize): string;`
- `export fn countLines(text: string): usize;`
