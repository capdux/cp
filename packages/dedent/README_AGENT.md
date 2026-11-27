# dedent@1.0.0 (Agent Quickref)

> Zero-heap common leading indentation detector and dedent engine for multiline strings (compatible with npm strip-indent, dedent & Rust indoc) | License: MIT
> Import: `import { ... } from "dedent";` | Full types: `src/index.d.cp`

## Functions
- `export fn isHorizSpace(c: u8): bool;`
- `export fn minIndent(input: string): usize;`
- `export fn stripIndent(input: string, out: []u8): usize;`
- `export fn indentLines(input: string, spaces: usize, out: []u8): usize;`
