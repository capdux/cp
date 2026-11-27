# indoc@1.0.0 (Agent Quickref)

> Zero-allocation multiline string dedent, indent, stripMargin, and newline normalization engine for CP | License: MIT
> Import: `import { ... } from "indoc";` | Full types: `src/index.d.cp`

## Functions
- `export fn isHws(c: u8): bool;`
- `export fn isLineBlank(s: string, start: usize, end: usize): bool;`
- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- `export fn stripOuterSingleBlankLines(input: string): string;`
- `export fn trimBlankLines(input: string): string;`
- `export fn minCommonIndent(input: string): usize;`
- `export fn dedent(input: string, out: []u8): string;`
- `export fn indent(input: string, prefix: string, skip_empty: bool, out: []u8): string;`
- `export fn stripMargin(input: string, margin_char: u8, out: []u8): string;`
- `export fn normalizeNewlines(input: string, out: []u8): string;`
