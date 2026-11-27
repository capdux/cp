// Type definitions for CP

import { validateRegexSyntax, extractLiteralPrefix, isWordByte } from "../../regex-syntax/src/index.cp";
import { memmemFind } from "../../memchr/src/index.cp";
export class RegexMatch {
    matched: bool,
    start: usize,
    end: usize,
    fn len(): usize;
}

export class RlInst {
    op: u8,
    arg1: usize,
    arg2: usize,
}

export class RlClass {
    bits: [4]u64,
}

export class RlCompileCtx {
    insts: [128]RlInst,
    inst_len: usize,
    classes: [16]RlClass,
    class_len: usize,
    ok: bool,
}

export fn rlEmptyInst(): RlInst;
export fn rlEmptyClass(): RlClass;
export fn rlNoMatch(): RegexMatch;
export fn rlClassSet(c: &mut RlClass, b: u8);
export fn rlClassTest(c: RlClass, b: u8): bool;
export fn rlClassSetRange(c: &mut RlClass, lo: u8, hi: u8);
export fn rlClassInvert(c: &mut RlClass);
export fn rlClassAddShorthand(c: &mut RlClass, esc: u8);
export fn rlHexVal(b: u8): usize;
export fn rlEmit(ctx: &mut RlCompileCtx, op: u8, arg1: usize, arg2: usize): usize;
export fn rlAllocClass(ctx: &mut RlCompileCtx): usize;
export fn rlInsertSlot(ctx: &mut RlCompileCtx, at_pc: usize);
export fn rlDecodeEscapeByte(pat: string, pos: usize): u8;
export fn rlEscapeLen(pat: string, pos: usize): usize;
export fn rlIsShorthand(esc: u8): bool;
export fn rlCompileCharClass(ctx: &mut RlCompileCtx, pat: string, start_pos: usize): usize;
export fn rlSkipGroupHeader(pat: string, pos: usize): usize;
export fn rlCompileAtom(ctx: &mut RlCompileCtx, pat: string, pos: usize): usize;
export fn rlCompileQuant(ctx: &mut RlCompileCtx, pat: string, pos: usize): usize;
export fn rlCompileConcat(ctx: &mut RlCompileCtx, pat: string, pos: usize): usize;
export fn rlCompileAlt(ctx: &mut RlCompileCtx, pat: string, pos: usize): usize;
export fn rlIsWordAt(text: string, pos: usize): bool;
export fn rlCopyBytes(dst: []u8, pos: usize, src: &[]u8): usize;
export class RegexLite {
    insts: [128]RlInst,
    inst_len: usize,
    classes: [16]RlClass,
    class_len: usize,
    prefix: [64]u8,
    prefix_len: usize,
    anchored_start: bool,
    valid: bool,
    err_code: i32,
    fn compile(pattern: string): RegexLite;
    fn isOk(): bool;
    fn errCode(): i32;
    fn matchAnchored(text: string, start_pos: usize): RegexMatch;
    fn findAt(text: string, start_pos: usize): RegexMatch;
    fn find(text: string): RegexMatch;
    fn isMatch(text: string): bool;
    fn findAll(text: string, out_matches: []RegexMatch): usize;
    fn replaceAll(text: string, replacement: string, out: []u8): usize;
    fn splitCount(text: string): usize;
}

