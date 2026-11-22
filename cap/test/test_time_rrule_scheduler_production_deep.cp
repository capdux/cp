// Production Deep Test Suite for cap:time RFC 5545 RRULE Scheduler (lib/time/rrule_scheduler.cp)
import { test, expect_eq, expect_true } from "cap:test";
import {
    RRuleFreq,
    RRuleWeekday,
    RRuleDateTime,
    isLeapYear,
    daysInMonth,
    rruleDateTimeToUnix,
    rruleUnixToDateTime,
    rruleWeekdayOfUnix,
    parseRRuleUtcTimestamp,
    parseRRule,
    formatRRule,
} from "cap:time";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn utc(y: i32, mo: u8, d: u8, h: u8, mi: u8, s: u8): i64 {
    return rruleDateTimeToUnix(RRuleDateTime{
        year: y,
        month: mo,
        day: d,
        hour: h,
        minute: mi,
        second: s,
    }) ?? 0;
}

// ============================================================================
// Test 1: Pure-integer Gregorian calendar, leap years & weekday accuracy
// ============================================================================
test("rrule_scheduler: pure-integer Gregorian calendar, leap years & weekday accuracy", fn() {
    expect_true(isLeapYear(2000));
    expect_true(!isLeapYear(1900));
    expect_true(isLeapYear(2024));
    expect_true(!isLeapYear(2025));
    expect_eq(daysInMonth(2024, 2), 29 as u8);
    expect_eq(daysInMonth(2025, 2), 28 as u8);
    expect_eq(daysInMonth(2026, 4), 30 as u8);
    expect_eq(daysInMonth(2026, 7), 31 as u8);

    // 1970-01-01 00:00:00 UTC is epoch 0 (Thursday)
    const epoch0 = utc(1970, 1, 1, 0, 0, 0);
    expect_eq(epoch0, 0 as i64);
    expect_eq(rruleWeekdayOfUnix(epoch0), RRuleWeekday.th);

    // 2024-02-29 12:30:45 UTC round-trip (Thursday)
    const leap_ts = utc(2024, 2, 29, 12, 30, 45);
    const back = rruleUnixToDateTime(leap_ts);
    expect_eq(back.year, 2024 as i32);
    expect_eq(back.month, 2 as u8);
    expect_eq(back.day, 29 as u8);
    expect_eq(back.hour, 12 as u8);
    expect_eq(back.minute, 30 as u8);
    expect_eq(back.second, 45 as u8);
    expect_eq(rruleWeekdayOfUnix(leap_ts), RRuleWeekday.th);

    // Invalid calendar dates rejected
    expect_true(rruleDateTimeToUnix(RRuleDateTime{ year: 2025, month: 2, day: 29, hour: 0, minute: 0, second: 0 }) == null);
    expect_true(rruleDateTimeToUnix(RRuleDateTime{ year: 2026, month: 4, day: 31, hour: 0, minute: 0, second: 0 }) == null);
});

// ============================================================================
// Test 2: parseRRule & formatRRule canonical round-trip
// ============================================================================
test("rrule_scheduler: parseRRule & formatRRule canonical round-trip", fn() {
    const raw = "RRULE:FREQ=WEEKLY;INTERVAL=2;UNTIL=20261231T235959Z;BYMONTH=1,6,12;BYDAY=MO,WE,FR;BYHOUR=9,17;BYMINUTE=0,30;WKST=SU";
    const rule = parseRRule(raw) ?? return;

    expect_eq(rule.freq, RRuleFreq.weekly);
    expect_eq(rule.interval, 2 as u32);
    expect_true(rule.has_until);
    expect_true(!rule.has_count);
    expect_eq(rule.until_ts, utc(2026, 12, 31, 23, 59, 59));
    expect_eq(rule.wkst, RRuleWeekday.su);

    let buf: [256]u8 = [0; 256];
    let buf_s: []u8 = buf[0..];
    if (written := formatRRule(rule, *buf_s)) {
        const expected = "FREQ=WEEKLY;INTERVAL=2;UNTIL=20261231T235959Z;BYMONTH=1,6,12;BYDAY=MO,WE,FR;BYHOUR=9,17;BYMINUTE=0,30;WKST=SU";
        expect_true(bytesEq(buf[0..written], expected));

        // Re-parsing formatted output produces identical fields
        const reparsed = parseRRule(buf[0..written]) ?? return;
        expect_eq(reparsed.freq, rule.freq);
        expect_eq(reparsed.interval, rule.interval);
        expect_eq(reparsed.until_ts, rule.until_ts);
        expect_eq(reparsed.by_day_mask, rule.by_day_mask);
        expect_eq(reparsed.by_hour_mask, rule.by_hour_mask);
        expect_eq(reparsed.by_minute_mask, rule.by_minute_mask);
    } else {
        expect_true(false);
    }

    // Undersized format buffer returns null safely
    let tiny: [8]u8 = [0; 8];
    let tiny_s: []u8 = tiny[0..];
    expect_true(formatRRule(rule, *tiny_s) == null);
});

