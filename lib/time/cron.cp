// Source: cp/lib/time/cron.cp (POSIX 5-Field Cron Expression Scheduler)
// lib/time/cron.cp
// Zero-allocation POSIX 5-field Cron parser, matcher, and deterministic next-trigger calculator

import { CivilDateTime } from "./iso8601.cp";

pub struct CronExpr {
    minutes: u64 = 0;       // bits 0..59
    hours: u32 = 0;         // bits 0..23
    days_of_month: u32 = 0; // bits 1..31
    months: u16 = 0;        // bits 1..12
    days_of_week: u8 = 0;   // bits 0..6 (0=SUN .. 6=SAT)
    dom_wildcard: bool = false;
    dow_wildcard: bool = false;

    priv static fn toUpperAscii(c: u8): u8 {
        if (c >= 'a' && c <= 'z') { return c - 32; }
        return c;
    }

    priv static fn eqIgnoreCase3(s: []const u8, a: u8, b: u8, c: u8): bool {
        if (s.len != 3) { return false; }
        return CronExpr.toUpperAscii(s[0]) == a
            && CronExpr.toUpperAscii(s[1]) == b
            && CronExpr.toUpperAscii(s[2]) == c;
    }

    priv static fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
        if (a.len != b.len) { return false; }
        let i: usize = 0;
        while (i < a.len) {
            if (CronExpr.toUpperAscii(a[i]) != CronExpr.toUpperAscii(b[i])) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    priv static fn trimAscii(s: []const u8): []const u8 {
        let start: usize = 0;
        let end: usize = s.len;
        while (start < end && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) {
            start += 1;
        }
        while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n')) {
            end -= 1;
        }
        return s[start..end];
    }

    priv static fn parseUint(s: []const u8): u8? {
        if (s.len == 0 || s.len > 3) { return null; }
        let acc: u16 = 0;
        let i: usize = 0;
        while (i < s.len) {
            const c = s[i];
            if (c < '0' || c > '9') { return null; }
            acc = acc * 10 + ((c - '0') as u16);
            i += 1;
        }
        if (acc > 255) { return null; }
        return acc as u8;
    }

    priv static fn parseMonthToken(tok: []const u8): u8? {
        if (tok.len == 3) {
            if (CronExpr.eqIgnoreCase3(tok, 'J', 'A', 'N')) { return 1; }
            if (CronExpr.eqIgnoreCase3(tok, 'F', 'E', 'B')) { return 2; }
            if (CronExpr.eqIgnoreCase3(tok, 'M', 'A', 'R')) { return 3; }
            if (CronExpr.eqIgnoreCase3(tok, 'A', 'P', 'R')) { return 4; }
            if (CronExpr.eqIgnoreCase3(tok, 'M', 'A', 'Y')) { return 5; }
            if (CronExpr.eqIgnoreCase3(tok, 'J', 'U', 'N')) { return 6; }
            if (CronExpr.eqIgnoreCase3(tok, 'J', 'U', 'L')) { return 7; }
            if (CronExpr.eqIgnoreCase3(tok, 'A', 'U', 'G')) { return 8; }
            if (CronExpr.eqIgnoreCase3(tok, 'S', 'E', 'P')) { return 9; }
            if (CronExpr.eqIgnoreCase3(tok, 'O', 'C', 'T')) { return 10; }
            if (CronExpr.eqIgnoreCase3(tok, 'N', 'O', 'V')) { return 11; }
            if (CronExpr.eqIgnoreCase3(tok, 'D', 'E', 'C')) { return 12; }
        }
        const n = CronExpr.parseUint(tok) ?? return null;
        if (n < 1 || n > 12) { return null; }
        return n;
    }

