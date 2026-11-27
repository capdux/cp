# string-width@1.0.0 (Agent Quickref)

> Zero-allocation visual terminal column width calculator (npm string-width v7 parity) composing @cap/ansi-strip and @cap/unicode-width for CP | License: MIT
> Import: `import { ... } from "string-width";` | Full types: `src/index.d.cp`
> Deps: `strip-ansi`, `unicode-width`

## Functions
- `export fn stringWidthWithOptions(input: string, opts: StringWidthOptions): usize;`
- `export fn stringWidth(input: string): usize;`
- `export fn maxLineWidthWithOptions(multiline: string, opts: StringWidthOptions): usize;`
- `export fn maxLineWidth(multiline: string): usize;`
- `export fn padEndVisible(input: string, target_cols: usize, fill_byte: u8, out: []u8): usize;`
- `export fn padStartVisible(input: string, target_cols: usize, fill_byte: u8, out: []u8): usize;`
