# indent-string@1.0.0 (Agent Quickref)

> Zero-heap multi-line string indenter matching npm indent-string (180M+/wk) in pure CP | License: MIT
> Import: `import { ... } from "indent-string";` | Full types: `src/index.d.cp`

## Functions
- `export fn writeRepeat(out: []u8, pos: usize, unit: string, count: usize): usize;`
- `export fn isLineBlank(input: string, start: usize, end: usize): bool;`
- `export fn indentStringWith(input: string, count: usize, indent_unit: string, include_empty_lines: bool, out: []u8): usize;`
- `export fn indentString(input: string, count: usize, out: []u8): usize;`
