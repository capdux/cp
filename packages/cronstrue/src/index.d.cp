// Type definitions for CP

import { isSpace, isDigit, eqIgnoreCase, trim } from "cap:ascii";
import { writeStr, writeU64 } from "cap:fmt";
export enum Locale {
    En = 0,
    Zh = 1
}

export class CronDescOptions {
    locale: Locale,
    use_24h: bool,
    fn english(): CronDescOptions;
    fn chinese(): CronDescOptions;
}

export class FieldItem {
    is_any: bool,
    is_range: bool,
    start: u8,
    end: u8,
    step: u8,
}

export class CronField {
    kind: u8,
    items: [16]FieldItem,
    item_len: u8,
}

export class ParsedCron {
    valid: bool,
    minute: CronField,
    hour: CronField,
    dom: CronField,
    month: CronField,
    dow: CronField,
}

export fn pushByte(out: []u8, pos: usize, b: u8): usize;
export fn pushStr(out: []u8, pos: usize, s: string): usize;
export fn pushU8(out: []u8, pos: usize, val: u8): usize;
export fn pushU8Pad2(out: []u8, pos: usize, val: u8): usize;
export fn pushTime(out: []u8, pos: usize, hour: u8, minute: u8, opts: CronDescOptions): usize;
export fn dayName(val: u8, locale: Locale): string;
export fn monthName(val: u8, locale: Locale): string;
export fn parseAtom(tok: string, min_val: u8, max_val: u8, allow_month: bool, allow_dow: bool): i32;
export class ItemParseRes {
    ok: bool,
    item: FieldItem,
}

export fn parseFieldItem(tok: string, min_val: u8, max_val: u8, allow_month: bool, allow_dow: bool): ItemParseRes;
export class FieldParseRes {
    ok: bool,
    field: CronField,
}

export fn parseField(tok: string, min_val: u8, max_val: u8, allow_month: bool, allow_dow: bool): FieldParseRes;
export fn expandMacro(trimmed: string): string;
export fn parseCron(raw: string): ParsedCron;
export fn writeItemNumeric(out: []u8, pos: usize, it: FieldItem, locale: Locale): usize;
export fn writeItemDay(out: []u8, pos: usize, it: FieldItem, locale: Locale): usize;
export fn writeItemMonth(out: []u8, pos: usize, it: FieldItem, locale: Locale): usize;
export fn isValidCron(expr: string): bool;
export fn describeCronWith(expr: string, opts: CronDescOptions, out: []u8): string;
export fn describeCron(expr: string, out: []u8): string;
