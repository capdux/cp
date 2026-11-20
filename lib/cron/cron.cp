// Source: cp/lib/cron/cron.cp (5/6-Field Cron Expression Parser & Next/Prev Trigger Calculator)
// Pure CP zero-dependency Cron parser & UTC trigger calculator (`cap:cron` & `cap.cron`).
// Supports 5-field (min hour dom mon dow) and 6-field (sec min hour dom mon dow) expressions,
// @yearly/@annually/@monthly/@weekly/@daily/@midnight/@hourly aliases, JAN..DEC, SUN..SAT (0/7=SUN),
// Vixie Cron dom/dow OR/AND semantics, and O(1) Howard Hinnant UTC calendar decomposition.

const std = @import("std");

/// Error set for Cron expression parsing and trigger calculation.
pub err CronError {
    InvalidFieldCount,
    InvalidFieldValue,
    InvalidStep,
    InvalidRange,
    EmptyExpression,
    NoMatchingTime,
    OutOfMemory,
}

/// Decomposed UTC calendar date-time components.
pub struct UtcDateTime {
    year: i32,
    month: u8,
    day: u8,
    hour: u8,
    minute: u8,
    second: u8,
    dow: u8,
}

/// Parsed 5-field or 6-field Cron schedule represented as compact bitmasks.
pub struct CronSchedule {
    seconds: u64,
    minutes: u64,
    hours: u32,
    days_of_month: u32,
    months: u16,
    days_of_week: u8,
    dom_wildcard: bool,
    dow_wildcard: bool,
    has_seconds: bool,

    /// Parses a 5-field, 6-field, or `@macro` Cron expression into a `CronSchedule`.
    pub static fn parse(expr: []const u8): CronSchedule ! CronError {
        const trimmed = trimAscii(expr);
        if (trimmed.len == 0) {
            return CronError.EmptyExpression;
        }

        if (trimmed[0] == '@') {
            if (eqlIgnoreCase(trimmed, "@yearly") || eqlIgnoreCase(trimmed, "@annually")) {
                return CronSchedule.parse("0 0 1 1 *");
            }
            if (eqlIgnoreCase(trimmed, "@monthly")) {
                return CronSchedule.parse("0 0 1 * *");
            }
            if (eqlIgnoreCase(trimmed, "@weekly")) {
                return CronSchedule.parse("0 0 * * 0");
            }
            if (eqlIgnoreCase(trimmed, "@daily") || eqlIgnoreCase(trimmed, "@midnight")) {
                return CronSchedule.parse("0 0 * * *");
            }
            if (eqlIgnoreCase(trimmed, "@hourly")) {
                return CronSchedule.parse("0 * * * *");
            }
            return CronError.InvalidFieldValue;
        }

        let tokens: [6][]const u8 = undefined;
        let count: usize = 0;
        let i: usize = 0;
        while (i < trimmed.len) {
            while (i < trimmed.len && isWhitespace(trimmed[i])) {
                i += 1;
            }
            if (i >= trimmed.len) {
                break;
            }
            const start = i;
            while (i < trimmed.len && !isWhitespace(trimmed[i])) {
                i += 1;
            }
            if (count >= 6) {
                return CronError.InvalidFieldCount;
            }
            tokens[count] = trimmed[start..i];
            count += 1;
        }

        if (count != 5 && count != 6) {
            return CronError.InvalidFieldCount;
        }

        const has_seconds = count == 6;
        let sec_mask: u64 = 1; // bit 0 (second == 0) for 5-field expressions
        let idx: usize = 0;
        if (has_seconds) {
            sec_mask = parseFieldMask(tokens[0], 0, 59, FieldKind.Normal)?;
            idx = 1;
        }

        const min_mask = parseFieldMask(tokens[idx], 0, 59, FieldKind.Normal)?;
        const hour_mask = @as(u32, @intCast(parseFieldMask(tokens[idx + 1], 0, 23, FieldKind.Normal)?));
        const dom_str = tokens[idx + 2];
        const mon_str = tokens[idx + 3];
        const dow_str = tokens[idx + 4];

        const dom_mask = @as(u32, @intCast(parseFieldMask(dom_str, 1, 31, FieldKind.Normal)?));
        const mon_mask = @as(u16, @intCast(parseFieldMask(mon_str, 1, 12, FieldKind.Month)?));
        const dow_mask = @as(u8, @intCast(parseFieldMask(dow_str, 0, 7, FieldKind.DayOfWeek)?));

        return CronSchedule{
            seconds: sec_mask,
            minutes: min_mask,
            hours: hour_mask,
            days_of_month: dom_mask,
            months: mon_mask,
            days_of_week: dow_mask,
            dom_wildcard: std.mem.eql(u8, dom_str, "*"),
            dow_wildcard: std.mem.eql(u8, dow_str, "*"),
            has_seconds: has_seconds,
        };
    }

