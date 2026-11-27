// packages/@cap/rate-header/src/index.cp
// Zero-allocation IETF HTTP RateLimit headers parser, formatter, and delay calculator.
// MIT clean-room pure CP implementation.

/// Parsed IETF `RateLimit-Policy` quota entry (e.g. `100;w=60;burst=20`).
pub struct RateLimitPolicy {
    pub quota: u64,
    pub window_secs: u64,
    pub burst: u64,
    pub has_burst: bool,
    pub valid: bool,
}

/// Parsed IETF structured `RateLimit` state (e.g. `limit=100, remaining=42, reset=15`).
pub struct RateLimitState {
    pub limit: u64,
    pub remaining: u64,
    pub reset_secs: u64,
    pub valid: bool,
}

fn rhToLower(c: u8): u8 {
    if (c >= ('A' as u8) && c <= ('Z' as u8)) {
        return c + (32 as u8);
    }
    return c;
}

fn rhIsOws(c: u8): bool {
    return c == (' ' as u8) || c == ('\t' as u8);
}

fn rhTrimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && rhIsOws(s[start])) {
        start += 1;
    }
    while (end > start && rhIsOws(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn rhEqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (rhToLower(a[i]) != rhToLower(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Format an unsigned 64-bit integer in decimal into `buf` (at least 20 bytes).
/// Returns number of digits written.
fn rhFormatU64(val: u64, buf: []u8): usize {
    if (val == (0 as u64)) {
        buf[0] = '0' as u8;
        return 1;
    }
    let rev: [24]u8 = undefined;
    let n: usize = 0;
    let cur = val;
    while (cur > (0 as u64)) {
        const digit = (cur % (10 as u64)) as u8;
        rev[n] = ('0' as u8) + digit;
        n += 1;
        cur = cur / (10 as u64);
    }
    let i: usize = 0;
    while (i < n) {
        buf[i] = rev[n - 1 - i];
        i += 1;
    }
    return n;
}

fn rhCopyInto(dst: []u8, pos: *usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[pos.*] = src[i];
        pos.* += 1;
        i += 1;
    }
}

/// Strictly parse a non-negative decimal integer HTTP header value (ignoring leading/trailing OWS).
/// Returns `-1` if empty, negative, non-numeric, or overflowing `i64`.
pub fn parseUintHeader(value: string): i64 {
    const t = rhTrimOws(value);
    if (t.len == 0) {
        return -1 as i64;
    }
    let acc: u64 = 0 as u64;
    const maxDiv10: u64 = 922337203685477580 as u64;
    let i: usize = 0;
    while (i < t.len) {
        const c = t[i];
        if (c < ('0' as u8) || c > ('9' as u8)) {
            return -1 as i64;
        }
        const d = (c - ('0' as u8)) as u64;
        if (acc > maxDiv10 || (acc == maxDiv10 && d > (7 as u64))) {
            return -1 as i64;
        }
        acc = acc * (10 as u64) + d;
        i += 1;
    }
    return acc as i64;
}

/// Parse an IETF `RateLimit-Policy` header (e.g. `100;w=60`, `100;w=60;burst=20`,
/// `100; w=60; comment="sliding"`).
pub fn parsePolicy(header: string): RateLimitPolicy {
    let res = RateLimitPolicy{
        quota: 0 as u64,
        window_secs: 0 as u64,
        burst: 0 as u64,
        has_burst: false,
        valid: false,
    };

    const trimmed = rhTrimOws(header);
    if (trimmed.len == 0) {
        return res;
    }

    // Extract first comma-separated policy item (respecting quoted strings).
    let itemEnd: usize = 0;
    let inQuotes = false;
    while (itemEnd < trimmed.len) {
        const c = trimmed[itemEnd];
        if (inQuotes) {
            if (c == ('\\' as u8) && itemEnd + 1 < trimmed.len) {
                itemEnd += 2;
            } else if (c == ('"' as u8)) {
                inQuotes = false;
                itemEnd += 1;
            } else {
                itemEnd += 1;
            }
        } else {
            if (c == ('"' as u8)) {
                inQuotes = true;
                itemEnd += 1;
            } else if (c == (',' as u8)) {
                break;
            } else {
                itemEnd += 1;
            }
        }
    }

    const item = rhTrimOws(trimmed[0..itemEnd]);
    if (item.len == 0) {
        return res;
    }

    // First segment before ';' is the quota integer.
    let semi: usize = 0;
    while (semi < item.len && item[semi] != (';' as u8)) {
        semi += 1;
    }
    const qVal = parseUintHeader(item[0..semi]);
    if (qVal < (0 as i64)) {
        return res;
    }
    res.quota = qVal as u64;

    let hasWindow = false;
    let pos = semi;
    while (pos < item.len) {
        // Skip ';'.
        pos += 1;
        while (pos < item.len && rhIsOws(item[pos])) {
            pos += 1;
        }
        if (pos >= item.len) {
            break;
        }

        // Scan parameter up to next unquoted ';'.
        const segStart = pos;
        let qMode = false;
        while (pos < item.len) {
            const c = item[pos];
            if (qMode) {
                if (c == ('\\' as u8) && pos + 1 < item.len) {
                    pos += 2;
                } else if (c == ('"' as u8)) {
                    qMode = false;
                    pos += 1;
                } else {
                    pos += 1;
                }
            } else {
                if (c == ('"' as u8)) {
                    qMode = true;
                    pos += 1;
                } else if (c == (';' as u8)) {
                    break;
                } else {
                    pos += 1;
                }
            }
        }

        const param = rhTrimOws(item[segStart..pos]);
        if (param.len == 0) {
            continue;
        }

        let eq: usize = 0;
        while (eq < param.len && param[eq] != ('=' as u8)) {
            eq += 1;
        }
        if (eq >= param.len) {
            continue;
        }

        const key = rhTrimOws(param[0..eq]);
        const val = rhTrimOws(param[(eq + 1)..param.len]);

        if (rhEqIgnoreCase(key, "w")) {
            const wParsed = parseUintHeader(val);
            if (wParsed < (0 as i64)) {
                return res;
            }
            res.window_secs = wParsed as u64;
            hasWindow = true;
        } else if (rhEqIgnoreCase(key, "burst")) {
            const bParsed = parseUintHeader(val);
            if (bParsed < (0 as i64)) {
                return res;
            }
            res.burst = bParsed as u64;
            res.has_burst = true;
        }
    }

    if (hasWindow) {
        res.valid = true;
    }
    return res;
}

/// Format an IETF `RateLimit-Policy` string `"<quota>;w=<window_secs>"` into `out`.
/// Returns bytes written, or `0` if `out` is too small.
pub fn formatPolicy(quota: u64, window_secs: u64, out: []u8): usize {
    let qBuf: [24]u8 = undefined;
    let wBuf: [24]u8 = undefined;
    const qLen = rhFormatU64(quota, qBuf[0..24]);
    const wLen = rhFormatU64(window_secs, wBuf[0..24]);
    const total = qLen + 3 + wLen; // ";w=" = 3
    if (total > out.len) {
        return 0;
    }
    let pos: usize = 0;
    rhCopyInto(out, &pos, qBuf[0..qLen]);
    rhCopyInto(out, &pos, ";w=");
    rhCopyInto(out, &pos, wBuf[0..wLen]);
    return pos;
}

/// Parse an IETF structured `RateLimit` header (e.g. `"limit=100, remaining=42, reset=15"`).
pub fn parseStructuredRateLimit(header: string): RateLimitState {
    let res = RateLimitState{
        limit: 0 as u64,
        remaining: 0 as u64,
        reset_secs: 0 as u64,
        valid: false,
    };

    let hasLimit = false;
    let hasRemaining = false;
    let hasReset = false;

    let pos: usize = 0;
    while (pos < header.len) {
        let segEnd = pos;
        while (segEnd < header.len && header[segEnd] != (',' as u8)) {
            segEnd += 1;
        }
        const seg = rhTrimOws(header[pos..segEnd]);
        if (segEnd < header.len) {
            pos = segEnd + 1;
        } else {
            pos = header.len;
        }

        if (seg.len == 0) {
            continue;
        }

        let eq: usize = 0;
        while (eq < seg.len && seg[eq] != ('=' as u8)) {
            eq += 1;
        }
        if (eq >= seg.len) {
            continue;
        }

        const key = rhTrimOws(seg[0..eq]);
        const val = rhTrimOws(seg[(eq + 1)..seg.len]);

        if (rhEqIgnoreCase(key, "limit")) {
            const v = parseUintHeader(val);
            if (v < (0 as i64)) {
                return res;
            }
            res.limit = v as u64;
            hasLimit = true;
        } else if (rhEqIgnoreCase(key, "remaining")) {
            const v = parseUintHeader(val);
            if (v < (0 as i64)) {
                return res;
            }
            res.remaining = v as u64;
            hasRemaining = true;
        } else if (rhEqIgnoreCase(key, "reset")) {
            const v = parseUintHeader(val);
            if (v < (0 as i64)) {
                return res;
            }
            res.reset_secs = v as u64;
            hasReset = true;
        }
    }

    if (hasLimit && hasRemaining && hasReset) {
        res.valid = true;
    }
    return res;
}

/// Format a `RateLimitState` as `"limit=<limit>, remaining=<remaining>, reset=<reset_secs>"`.
/// Returns bytes written, or `0` if `out` is too small.
pub fn formatStructuredRateLimit(state: RateLimitState, out: []u8): usize {
    let lBuf: [24]u8 = undefined;
    let rBuf: [24]u8 = undefined;
    let tBuf: [24]u8 = undefined;
    const lLen = rhFormatU64(state.limit, lBuf[0..24]);
    const rLen = rhFormatU64(state.remaining, rBuf[0..24]);
    const tLen = rhFormatU64(state.reset_secs, tBuf[0..24]);
    // "limit=" (6) + lLen + ", remaining=" (12) + rLen + ", reset=" (8) + tLen
    const total = 6 + lLen + 12 + rLen + 8 + tLen;
    if (total > out.len) {
        return 0;
    }
    let pos: usize = 0;
    rhCopyInto(out, &pos, "limit=");
    rhCopyInto(out, &pos, lBuf[0..lLen]);
    rhCopyInto(out, &pos, ", remaining=");
    rhCopyInto(out, &pos, rBuf[0..rLen]);
    rhCopyInto(out, &pos, ", reset=");
    rhCopyInto(out, &pos, tBuf[0..tLen]);
    return pos;
}

/// Returns true if `state` is valid and remaining quota has reached zero.
pub fn isExhausted(state: RateLimitState): bool {
    return state.valid && state.remaining == (0 as u64);
}

/// Compute delay in seconds until quota reset.
/// Values `<= 1_000_000_000` are treated as relative delta-seconds (IETF standard);
/// larger values are treated as absolute Unix Epoch timestamps (legacy `X-RateLimit-Reset`).
pub fn computeResetDelaySecs(reset_val: u64, now_unix_secs: u64): u64 {
    if (reset_val <= (1000000000 as u64)) {
        return reset_val;
    }
    if (reset_val > now_unix_secs) {
        return reset_val - now_unix_secs;
    }
    return 0 as u64;
}

export {
    RateLimitPolicy,
    RateLimitState,
    parsePolicy,
    formatPolicy,
    parseUintHeader,
    parseStructuredRateLimit,
    formatStructuredRateLimit,
    isExhausted,
    computeResetDelaySecs,
};
