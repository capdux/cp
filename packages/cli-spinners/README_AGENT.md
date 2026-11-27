# cli-spinners@1.0.0 (Agent Quickref)

> Zero-allocation terminal spinner frame presets and Unicode/ASCII status symbols (cli-spinners / ora / log-symbols parity) for CP | License: MIT
> Import: `import { ... } from "cli-spinners";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum SpinnerKind`
- `export enum SymbolKind`

## Functions
- `export fn intervalMs(kind: SpinnerKind): u32;`
- `export fn frameCount(kind: SpinnerKind): usize;`
- `export fn frameAt(kind: SpinnerKind, tick: usize): string;`
- `export fn frameAtTime(kind: SpinnerKind, elapsed_ms: u64): string;`
- `export fn findSpinnerByName(name: string): SpinnerKind?;`
- `export fn logSymbol(kind: SymbolKind, ascii_only: bool): string;`
- `export fn renderSpinnerLine(kind: SpinnerKind, tick: usize, text: string, out: []u8): string;`
- `export fn renderStatusLine(sym: SymbolKind, ascii_only: bool, text: string, out: []u8): string;`
