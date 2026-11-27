# combine — AI Agent Quick Ref

Zero-copy streaming parser combinators with line/column cursor tracking and backtracking (Rust combine v4 parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    CharPred,
    CursorMark,
    ParseCursor,
    cmbIsAlpha,
    cmbIsDigit,
    cmbIsSpace,
    cmbIsHex,
    cmbMatchPred,
} from "combine";
```

## Core Types & Constants

```cp
enum CharPred { Alpha, Digit, AlphaNum, Space, HexDigit, Ident, ... }
class CursorMark { pos: usize, line: usize, col: usize }
class ParseCursor { input: string, pos: usize, line: usize, col: usize } // methods: new(...) +6
```

## API Signatures

```cp
fn cmbIsAlpha(ch: u8): bool;
fn cmbIsDigit(ch: u8): bool;
fn cmbIsSpace(ch: u8): bool;
fn cmbIsHex(ch: u8): bool;
fn cmbMatchPred(ch: u8, kind: CharPred): bool;
fn cmbStartsWithAt(hay: string, pos: usize, needle: string): bool;
fn tag(cursor: &mut ParseCursor, lit: string): bool;
fn charByte(cursor: &mut ParseCursor, expected: u8): bool;
fn takeWhile(cursor: &mut ParseCursor, pred_kind: CharPred): string;
fn takeWhile1(cursor: &mut ParseCursor, pred_kind: CharPred): string?;
fn takeUntil(cursor: &mut ParseCursor, delim: string): string?;
fn skipSpaces(cursor: &mut ParseCursor): usize;
fn parseU64(cursor: &mut ParseCursor): u64?;
fn parseI64(cursor: &mut ParseCursor): i64?;
fn parseQuotedString(cursor: &mut ParseCursor): string?;
fn delimited(cursor: &mut ParseCursor, open: string, close: string): string?;
fn separatedList(cursor: &mut ParseCursor, sep: string, out: []string): usize;
```

## Minimal Usage

```cp
import { cmbStartsWithAt } from "combine";

pub fn main() {
    let result = cmbStartsWithAt("example", 0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"combine"`; install via `cap add combine`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
