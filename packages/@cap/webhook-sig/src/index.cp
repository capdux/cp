// @cap/webhook-sig — Zero-allocation Stripe, GitHub & Standard Webhooks Signature Utilities
// Clean-room MIT implementation in pure CP.

pub const MAX_V1_SIGS: usize = 4;

pub struct StripeSig {
    pub timestamp: i64,
    pub v1_sigs: [4]string,
    pub v1_count: usize,
    pub valid: bool,
}

fn isSpace(b: u8) -> bool {
    return b == 32 || b == 9;
}

fn isHex(b: u8) -> bool {
    if (b >= 48 && b <= 57) { return true; }
    if (b >= 65 && b <= 70) { return true; }
    if (b >= 97 && b <= 102) { return true; }
    return false;
}

fn trimSlice(s: string) -> string {
    let a: usize = 0;
    let b: usize = s.len;
    while (a < b && isSpace(s[a])) {
        a += 1;
    }
    while (b > a && isSpace(s[b - 1])) {
        b -= 1;
    }
    return s[a..b];
}

fn parseNonNegI64(s: string) -> i64 {
    if (s.len == 0) {
        return -1;
    }
    const max_div10: u64 = 922337203685477580;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const ch = s[i];
        if (ch < 48 || ch > 57) {
            return -1;
        }
        const digit = (ch - 48) as u64;
        if (acc > max_div10 || (acc == max_div10 && digit > 7)) {
            return -1;
        }
        acc = acc * 10 + digit;
        i += 1;
    }
    return acc as i64;
}

/// Parses a `Stripe-Signature` header (`t=1492774577,v1=5257a869...,v1=...`).
/// Supports up to 4 rotated `v1` signatures.
pub fn parseStripeHeader(header: string) -> StripeSig {
    let sigs: [4]string = ["", "", "", ""];
    let has_ts = false;
    let ts: i64 = 0;
    let v1_count: usize = 0;

    let pos: usize = 0;
    const n = header.len;
    while (pos < n) {
        const start = pos;
        while (pos < n && header[pos] != 44) {
            pos += 1;
        }
        const part = trimSlice(header[start..pos]);
        if (pos < n && header[pos] == 44) {
            pos += 1;
        }
        if (part.len == 0) {
            continue;
        }

        let eq: usize = 0;
        while (eq < part.len && part[eq] != 61) {
            eq += 1;
        }
        if (eq >= part.len || eq == 0) {
            continue;
        }
        const k = trimSlice(part[0..eq]);
        const v = trimSlice(part[eq + 1..part.len]);

        if (k.len == 1 && k[0] == 116) {
            // "t"
            const parsed_ts = parseNonNegI64(v);
            if (parsed_ts >= 0) {
                ts = parsed_ts;
                has_ts = true;
            }
        } else if (k.len == 2 && k[0] == 118 && k[1] == 49) {
            // "v1"
            if (v.len > 0 && v1_count < MAX_V1_SIGS) {
                sigs[v1_count] = v;
                v1_count += 1;
            }
        }
    }

    return StripeSig {
        timestamp: ts,
        v1_sigs: sigs,
        v1_count: v1_count,
        valid: has_ts && v1_count > 0,
    };
}

/// Parses a GitHub `X-Hub-Signature-256` header (`sha256=<64-hex>`).
/// Returns the 64-character hex digest slice, or `""` if malformed.
pub fn parseGitHubSig256(header: string) -> string {
    const s = trimSlice(header);
    if (s.len != 71) {
        return "";
    }
    // Must start with "sha256="
    if (s[0] != 115 || s[1] != 104 || s[2] != 97 || s[3] != 50 || s[4] != 53 || s[5] != 54 || s[6] != 61) {
        return "";
    }
    let i: usize = 7;
    while (i < 71) {
        if (!isHex(s[i])) {
            return "";
        }
        i += 1;
    }
    return s[7..71];
}

/// Parses a Standard Webhooks `webhook-signature` header (`v1,<sig1> v1,<sig2> ...`).
/// Extracts all `v1,` signatures into `out_sigs` and returns the count written.
pub fn parseStandardWebhookSig(header: string, out_sigs: []string) -> usize {
    let pos: usize = 0;
    const n = header.len;
    let count: usize = 0;

    while (pos < n && count < out_sigs.len) {
        while (pos < n && isSpace(header[pos])) {
            pos += 1;
        }
        if (pos >= n) {
            break;
        }
        const start = pos;
        while (pos < n && !isSpace(header[pos])) {
            pos += 1;
        }
        const tok = header[start..pos];
        if (tok.len > 3 && tok[0] == 118 && tok[1] == 49 && tok[2] == 44) {
            // "v1,"
            out_sigs[count] = tok[3..tok.len];
            count += 1;
        }
    }

    return count;
}

/// Checks whether `|now_secs - timestamp_secs| <= tolerance_secs` to guard against replay & clock skew.
pub fn isWithinTolerance(timestamp_secs: i64, now_secs: i64, tolerance_secs: i64) -> bool {
    if (tolerance_secs < 0) {
        return false;
    }
    let diff: i64 = 0;
    if (now_secs >= timestamp_secs) {
        diff = now_secs - timestamp_secs;
    } else {
        diff = timestamp_secs - now_secs;
    }
    return diff <= tolerance_secs;
}

/// Constant-time byte comparison resistant to timing side-channels.
pub fn constantTimeEq(a: string, b: string) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) {
        diff |= a[i] ^ b[i];
        i += 1;
    }
    return diff == 0;
}

/// Verifies a `Stripe-Signature` header against `expected_v1_hex` within `tolerance_secs` of `now_secs`.
pub fn verifyStripeExpected(header: string, expected_v1_hex: string, now_secs: i64, tolerance_secs: i64) -> bool {
    if (expected_v1_hex.len == 0) {
        return false;
    }
    const parsed = parseStripeHeader(header);
    if (!parsed.valid) {
        return false;
    }
    if (!isWithinTolerance(parsed.timestamp, now_secs, tolerance_secs)) {
        return false;
    }
    let any_ok = false;
    let i: usize = 0;
    while (i < parsed.v1_count) {
        if (constantTimeEq(parsed.v1_sigs[i], expected_v1_hex)) {
            any_ok = true;
        }
        i += 1;
    }
    return any_ok;
}

/// Formats Stripe's signed payload string `"<timestamp_secs>.<payload>"` into `out`.
/// Returns total bytes written, or `0` if `timestamp_secs < 0` or `out` is too small.
pub fn formatStripeSignedPayload(timestamp_secs: i64, payload: string, out: []u8) -> usize {
    if (timestamp_secs < 0) {
        return 0;
    }
    let rev: [20]u8 = undefined;
    let tlen: usize = 0;
    let v = timestamp_secs as u64;
    if (v == 0) {
        rev[0] = 48;
        tlen = 1;
    } else {
        while (v > 0) {
            rev[tlen] = (48 + (v % 10)) as u8;
            v = v / 10;
            tlen += 1;
        }
    }

    const needed = tlen + 1 + payload.len;
    if (needed > out.len) {
        return 0;
    }
    let w: usize = 0;
    let i: usize = 0;
    while (i < tlen) {
        out[w] = rev[tlen - 1 - i];
        w += 1;
        i += 1;
    }
    out[w] = 46; // '.'
    w += 1;
    let j: usize = 0;
    while (j < payload.len) {
        out[w] = payload[j];
        w += 1;
        j += 1;
    }
    return w;
}
