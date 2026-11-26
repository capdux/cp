// Type definitions for CP

import { PI, TAU, E, fabs, floor, ceil, round, fmod, fmin, fmax, sqrt, exp, ln, pow, sin, cos, tan } from "../../libm/src/index.cp";
export fn mjStrEq(a: string, b: string): bool;
export fn mjCopyBuf(dst: []u8, src: string): usize;
export fn mjIsSpace(ch: u8): bool;
export fn mjIsDigit(ch: u8): bool;
export fn mjIsIdentStart(ch: u8): bool;
export fn mjIsIdentChar(ch: u8): bool;
export class ScopeVar {
    name: [32]u8,
    name_len: usize,
    value: f64,
}

export class ScopeLookup {
    found: bool,
    value: f64,
}

export class EvalResult {
    ok: bool,
    value: f64,
    err_code: i32,
}

export fn mjOk(v: f64): EvalResult;
export fn mjErr(code: i32): EvalResult;
export class MathScope {
    vars: [32]ScopeVar,
    count: usize,
    fn new(): MathScope;
    fn set(_: auto, name: string, val: f64): bool;
    fn get(name: string): ScopeLookup;
}

export class MjParser {
    src: string,
    pos: usize,
    depth: usize,
    scope: MathScope,
}

export fn mjSkipWs(p: &mut MjParser);
export fn mjParseNumber(p: &mut MjParser): EvalResult;
export fn mjEvalBuiltin(name: string, args: &[]f64, argc: usize): EvalResult;
export fn mjParsePrimary(p: &mut MjParser): EvalResult;
export fn mjParsePower(p: &mut MjParser): EvalResult;
export fn mjParseUnary(p: &mut MjParser): EvalResult;
export fn mjParseMulDiv(p: &mut MjParser): EvalResult;
export fn mjParseAddSub(p: &mut MjParser): EvalResult;
export fn evalExprWithScope(expr: string, scope: MathScope): EvalResult;
export fn evalExpr(expr: string): EvalResult;
