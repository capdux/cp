[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# codespan-reporting

Zero-heap Rustc-style multi-line source code diagnostic renderer with line numbers, gutter, and caret underlines

| Metadata | Value |
| --- | --- |
| Package | `codespan-reporting` |
| Version | `0.11.1` |
| License | `Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add codespan-reporting
```

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

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `DiagSeverity` | `Bug, Error, Warning, Note, Help` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `LineCol` | `class` | Fields: `line: usize, col: usize, line_start: usize, line_end: usize` |
| `Diagnostic` | `class` | Fields: `severity: DiagSeverity, code: string, message: string, file_name: string, span_start: usize, span_end: usize, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `csrDigitWidth` | `(val: usize): usize` | Invokes `csrDigitWidth` and returns `usize`. |
| `severityLabel` | `(sev: DiagSeverity): string` | Invokes `severityLabel` and returns `string`. |
| `locateOffset` | `(source: string, offset: usize): LineCol` | Invokes `locateOffset` and returns `LineCol`. |
| `renderDiagnostic` | `(source: string, diag: Diagnostic, out: []u8): usize` | Invokes `renderDiagnostic` and returns `usize`. |

## Minimal Example

```cp
import { locateOffset } from "codespan-reporting";

pub fn main() {
    let result = locateOffset("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
