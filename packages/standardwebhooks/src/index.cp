// @cap/webhook-sig — Zero-allocation Stripe, GitHub & Standard Webhooks Signature Utilities
// Clean-room MIT implementation in pure CP.

import { isSpace, isDigit, isHexDigit, trim, startsWith, strEq, findByte } from "cap:ascii";
import { ctEqual } from "cap:bin";
import { parseI64, writeStr, writeU64 } from "cap:fmt";

pub const MAX_V1_SIGS: usize = 4;

pub struct StripeSig {
    pub timestamp: i64,
    pub v1_sigs: [4]string,
    pub v1_count: usize,
    pub valid: bool,
}

fn parseNonNegI64(s: string) -> i64 {
    if (s.len == 0 || !isDigit(s[0])) {
        return -1;
    }
    return parseI64(s) ?? -1;
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
        pos = findByte(header, ',', pos) ?? n;
        const part = trim(header[start..pos]);
        if (pos < n && header[pos] == ',') {
            pos += 1;
        }
        if (part.len == 0) {
            continue;
        }

        const eq = findByte(part, '=', 0) ?? part.len;
        if (eq >= part.len || eq == 0) {
            continue;
        }
        const k = trim(part[0..eq]);
        const v = trim(part[eq + 1..part.len]);

        if (strEq(k, "t")) {
            const parsed_ts = parseNonNegI64(v);
            if (parsed_ts >= 0) {
                ts = parsed_ts;
                has_ts = true;
            }
        } else if (strEq(k, "v1")) {
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
    const s = trim(header);
    if (s.len != 71 || !startsWith(s, "sha256=")) {
        return "";
    }
    let i: usize = 7;
    while (i < 71) {
        if (!isHexDigit(s[i])) {
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
        if (tok.len > 3 && startsWith(tok, "v1,")) {
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
    return ctEqual(a, b);
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
    let tmp: [24]u8 = undefined;
    const tlen = writeU64(timestamp_secs as u64, tmp[0..], 0);
    if (tlen + 1 + payload.len > out.len) {
        return 0;
    }
    let w = writeStr(tmp[0..tlen], out, 0);
    out[w] = '.';
    return writeStr(payload, out, w + 1);
}