    priv static fn parseDowToken(tok: []const u8): u8? {
        if (tok.len == 3) {
            if (CronExpr.eqIgnoreCase3(tok, 'S', 'U', 'N')) { return 0; }
            if (CronExpr.eqIgnoreCase3(tok, 'M', 'O', 'N')) { return 1; }
            if (CronExpr.eqIgnoreCase3(tok, 'T', 'U', 'E')) { return 2; }
            if (CronExpr.eqIgnoreCase3(tok, 'W', 'E', 'D')) { return 3; }
            if (CronExpr.eqIgnoreCase3(tok, 'T', 'H', 'U')) { return 4; }
            if (CronExpr.eqIgnoreCase3(tok, 'F', 'R', 'I')) { return 5; }
            if (CronExpr.eqIgnoreCase3(tok, 'S', 'A', 'T')) { return 6; }
        }
        const n = CronExpr.parseUint(tok) ?? return null;
        if (n > 7) { return null; }
        return n;
    }

    priv static fn parseTokenValue(tok: []const u8, min_val: u8, max_val: u8, kind: u8): u8? {
        // kind: 0=numeric, 1=month, 2=dow
        if (kind == 1) { return CronExpr.parseMonthToken(tok); }
        if (kind == 2) { return CronExpr.parseDowToken(tok); }
        const n = CronExpr.parseUint(tok) ?? return null;
        if (n < min_val || n > max_val) { return null; }
        return n;
    }

    priv static fn indexOfByte(s: []const u8, needle: u8): usize? {
        let i: usize = 0;
        while (i < s.len) {
            if (s[i] == needle) { return i; }
            i += 1;
        }
        return null;
    }

    priv static fn parseFieldMask(field: []const u8, min_val: u8, max_val: u8, kind: u8): u64? {
        if (field.len == 0) { return null; }

        let mask: u64 = 0;
        let pos: usize = 0;

        while (pos <= field.len) {
            // Find next ','
            let end = pos;
            while (end < field.len && field[end] != ',') {
                end += 1;
            }
            const item = field[pos..end];
            if (item.len == 0) { return null; }

            let range_part = item;
            let step: u8 = 1;
            let has_step = false;
            if (slash_idx := CronExpr.indexOfByte(item, '/')) {
                has_step = true;
                range_part = item[0..slash_idx];
                const step_str = item[(slash_idx + 1)..];
                if (range_part.len == 0 || step_str.len == 0) { return null; }
                const parsed_step = CronExpr.parseUint(step_str) ?? return null;
                if (parsed_step == 0) { return null; }
                step = parsed_step;
            }

            let lo: u8 = min_val;
            let hi: u8 = max_val;

            if (range_part.len == 1 && range_part[0] == '*') {
                lo = min_val;
                hi = if (kind == 2) (6 as u8) else max_val;
            } else if (dash_idx := CronExpr.indexOfByte(range_part, '-')) {
                const left = range_part[0..dash_idx];
                const right = range_part[(dash_idx + 1)..];
                if (left.len == 0 || right.len == 0) { return null; }
                const p_lo = CronExpr.parseTokenValue(left, min_val, max_val, kind) ?? return null;
                const p_hi = CronExpr.parseTokenValue(right, min_val, max_val, kind) ?? return null;
                lo = p_lo;
                hi = p_hi;
                if (lo > hi) { return null; }
            } else {
                const p_lo = CronExpr.parseTokenValue(range_part, min_val, max_val, kind) ?? return null;
                lo = p_lo;
                hi = if (has_step) (if (kind == 2) (6 as u8) else max_val) else lo;
                if (has_step && lo > hi) { return null; }
            }

            let v: u16 = lo as u16;
            const hi16: u16 = hi as u16;
            const step16: u16 = step as u16;
            while (v <= hi16) {
                const bit: u8 = (if (kind == 2 && v == 7) (0 as u16) else v) as u8;
                mask |= (1 as u64) << bit;
                v += step16;
            }

            if (end == field.len) { break; }
            pos = end + 1;
            if (pos == field.len) { return null; }
        }

        if (mask == 0) { return null; }
        return mask;
    }

