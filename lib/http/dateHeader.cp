// Source: Absorbed from /data/dev/bun/src/runtime/timer/DateHeaderTimer.cp | Synced: 2026-11-23T20:39:52Z
// ============================================================================
// lib/http/dateHeader.cp — Zero-Alloc HTTP Date Header Timer & IMF-fixdate Cache
// ============================================================================
// Implements Bun.serve()'s `DateHeaderTimer` scheduling semantics and RFC 7231
// Section 7.1.1.1 IMF-fixdate formatting (`"Sun, 06 Nov 1994 08:49:37 GMT"`):
//   - Caches the 29-byte Date value and 37-byte `"Date: ...\r\n"` header line
//     in fixed inline buffers with zero heap allocation.
//   - Refreshes at most once per second (1000ms) while active HTTP connections
//     exist, and automatically transitions to `.Idle` when `active_connections == 0`
//     to eliminate idle CPU wakeups.

/// Fixed byte length of an RFC 7231 IMF-fixdate string (e.g. `"Thu, 01 Jan 1970 00:00:00 GMT"`).
pub const HTTP_DATE_HEADER_LEN: usize = 29;
/// Fixed byte length of `"Date: <IMF-fixdate>\r\n"`.
pub const HTTP_DATE_HEADER_LINE_LEN: usize = 37;
/// Periodic refresh interval in milliseconds (aligned with Bun `std.time.ms_per_s`).
pub const HTTP_DATE_REFRESH_MS: i64 = 1000;
/// Maximum supported Unix timestamp (9999-12-31 23:59:59 UTC).
pub const HTTP_DATE_MAX_EPOCH_SECS: i64 = 253402300799;

pub err DateHeaderError {
    BufferTooSmall,
    AlreadyActive,
    NotActive,
    InvalidTimestamp,
}

pub enum DateHeaderTimerState {
    Idle,
    Active,
    Fired,
}

fn dayName3(dow: usize): []const u8 {
    if (dow == 0) return "Sun";
    if (dow == 1) return "Mon";
    if (dow == 2) return "Tue";
    if (dow == 3) return "Wed";
    if (dow == 4) return "Thu";
    if (dow == 5) return "Fri";
    return "Sat";
}

fn monthName3(month: usize): []const u8 {
    if (month == 1) return "Jan";
    if (month == 2) return "Feb";
    if (month == 3) return "Mar";
    if (month == 4) return "Apr";
    if (month == 5) return "May";
    if (month == 6) return "Jun";
    if (month == 7) return "Jul";
    if (month == 8) return "Aug";
    if (month == 9) return "Sep";
    if (month == 10) return "Oct";
    if (month == 11) return "Nov";
    return "Dec";
}

fn write2Digits(out: []u8, pos: usize, v: u64) {
    out[pos] = @as(u8, @intCast('0' + ((v / 10) % 10)));
    out[pos + 1] = @as(u8, @intCast('0' + (v % 10)));
}

fn write4Digits(out: []u8, pos: usize, v: u64) {
    out[pos] = @as(u8, @intCast('0' + ((v / 1000) % 10)));
    out[pos + 1] = @as(u8, @intCast('0' + ((v / 100) % 10)));
    out[pos + 2] = @as(u8, @intCast('0' + ((v / 10) % 10)));
    out[pos + 3] = @as(u8, @intCast('0' + (v % 10)));
}

fn copySlice(out: []u8, pos: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
}

/// Formats a Unix epoch timestamp (UTC seconds) into an RFC 7231 IMF-fixdate
/// string (`"DDD, DD MMM YYYY HH:MM:SS GMT"`, always 29 bytes) with zero heap
/// allocation using Howard Hinnant's O(1) civil-from-days algorithm.
pub fn formatImfFixdate(epoch_secs: i64, out: []u8): DateHeaderError!usize {
    if (epoch_secs < 0 || epoch_secs > HTTP_DATE_MAX_EPOCH_SECS) {
        return DateHeaderError.InvalidTimestamp;
    }
    if (out.len < HTTP_DATE_HEADER_LEN) return DateHeaderError.BufferTooSmall;

    const secs_u: u64 = @as(u64, @intCast(epoch_secs));
    const days: i64 = @as(i64, @intCast(secs_u / 86400));
    const rem: u64 = secs_u % 86400;

    const hour: u64 = rem / 3600;
    const minute: u64 = (rem % 3600) / 60;
    const second: u64 = rem % 60;

    // 1970-01-01 was Thursday (dow == 4).
    const dow: usize = @as(usize, @intCast((days + 4) % 7));

    // Howard Hinnant civil-from-days algorithm (Gregorian proleptic calendar).
    const z: i64 = days + 719468;
    const era: i64 = @divFloor(z, 146097);
    const doe: u64 = @as(u64, @intCast(z - era * 146097));
    const yoe: u64 = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const y: i64 = @as(i64, @intCast(yoe)) + era * 400;
    const doy: u64 = doe - (yoe * 365 + yoe / 4 - yoe / 100);
    const mp: u64 = (doy * 5 + 2) / 153;
    const day: u64 = doy - (mp * 153 + 2) / 5 + 1;
    const month: u64 = if (mp < 10) (mp + 3) else (mp - 9);
    const year_adj: i64 = if (month <= 2) 1 else 0;
    const year: u64 = @as(u64, @intCast(y + year_adj));

    copySlice(out, 0, dayName3(dow));
    out[3] = ',';
    out[4] = ' ';
    write2Digits(out, 5, day);
    out[7] = ' ';
    copySlice(out, 8, monthName3(@as(usize, @intCast(month))));
    out[11] = ' ';
    write4Digits(out, 12, year);
    out[16] = ' ';
    write2Digits(out, 17, hour);
    out[19] = ':';
    write2Digits(out, 20, minute);
    out[22] = ':';
    write2Digits(out, 23, second);
    copySlice(out, 25, " GMT");
    return HTTP_DATE_HEADER_LEN;
}

