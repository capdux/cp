# mathjs — AI Agent Quick Ref

Zero-allocation mathematical expression parser & evaluator (npm mathjs parity) composing libm with scoped variables, constants, 14 built-in functions, and strict error reporting for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ScopeVar,
    ScopeLookup,
    EvalResult,
    MathScope,
    MjParser,
    mjStrEq,
    mjCopyBuf,
    mjIsSpace,
} from "mathjs";
```

## Core Types & Constants

```cp
class ScopeVar { name: [32]u8, name_len: usize, value: f64 }
class ScopeLookup { found: bool, value: f64 }
class EvalResult { ok: bool, value: f64, err_code: i32 }
class MathScope { vars: [32]ScopeVar, count: usize } // methods: new(...) +2
class MjParser { src: string, pos: usize, depth: usize, scope: MathScope }
```

## API Signatures

```cp
fn mjStrEq(a: string, b: string): bool;
fn mjCopyBuf(dst: []u8, src: string): usize;
fn mjIsSpace(ch: u8): bool;
fn mjIsDigit(ch: u8): bool;
fn mjIsIdentStart(ch: u8): bool;
fn mjIsIdentChar(ch: u8): bool;
fn mjOk(v: f64): EvalResult;
fn mjErr(code: i32): EvalResult;
fn mjSkipWs(p: &mut MjParser);
fn mjParseNumber(p: &mut MjParser): EvalResult;
fn mjEvalBuiltin(name: string, args: &[]f64, argc: usize): EvalResult;
fn mjParsePrimary(p: &mut MjParser): EvalResult;
fn mjParsePower(p: &mut MjParser): EvalResult;
fn mjParseUnary(p: &mut MjParser): EvalResult;
fn mjParseMulDiv(p: &mut MjParser): EvalResult;
fn mjParseAddSub(p: &mut MjParser): EvalResult;
fn evalExprWithScope(expr: string, scope: MathScope): EvalResult;
fn evalExpr(expr: string): EvalResult;
```

## Minimal Usage

```cp
import { mjStrEq } from "mathjs";

pub fn main() {
    let result = mjStrEq("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"mathjs"`; install via `cap add mathjs`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
