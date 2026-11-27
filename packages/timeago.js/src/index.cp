// @cap/timeago — Zero-allocation deterministic relative time formatting engine for CP
// Clean-room MIT implementation inspired by npm `timeago.js`, `date-fns/formatDistance`,
// and Rust `chrono-humanize` / `timeago`.

pub enum TimeagoLocale: u8 {
    En = 0,
    Zh = 1,
}

pub enum TimeagoStyle: u8 {
    Long = 0,
    Short = 1,
}

pub const SEC_MINUTE: u64 = 60;
pub const SEC_HOUR: u64 = 3600;
pub const SEC_DAY: u64 = 86400;
pub const SEC_WEEK: u64 = 604800;
pub const SEC_MONTH: u64 = 2592000;
pub const SEC_YEAR: u64 = 31536000;

enum TimeUnit: u8 {
    Second = 0,
    Minute = 1,
    Hour = 2,
    Day = 3,
    Week = 4,
    Month = 5,
    Year = 6,
}

struct ScaledDelta {
    value: u64,
    unit: TimeUnit,
}

fn absU64(n: i64) -> u64 {
    if (n >= 0) {
        return n as u64;
    }
    return ((0 - (n + 1)) as u64) + 1;
}

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

fn appendStr(out: []u8, pos: usize, s: string) -> usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn appendU64(out: []u8, pos: usize, val: u64) -> usize {
    if (val == 0) {
        return appendByte(out, pos, '0');
    }
    let digits: [24]u8 = undefined;
    let len: usize = 0;
    let v = val;
    while (v > 0) {
        digits[len] = ('0' as u64 + (v % 10)) as u8;
        v /= 10;
        len += 1;
    }
    let p = pos;
    let k = len;
    while (k > 0) {
        k -= 1;
        p = appendByte(out, p, digits[k]);
    }
    return p;
}

fn scaleDelta(abs_sec: u64) -> ScaledDelta {
    if (abs_sec < SEC_MINUTE) {
        return ScaledDelta{ value: abs_sec, unit: TimeUnit.Second };
    }
    if (abs_sec < SEC_HOUR) {
        return ScaledDelta{ value: abs_sec / SEC_MINUTE, unit: TimeUnit.Minute };
    }
    if (abs_sec < SEC_DAY) {
        return ScaledDelta{ value: abs_sec / SEC_HOUR, unit: TimeUnit.Hour };
    }
    if (abs_sec < SEC_WEEK) {
        return ScaledDelta{ value: abs_sec / SEC_DAY, unit: TimeUnit.Day };
    }
    if (abs_sec < SEC_MONTH) {
        return ScaledDelta{ value: abs_sec / SEC_WEEK, unit: TimeUnit.Week };
    }
    if (abs_sec < SEC_YEAR) {
        return ScaledDelta{ value: abs_sec / SEC_MONTH, unit: TimeUnit.Month };
    }
    return ScaledDelta{ value: abs_sec / SEC_YEAR, unit: TimeUnit.Year };
}

fn enLongUnit(unit: TimeUnit, plural: bool) -> string {
    switch (unit) {
        .Second => return if (plural) "seconds" else "second",
        .Minute => return if (plural) "minutes" else "minute",
        .Hour => return if (plural) "hours" else "hour",
        .Day => return if (plural) "days" else "day",
        .Week => return if (plural) "weeks" else "week",
        .Month => return if (plural) "months" else "month",
        .Year => return if (plural) "years" else "year",
    }
}

fn enShortUnit(unit: TimeUnit) -> string {
    switch (unit) {
        .Second => return "s",
        .Minute => return "m",
        .Hour => return "h",
        .Day => return "d",
        .Week => return "w",
        .Month => return "mo",
        .Year => return "y",
    }
}

fn zhLongUnit(unit: TimeUnit) -> string {
    switch (unit) {
        .Second => return "秒",
        .Minute => return "分钟",
        .Hour => return "小时",
        .Day => return "天",
        .Week => return "周",
        .Month => return "个月",
        .Year => return "年",
    }
}

