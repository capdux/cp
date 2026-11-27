# pino@1.0.0 (Agent Quickref)

> Zero-heap structured NDJSON logger with numeric levels (10..60), safe JSON escaping, and child fields (clean-room port of npm pino) | License: MIT
> Import: `import { ... } from "pino";` | Full types: `src/index.d.cp`
> Deps: `itoa`

## Functions
- `export fn pinoPush(out: []u8, pos: usize, s: string): usize;`
- `export fn pinoPushJsonStr(out: []u8, pos: usize, s: string): usize;`
- `export fn pinoPushU64(out: []u8, pos: usize, v: u64): usize;`
- `export fn pinoPushI64(out: []u8, pos: usize, v: i64): usize;`
- `export fn levelLabel(level: u8): string;`