    /// Returns `true` if `epoch_sec` (UTC seconds) matches this Cron schedule.
    pub fn matches(epoch_sec: i64): bool {
        const dt = decomposeUtc(epoch_sec);
        if ((_.seconds & (@as(u64, 1) << @as(u6, @intCast(dt.second)))) == 0) {
            return false;
        }
        if ((_.minutes & (@as(u64, 1) << @as(u6, @intCast(dt.minute)))) == 0) {
            return false;
        }
        if ((_.hours & (@as(u32, 1) << @as(u5, @intCast(dt.hour)))) == 0) {
            return false;
        }
        if ((_.months & (@as(u16, 1) << @as(u4, @intCast(dt.month)))) == 0) {
            return false;
        }
        return _.dayMatches(dt.day, dt.dow);
    }

    /// Returns the next UTC trigger timestamp strictly greater than `epoch_sec`.
    pub fn nextAfter(epoch_sec: i64): i64 ! CronError {
        const start_dt = decomposeUtc(epoch_sec + 1);
        let year = start_dt.year;
        let month = start_dt.month;
        let day = start_dt.day;
        let hour = start_dt.hour;
        let minute = start_dt.minute;
        let second = start_dt.second;

        const max_year: i32 = year + 28;
        while (year <= max_year) {
            // 1. Month
            const next_m = nextBitU16(_.months, month, 12);
            if (next_m == null) {
                year += 1;
                month = 1;
                day = 1;
                hour = 0;
                minute = 0;
                second = 0;
                continue;
            }
            const m = next_m.?;
            if (m != month) {
                month = m;
                day = 1;
                hour = 0;
                minute = 0;
                second = 0;
            }

            // 2. Day of month (with Vixie dom/dow semantics)
            const dim = daysInMonth(year, month);
            let found_day: u8? = null;
            let d = day;
            while (d <= dim) {
                const dow = dayOfWeek(year, month, d);
                if (_.dayMatches(d, dow)) {
                    found_day = d;
                    break;
                }
                d += 1;
            }
            if (found_day == null) {
                month += 1;
                if (month > 12) {
                    month = 1;
                    year += 1;
                }
                day = 1;
                hour = 0;
                minute = 0;
                second = 0;
                continue;
            }
            const matched_d = found_day.?;
            if (matched_d != day) {
                day = matched_d;
                hour = 0;
                minute = 0;
                second = 0;
            }

            // 3. Hour
            const next_h = nextBitU32(_.hours, hour, 23);
            if (next_h == null) {
                day += 1;
                if (day > dim) {
                    day = 1;
                    month += 1;
                    if (month > 12) {
                        month = 1;
                        year += 1;
                    }
                }
                hour = 0;
                minute = 0;
                second = 0;
                continue;
            }
            const h = next_h.?;
            if (h != hour) {
                hour = h;
                minute = 0;
                second = 0;
            }

            // 4. Minute
            const next_min = nextBitU64(_.minutes, minute, 59);
            if (next_min == null) {
                hour += 1;
                if (hour > 23) {
                    hour = 0;
                    day += 1;
                    if (day > dim) {
                        day = 1;
                        month += 1;
                        if (month > 12) {
                            month = 1;
                            year += 1;
                        }
                    }
                }
                minute = 0;
                second = 0;
                continue;
            }
            const min = next_min.?;
            if (min != minute) {
                minute = min;
                second = 0;
            }

            // 5. Second
            const next_s = nextBitU64(_.seconds, second, 59);
            if (next_s == null) {
                minute += 1;
                if (minute > 59) {
                    minute = 0;
                    hour += 1;
                    if (hour > 23) {
                        hour = 0;
                        day += 1;
                        if (day > dim) {
                            day = 1;
                            month += 1;
                            if (month > 12) {
                                month = 1;
                                year += 1;
                            }
                        }
                    }
                }
                second = 0;
                continue;
            }
            const sec = next_s.?;
            return toEpochSec(year, month, day, hour, minute, sec);
        }

        return CronError.NoMatchingTime;
    }

