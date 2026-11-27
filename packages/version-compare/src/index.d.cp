// Type definitions for CP

import { isSpace, isDigit, isAlpha, toLowerByte, strEq, eqIgnoreCase, findByte } from "cap:ascii";
export enum VersionCmp {
    Lt,
    Eq,
    Gt,
    Invalid
}

export class VcSeg {
    isNum: bool,
    numVal: u64,
    start: usize,
    len: usize,
}

export class VcParsed {
    segs: [32]VcSeg,
    count: usize,
    valid: bool,
}

export fn vcIsSep(b: u8): bool;
export fn vcTagRank(s: string, start: usize, len: usize): i32;
export fn vcCmpLexLower(a: string, aStart: usize, aLen: usize, b: string, bStart: usize, bLen: usize): i32;
export fn vcParse(raw: string): VcParsed;
export fn compareVersions(a: string, b: string): VersionCmp;
export fn isLess(a: string, b: string): bool;
export fn isEqual(a: string, b: string): bool;
export fn isGreater(a: string, b: string): bool;
export fn satisfies(ver: string, op: string, target: string): bool;
