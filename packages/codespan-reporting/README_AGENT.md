# codespan-reporting — AI Agent Quick Ref

Zero-heap Rustc-style multi-line source code diagnostic renderer with line numbers, gutter, and caret underlines (`v0.11.1`, `Apache-2.0`)

## Import

```cp
import {
    DiagSeverity,
    LineCol,
    Diagnostic,
    csrDigitWidth,
    severityLabel,
    locateOffset,
    renderDiagnostic,
} from "codespan-reporting";
```

## Core Types & Constants

```cp
enum DiagSeverity { Bug, Error, Warning, Note, Help }
class LineCol { line: usize, col: usize, line_start: usize, line_end: usize }
class Diagnostic { severity: DiagSeverity, code: string, message: string, file_name: string, /* +4 fields */ }
```

## API Signatures

```cp
fn csrDigitWidth(val: usize): usize;
fn severityLabel(sev: DiagSeverity): string;
fn locateOffset(source: string, offset: usize): LineCol;
fn renderDiagnostic(source: string, diag: Diagnostic, out: []u8): usize;
```

## Minimal Usage

```cp
import { locateOffset } from "codespan-reporting";

pub fn main() {
    let result = locateOffset("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"codespan-reporting"`; install via `cap add codespan-reporting`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