    /// Computes the next `count` trigger timestamps strictly after `epoch_sec`.
    pub fn nextN(alloc: std.mem.Allocator, epoch_sec: i64, count: usize): ([]i64) ! CronError {
        let out = alloc.alloc(i64, count) catch return CronError.OutOfMemory;
        errdefer alloc.free(out);
        let cursor = epoch_sec;
        let i: usize = 0;
        while (i < count) {
            cursor = _.nextAfter(cursor)?;
            out[i] = cursor;
            i += 1;
        }
        return out;
    }

    /// Returns the previous UTC trigger timestamp strictly less than `epoch_sec`.
    pub fn prevBefore(epoch_sec: i64): i64 ! CronError {
        const start_dt = decomposeUtc(epoch_sec - 1);
        let year = start_dt.year;
        let month = start_dt.month;
        let day = start_dt.day;
        let hour = start_dt.hour;
        let minute = start_dt.minute;
        let second = start_dt.second;

        const min_year: i32 = year - 28;
        while (year >= min_year) {
            // 1. Month
            const prev_m = prevBitU16(_.months, month, 1);
            if (prev_m == null) {
                year -= 1;
                month = 12;
                day = daysInMonth(year, month);
                hour = 23;
                minute = 59;
                second = 59;
                continue;
            }
            const m = prev_m.?;
            if (m != month) {
                month = m;
                day = daysInMonth(year, month);
                hour = 23;
                minute = 59;
                second = 59;
            }

            // 2. Day of month
            const dim = daysInMonth(year, month);
            let d = if (day > dim) dim else day;
            let found_day: u8? = null;
            while (d >= 1) {
                const dow = dayOfWeek(year, month, d);
                if (_.dayMatches(d, dow)) {
                    found_day = d;
                    break;
                }
                if (d == 1) {
                    break;
                }
                d -= 1;
            }
            if (found_day == null) {
                if (month == 1) {
                    month = 12;
                    year -= 1;
                } else {
                    month -= 1;
                }
                day = daysInMonth(year, month);
                hour = 23;
                minute = 59;
                second = 59;
                continue;
            }
            const matched_d = found_day.?;
            if (matched_d != day) {
                day = matched_d;
                hour = 23;
                minute = 59;
                second = 59;
            }

            // 3. Hour
            const prev_h = prevBitU32(_.hours, hour, 0);
            if (prev_h == null) {
                if (day <= 1) {
                    if (month == 1) {
                        month = 12;
                        year -= 1;
                    } else {
                        month -= 1;
                    }
                    day = daysInMonth(year, month);
                } else {
                    day -= 1;
                }
                hour = 23;
                minute = 59;
                second = 59;
                continue;
            }
            const h = prev_h.?;
            if (h != hour) {
                hour = h;
                minute = 59;
                second = 59;
            }

            // 4. Minute
            const prev_min = prevBitU64(_.minutes, minute, 0);
            if (prev_min == null) {
                if (hour == 0) {
                    hour = 23;
                    if (day <= 1) {
                        if (month == 1) {
                            month = 12;
                            year -= 1;
                        } else {
                            month -= 1;
                        }
                        day = daysInMonth(year, month);
                    } else {
                        day -= 1;
                    }
                } else {
                    hour -= 1;
                }
                minute = 59;
                second = 59;
                continue;
            }
            const min = prev_min.?;
            if (min != minute) {
                minute = min;
                second = 59;
            }

            // 5. Second
            const prev_s = prevBitU64(_.seconds, second, 0);
            if (prev_s == null) {
                if (minute == 0) {
                    minute = 59;
                    if (hour == 0) {
                        hour = 23;
                        if (day <= 1) {
                            if (month == 1) {
                                month = 12;
                                year -= 1;
                            } else {
                                month -= 1;
                            }
                            day = daysInMonth(year, month);
                        } else {
                            day -= 1;
                        }
                    } else {
                        hour -= 1;
                    }
                } else {
                    minute -= 1;
                }
                second = 59;
                continue;
            }
            const sec = prev_s.?;
            return toEpochSec(year, month, day, hour, minute, sec);
        }

        return CronError.NoMatchingTime;
    }

