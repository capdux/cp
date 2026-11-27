# js-tokens — AI Agent Quick Ref

Zero-allocation tiny JavaScript/TypeScript lexer tokenizer (clean-room MIT implementation of npm js-tokens, #61 / 381M wk) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    JsTokenKind,
    JsToken,
    jstIsIdentStart,
    jstIsIdentPart,
    jstIsDigit,
    jstIsHex,
    jstIsSpace,
    jstStartsWith,
} from "js-tokens";
```

## Core Types & Constants

```cp
enum JsTokenKind { IdentifierName, NumericLiteral, StringLiteral, NoSubstitutionTemplate, TemplateHead, Punctuator, ... }
class JsToken { kind: JsTokenKind, start: usize, end: usize, closed: bool } // methods: text(...)
```

## API Signatures

```cp
fn jstIsIdentStart(b: u8): bool;
fn jstIsIdentPart(b: u8): bool;
fn jstIsDigit(b: u8): bool;
fn jstIsHex(b: u8): bool;
fn jstIsSpace(b: u8): bool;
fn jstStartsWith(src: string, pos: usize, lit: string): bool;
fn nextJsToken(src: string, pos: usize): JsToken?;
fn tokenizeJs(src: string, out: []JsToken): usize;
fn countJsTokens(src: string): usize;
```

## Minimal Usage

```cp
import { jstStartsWith } from "js-tokens";

pub fn main() {
    let result = jstStartsWith("example", 0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"js-tokens"`; install via `cap add js-tokens`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
