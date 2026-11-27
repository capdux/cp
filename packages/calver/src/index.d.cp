// Type definitions for CP

import { isDigit, isAlpha, toLowerByte, eqIgnoreCase, trim, findByte } from "cap:ascii";
import { writeStr, writeU64, parseU64 } from "cap:fmt";
export enum CalverFormat {
    YyyyMmMicro = 0,
    YyMmMicro = 1,
    Yyyy0mMicro = 2,
    YyyyMmDd = 3,
    YyyyMmDdMicro = 4
}

export class Calver {
    valid: bool,
    year: u16,
    month: u8,
    day: u8,
    micro: u32,
    modifier: string,
    modifier_num: u32,
    fn invalid(): Calver;
}

export fn cmpIgnoreCase(a: string, b: string): i32;
export fn isLeapYear(year: u16): bool;
export fn daysInMonth(year: u16, month: u8): u8;
export fn parseU32Strict(s: string): u32?;
export class ModifierParts {
    ok: bool,
    name: string,
    num: u32,
}

export fn parseModifierTail(s: string): ModifierParts;
export fn parseCalverCore(raw: string, force_day_on_3: bool): Calver;
export fn parseCalver(raw: string): Calver;
export fn parseCalverWith(raw: string, fmt: CalverFormat): Calver;
export fn modifierRank(name: string): u8;
export fn compareCalver(a: Calver, b: Calver): i32;
export fn bumpCalver(cur: Calver, year: u16, month: u8, day: u8, use_day: bool): Calver;
export fn appendByte(out: []u8, pos: usize, c: u8): usize;
export fn append2Digits(out: []u8, pos: usize, val: u32): usize;
export fn formatCalver(v: Calver, fmt: CalverFormat, out: []u8): string;
