# strip-indent@1.0.0 (Agent Quickref)

> Zero-allocation leading whitespace stripper (npm strip-indent + min-indent parity) for CP | License: MIT
> Import: `import { ... } from "strip-indent";` | Full types: `src/index.d.cp`

## Functions
- `export fn siIsIndentChar(ch: u8): bool;`
- `export fn siCopyRange(out: []u8, pos: usize, s: string, start: usize, end: usize): usize;`
- `export fn minIndent(s: string): usize;`
- `export fn stripIndentBy(s: string, n: usize, out: []u8): usize;`
- `export fn stripIndent(s: string, out: []u8): usize;`
