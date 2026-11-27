// Type definitions for CP

import { isSpace, isDigit, hexVal } from "cap:ascii";
export class EvalResult {
    value: i64,
    ok: bool,
}

export class Parser {
    pos: usize,
    ok: bool,
    depth: usize,
}

export fn skipWs(p: &mut Parser, src: string);
export fn peek(p: &mut Parser, src: string): u8;
export fn peek2(p: &mut Parser, src: string): u8;
export fn absI64(x: i64): u64;
export fn signedDivI64(a: i64, b: i64): i64;
export fn signedRemI64(a: i64, b: i64): i64;
export fn skipIntSuffix(p: &mut Parser, src: string);
export fn parseCharLit(p: &mut Parser, src: string): i64;
export fn parseIntLit(p: &mut Parser, src: string): i64;
export fn parsePrimary(p: &mut Parser, src: string): i64;
export fn parseUnary(p: &mut Parser, src: string): i64;
export fn parseMul(p: &mut Parser, src: string): i64;
export fn parseAdd(p: &mut Parser, src: string): i64;
export fn parseShift(p: &mut Parser, src: string): i64;
export fn parseRel(p: &mut Parser, src: string): i64;
export fn parseEq(p: &mut Parser, src: string): i64;
export fn parseBitAnd(p: &mut Parser, src: string): i64;
export fn parseBitXor(p: &mut Parser, src: string): i64;
export fn parseBitOr(p: &mut Parser, src: string): i64;
export fn parseLogAnd(p: &mut Parser, src: string): i64;
export fn parseLogOr(p: &mut Parser, src: string): i64;
export fn parseTernary(p: &mut Parser, src: string): i64;
export fn evalCExpr(expr: string): EvalResult;
