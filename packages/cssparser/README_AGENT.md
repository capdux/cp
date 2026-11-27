# cssparser@0.34.0 (Agent Quickref)

> Zero-heap CSS Syntax Level 3 tokenizer and lexer in pure CP | License: MIT
> Import: `import { ... } from "cssparser";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum CssTokKind`

## Functions
- `export fn csspIsWs(c: u8): bool;`
- `export fn csspIsDigit(c: u8): bool;`
- `export fn csspIsIdentStart(c: u8): bool;`
- `export fn csspIsIdentChar(c: u8): bool;`
- `export fn csspStartsIdent(s: string, pos: usize): bool;`
- `export fn csspStartsNumber(s: string, pos: usize): bool;`
- `export fn csspScanIdent(s: string, start: usize): usize;`
- `export fn csspScanNumber(s: string, start: usize): usize;`
- `export fn csspSkipWsAndComments(s: string, start: usize): usize;`
