# line-column — AI Agent Quick Ref

Zero-allocation byte offset <-> 1-based (line, col) locator, zero-copy line extractor, and LF/CRLF line counter in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    LineCol,
    offsetToLineCol,
    lineColToOffset,
    getLine,
    countLines,
} from "line-column";
```

## Core Types & Constants

```cp
class LineCol { line: usize, col: usize, valid: bool }
```

## API Signatures

```cp
fn offsetToLineCol(text: string, offset: usize): LineCol;
fn lineColToOffset(text: string, line: usize, col: usize): i64;
fn getLine(text: string, line_1based: usize): string;
fn countLines(text: string): usize;
```

## Minimal Usage

```cp
import { getLine } from "line-column";

pub fn main() {
    let result = getLine("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"line-column"`; install via `cap add line-column`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