    fn dayMatches(day: u8, dow: u8): bool {
        const dom_match = (_.days_of_month & (@as(u32, 1) << @as(u5, @intCast(day)))) != 0;
        const dow_match = (_.days_of_week & (@as(u8, 1) << @as(u3, @intCast(dow)))) != 0;
        if (!_.dom_wildcard && !_.dow_wildcard) {
            return dom_match || dow_match;
        }
        return dom_match && dow_match;
    }
}

enum FieldKind: u8 {
    Normal = 0,
    Month = 1,
    DayOfWeek = 2,
}

fn parseFieldMask(field: []const u8, min_val: u8, max_val: u8, kind: FieldKind): u64 ! CronError {
    if (field.len == 0) {
        return CronError.InvalidFieldValue;
    }
    let mask: u64 = 0;
    let start: usize = 0;
    let i: usize = 0;
    while (i <= field.len) {
        if (i == field.len || field[i] == ',') {
            const part = field[start..i];
            if (part.len == 0) {
                return CronError.InvalidFieldValue;
            }
            const part_mask = parseFieldPart(part, min_val, max_val, kind)?;
            mask |= part_mask;
            start = i + 1;
        }
        i += 1;
    }
    return mask;
}

fn parseFieldPart(part: []const u8, min_val: u8, max_val: u8, kind: FieldKind): u64 ! CronError {
    let slash_idx: usize? = null;
    let i: usize = 0;
    while (i < part.len) {
        if (part[i] == '/') {
            if (slash_idx != null) {
                return CronError.InvalidStep;
            }
            slash_idx = i;
        }
        i += 1;
    }

    let range_part = part;
    let step: u8 = 1;
    if (slash_idx != null) {
        const idx = slash_idx.?;
        range_part = part[0..idx];
        const step_str = part[idx + 1..];
        if (range_part.len == 0 || step_str.len == 0) {
            return CronError.InvalidStep;
        }
        const parsed_step = parsePositiveInt(step_str) catch return CronError.InvalidStep;
        if (parsed_step == 0 || parsed_step > 60) {
            return CronError.InvalidStep;
        }
        step = @as(u8, @intCast(parsed_step));
    }

    let range_start: u8 = min_val;
    let range_end: u8 = max_val;

    if (std.mem.eql(u8, range_part, "*")) {
        range_start = min_val;
        range_end = if (kind == FieldKind.DayOfWeek) 6 else max_val;
    } else {
        let dash_idx: usize? = null;
        let j: usize = 0;
        while (j < range_part.len) {
            if (range_part[j] == '-') {
                if (dash_idx != null) {
                    return CronError.InvalidRange;
                }
                dash_idx = j;
            }
            j += 1;
        }

        if (dash_idx != null) {
            const d_idx = dash_idx.?;
            const s_str = range_part[0..d_idx];
            const e_str = range_part[d_idx + 1..];
            if (s_str.len == 0 || e_str.len == 0) {
                return CronError.InvalidRange;
            }
            range_start = parseAtom(s_str, min_val, max_val, kind)?;
            range_end = parseAtom(e_str, min_val, max_val, kind)?;
            if (range_start > range_end) {
                return CronError.InvalidRange;
            }
        } else {
            const val = parseAtom(range_part, min_val, max_val, kind)?;
            if (slash_idx != null) {
                range_start = val;
                range_end = if (kind == FieldKind.DayOfWeek) 6 else max_val;
            } else {
                const bit_val: u8 = if (kind == FieldKind.DayOfWeek && val == 7) 0 else val;
                return @as(u64, 1) << @as(u6, @intCast(bit_val));
            }
        }
    }

    let mask: u64 = 0;
    let cur: u16 = range_start;
    while (cur <= range_end) {
        const v: u8 = @as(u8, @intCast(cur));
        const bit_val: u8 = if (kind == FieldKind.DayOfWeek && v == 7) 0 else v;
        mask |= @as(u64, 1) << @as(u6, @intCast(bit_val));
        cur += step;
    }
    return mask;
}

