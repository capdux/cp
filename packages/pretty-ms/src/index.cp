// @cap/pretty-ms — Zero-allocation millisecond decomposition & duration formatting engine for CP
// Clean-room MIT implementation composing @cap/parse-ms directly.
//
// Features:
// - `parseMs(ms: i64) -> ParsedMs`: delegates integer decomposition to `@cap/parse-ms`.
// - `prettyMs(ms, out)` & `prettyMsWith(ms, opts, out)`:
//   * Default compact format (`"15d 11h 23m 20s"`, `"1.3s"`, `"42ms"`, `"0ms"`, negative `"-1.3s"`).
//   * `verbose`: full singular/plural unit words (`"1 day 2 hours 3 minutes 4 seconds"`).
//   * `compact`: single most-significant unit (`"15d"`).
//   * `colon_notation`: clock/player format (`"0:05"`, `"1:02:03"`).
//   * `separate_ms`: separate seconds and milliseconds (`"1s 337ms"`).
//   * `unit_count`: cap maximum emitted units.
// - Zero heap allocation and pure integer arithmetic.

import { ParsedMs, parseMs } from "../../parse-ms/src/index.cp";

pub struct PrettyMsOptions {
    pub verbose: bool,
    pub compact: bool,
    pub colon_notation: bool,
    pub separate_ms: bool,
    pub unit_count: u8,

    pub static fn defaults() -> PrettyMsOptions {
        return PrettyMsOptions{
            verbose: false,
            compact: false,
            colon_notation: false,
            separate_ms: false,
            unit_count: 0,
        };
    }
}

fn prettyAbsI64(v: i64) -> u64 {
    return if (v < 0) (-v) as u64 else v as u64;
}

fn pushByte(out: []u8, pos: usize, b: u8) -> usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn pushStr(out: []u8, pos: usize, s: string) -> usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn pushU64(out: []u8, pos: usize, val: u64) -> usize {
    let digits: [20]u8 = undefined;
    let count: usize = 0;
    let v = val;
    if (v == 0) {
        return pushByte(out, pos, '0');
    }
    while (v > 0 && count < 20) {
        digits[count] = (48 as u8) + ((v % 10) as u8);
        count += 1;
        v = v / 10;
    }
    let p = pos;
    let k = count;
    while (k > 0) {
        k -= 1;
        p = pushByte(out, p, digits[k]);
    }
    return p;
}

fn pushTwoDigits(out: []u8, pos: usize, val: u8) -> usize {
    let p = pushByte(out, pos, (48 as u8) + (val / 10));
    p = pushByte(out, p, (48 as u8) + (val % 10));
    return p;
}

fn emitUnit(
    out: []u8,
    pos: usize,
    emitted: u8,
    val: u64,
    short_unit: string,
    singular: string,
    plural: string,
    verbose: bool,
) -> usize {
    let p = pos;
    if (emitted > 0) {
        p = pushByte(out, p, ' ');
    }
    p = pushU64(out, p, val);
    if (verbose) {
        p = pushByte(out, p, ' ');
        if (val == 1) {
            p = pushStr(out, p, singular);
        } else {
            p = pushStr(out, p, plural);
        }
    } else {
        p = pushStr(out, p, short_unit);
    }
    return p;
}

/// Formats `ms` into a human-readable duration string using default options into `out`.
pub fn prettyMs(ms: i64, out: []u8) -> string {
    return prettyMsWith(ms, PrettyMsOptions.defaults(), out);
}

