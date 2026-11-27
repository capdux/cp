# cexpr — AI Agent Quick Ref

Zero-allocation C constant expression parser and evaluator (full C operator precedence, hex/bin/oct/char literals) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    EvalResult,
    Parser,
    skipWs,
    peek,
    peek2,
    absI64,
    signedDivI64,
    signedRemI64,
} from "cexpr";
```

## Core Types & Constants

```cp
class EvalResult { value: i64, ok: bool }
class Parser { pos: usize, ok: bool, depth: usize }
```

## API Signatures

```cp
fn skipWs(p: &mut Parser, src: string);
fn peek(p: &mut Parser, src: string): u8;
fn peek2(p: &mut Parser, src: string): u8;
fn absI64(x: i64): u64;
fn signedDivI64(a: i64, b: i64): i64;
fn signedRemI64(a: i64, b: i64): i64;
fn skipIntSuffix(p: &mut Parser, src: string);
fn parseCharLit(p: &mut Parser, src: string): i64;
fn parseIntLit(p: &mut Parser, src: string): i64;
fn parsePrimary(p: &mut Parser, src: string): i64;
fn parseUnary(p: &mut Parser, src: string): i64;
fn parseMul(p: &mut Parser, src: string): i64;
fn parseAdd(p: &mut Parser, src: string): i64;
fn parseShift(p: &mut Parser, src: string): i64;
fn parseRel(p: &mut Parser, src: string): i64;
fn parseEq(p: &mut Parser, src: string): i64;
fn parseBitAnd(p: &mut Parser, src: string): i64;
fn parseBitXor(p: &mut Parser, src: string): i64;
fn parseBitOr(p: &mut Parser, src: string): i64;
fn parseLogAnd(p: &mut Parser, src: string): i64;
fn parseLogOr(p: &mut Parser, src: string): i64;
fn parseTernary(p: &mut Parser, src: string): i64;
fn evalCExpr(expr: string): EvalResult;
```

## Minimal Usage

```cp
import { parseCharLit } from "cexpr";

pub fn main() {
    let result = parseCharLit(/* p: &mut Parser */, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"cexpr"`; install via `cap add cexpr`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
