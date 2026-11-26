// Type definitions for CP

import { IsoDate, IsoTime, IsoDateTime, parseDateTime, formatDateTime, isLeapYear } from "../../iso8601/src/index.cp";
export class Rfc3339Timestamp {
    dt: IsoDateTime,
    unix_seconds: i64,
    millisecond: u16,
}

export fn rfcIsDigit(c: u8): bool;
export fn daysFromCivil(year: i32, month: u8, day: u8): i64;
export fn civilFromDays(z_in: i64): IsoDate;
export fn isValidRfc3339(s: string): bool;
export fn parseRfc3339(s: string): Rfc3339Timestamp?;
export fn fromUnixSeconds(unix_secs: i64): IsoDateTime;
export fn formatRfc3339Utc(unix_secs: i64, out: []u8): usize;
export fn isRfcLeapYear(year: i32): bool;