    /// Parses a POSIX 5-field cron expression or `@yearly/@monthly/@weekly/@daily/@hourly` alias.
    static fn parse(expr: []const u8): CronExpr? {
        const trimmed = CronExpr.trimAscii(expr);
        if (trimmed.len == 0) { return null; }

        if (trimmed[0] == '@') {
            if (CronExpr.eqIgnoreCase(trimmed, "@yearly") || CronExpr.eqIgnoreCase(trimmed, "@annually")) {
                return CronExpr.parse("0 0 1 1 *");
            }
            if (CronExpr.eqIgnoreCase(trimmed, "@monthly")) {
                return CronExpr.parse("0 0 1 * *");
            }
            if (CronExpr.eqIgnoreCase(trimmed, "@weekly")) {
                return CronExpr.parse("0 0 * * 0");
            }
            if (CronExpr.eqIgnoreCase(trimmed, "@daily") || CronExpr.eqIgnoreCase(trimmed, "@midnight")) {
                return CronExpr.parse("0 0 * * *");
            }
            if (CronExpr.eqIgnoreCase(trimmed, "@hourly")) {
                return CronExpr.parse("0 * * * *");
            }
            return null;
        }

        let fields: [5][]const u8 = ["", "", "", "", ""];
        let count: usize = 0;
        let i: usize = 0;
        while (i < trimmed.len) {
            while (i < trimmed.len && (trimmed[i] == ' ' || trimmed[i] == '\t')) {
                i += 1;
            }
            if (i >= trimmed.len) { break; }
            const f_start = i;
            while (i < trimmed.len && trimmed[i] != ' ' && trimmed[i] != '\t') {
                i += 1;
            }
            if (count >= 5) { return null; }
            fields[count] = trimmed[f_start..i];
            count += 1;
        }

        if (count != 5) { return null; }

        const min_mask = CronExpr.parseFieldMask(fields[0], 0, 59, 0) ?? return null;
        const hr_mask = CronExpr.parseFieldMask(fields[1], 0, 23, 0) ?? return null;
        const dom_mask = CronExpr.parseFieldMask(fields[2], 1, 31, 0) ?? return null;
        const mon_mask = CronExpr.parseFieldMask(fields[3], 1, 12, 1) ?? return null;
        const dow_mask = CronExpr.parseFieldMask(fields[4], 0, 7, 2) ?? return null;

        const dom_wild = fields[2].len == 1 && fields[2][0] == '*';
        const dow_wild = fields[4].len == 1 && fields[4][0] == '*';

        return CronExpr {
            minutes: min_mask,
            hours: hr_mask as u32,
            days_of_month: dom_mask as u32,
            months: mon_mask as u16,
            days_of_week: dow_mask as u8,
            dom_wildcard: dom_wild,
            dow_wildcard: dow_wild,
        };
    }

    priv fn dayMatches(dt: CivilDateTime): bool {
        const dom_ok = (_.days_of_month & ((1 as u32) << dt.day)) != 0;
        const dow = dt.dayOfWeek();
        const dow_ok = (_.days_of_week & ((1 as u8) << dow)) != 0;

        if (!_.dom_wildcard && !_.dow_wildcard) {
            return dom_ok || dow_ok;
        }
        return dom_ok && dow_ok;
    }

    /// Returns true if `dt` matches this cron schedule (minute, hour, day-of-month/day-of-week, month).
    fn matches(dt: CivilDateTime): bool {
        if (!dt.isValid()) { return false; }
        if ((_.minutes & ((1 as u64) << dt.minute)) == 0) { return false; }
        if ((_.hours & ((1 as u32) << dt.hour)) == 0) { return false; }
        if ((_.months & ((1 as u16) << dt.month)) == 0) { return false; }
        return _.dayMatches(dt);
    }

    fn matchesEpochSec(epoch_sec: i64): bool {
        return _.matches(CivilDateTime.fromUnixSeconds(epoch_sec));
    }

