# wrap-ansi@1.0.0 (Agent Quickref)

> Zero-allocation ANSI-preserving word/hard wrapper and visible-column slicer with CJK double-width awareness (wrap-ansi / slice-ansi parity) for CP | License: MIT
> Import: `import { ... } from "wrap-ansi";` | Full types: `src/index.d.cp`
> Deps: `unicode-width`

## Functions
- `export fn decodeUtf8At(s: string, i: usize): Utf8Step;`
- `export fn codepointWidth(cp: u32): usize;`
- `export fn scanAnsiCsi(s: string, i: usize): usize;`
- `export fn isResetSgr(seq: string): bool;`
- `export fn startsWithZeroReset(seq: string): bool;`
- `export fn pushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn pushStr(out: []u8, pos: usize, s: string): usize;`
- `export fn visibleWidth(s: string): usize;`
- `export fn emitLineBreak(out: []u8, pos: usize, style: AnsiStyleState): usize;`
- `export fn wrapAnsi(text: string, max_cols: usize, hard_wrap: bool, out: []u8): string;`
- `export fn sliceAnsi(text: string, start_col: usize, end_col: usize, out: []u8): string;`
