// cap/test/testHttpDateHeader.cp
// Production-grade deep tests for cap:http dateHeader.cp (Bun DateHeaderTimer & RFC 7231 IMF-fixdate)

import { test, assert, assertEq } from "cap:test";
import {
    HTTP_DATE_HEADER_LEN,
    HTTP_DATE_HEADER_LINE_LEN,
    HTTP_DATE_REFRESH_MS,
    HTTP_DATE_MAX_EPOCH_SECS,
    DateHeaderError,
    DateHeaderTimerState,
    formatImfFixdate,
    formatDateHeaderLine,
    DateHeaderTimer,
} from "../../lib/http/dateHeader.cp";

fn eqStr(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("DateHeader: RFC 7231 canonical IMF-fixdate timestamps and leap years", () => {
    let buf: [40]u8 = [0; 40];

    // Unix epoch 0 -> Thu, 01 Jan 1970 00:00:00 GMT
    const n0 = formatImfFixdate(0, buf[0..]) catch unreachable;
    assertEq(n0, HTTP_DATE_HEADER_LEN);
    assert(eqStr(buf[0..n0], "Thu, 01 Jan 1970 00:00:00 GMT"));

    // RFC 7231 Section 7.1.1.1 canonical example: Sun, 06 Nov 1994 08:49:37 GMT
    const n1 = formatImfFixdate(784111777, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n1], "Sun, 06 Nov 1994 08:49:37 GMT"));

    // Leap day 2000-02-29 12:34:56 UTC (divisible by 400)
    const n2 = formatImfFixdate(951827696, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n2], "Tue, 29 Feb 2000 12:34:56 GMT"));

    // Leap day 2024-02-29 23:59:59 UTC
    const n3 = formatImfFixdate(1709251199, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n3], "Thu, 29 Feb 2024 23:59:59 GMT"));

    // Full header line formatting (37 bytes)
    const nl = formatDateHeaderLine(784111777, buf[0..]) catch unreachable;
    assertEq(nl, HTTP_DATE_HEADER_LINE_LEN);
    assert(eqStr(buf[0..nl], "Date: Sun, 06 Nov 1994 08:49:37 GMT\r\n"));
});

test("DateHeaderTimer: initial enable refreshes stale cache and schedules +1000ms", () => {
    let t = DateHeaderTimer.init();
    assert(!t.isActive());
    assertEq(t.dateSlice().len, 0);
    assertEq(t.headerLineSlice().len, 0);
    assertEq(t.refreshCount(), 0);

    const refreshed = t.enable(784111777250) catch unreachable;
    assert(refreshed);
    assert(t.isActive());
    assert(t.state == DateHeaderTimerState.Active);
    assertEq(t.nextFireMs(), 784111777250 + HTTP_DATE_REFRESH_MS);
    assertEq(t.refreshCount(), 1);
    assert(eqStr(t.dateSlice(), "Sun, 06 Nov 1994 08:49:37 GMT"));
    assert(eqStr(t.headerLineSlice(), "Date: Sun, 06 Nov 1994 08:49:37 GMT\r\n"));
});

test("DateHeaderTimer: onTick reschedules while connections active and idles at 0", () => {
    let t = DateHeaderTimer.init();
    * = t.enable(1000000) catch unreachable;
    assertEq(t.refreshCount(), 1);

    // Tick 1 second later with 5 active connections -> refreshes and stays Active
    const keep1 = t.onTick(1001000, 5) catch unreachable;
    assert(keep1);
    assert(t.isActive());
    assertEq(t.nextFireMs(), 1002000);
    assertEq(t.refreshCount(), 2);
    assert(eqStr(t.dateSlice(), "Thu, 01 Jan 1970 00:16:41 GMT"));

    // Tick within same second -> no redundant re-format, still reschedules
    const keep2 = t.onTick(1001500, 2) catch unreachable;
    assert(keep2);
    assertEq(t.refreshCount(), 2);

    // Tick 1 second later with 0 active connections -> refreshes once then goes Idle
    const keep3 = t.onTick(1002500, 0) catch unreachable;
    assert(!keep3);
    assert(!t.isActive());
    assert(t.state == DateHeaderTimerState.Idle);
    assertEq(t.refreshCount(), 3);
    assert(eqStr(t.dateSlice(), "Thu, 01 Jan 1970 00:16:42 GMT"));
});

test("DateHeaderTimer: re-enable within 1s reuses fresh cache without re-formatting", () => {
    let t = DateHeaderTimer.init();
    * = t.enable(5000000) catch unreachable;
    * = t.onTick(5001000, 0) catch unreachable; // now Idle, next_fire_ms == 5001000
    assertEq(t.refreshCount(), 2);

    // New connection arrives 400ms later (< 1000ms stale threshold): reuse cache
    const refreshed_soon = t.enable(5001400) catch unreachable;
    assert(!refreshed_soon);
    assert(t.isActive());
    assertEq(t.refreshCount(), 2);

    // Disable and re-enable > 1000ms later: stale -> immediate refresh
    t.disable();
    assert(!t.isActive());
    const refreshed_late = t.enable(5003000) catch unreachable;
    assert(refreshed_late);
    assertEq(t.refreshCount(), 3);
    assertEq(t.nextFireMs(), 5004000);
});

test("DateHeaderTimer: state machine guards (AlreadyActive / NotActive)", () => {
    let t = DateHeaderTimer.init();

    let not_active_err = false;
    * = t.onTick(1000, 1) catch |e| {
        not_active_err = (e == DateHeaderError.NotActive);
    };
    assert(not_active_err);

    * = t.enable(1000) catch unreachable;
    let already_active_err = false;
    * = t.enable(1500) catch |e| {
        already_active_err = (e == DateHeaderError.AlreadyActive);
    };
    assert(already_active_err);
});

test("DateHeader: invalid timestamps and tiny buffer error guards", () => {
    let buf: [40]u8 = [0; 40];

    let neg_err = false;
    * = formatImfFixdate(-1, buf[0..]) catch |e| {
        neg_err = (e == DateHeaderError.InvalidTimestamp);
    };
    assert(neg_err);

    let overflow_err = false;
    * = formatImfFixdate(HTTP_DATE_MAX_EPOCH_SECS + 1, buf[0..]) catch |e| {
        overflow_err = (e == DateHeaderError.InvalidTimestamp);
    };
    assert(overflow_err);

    // Exact upper bound 9999-12-31 23:59:59 GMT succeeds
    const nmax = formatImfFixdate(HTTP_DATE_MAX_EPOCH_SECS, buf[0..]) catch unreachable;
    assert(eqStr(buf[0..nmax], "Fri, 31 Dec 9999 23:59:59 GMT"));

    let tiny28: [28]u8 = [0; 28];
    let small_date_err = false;
    * = formatImfFixdate(0, tiny28[0..]) catch |e| {
        small_date_err = (e == DateHeaderError.BufferTooSmall);
    };
    assert(small_date_err);

    let tiny36: [36]u8 = [0; 36];
    let small_line_err = false;
    * = formatDateHeaderLine(0, tiny36[0..]) catch |e| {
        small_line_err = (e == DateHeaderError.BufferTooSmall);
    };
    assert(small_line_err);
});
