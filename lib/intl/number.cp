// Source: cp/lib/intl/number.cp (Grouped, Compact, Ordinal & Percent Number Formatter)
// Pure CP internationalization number formatter supporting locale-configurable grouping,
// compact human notation (K/M/B/T), English ordinals (1st/2nd/3rd/11th), and percentages.

const std = @import("std");

/// Error set returned by `cap:intl` number formatting operations.
pub err IntlError {
    InvalidOption,
    OutOfMemory,
}

/// Configuration options for `NumberFormatter.formatFloat`.
pub struct NumberFormatOptions {
    thousands_sep: u8 = ',';
    decimal_sep: u8 = '.';
    decimals: u8 = 2;
}

fn appendReversedGroupedDigits(
    alloc: std.mem.Allocator,
    out: *std.ArrayList(u8),
    digits: []const u8,
    sep: u8,
)! {
    const n = digits.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        if (sep != 0 && i > 0 && ((n - i) % 3) == 0) {
            out.append(alloc, sep) catch return IntlError.OutOfMemory;
        }
        out.append(alloc, digits[i]) catch return IntlError.OutOfMemory;
    }
}

fn writeU64ToBuf(buf: []u8, val: u64): []const u8 {
    if (val == 0) {
        buf[0] = '0';
        return buf[0..1];
    }
    let tmp: [32]u8 = undefined;
    let tlen: usize = 0;
    let v = val;
    while (v > 0) {
        tmp[tlen] = ('0' as u8) + ((v % 10) as u8);
        v /= 10;
        tlen += 1;
    }
    let i: usize = 0;
    while (i < tlen) : (i += 1) {
        buf[i] = tmp[tlen - 1 - i];
    }
    return buf[0..tlen];
}

fn formatNonFinite(alloc: std.mem.Allocator, val: f64): ([]u8)! {
    if (std.math.isNan(val)) {
        return alloc.dupe(u8, "NaN") catch return IntlError.OutOfMemory;
    }
    if (val < 0.0) {
        return alloc.dupe(u8, "-Infinity") catch return IntlError.OutOfMemory;
    }
    return alloc.dupe(u8, "Infinity") catch return IntlError.OutOfMemory;
}

/// Locale-aware number formatter for integers, decimals, compact metrics, ordinals, and percents.
pub static struct NumberFormatter {
    /// Formats a signed 64-bit integer with thousands grouping separator `sep` (pass `0` for no separator).
    static fn formatInt(alloc: std.mem.Allocator, val: i64, sep: u8): ([]u8)! {
        const is_neg = val < 0;
        let mag: u64 = 0;
        if (is_neg) {
            const raw: u64 = @bitCast(val);
            mag = (0 as u64) -% raw;
        } else {
            mag = val as u64;
        }

        let dbuf: [32]u8 = undefined;
        const digits = writeU64ToBuf(&dbuf, mag);

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        if (is_neg) {
            out.append(alloc, '-') catch return IntlError.OutOfMemory;
        }
        appendReversedGroupedDigits(alloc, *out, digits, sep)?;
        return out.toOwnedSlice(alloc) catch return IntlError.OutOfMemory;
    }

