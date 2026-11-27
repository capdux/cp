// Type definitions for CP

import { isDigit, hexVal, hexCharLower, startsWith } from "cap:ascii";
import { bytesEq } from "cap:bin";
import { writeStr } from "cap:fmt";
export class JcsResult {
    ok: bool,
    len: usize,
    output: string,
    fn fail(out: []u8): JcsResult;
}

export class ObjEntry {
    key_start: usize,
    key_end: usize,
    val_start: usize,
    val_end: usize,
}

export class StepRes {
    ok: bool,
    next_in: usize,
    next_out: usize,
}

export class SkipRes {
    ok: bool,
    next_in: usize,
}

export class KeyStep {
    ok: bool,
    done: bool,
    unit: u16,
    next_pos: usize,
    pending_low: u16,
}

export fn isJsonSpace(c: u8): bool;
export fn skipWs(s: string, start: usize): usize;
export fn parseHex4(s: string, pos: usize): i32;
export fn pushByteChecked(out: []u8, pos: usize, b: u8): usize?;
export fn pushSliceChecked(out: []u8, pos: usize, s: string): usize?;
export fn writeUtf8Checked(out: []u8, pos: usize, cp: u32): usize?;
export fn emitEscapedCodepoint(out: []u8, pos: usize, cp: u32): usize?;
export fn skipString(input: string, pos: usize): SkipRes;
export fn skipNumber(input: string, pos: usize): SkipRes;
export fn skipValue(input: string, start: usize, depth: u8): SkipRes;
export fn nextKeyUtf16Unit(input: string, pos: usize, end: usize, pending_low: u16): KeyStep;
export fn compareJsonKeys(input: string, s1: usize, e1: usize, s2: usize, e2: usize): i8;
export fn emitCanonicalString(input: string, pos: usize, out: []u8, out_pos: usize): StepRes;
export fn emitCanonicalNumber(input: string, pos: usize, out: []u8, out_pos: usize): StepRes;
export fn emitCanonicalValue(input: string, start: usize, out: []u8, out_pos: usize, depth: u8): StepRes;
export fn canonicalizeJson(input: string, out: []u8): JcsResult;
export fn areJsonEquivalent(a: string, b: string): bool;
