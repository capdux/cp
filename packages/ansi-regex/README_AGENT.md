# ansi-regex@1.0.0 (Agent Quickref)

> Zero-heap ANSI escape sequence matcher & locator reusing anstyle-parse, matching npm ansi-regex (400M+/wk) in pure CP | License: MIT
> Import: `import { ... } from "ansi-regex";` | Full types: `src/index.d.cp`
> Deps: `anstyle-parse`

## Functions
- `export fn testAnsi(input: string): bool;`
- `export fn findFirstAnsi(input: string): AnsiSpan;`
- `export fn findAllAnsi(input: string, out: []AnsiSpan): usize;`
- `export fn countAnsiSequences(input: string): usize;`
- `export fn stripAllAnsi(input: string, out: []u8): usize;`
