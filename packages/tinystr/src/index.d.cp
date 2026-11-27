// Type definitions for CP

import { toLowerByte, toUpperByte, isAlpha, isDigit, isAlnum, eqIgnoreCase } from "cap:ascii";
import { bytesEq } from "cap:bin";
export fn tsIsLower(b: u8): bool;
export fn tsIsUpper(b: u8): bool;
export fn tsValidateBytes(src: &[]u8, max_len: usize): i64;
export fn tsValidatePacked(buf: &[]u8): i64;
export fn tsSliceLen(buf: &[]u8): usize;
export fn tsCopyOut(src: &[]u8, n: usize, out: []u8): usize;
export fn tsSliceCmp(a: &[]u8, b: &[]u8): i32;
export fn tsAllAlpha(buf: &[]u8, n: usize): bool;
export fn tsAllAlnum(buf: &[]u8, n: usize): bool;
export fn tsAllDigit(buf: &[]u8, n: usize): bool;
export fn tsAllLower(buf: &[]u8, n: usize): bool;
export fn tsAllUpper(buf: &[]u8, n: usize): bool;
export class TinyAsciiStr4 {
    bytes: [4]u8,
    fn empty(): TinyAsciiStr4;
    fn fromBytes(src: &[]u8): TinyAsciiStr4Result;
    fn fromStr(s: string): TinyAsciiStr4Result;
    fn fromPackedU32(v: u32): TinyAsciiStr4Result;
    fn toPackedU32(): u32;
    fn len(): usize;
    fn isEmpty(): bool;
    fn asBytes(out: []u8): usize;
    fn byteAt(idx: usize): u8;
    fn eq(other: TinyAsciiStr4): bool;
    fn cmp(other: TinyAsciiStr4): i32;
    fn eqIgnoreAsciiCase(other: TinyAsciiStr4): bool;
    fn toAsciiLowercase(): TinyAsciiStr4;
    fn toAsciiUppercase(): TinyAsciiStr4;
    fn toAsciiTitlecase(): TinyAsciiStr4;
    fn isAsciiAlphabetic(): bool;
    fn isAsciiAlphanumeric(): bool;
    fn isAsciiNumeric(): bool;
    fn isAsciiLowercase(): bool;
    fn isAsciiUppercase(): bool;
}

export class TinyAsciiStr4Result {
    ok: bool,
    value: TinyAsciiStr4,
    err: i64,
}

export class TinyAsciiStr8 {
    bytes: [8]u8,
    fn empty(): TinyAsciiStr8;
    fn fromBytes(src: &[]u8): TinyAsciiStr8Result;
    fn fromStr(s: string): TinyAsciiStr8Result;
    fn fromPackedU64(v: u64): TinyAsciiStr8Result;
    fn toPackedU64(): u64;
    fn len(): usize;
    fn isEmpty(): bool;
    fn asBytes(out: []u8): usize;
    fn byteAt(idx: usize): u8;
    fn eq(other: TinyAsciiStr8): bool;
    fn cmp(other: TinyAsciiStr8): i32;
    fn eqIgnoreAsciiCase(other: TinyAsciiStr8): bool;
    fn toAsciiLowercase(): TinyAsciiStr8;
    fn toAsciiUppercase(): TinyAsciiStr8;
    fn toAsciiTitlecase(): TinyAsciiStr8;
    fn isAsciiAlphabetic(): bool;
    fn isAsciiAlphanumeric(): bool;
    fn isAsciiNumeric(): bool;
    fn isAsciiLowercase(): bool;
    fn isAsciiUppercase(): bool;
}

export class TinyAsciiStr8Result {
    ok: bool,
    value: TinyAsciiStr8,
    err: i64,
}

export class TinyAsciiStr16 {
    bytes: [16]u8,
    fn empty(): TinyAsciiStr16;
    fn fromBytes(src: &[]u8): TinyAsciiStr16Result;
    fn fromStr(s: string): TinyAsciiStr16Result;
    fn len(): usize;
    fn isEmpty(): bool;
    fn asBytes(out: []u8): usize;
    fn byteAt(idx: usize): u8;
    fn eq(other: TinyAsciiStr16): bool;
    fn cmp(other: TinyAsciiStr16): i32;
    fn eqIgnoreAsciiCase(other: TinyAsciiStr16): bool;
    fn toAsciiLowercase(): TinyAsciiStr16;
    fn toAsciiUppercase(): TinyAsciiStr16;
    fn toAsciiTitlecase(): TinyAsciiStr16;
    fn isAsciiAlphabetic(): bool;
    fn isAsciiAlphanumeric(): bool;
    fn isAsciiNumeric(): bool;
    fn isAsciiLowercase(): bool;
    fn isAsciiUppercase(): bool;
}

export class TinyAsciiStr16Result {
    ok: bool,
    value: TinyAsciiStr16,
    err: i64,
}

