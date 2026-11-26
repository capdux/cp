// Type definitions for CP

export enum TimeagoLocale {
    En = 0,
    Zh = 1
}

export enum TimeagoStyle {
    Long = 0,
    Short = 1
}

export enum TimeUnit {
    Second = 0,
    Minute = 1,
    Hour = 2,
    Day = 3,
    Week = 4,
    Month = 5,
    Year = 6
}

export class ScaledDelta {
    value: u64,
    unit: TimeUnit,
}

export fn absU64(n: i64): u64;
export fn appendByte(out: []u8, pos: usize, c: u8): usize;
export fn appendStr(out: []u8, pos: usize, s: string): usize;
export fn appendU64(out: []u8, pos: usize, val: u64): usize;
export fn scaleDelta(abs_sec: u64): ScaledDelta;
export fn enLongUnit(unit: TimeUnit, plural: bool): string;
export fn enShortUnit(unit: TimeUnit): string;
export fn zhLongUnit(unit: TimeUnit): string;
export fn zhShortUnit(unit: TimeUnit): string;
export fn formatTimeago(delta_sec: i64, locale: TimeagoLocale, style: TimeagoStyle, out: []u8): string;
export fn formatBetween(target_sec: i64, now_sec: i64, locale: TimeagoLocale, style: TimeagoStyle, out: []u8): string;
export fn formatTimeagoEn(delta_sec: i64, out: []u8): string;
export fn formatTimeagoZh(delta_sec: i64, out: []u8): string;
export fn formatCompact(delta_sec: i64, out: []u8): string;