fn zhShortUnit(unit: TimeUnit) -> string {
    switch (unit) {
        .Second => return "秒",
        .Minute => return "分",
        .Hour => return "时",
        .Day => return "天",
        .Week => return "周",
        .Month => return "月",
        .Year => return "年",
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

/// Formats a signed delta in seconds (`delta_sec < 0` for past, `delta_sec > 0` for future)
/// into `out` according to `locale` (`En` / `Zh`) and `style` (`Long` / `Short`).
pub fn formatTimeago(delta_sec: i64, locale: TimeagoLocale, style: TimeagoStyle, out: []u8) -> string {
    const abs = absU64(delta_sec);
    let pos: usize = 0;

    if (abs < 10) {
        switch (locale) {
            .En => {
                switch (style) {
                    .Long => pos = appendStr(out, pos, "just now"),
                    .Short => pos = appendStr(out, pos, "now"),
                }
            },
            .Zh => {
                pos = appendStr(out, pos, "刚刚");
            },
        }
        return out[0..pos];
    }

    const is_future = delta_sec > 0;
    const scaled = scaleDelta(abs);

    switch (locale) {
        .En => {
            switch (style) {
                .Long => {
                    const unit_str = enLongUnit(scaled.unit, scaled.value != 1);
                    if (is_future) {
                        pos = appendStr(out, pos, "in ");
                        pos = appendU64(out, pos, scaled.value);
                        pos = appendByte(out, pos, ' ');
                        pos = appendStr(out, pos, unit_str);
                    } else {
                        pos = appendU64(out, pos, scaled.value);
                        pos = appendByte(out, pos, ' ');
                        pos = appendStr(out, pos, unit_str);
                        pos = appendStr(out, pos, " ago");
                    }
                },
                .Short => {
                    const unit_str = enShortUnit(scaled.unit);
                    if (is_future) {
                        pos = appendStr(out, pos, "in ");
                        pos = appendU64(out, pos, scaled.value);
                        pos = appendStr(out, pos, unit_str);
                    } else {
                        pos = appendU64(out, pos, scaled.value);
                        pos = appendStr(out, pos, unit_str);
                        pos = appendStr(out, pos, " ago");
                    }
                },
            }
        },
        .Zh => {
            switch (style) {
                .Long => {
                    pos = appendU64(out, pos, scaled.value);
                    pos = appendByte(out, pos, ' ');
                    pos = appendStr(out, pos, zhLongUnit(scaled.unit));
                    pos = appendStr(out, pos, if (is_future) "后" else "前");
                },
                .Short => {
                    pos = appendU64(out, pos, scaled.value);
                    pos = appendStr(out, pos, zhShortUnit(scaled.unit));
                    pos = appendStr(out, pos, if (is_future) "后" else "前");
                },
            }
        },
    }

    return out[0..pos];
}

/// Formats the relative time between `target_sec` and `now_sec` (`target_sec - now_sec`).
pub fn formatBetween(
    target_sec: i64,
    now_sec: i64,
    locale: TimeagoLocale,
    style: TimeagoStyle,
    out: []u8,
) -> string {
    return formatTimeago(target_sec - now_sec, locale, style, out);
}

/// Convenience wrapper for English Long style (`"5 minutes ago"` / `"in 3 hours"` / `"just now"`).
pub fn formatTimeagoEn(delta_sec: i64, out: []u8) -> string {
    return formatTimeago(delta_sec, TimeagoLocale.En, TimeagoStyle.Long, out);
}

/// Convenience wrapper for Chinese Long style (`"5 分钟前"` / `"3 小时后"` / `"刚刚"`).
pub fn formatTimeagoZh(delta_sec: i64, out: []u8) -> string {
    return formatTimeago(delta_sec, TimeagoLocale.Zh, TimeagoStyle.Long, out);
}

/// Formats `delta_sec` in ultra-compact signed CLI/monitoring style:
/// `"now"` (`|delta| < 10`), `"-45s"` / `"-5m"` / `"-2h"` / `"-3d"` / `"-1y"` for past,
/// `"+45s"` / `"+5m"` / `"+2h"` for future.
pub fn formatCompact(delta_sec: i64, out: []u8) -> string {
    const abs = absU64(delta_sec);
    let pos: usize = 0;
    if (abs < 10) {
        pos = appendStr(out, pos, "now");
        return out[0..pos];
    }
    const scaled = scaleDelta(abs);
    pos = appendByte(out, pos, if (delta_sec > 0) '+' else '-');
    pos = appendU64(out, pos, scaled.value);
    pos = appendStr(out, pos, enShortUnit(scaled.unit));
    return out[0..pos];
}