/// Formats a complete HTTP response Date header line:
/// `"Date: DDD, DD MMM YYYY HH:MM:SS GMT\r\n"` (always 37 bytes).
pub fn formatDateHeaderLine(epoch_secs: i64, out: []u8): DateHeaderError!usize {
    if (out.len < HTTP_DATE_HEADER_LINE_LEN) return DateHeaderError.BufferTooSmall;
    copySlice(out, 0, "Date: ");
    * = formatImfFixdate(epoch_secs, out[6..35])?;
    out[35] = '\r';
    out[36] = '\n';
    return HTTP_DATE_HEADER_LINE_LEN;
}

/// Periodic 1-second HTTP `Date` header cache & timer state machine aligned
/// with Bun's `DateHeaderTimer` in `Bun.serve()`.
pub struct DateHeaderTimer {
    state: DateHeaderTimerState = DateHeaderTimerState.Idle,
    next_fire_ms: i64 = 0,
    last_epoch_sec: i64 = -1,
    refresh_count: u64 = 0,
    cached_date: [29]u8 = [0; 29],
    cached_line: [37]u8 = [0; 37],
    has_cached: bool = false,

    static fn init(): DateHeaderTimer {
        return DateHeaderTimer{};
    }

    fn refreshForMs(*_, now_ms: i64): DateHeaderError!void {
        if (now_ms < 0) return DateHeaderError.InvalidTimestamp;
        const sec = @divFloor(now_ms, 1000);
        if (!_.has_cached || sec != _.last_epoch_sec) {
            * = formatImfFixdate(sec, _.cached_date[0..])?;
            * = formatDateHeaderLine(sec, _.cached_line[0..])?;
            _.last_epoch_sec = sec;
            _.has_cached = true;
            _.refresh_count += 1;
        }
    }

    /// Schedules the Date header timer (aligned with Bun `DateHeaderTimer.enable`).
    /// - Rejects double activation (`AlreadyActive`).
    /// - If uninitialized or stale (`now_ms - next_fire_ms >= 1000`), refreshes
    ///   the cached header immediately, schedules `next_fire_ms = now_ms + 1000`,
    ///   and returns `true`.
    /// - Otherwise preserves the still-fresh cache & deadline, marks `.Active`,
    ///   and returns `false`.
    fn enable(*_, now_ms: i64): DateHeaderError!bool {
        if (_.state == DateHeaderTimerState.Active) return DateHeaderError.AlreadyActive;
        if (now_ms < 0) return DateHeaderError.InvalidTimestamp;

        const is_stale = !_.has_cached || (now_ms - _.next_fire_ms >= HTTP_DATE_REFRESH_MS);
        if (is_stale) {
            _.refreshForMs(now_ms)?;
            _.next_fire_ms = now_ms + HTTP_DATE_REFRESH_MS;
            _.state = DateHeaderTimerState.Active;
            return true;
        }

        _.state = DateHeaderTimerState.Active;
        return false;
    }

    /// Handles a timer firing event (aligned with Bun `DateHeaderTimer.run`).
    /// Refreshes the cached Date header for `now_ms`. If `active_connections > 0`,
    /// reschedules for `now_ms + 1000` (state remains `.Active`) and returns `true`;
    /// otherwise transitions to `.Idle` and returns `false` to prevent idle wakeups.
    fn onTick(*_, now_ms: i64, active_connections: usize): DateHeaderError!bool {
        if (_.state != DateHeaderTimerState.Active) return DateHeaderError.NotActive;
        if (now_ms < 0) return DateHeaderError.InvalidTimestamp;

        _.state = DateHeaderTimerState.Fired;
        _.refreshForMs(now_ms)?;

        if (active_connections > 0) {
            _.next_fire_ms = now_ms + HTTP_DATE_REFRESH_MS;
            _.state = DateHeaderTimerState.Active;
            return true;
        }

        _.next_fire_ms = now_ms;
        _.state = DateHeaderTimerState.Idle;
        return false;
    }

    /// Explicitly stops the timer and transitions to `.Idle`.
    fn disable(*_) {
        _.state = DateHeaderTimerState.Idle;
    }

    fn isActive(): bool {
        return _.state == DateHeaderTimerState.Active;
    }

    fn nextFireMs(): i64 {
        return _.next_fire_ms;
    }

    fn refreshCount(): u64 {
        return _.refresh_count;
    }

    /// Returns the cached 29-byte RFC 7231 IMF-fixdate value, or `""` if never formatted.
    fn dateSlice(): []const u8 {
        if (!_.has_cached) return "";
        return _.cached_date[0..HTTP_DATE_HEADER_LEN];
    }

    /// Returns the cached 37-byte `"Date: ...\r\n"` header line, or `""` if never formatted.
    fn headerLineSlice(): []const u8 {
        if (!_.has_cached) return "";
        return _.cached_line[0..HTTP_DATE_HEADER_LINE_LEN];
    }
}
