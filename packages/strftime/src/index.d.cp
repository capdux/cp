// Type definitions for CP

export class DateTime {
    year: i32,
    month: u8,
    day: u8,
    hour: u8,
    minute: u8,
    second: u8,
    weekday: u8,
    yearday: u16,
    tz_offset_sec: i32,
}

export fn isLeapYear(y: i32): bool;
export fn daysInMonth(y: i32, m: u8): u8;
export fn dayOfYear(y: i32, m: u8, d: u8): u16;
export fn fromUnixSeconds(unix_sec: i64, tz_offset_sec: i32): DateTime;
export fn weekdayShort(w: u8): string;
export fn weekdayFull(w: u8): string;
export fn monthShort(m: u8): string;
export fn monthFull(m: u8): string;
export class FmtWriter {
    out: []u8,
    pos: usize,
    fn putByte(_: auto, b: u8);
    fn putStr(_: auto, s: string);
    fn putPad2(_: auto, val: u32, padChar: u8);
    fn putPad3(_: auto, val: u32);
    fn putPad4(_: auto, val: i32);
}

export fn strftime(fmt: string, dt: DateTime, out: []u8): string;
export fn formatUnix(fmt: string, unix_sec: i64, tz_offset_sec: i32, out: []u8): string;
export fn formatIso8601Utc(unix_sec: i64, out: []u8): string;
