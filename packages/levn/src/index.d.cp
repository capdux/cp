// Type definitions for CP

import { trim, strEq } from "cap:ascii";
export enum LevnKind {
    Null,
    Bool,
    Int,
    Float,
    String,
    Invalid
}

export class LevnValue {
    kind: LevnKind,
    bool_val: bool,
    int_val: i64,
    float_val: f64,
    str_val: string,
}

export class LevnPair {
    key: string,
    value: LevnValue,
}

export fn lvnMakeInvalid(): LevnValue;
export fn lvnTryParseNumber(s: string): LevnValue;
export fn parseLevnScalar(input: string): LevnValue;
export fn lvnFindTopLevelChar(s: string, start: usize, target: u8): usize;
export fn lvnUnquoteKey(s: string): string;
export fn parseLevnList(input: string, out: []LevnValue): usize;
export fn parseLevnObject(input: string, out: []LevnPair): usize;