// ============================================================================
// Test 3: WEEKLY with INTERVAL, BYDAY, WKST & COUNT termination
// ============================================================================
test("rrule_scheduler: WEEKLY with INTERVAL=2, BYDAY=MO,WE,FR & COUNT=5", fn() {
    const rule = parseRRule("FREQ=WEEKLY;INTERVAL=2;BYDAY=MO,WE,FR;COUNT=5") ?? return;
    // 2026-01-05 is Monday
    const dtstart = utc(2026, 1, 5, 9, 0, 0);
    expect_eq(rruleWeekdayOfUnix(dtstart), RRuleWeekday.mo);

    let out: [8]i64 = [0; 8];
    let out_s: []i64 = out[0..];
    const n = rule.expandOccurrences(dtstart, dtstart, utc(2026, 3, 1, 0, 0, 0), *out_s);
    expect_eq(n, 5 as usize);
    // Week 0: Jan 5 (MO), Jan 7 (WE), Jan 9 (FR)
    // Week 2: Jan 19 (MO), Jan 21 (WE)
    expect_eq(out[0], utc(2026, 1, 5, 9, 0, 0));
    expect_eq(out[1], utc(2026, 1, 7, 9, 0, 0));
    expect_eq(out[2], utc(2026, 1, 9, 9, 0, 0));
    expect_eq(out[3], utc(2026, 1, 19, 9, 0, 0));
    expect_eq(out[4], utc(2026, 1, 21, 9, 0, 0));

    // After the 5th occurrence, nextOccurrence must return null (COUNT exhausted)
    expect_true(rule.nextOccurrence(dtstart, out[4]) == null);
});

// ============================================================================
// Test 4: MONTHLY 31st-day skipping & YEARLY leap-year Feb 29 handling
// ============================================================================
test("rrule_scheduler: MONTHLY 31st-day skipping & YEARLY leap-year Feb 29", fn() {
    // Monthly on the 31st -> skips Feb, Apr, Jun...
    const m_rule = parseRRule("FREQ=MONTHLY;COUNT=4") ?? return;
    const jan31 = utc(2026, 1, 31, 8, 0, 0);

    let out: [4]i64 = [0; 4];
    let out_s: []i64 = out[0..];
    const mn = m_rule.expandOccurrences(jan31, jan31, utc(2027, 1, 1, 0, 0, 0), *out_s);
    expect_eq(mn, 4 as usize);
    expect_eq(out[0], utc(2026, 1, 31, 8, 0, 0));
    expect_eq(out[1], utc(2026, 3, 31, 8, 0, 0));
    expect_eq(out[2], utc(2026, 5, 31, 8, 0, 0));
    expect_eq(out[3], utc(2026, 7, 31, 8, 0, 0));

    // Yearly on Feb 29 -> 2024, 2028, 2032 (skips non-leap years 2025, 2026, 2027)
    const y_rule = parseRRule("FREQ=YEARLY;COUNT=3") ?? return;
    const feb29 = utc(2024, 2, 29, 0, 0, 0);
    let y_out: [4]i64 = [0; 4];
    let y_s: []i64 = y_out[0..];
    const yn = y_rule.expandOccurrences(feb29, feb29, utc(2040, 1, 1, 0, 0, 0), *y_s);
    expect_eq(yn, 3 as usize);
    expect_eq(y_out[0], utc(2024, 2, 29, 0, 0, 0));
    expect_eq(y_out[1], utc(2028, 2, 29, 0, 0, 0));
    expect_eq(y_out[2], utc(2032, 2, 29, 0, 0, 0));
});

