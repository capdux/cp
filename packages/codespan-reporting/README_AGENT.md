# codespan-reporting@0.11.1 (Agent Quickref)

> Zero-heap Rustc-style multi-line source code diagnostic renderer with line numbers, gutter, and caret underlines | License: Apache-2.0
> Import: `import { ... } from "codespan-reporting";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum DiagSeverity`

## Functions
- `export fn severityLabel(sev: DiagSeverity): string;`
- `export fn locateOffset(source: string, offset: usize): LineCol;`
- `export fn renderDiagnostic(source: string, diag: Diagnostic, out: []u8): usize;`
