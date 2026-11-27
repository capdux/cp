# anstyle-parse@1.0.0 (Agent Quickref)

> CP package anstyle-parse | License: MIT
> Import: `import { ... } from "anstyle-parse";` | Full types: `src/index.d.cp`

## Functions
- `export fn nextToken(input: string, pos: usize): AnsiToken;`
- `export fn tokenizeAnsi(input: string, out: []AnsiToken): usize;`
- `export fn parseCsiParams(csi_seq: string): SgrParams;`
- `export fn stripAnsiToBuf(input: string, out: []u8): usize;`
- `export fn hasAnsi(input: string): bool;`
- `export fn visibleByteLen(input: string): usize;`
