# ora@1.0.0 (Agent Quickref)

> Zero-heap CLI terminal spinner animation and status line renderer (clean-room port of ora v8) | License: MIT
> Import: `import { ... } from "ora";` | Full types: `src/index.d.cp`
> Deps: `ansi-escapes`

## Functions
- `export fn oraWriteSlice(out: []u8, pos: usize, s: string): usize;`
- `export fn oraFrameCount(style: u8): usize;`
- `export fn oraFrameAt(style: u8, index: usize): string;`
- `export fn oraRenderStatus(symbol: string, msg: string, fallback: string, out: []u8): usize;`
