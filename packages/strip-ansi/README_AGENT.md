# strip-ansi@1.0.0 (Agent Quickref)

> Zero-heap ECMA-48 / ANSI terminal escape sequence stripper (CSI, OSC, 2-byte ESC) and visible character counter. | License: MIT
> Import: `import { ... } from "strip-ansi";` | Full types: `src/index.d.cp`
> Deps: `anstyle-parse`

## Functions
- `export fn strippedLen(input: string): usize;`
- `export fn stripAnsi(input: string, out: []u8): usize;`
- `export fn visibleAsciiCount(input: string): usize;`