fn parseAtom(s: []const u8, min_val: u8, max_val: u8, kind: FieldKind): u8 ! CronError {
    if (kind == FieldKind.Month) {
        if (eqlIgnoreCase(s, "JAN")) return 1;
        if (eqlIgnoreCase(s, "FEB")) return 2;
        if (eqlIgnoreCase(s, "MAR")) return 3;
        if (eqlIgnoreCase(s, "APR")) return 4;
        if (eqlIgnoreCase(s, "MAY")) return 5;
        if (eqlIgnoreCase(s, "JUN")) return 6;
        if (eqlIgnoreCase(s, "JUL")) return 7;
        if (eqlIgnoreCase(s, "AUG")) return 8;
        if (eqlIgnoreCase(s, "SEP")) return 9;
        if (eqlIgnoreCase(s, "OCT")) return 10;
        if (eqlIgnoreCase(s, "NOV")) return 11;
        if (eqlIgnoreCase(s, "DEC")) return 12;
    } else if (kind == FieldKind.DayOfWeek) {
        if (eqlIgnoreCase(s, "SUN")) return 0;
        if (eqlIgnoreCase(s, "MON")) return 1;
        if (eqlIgnoreCase(s, "TUE")) return 2;
        if (eqlIgnoreCase(s, "WED")) return 3;
        if (eqlIgnoreCase(s, "THU")) return 4;
        if (eqlIgnoreCase(s, "FRI")) return 5;
        if (eqlIgnoreCase(s, "SAT")) return 6;
    }

    const v = parsePositiveInt(s) catch return CronError.InvalidFieldValue;
    if (v < min_val || v > max_val) {
        return CronError.InvalidFieldValue;
    }
    return @as(u8, @intCast(v));
}

fn parsePositiveInt(s: []const u8): u32 ! CronError {
    if (s.len == 0) {
        return CronError.InvalidFieldValue;
    }
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const ch = s[i];
        if (ch < '0' || ch > '9') {
            return CronError.InvalidFieldValue;
        }
        acc = acc * 10 + @as(u32, @intCast(ch - '0'));
        if (acc > 1000) {
            return CronError.InvalidFieldValue;
        }
        i += 1;
    }
    return acc;
}

fn nextBitU16(mask: u16, start: u8, max_bit: u8): u8? {
    let b = start;
    while (b <= max_bit) {
        if ((mask & (@as(u16, 1) << @as(u4, @intCast(b)))) != 0) {
            return b;
        }
        b += 1;
    }
    return null;
}

fn prevBitU16(mask: u16, start: u8, min_bit: u8): u8? {
    let b = start;
    while (b >= min_bit) {
        if ((mask & (@as(u16, 1) << @as(u4, @intCast(b)))) != 0) {
            return b;
        }
        if (b == min_bit) {
            break;
        }
        b -= 1;
    }
    return null;
}

fn nextBitU32(mask: u32, start: u8, max_bit: u8): u8? {
    let b = start;
    while (b <= max_bit) {
        if ((mask & (@as(u32, 1) << @as(u5, @intCast(b)))) != 0) {
            return b;
        }
        b += 1;
    }
    return null;
}

fn prevBitU32(mask: u32, start: u8, min_bit: u8): u8? {
    let b = start;
    while (b >= min_bit) {
        if ((mask & (@as(u32, 1) << @as(u5, @intCast(b)))) != 0) {
            return b;
        }
        if (b == min_bit) {
            break;
        }
        b -= 1;
    }
    return null;
}