/// Formats `ms` into a human-readable duration string with custom `PrettyMsOptions` into `out`.
pub fn prettyMsWith(ms: i64, opts: PrettyMsOptions, out: []u8) -> string {
    const p: ParsedMs = parseMs(ms);
    const p_days: u64 = prettyAbsI64(p.days);
    const p_hours: u8 = prettyAbsI64(p.hours) as u8;
    const p_minutes: u8 = prettyAbsI64(p.minutes) as u8;
    const p_seconds: u8 = prettyAbsI64(p.seconds) as u8;
    const p_millis: u16 = prettyAbsI64(p.milliseconds) as u16;
    let pos: usize = 0;

    if (ms < 0) {
        pos = pushByte(out, pos, '-');
    }

    // Colon notation: e.g. "0:05", "1:02:03", "0:01.3"
    if (opts.colon_notation) {
        const total_hours = p_days * 24 + (p_hours as u64);
        if (total_hours > 0) {
            pos = pushU64(out, pos, total_hours);
            pos = pushByte(out, pos, ':');
            pos = pushTwoDigits(out, pos, p_minutes);
            pos = pushByte(out, pos, ':');
            pos = pushTwoDigits(out, pos, p_seconds);
        } else {
            pos = pushU64(out, pos, p_minutes as u64);
            pos = pushByte(out, pos, ':');
            pos = pushTwoDigits(out, pos, p_seconds);
        }
        if (p_millis > 0) {
            const tenth = p_millis / 100;
            if (tenth > 0) {
                pos = pushByte(out, pos, '.');
                pos = pushByte(out, pos, (48 as u8) + (tenth as u8));
            }
        }
        return out[0..pos];
    }

    // Zero duration
    if (ms == 0) {
        if (opts.verbose) {
            pos = pushStr(out, pos, "0 milliseconds");
        } else {
            pos = pushStr(out, pos, "0ms");
        }
        return out[0..pos];
    }

    // Sub-second duration (< 1000ms)
    if (p_days == 0 && p_hours == 0 && p_minutes == 0 && p_seconds == 0) {
        pos = emitUnit(out, pos, 0, p_millis as u64, "ms", "millisecond", "milliseconds", opts.verbose);
        return out[0..pos];
    }

    let max_units: u8 = 255;
    if (opts.compact) {
        max_units = 1;
    } else if (opts.unit_count > 0) {
        max_units = opts.unit_count;
    }

    let emitted: u8 = 0;

    if (p_days > 0 && emitted < max_units) {
        pos = emitUnit(out, pos, emitted, p_days, "d", "day", "days", opts.verbose);
        emitted += 1;
    }
    if (p_hours > 0 && emitted < max_units) {
        pos = emitUnit(out, pos, emitted, p_hours as u64, "h", "hour", "hours", opts.verbose);
        emitted += 1;
    }
    if (p_minutes > 0 && emitted < max_units) {
        pos = emitUnit(out, pos, emitted, p_minutes as u64, "m", "minute", "minutes", opts.verbose);
        emitted += 1;
    }

    if (opts.separate_ms) {
        if (p_seconds > 0 && emitted < max_units) {
            pos = emitUnit(out, pos, emitted, p_seconds as u64, "s", "second", "seconds", opts.verbose);
            emitted += 1;
        }
        if (p_millis > 0 && emitted < max_units) {
            pos = emitUnit(out, pos, emitted, p_millis as u64, "ms", "millisecond", "milliseconds", opts.verbose);
            emitted += 1;
        }
    } else {
        // Under 1 minute with fractional milliseconds (e.g. 1337ms -> "1.3s")
        if (p_days == 0 && p_hours == 0 && p_minutes == 0 && p_millis > 0 && !opts.compact && emitted < max_units) {
            let sec_whole = p_seconds as u64;
            let tenth = (p_millis + 50) / 100;
            if (tenth >= 10) {
                sec_whole += 1;
                tenth = 0;
            }
            if (tenth > 0) {
                if (emitted > 0) {
                    pos = pushByte(out, pos, ' ');
                }
                pos = pushU64(out, pos, sec_whole);
                pos = pushByte(out, pos, '.');
                pos = pushByte(out, pos, (48 as u8) + (tenth as u8));
                if (opts.verbose) {
                    pos = pushStr(out, pos, " seconds");
                } else {
                    pos = pushByte(out, pos, 's');
                }
                emitted += 1;
            } else {
                pos = emitUnit(out, pos, emitted, sec_whole, "s", "second", "seconds", opts.verbose);
                emitted += 1;
            }
        } else if (p_seconds > 0 && emitted < max_units) {
            pos = emitUnit(out, pos, emitted, p_seconds as u64, "s", "second", "seconds", opts.verbose);
            emitted += 1;
        }
    }

    return out[0..pos];
}

export {
    ParsedMs,
    PrettyMsOptions,
    parseMs,
    prettyMs,
    prettyMsWith,
};
