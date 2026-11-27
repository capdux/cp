# combine@1.0.0 (Agent Quickref)

> Zero-copy streaming parser combinators with line/column cursor tracking and backtracking (Rust combine v4 parity) for CP | License: MIT
> Import: `import { ... } from "combine";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum CharPred`

## Functions
- `export fn cmbIsAlpha(ch: u8): bool;`
- `export fn cmbIsDigit(ch: u8): bool;`
- `export fn cmbIsSpace(ch: u8): bool;`
- `export fn cmbIsHex(ch: u8): bool;`
- `export fn cmbMatchPred(ch: u8, kind: CharPred): bool;`
- `export fn cmbStartsWithAt(hay: string, pos: usize, needle: string): bool;`
- `export fn tag(cursor: &mut ParseCursor, lit: string): bool;`
- `export fn charByte(cursor: &mut ParseCursor, expected: u8): bool;`
- `export fn takeWhile(cursor: &mut ParseCursor, pred_kind: CharPred): string;`
- `export fn takeWhile1(cursor: &mut ParseCursor, pred_kind: CharPred): string?;`
- `export fn takeUntil(cursor: &mut ParseCursor, delim: string): string?;`
- `export fn skipSpaces(cursor: &mut ParseCursor): usize;`
- `export fn parseU64(cursor: &mut ParseCursor): u64?;`
- `export fn parseI64(cursor: &mut ParseCursor): i64?;`
- `export fn parseQuotedString(cursor: &mut ParseCursor): string?;`
- `export fn delimited(cursor: &mut ParseCursor, open: string, close: string): string?;`
- `export fn separatedList(cursor: &mut ParseCursor, sep: string, out: []string): usize;`