fn nextBitU64(mask: u64, start: u8, max_bit: u8): u8? {
    let b = start;
    while (b <= max_bit) {
        if ((mask & (@as(u64, 1) << @as(u6, @intCast(b)))) != 0) {
            return b;
        }
        b += 1;
    }
    return null;
}

fn prevBitU64(mask: u64, start: u8, min_bit: u8): u8? {
    let b = start;
    while (b >= min_bit) {
        if ((mask & (@as(u64, 1) << @as(u6, @intCast(b)))) != 0) {
            return b;
        }
        if (b == min_bit) {
            break;
        }
        b -= 1;
    }
    return null;
}

/// Decomposes UTC epoch seconds into `UtcDateTime` using Howard Hinnant's O(1) civil calendar algorithm.
pub fn decomposeUtc(epoch_sec: i64): UtcDateTime {
    const days = @divFloor(epoch_sec, 86400);
    const rem_sec = @mod(epoch_sec, 86400);
    const hour: u8 = @as(u8, @intCast(@divTrunc(rem_sec, 3600)));
    const minute: u8 = @as(u8, @intCast(@divTrunc(@mod(rem_sec, 3600), 60)));
    const second: u8 = @as(u8, @intCast(@mod(rem_sec, 60)));
    const dow: u8 = @as(u8, @intCast(@mod(days + 4, 7)));

    const z = days + 719468;
    const era = @divFloor(z, 146097);
    const doe = z - era * 146097;
    const yoe = @divFloor(doe - @divFloor(doe, 1460) + @divFloor(doe, 36524) - @divFloor(doe, 146096), 365);
    const y = yoe + era * 400;
    const doy = doe - (365 * yoe + @divFloor(yoe, 4) - @divFloor(yoe, 100));
    const mp = @divFloor(5 * doy + 2, 153);
    const d = doy - @divFloor(153 * mp + 2, 5) + 1;
    const m = if (mp < 10) mp + 3 else mp - 9;
    const year_adj: i64 = if (m <= 2) 1 else 0;

    return UtcDateTime{
        year: @as(i32, @intCast(y + year_adj)),
        month: @as(u8, @intCast(m)),
        day: @as(u8, @intCast(d)),
        hour: hour,
        minute: minute,
        second: second,
        dow: dow,
    };
}

/// Converts UTC calendar components to UTC epoch seconds.
pub fn toEpochSec(year: i32, month: u8, day: u8, hour: u8, minute: u8, second: u8): i64 {
    const days = daysFromCivil(year, month, day);
    return days * 86400 + @as(i64, hour) * 3600 + @as(i64, minute) * 60 + @as(i64, second);
}

fn daysFromCivil(year: i32, month: u8, day: u8): i64 {
    const y_adj: i64 = if (month <= 2) 1 else 0;
    const y = @as(i64, year) - y_adj;
    const era = @divFloor(y, 400);
    const yoe = y - era * 400;
    const m_adj: i64 = @as(i64, month) + (if (month > 2) @as(i64, -3) else @as(i64, 9));
    const doy = @divFloor(153 * m_adj + 2, 5) + @as(i64, day) - 1;
    const doe = yoe * 365 + @divFloor(yoe, 4) - @divFloor(yoe, 100) + doy;
    return era * 146097 + doe - 719468;
}

fn dayOfWeek(year: i32, month: u8, day: u8): u8 {
    const days = daysFromCivil(year, month, day);
    return @as(u8, @intCast(@mod(days + 4, 7)));
}

fn isLeapYear(year: i32): bool {
    return (@mod(year, 4) == 0 && @mod(year, 100) != 0) || (@mod(year, 400) == 0);
}

fn daysInMonth(year: i32, month: u8): u8 {
    if (month == 2) {
        return if (isLeapYear(year)) 29 else 28;
    }
    if (month == 4 || month == 6 || month == 9 || month == 11) {
        return 30;
    }
    return 31;
}

fn isWhitespace(ch: u8): bool {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWhitespace(s[start])) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && isWhitespace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn toUpperAscii(ch: u8): u8 {
    if (ch >= 'a' && ch <= 'z') {
        return ch - 32;
    }
    return ch;
}

fn eqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toUpperAscii(a[i]) != toUpperAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}
