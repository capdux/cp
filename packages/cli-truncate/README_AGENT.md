# cli-truncate@1.0.0 (Agent Quickref)

> Zero-allocation ANSI-aware CLI string truncator (end/start/middle, CJK double-width, space boundary preference) in pure CP | License: MIT
> Import: `import { ... } from "cli-truncate";` | Full types: `src/index.d.cp`
> Deps: `slice-ansi`, `string-width`

## Types & Constants
- `export enum TruncatePosition`

## Functions
- `export fn cliTruncateInto(text: string, columns: usize, opts: TruncateOptions, out: []u8): usize;`
- `export fn cliTruncateEndInto(text: string, columns: usize, out: []u8): usize;`
- `export fn cliTruncateStartInto(text: string, columns: usize, out: []u8): usize;`
- `export fn cliTruncateMiddleInto(text: string, columns: usize, out: []u8): usize;`
