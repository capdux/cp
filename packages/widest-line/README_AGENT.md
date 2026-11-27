# widest-line@1.0.0 (Agent Quickref)

> Zero-allocation widest visual terminal line width calculator (npm widest-line parity) composing @cap/string-width for CP | License: MIT
> Import: `import { ... } from "widest-line";` | Full types: `src/index.d.cp`

## Functions
- `export fn wlTrimCr(line: string): string;`
- `export fn wlLineWidthWithTab(line: string, tab_width: usize): usize;`
- `export fn countLines(s: string): usize;`
- `export fn widestLine(s: string): usize;`
- `export fn widestLineWithOptions(s: string, opts: StringWidthOptions): usize;`
- `export fn lineWidths(s: string, out: []usize): usize;`
- `export fn lineWidthsWithOptions(s: string, opts: StringWidthOptions, out: []usize): usize;`
- `export fn widestLineWithTab(s: string, tab_width: usize): usize;`
