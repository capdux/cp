# ansi-align@1.0.0 (Agent Quickref)

> Zero-allocation ANSI-aware multiline text alignment (npm ansi-align parity) composing @cap/string-width and @cap/widest-line for CP | License: ISC
> Import: `import { ... } from "ansi-align";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum AlignMode`

## Functions
- `export fn aaWriteSpaces(out: []u8, pos: usize, count: usize): usize;`
- `export fn aaCopyRange(out: []u8, pos: usize, s: string, start: usize, end: usize): usize;`
- `export fn alignAnsi(text: string, mode: AlignMode, out: []u8): usize;`
- `export fn alignCenter(text: string, out: []u8): usize;`
- `export fn alignRight(text: string, out: []u8): usize;`
- `export fn alignLeft(text: string, out: []u8): usize;`