    /// Formats a 64-bit float using `NumberFormatOptions` (grouped integer part + custom decimal separator).
    static fn formatFloat(alloc: std.mem.Allocator, val: f64, opts: NumberFormatOptions): ([]u8)! {
        if (std.math.isNan(val) || std.math.isInf(val)) {
            return formatNonFinite(alloc, val);
        }

        let is_neg = val < 0.0;
        const abs_val = if (is_neg) -val else val;
        if (abs_val == 0.0) is_neg = false;

        // Render shortest decimal string via Zig std.fmt, then round cleanly to `opts.decimals`.
        let raw_buf: [512]u8 = undefined;
        const raw = std.fmt.bufPrint(&raw_buf, "{d}", .{abs_val}) catch return IntlError.OutOfMemory;

        let dot_pos: usize = raw.len;
        let scan_i: usize = 0;
        while (scan_i < raw.len) : (scan_i += 1) {
            if (raw[scan_i] == '.') {
                dot_pos = scan_i;
                break;
            }
        }

        const src_int = raw[0..dot_pos];
        const src_frac = if (dot_pos < raw.len) raw[(dot_pos + 1)..] else "";

        const dec_cnt = opts.decimals as usize;

        // Copy integer digits into mutable work buffer with 1 leading slot for carry propagation.
        let int_work: [512]u8 = undefined;
        let int_start: usize = 1;
        const int_end: usize = src_int.len + 1;
        if (int_end > int_work.len) return IntlError.OutOfMemory;
        let ci: usize = 0;
        while (ci < src_int.len) : (ci += 1) {
            int_work[ci + 1] = src_int[ci];
        }

        let frac_work: std.ArrayList(u8) = .empty;
        defer frac_work.deinit(alloc);

        if (dec_cnt > 0) {
            frac_work.ensureTotalCapacity(alloc, dec_cnt) catch return IntlError.OutOfMemory;
            let fi: usize = 0;
            while (fi < dec_cnt) : (fi += 1) {
                const ch = if (fi < src_frac.len) src_frac[fi] else '0';
                frac_work.appendAssumeCapacity(ch);
            }
        }

        // Check rounding digit at index `dec_cnt` in `src_frac`.
        let carry = src_frac.len > dec_cnt && src_frac[dec_cnt] >= '5';
        if (carry && dec_cnt > 0) {
            let idx = dec_cnt;
            while (idx > 0 && carry) {
                idx -= 1;
                if (frac_work.items[idx] == '9') {
                    frac_work.items[idx] = '0';
                } else {
                    frac_work.items[idx] += 1;
                    carry = false;
                }
            }
        }

        if (carry) {
            let idx = int_end;
            while (idx > int_start && carry) {
                idx -= 1;
                if (int_work[idx] == '9') {
                    int_work[idx] = '0';
                } else {
                    int_work[idx] += 1;
                    carry = false;
                }
            }
            if (carry) {
                int_start = 0;
                int_work[0] = '1';
            }
        }

        const final_int = int_work[int_start..int_end];
        let all_zero = true;
        for (ch of final_int) {
            if (ch != '0') {
                all_zero = false;
                break;
            }
        }
        if (all_zero) {
            for (ch of frac_work.items) {
                if (ch != '0') {
                    all_zero = false;
                    break;
                }
            }
        }
        if (all_zero) is_neg = false;

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        if (is_neg) {
            out.append(alloc, '-') catch return IntlError.OutOfMemory;
        }
        appendReversedGroupedDigits(alloc, *out, final_int, opts.thousands_sep)?;
        if (dec_cnt > 0) {
            out.append(alloc, opts.decimal_sep) catch return IntlError.OutOfMemory;
            out.appendSlice(alloc, frac_work.items) catch return IntlError.OutOfMemory;
        }
        return out.toOwnedSlice(alloc) catch return IntlError.OutOfMemory;
    }