// ============================================================================
// Test 5: DAILY with BYHOUR/BYMINUTE & UNTIL boundary truncation
// ============================================================================
test("rrule_scheduler: DAILY with BYHOUR/BYMINUTE & UNTIL truncation", fn() {
    const rule = parseRRule("FREQ=DAILY;BYHOUR=8,20;BYMINUTE=0,30;UNTIL=20260502T081500Z") ?? return;
    const dtstart = utc(2026, 5, 1, 8, 15, 0);

    let out: [8]i64 = [0; 8];
    let out_s: []i64 = out[0..];
    const n = rule.expandOccurrences(dtstart, dtstart, utc(2026, 5, 10, 0, 0, 0), *out_s);
    // May 1: 08:00 (< dtstart skipped), 08:30, 20:00, 20:30
    // May 2: 08:00 (<= UNTIL 08:15), 08:30 (> UNTIL stops!)
    expect_eq(n, 4 as usize);
    expect_eq(out[0], utc(2026, 5, 1, 8, 30, 0));
    expect_eq(out[1], utc(2026, 5, 1, 20, 0, 0));
    expect_eq(out[2], utc(2026, 5, 1, 20, 30, 0));
    expect_eq(out[3], utc(2026, 5, 2, 8, 0, 0));
});

// ============================================================================
// Test 6: nextOccurrence sequential stepping & window-bounded expansion
// ============================================================================
test("rrule_scheduler: nextOccurrence sequential stepping & window-bounded expansion", fn() {
    const rule = parseRRule("FREQ=DAILY;INTERVAL=3") ?? return;
    const dtstart = utc(2026, 6, 1, 10, 0, 0);

    // Step via nextOccurrence starting before dtstart
    const n1 = rule.nextOccurrence(dtstart, dtstart - 1) ?? 0;
    const n2 = rule.nextOccurrence(dtstart, n1) ?? 0;
    const n3 = rule.nextOccurrence(dtstart, n2) ?? 0;
    expect_eq(n1, utc(2026, 6, 1, 10, 0, 0));
    expect_eq(n2, utc(2026, 6, 4, 10, 0, 0));
    expect_eq(n3, utc(2026, 6, 7, 10, 0, 0));

    // Window [June 4, June 10] captures June 4, June 7, June 10 (3 hits)
    let out: [8]i64 = [0; 8];
    let out_s: []i64 = out[0..];
    const cnt = rule.expandOccurrences(dtstart, utc(2026, 6, 4, 0, 0, 0), utc(2026, 6, 10, 23, 59, 59), *out_s);
    expect_eq(cnt, 3 as usize);
    expect_eq(out[0], utc(2026, 6, 4, 10, 0, 0));
    expect_eq(out[1], utc(2026, 6, 7, 10, 0, 0));
    expect_eq(out[2], utc(2026, 6, 10, 10, 0, 0));

    // Reversed window safely returns 0
    expect_eq(rule.expandOccurrences(dtstart, utc(2026, 6, 10, 0, 0, 0), utc(2026, 6, 1, 0, 0, 0), *out_s), 0 as usize);
});

// ============================================================================
// Test 7: Strict RFC 5545 boundary guards
// ============================================================================
test("rrule_scheduler: strict RFC 5545 boundary guards", fn() {
    expect_true(parseRRule("") == null);
    expect_true(parseRRule("RRULE:") == null);
    expect_true(parseRRule("INTERVAL=2;COUNT=5") == null); // missing FREQ
    expect_true(parseRRule("FREQ=HOURLY") == null); // unsupported/invalid FREQ
    expect_true(parseRRule("FREQ=DAILY;INTERVAL=0") == null); // INTERVAL=0
    expect_true(parseRRule("FREQ=DAILY;COUNT=0") == null); // COUNT=0
    expect_true(parseRRule("FREQ=DAILY;COUNT=5;UNTIL=20261231T000000Z") == null); // COUNT + UNTIL forbidden by RFC 5545
    expect_true(parseRRule("FREQ=DAILY;UNTIL=20250229T000000Z") == null); // non-leap Feb 29
    expect_true(parseRRule("FREQ=DAILY;UNTIL=20261231") == null); // malformed UNTIL
    expect_true(parseRRule("FREQ=WEEKLY;BYDAY=XX") == null); // invalid weekday
    expect_true(parseRRule("FREQ=MONTHLY;BYMONTH=13") == null); // month > 12
    expect_true(parseRRule("FREQ=MONTHLY;BYMONTHDAY=32") == null); // day > 31
    expect_true(parseRRule("FREQ=DAILY;BYHOUR=24") == null); // hour > 23
    expect_true(parseRRule("FREQ=DAILY;BYMINUTE=60") == null); // minute > 59
    expect_true(parseRRule("FREQ=DAILY;UNKNOWN=1") == null); // unknown key
    expect_true(parseRRule("FREQ=DAILY;FREQ=WEEKLY") == null); // duplicate key
    expect_true(parseRRuleUtcTimestamp("20261301T000000Z") == null);
});
