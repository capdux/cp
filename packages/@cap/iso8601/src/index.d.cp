// Type definitions for CP

export class IsoDate {
    year: i32,
    month: u8,
    day: u8,
    ordinal: u16,
    week: u8,
    weekday: u8,
    kind: u8,
}

export class IsoTime {
    hour: u8,
    minute: u8,
    second: u8,
    millisecond: u16,
    tz_offset_minutes: i16,
    has_tz: bool,
}

export class IsoDateTime {
    date: IsoDate,
    time: IsoTime,
}

export class IsoDuration {
    years: u32,
    months: u32,
    weeks: u32,
    days: u32,
    hours: u32,
    minutes: u32,
    seconds: u32,
    milliseconds: u32,
}

export fn isoIsDigit(c: u8): bool;
export fn isoParseDigits(s: string, start: usize, count: usize): i64?;
export fn isLeapYear(year: i32): bool;
export fn daysInMonth(year: i32, month: u8): u8;
export fn parseDate(s: string): IsoDate?;
export fn parseTime(s: string): IsoTime?;
export fn parseDateTime(s: string): IsoDateTime?;
export fn parseDuration(s: string): IsoDuration?;
export fn isoWrite2(out: []u8, pos: usize, val: u8): usize;
export fn isoWrite4(out: []u8, pos: usize, val: i32): usize;
export fn formatDateTime(dt: IsoDateTime, out: []u8): usize;
