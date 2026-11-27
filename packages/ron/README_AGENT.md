# ron — AI Agent Quick Ref

Zero-heap Rusty Object Notation (RON) lexer and top-level field extractor in pure CP (`v0.8.1`, `MIT`)

## Import

```cp
import {
    RonTokenKind,
    RonToken,
    RonLexer,
    ronpIsWs,
    ronpIsDigit,
    ronpIsHexDigit,
    ronpIsIdentStart,
    ronpIsIdentCont,
} from "ron";
```

## Core Types & Constants

```cp
enum RonTokenKind { Ident, String, Int, Float, Bool, LParen, ... }
class RonToken { kind: RonTokenKind, text: string, start: usize, end: usize }
class RonLexer { src: string, pos: usize } // methods: new(...) +1
```

## API Signatures

```cp
fn ronpIsWs(c: u8): bool;
fn ronpIsDigit(c: u8): bool;
fn ronpIsHexDigit(c: u8): bool;
fn ronpIsIdentStart(c: u8): bool;
fn ronpIsIdentCont(c: u8): bool;
fn ronpEq(a: string, b: string): bool;
fn ronpSkipTrivia(src: string, start_pos: usize): usize;
fn ronpIsOpenDelim(k: RonTokenKind): bool;
fn ronpIsCloseDelim(k: RonTokenKind): bool;
fn ronGetField(ron_src: string, field_name: string): string?;
```

## Minimal Usage

```cp
import { ronpEq } from "ron";

pub fn main() {
    let result = ronpEq("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ron"`; install via `cap add ron`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
