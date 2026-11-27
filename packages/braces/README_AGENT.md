# braces@1.0.0 (Agent Quickref)

> Zero-allocation Bash/Glob brace expansion and sequence range generator (braces + fill-range parity) for CP | License: MIT
> Import: `import { ... } from "braces";` | Full types: `src/index.d.cp`
> Deps: `fill-range`

## Functions
- `export fn expandBraces(pattern: string, out: []u8): BraceResult;`
- `export fn fillRange(start: string, end: string, step: i32, out: []u8): BraceResult;`