    /// Formats a number into compact human-readable notation (`1.2K`, `3.4M`, `5.6B`, `7.8T`).
    static fn formatCompact(alloc: std.mem.Allocator, val: f64): ([]u8)! {
        if (std.math.isNan(val) || std.math.isInf(val)) {
            return formatNonFinite(alloc, val);
        }

        let is_neg = val < 0.0;
        const abs_val = if (is_neg) -val else val;

        const divisors: [5]f64 = [1.0, 1.0e3, 1.0e6, 1.0e9, 1.0e12];
        const suffixes: [5]u8 = [0, 'K', 'M', 'B', 'T'];

        let tier: usize = 0;
        if (abs_val >= 1.0e12) {
            tier = 4;
        } else if (abs_val >= 1.0e9) {
            tier = 3;
        } else if (abs_val >= 1.0e6) {
            tier = 2;
        } else if (abs_val >= 1.0e3) {
            tier = 1;
        }

        let scaled = abs_val / divisors[tier];
        let tenths = @round(scaled * 10.0);
        if (tier < 4 && tenths >= 10000.0) {
            tier += 1;
            scaled = abs_val / divisors[tier];
            tenths = @round(scaled * 10.0);
        }

        if (tenths == 0.0) {
            return alloc.dupe(u8, "0") catch return IntlError.OutOfMemory;
        }

        // Extremely large numbers beyond u64 tenths fall back to 1-decimal formatted trillion count.
        if (tenths >= 1.0e18) {
            const base_str = NumberFormatter.formatFloat(alloc, if (is_neg) -scaled else scaled, NumberFormatOptions {
                thousands_sep: ',',
                decimal_sep: '.',
                decimals: 1,
            })?;
            defer alloc.free(base_str);
            let out: std.ArrayList(u8) = .empty;
            errdefer out.deinit(alloc);
            out.appendSlice(alloc, base_str) catch return IntlError.OutOfMemory;
            if (suffixes[tier] != 0) {
                out.append(alloc, suffixes[tier]) catch return IntlError.OutOfMemory;
            }
            return out.toOwnedSlice(alloc) catch return IntlError.OutOfMemory;
        }

        const tenths_u64: u64 = @intFromFloat(tenths);
        const whole = tenths_u64 / 10;
        const frac = (tenths_u64 % 10) as u8;

        let wbuf: [32]u8 = undefined;
        const wdigits = writeU64ToBuf(&wbuf, whole);

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        if (is_neg) {
            out.append(alloc, '-') catch return IntlError.OutOfMemory;
        }
        out.appendSlice(alloc, wdigits) catch return IntlError.OutOfMemory;
        if (frac != 0) {
            out.append(alloc, '.') catch return IntlError.OutOfMemory;
            out.append(alloc, ('0' as u8) + frac) catch return IntlError.OutOfMemory;
        }
        if (suffixes[tier] != 0) {
            out.append(alloc, suffixes[tier]) catch return IntlError.OutOfMemory;
        }
        return out.toOwnedSlice(alloc) catch return IntlError.OutOfMemory;
    }

    /// Formats an unsigned integer as an English ordinal (`1st`, `2nd`, `3rd`, `4th`, `11th`, `21st`, `112th`).
    static fn formatOrdinal(alloc: std.mem.Allocator, n: u64): ([]u8)! {
        const mod100 = n % 100;
        const mod10 = n % 10;
        let suffix: []const u8 = "th";
        if (mod100 < 11 || mod100 > 13) {
            if (mod10 == 1) {
                suffix = "st";
            } else if (mod10 == 2) {
                suffix = "nd";
            } else if (mod10 == 3) {
                suffix = "rd";
            }
        }

        let dbuf: [32]u8 = undefined;
        const digits = writeU64ToBuf(&dbuf, n);

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);
        out.appendSlice(alloc, digits) catch return IntlError.OutOfMemory;
        out.appendSlice(alloc, suffix) catch return IntlError.OutOfMemory;
        return out.toOwnedSlice(alloc) catch return IntlError.OutOfMemory;
    }

    /// Formats a ratio (`0.256 -> "25.60%"`) as a percentage string with `decimals` fractional places.
    static fn formatPercent(alloc: std.mem.Allocator, ratio: f64, decimals: u8): ([]u8)! {
        if (std.math.isNan(ratio) || std.math.isInf(ratio)) {
            return formatNonFinite(alloc, ratio);
        }

        const pct = ratio * 100.0;
        const num_str = NumberFormatter.formatFloat(alloc, pct, NumberFormatOptions {
            thousands_sep: ',',
            decimal_sep: '.',
            decimals: decimals,
        })?;
        defer alloc.free(num_str);

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);
        out.appendSlice(alloc, num_str) catch return IntlError.OutOfMemory;
        out.append(alloc, '%') catch return IntlError.OutOfMemory;
        return out.toOwnedSlice(alloc) catch return IntlError.OutOfMemory;
    }
}
