# extract-stack@1.0.0 (Agent Quickref)

> Zero-allocation Error.stack frame extractor (npm extract-stack v2 parity) for CP | License: MIT
> Import: `import { ... } from "extract-stack";` | Full types: `src/index.d.cp`

## Functions
- `export fn exsIsHspace(c: u8): bool;`
- `export fn exsIsFrameLine(line: string): bool;`
- `export fn exsFindFirstFrameOffset(stack: string): usize;`
- `export fn exsTrimLine(line: string): string;`
- `export fn extractStack(stack: string): string;`
- `export fn extractErrorHeader(stack: string): string;`
- `export fn extractStackLines(stack: string, out: []string): usize;`