    priv fn hasPossibleMonthDay(): bool {
        if (!_.dow_wildcard) { return true; }
        let m: u8 = 1;
        while (m <= 12) {
            if ((_.months & ((1 as u16) << m)) != 0) {
                const max_d: u8 = if (m == 2) (29 as u8) else CivilDateTime.daysInMonth(2024, m);
                let d: u8 = 1;
                while (d <= max_d) {
                    if ((_.days_of_month & ((1 as u32) << d)) != 0) {
                        return true;
                    }
                    d += 1;
                }
            }
            m += 1;
        }
        return false;
    }

    priv static fn nextSetBitFromU64(mask: u64, start_val: u8, max_val: u8): u8? {
        let v: u8 = start_val;
        while (v <= max_val) {
            if ((mask & ((1 as u64) << v)) != 0) {
                return v;
            }
            v += 1;
        }
        return null;
    }

    /// Computes the next Unix timestamp (seconds, UTC) strictly after `from_epoch_sec`.
    fn nextAfter(from_epoch_sec: i64): i64? {
        if (!_.hasPossibleMonthDay()) { return null; }

        const start_sec = from_epoch_sec - @mod(from_epoch_sec, 60) + 60;
        let dt = CivilDateTime.fromUnixSeconds(start_sec);
        const max_year: i32 = dt.year + 400;

        while (dt.year <= max_year) {
            // 1. Align Month
            if ((_.months & ((1 as u16) << dt.month)) == 0) {
                if (nm := CronExpr.nextSetBitFromU64(_.months as u64, dt.month, 12)) {
                    dt.month = nm;
                    dt.day = 1;
                    dt.hour = 0;
                    dt.minute = 0;
                } else {
                    dt.year += 1;
                    dt.month = 1;
                    dt.day = 1;
                    dt.hour = 0;
                    dt.minute = 0;
                }
                continue;
            }

            // 2. Align Day within Month
            const dim = CivilDateTime.daysInMonth(dt.year, dt.month);
            if (dt.day > dim || !_.dayMatches(dt)) {
                dt.day += 1;
                dt.hour = 0;
                dt.minute = 0;
                if (dt.day > dim) {
                    dt.day = 1;
                    if (dt.month < 12) {
                        dt.month += 1;
                    } else {
                        dt.month = 1;
                        dt.year += 1;
                    }
                }
                continue;
            }

            // 3. Align Hour
            if ((_.hours & ((1 as u32) << dt.hour)) == 0) {
                if (nh := CronExpr.nextSetBitFromU64(_.hours as u64, dt.hour, 23)) {
                    dt.hour = nh;
                    dt.minute = 0;
                } else {
                    dt.day += 1;
                    dt.hour = 0;
                    dt.minute = 0;
                    if (dt.day > dim) {
                        dt.day = 1;
                        if (dt.month < 12) {
                            dt.month += 1;
                        } else {
                            dt.month = 1;
                            dt.year += 1;
                        }
                    }
                }
                continue;
            }

            // 4. Align Minute
            if ((_.minutes & ((1 as u64) << dt.minute)) == 0) {
                if (nmin := CronExpr.nextSetBitFromU64(_.minutes, dt.minute, 59)) {
                    dt.minute = nmin;
                } else {
                    dt.minute = 0;
                    if (dt.hour < 23) {
                        dt.hour += 1;
                    } else {
                        dt.hour = 0;
                        dt.day += 1;
                        if (dt.day > dim) {
                            dt.day = 1;
                            if (dt.month < 12) {
                                dt.month += 1;
                            } else {
                                dt.month = 1;
                                dt.year += 1;
                            }
                        }
                    }
                }
                continue;
            }

            return dt.toUnixSeconds();
        }

        return null;
    }

    fn nextAfterDateTime(dt: CivilDateTime): CivilDateTime? {
        const next_sec = _.nextAfter(dt.toUnixSeconds()) ?? return null;
        if (dt.tz_offset_minutes == 0) {
            return CivilDateTime.fromUnixSeconds(next_sec);
        }
        return CivilDateTime.fromUnixSecondsWithOffset(next_sec, dt.tz_offset_minutes);
    }
}
