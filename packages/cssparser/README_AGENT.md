# cssparser — AI Agent Quick Ref

Zero-heap CSS Syntax Level 3 tokenizer and lexer in pure CP (`v0.34.0`, `MIT`)

## Import

```cp
import {
    CssTokKind,
    CssToken,
    CssLexer,
    csspIsIdentStart,
    csspIsIdentChar,
    csspStartsIdent,
    csspStartsNumber,
    csspScanIdent,
} from "cssparser";
```

## Core Types & Constants

```cp
enum CssTokKind { Ident, AtKeyword, Hash, String, Number, Percentage, ... }
class CssToken { kind: CssTokKind, text: string, unit: string, start: usize, /* +1 fields */ }
class CssLexer { src: string, pos: usize } // methods: new(...) +1
```

## API Signatures

```cp
fn csspIsIdentStart(c: u8): bool;
fn csspIsIdentChar(c: u8): bool;
fn csspStartsIdent(s: string, pos: usize): bool;
fn csspStartsNumber(s: string, pos: usize): bool;
fn csspScanIdent(s: string, start: usize): usize;
fn csspScanNumber(s: string, start: usize): usize;
fn csspSkipWsAndComments(s: string, start: usize): usize;
```

## Minimal Usage

```cp
import { csspStartsIdent } from "cssparser";

pub fn main() {
    let result = csspStartsIdent("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"cssparser"`; install via `cap add cssparser`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
