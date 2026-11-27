// Type definitions for CP

import { isSpace, isDigit, isAlpha, isAlnum, strEq } from "cap:ascii";
export enum ExprError {
    None = 0,
    SyntaxError = 1,
    UnexpectedToken = 2,
    DivisionByZero = 3,
    UnknownVariable = 4,
    UnknownFunction = 5,
    InvalidArgs = 6,
    MaxDepthExceeded = 7
}

export class EvalResult {
    ok: bool,
    value: f64,
    err: ExprError,
    pos: usize,
    fn success(v: f64): EvalResult;
    fn fail(err: ExprError, pos: usize): EvalResult;
}

export class EnvEntry {
    name: string,
    value: f64,
}

export class ExprEnv {
    entries: [64]EnvEntry,
    count: usize,
    fn init(): ExprEnv;
    fn set(_: auto, name: string, value: f64);
    fn get(name: string): f64?;
    fn has(name: string): bool;
    fn clear(_: auto);
    fn len(): usize;
}

export enum TokenKind {
    Eof = 0,
    Number = 1,
    Ident = 2,
    Plus = 3,
    Minus = 4,
    Star = 5,
    Slash = 6,
    Percent = 7,
    Caret = 8,
    LParen = 9,
    RParen = 10,
    Comma = 11,
    Question = 12,
    Colon = 13,
    Bang = 14,
    EqEq = 15,
    BangEq = 16,
    Lt = 17,
    LtEq = 18,
    Gt = 19,
    GtEq = 20,
    AndAnd = 21,
    OrOr = 22,
    Invalid = 23
}

export class Token {
    kind: TokenKind,
    start: usize,
    end: usize,
    num: f64,
}

export fn isIdentStart(c: u8): bool;
export fn isIdentCont(c: u8): bool;
export fn isTruthy(v: f64): bool;
export fn parseFloatLiteral(slice: string): f64?;
export class Parser {
    src: string,
    pos: usize,
    tok: Token,
    env: &ExprEnv,
    depth: usize,
    err: ExprError,
    err_pos: usize,
    fn init(src: string, env: &ExprEnv): Parser;
    fn hasError(): bool;
    fn setError(_: auto, err: ExprError, pos: usize);
    fn enterDepth(_: auto, pos: usize): bool;
    fn leaveDepth(_: auto);
    fn advance(_: auto);
    fn parseExpr(_: auto, active: bool): f64;
    fn parseTernary(_: auto, active: bool): f64;
    fn parseOr(_: auto, active: bool): f64;
    fn parseAnd(_: auto, active: bool): f64;
    fn parseEquality(_: auto, active: bool): f64;
    fn parseComparison(_: auto, active: bool): f64;
    fn parseAdditive(_: auto, active: bool): f64;
    fn parseMultiplicative(_: auto, active: bool): f64;
    fn parsePower(_: auto, active: bool): f64;
    fn parseUnary(_: auto, active: bool): f64;
    fn parsePrimary(_: auto, active: bool): f64;
    fn parseCall(_: auto, name: string, name_pos: usize, active: bool): f64;
    fn evalBuiltin(_: auto, name: string, pos: usize, args: &[]f64): f64;
}

export fn evalExpr(source: string): EvalResult;
export fn evalExprWith(source: string, env: ExprEnv): EvalResult;
export fn evalBool(source: string, env: ExprEnv): bool;
