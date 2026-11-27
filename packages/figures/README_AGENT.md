# figures@1.0.0 (Agent Quickref)

> Zero-allocation Unicode terminal symbols with automatic ASCII/Windows fallbacks composing @cap/is-unicode-supported in pure CP | License: MIT
> Import: `import { ... } from "figures";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum FigureKind`

## Functions
- `export fn figStartsAt(input: string, pos: usize, needle: string): bool;`
- `export fn figWriteStr(out: []u8, pos: usize, s: string): usize;`
- `export fn figureFor(kind: FigureKind, unicode_ok: bool): string;`
- `export fn figureForEnv(kind: FigureKind, env: EnvInfo): string;`
- `export fn replaceSymbols(input: string, unicode_ok: bool, out: []u8): usize;`
