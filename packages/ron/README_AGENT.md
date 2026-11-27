# ron@0.8.1 (Agent Quickref)

> Zero-heap Rusty Object Notation (RON) lexer and top-level field extractor in pure CP | License: MIT
> Import: `import { ... } from "ron";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum RonTokenKind`

## Functions
- `export fn ronpIsWs(c: u8): bool;`
- `export fn ronpIsDigit(c: u8): bool;`
- `export fn ronpIsHexDigit(c: u8): bool;`
- `export fn ronpIsIdentStart(c: u8): bool;`
- `export fn ronpIsIdentCont(c: u8): bool;`
- `export fn ronpEq(a: string, b: string): bool;`
- `export fn ronpSkipTrivia(src: string, start_pos: usize): usize;`
- `export fn ronpIsOpenDelim(k: RonTokenKind): bool;`
- `export fn ronpIsCloseDelim(k: RonTokenKind): bool;`
- `export fn ronGetField(ron_src: string, field_name: string): string?;`
