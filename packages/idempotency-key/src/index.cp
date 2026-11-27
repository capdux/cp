// packages/@cap/idempotency/src/index.cp
// Zero-allocation IETF Idempotency-Key HTTP header parser, formatter & state engine.
// MIT clean-room pure CP implementation.

pub const IDEMP_NEW: i32 = 0;
pub const IDEMP_REPLAY: i32 = 1;
pub const IDEMP_IN_FLIGHT: i32 = 2;
pub const IDEMP_CONFLICT: i32 = 3;

fn idempIsOws(c: u8): bool {
    return c == (' ' as u8) || c == ('\t' as u8);
}

fn idempTrimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && idempIsOws(s[start])) {
        start += 1;
    }
    while (end > start && idempIsOws(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn idempIsVisibleAscii(c: u8): bool {
    return c >= (0x21 as u8) && c <= (0x7E as u8);
}

fn idempIsHex(c: u8): bool {
    return (c >= ('0' as u8) && c <= ('9' as u8)) || (c >= ('a' as u8) && c <= ('f' as u8)) || (c >= ('A' as u8) && c <= ('F' as u8));
}

/// Validate an idempotency key: length 1..255 and strictly visible ASCII (0x21..0x7E).
pub fn isValidKey(key: string): bool {
    if (key.len == 0 || key.len > 255) {
        return false;
    }
    let i: usize = 0;
    while (i < key.len) {
        if (!idempIsVisibleAscii(key[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Parse an `Idempotency-Key` HTTP header into `out`.
/// Supports RFC 8941 quoted strings (`"key-123"`, unescaping `\"` and `\\`) as well as bare keys (`key-123`).
/// Enforces decoded length 1..255 and visible ASCII (0x21..0x7E). Returns bytes written, or `0` if invalid.
pub fn parseKeyHeader(header: string, out: []u8): usize {
    const t = idempTrimOws(header);
    if (t.len == 0) {
        return 0;
    }

    if (t[0] == ('"' as u8)) {
        // RFC 8941 quoted-string.
        if (t.len < 3) {
            return 0;
        }
        let i: usize = 1;
        let pos: usize = 0;
        let closed = false;
        while (i < t.len) {
            const c = t[i];
            if (c == ('\\' as u8)) {
                if (i + 1 >= t.len) {
                    return 0;
                }
                const next = t[i + 1];
                if (next != ('"' as u8) && next != ('\\' as u8)) {
                    return 0;
                }
                if (pos >= 255 || pos >= out.len) {
                    return 0;
                }
                out[pos] = next;
                pos += 1;
                i += 2;
            } else if (c == ('"' as u8)) {
                i += 1;
                closed = true;
                break;
            } else {
                if (!idempIsVisibleAscii(c)) {
                    return 0;
                }
                if (pos >= 255 || pos >= out.len) {
                    return 0;
                }
                out[pos] = c;
                pos += 1;
                i += 1;
            }
        }
        if (!closed || i != t.len || pos == 0) {
            return 0;
        }
        return pos;
    }

    // Bare key.
    if (!isValidKey(t) || t.len > out.len) {
        return 0;
    }
    let i: usize = 0;
    while (i < t.len) {
        out[i] = t[i];
        i += 1;
    }
    return t.len;
}

/// Format a valid idempotency key as an RFC 8941 quoted string `"..."` into `out`,
/// escaping `"` and `\`. Returns bytes written, or `0` if `key` is invalid or `out` is too small.
pub fn formatKeyHeader(key: string, out: []u8): usize {
    if (!isValidKey(key)) {
        return 0;
    }
    let need: usize = 2;
    let i: usize = 0;
    while (i < key.len) {
        if (key[i] == ('"' as u8) || key[i] == ('\\' as u8)) {
            need += 2;
        } else {
            need += 1;
        }
        i += 1;
    }
    if (need > out.len) {
        return 0;
    }

    let pos: usize = 0;
    out[pos] = '"' as u8;
    pos += 1;
    let j: usize = 0;
    while (j < key.len) {
        const c = key[j];
        if (c == ('"' as u8) || c == ('\\' as u8)) {
            out[pos] = '\\' as u8;
            out[pos + 1] = c;
            pos += 2;
        } else {
            out[pos] = c;
            pos += 1;
        }
        j += 1;
    }
    out[pos] = '"' as u8;
    pos += 1;
    return pos;
}

/// Fast 36-byte canonical UUID (`8-4-4-4-12` hex) format check.
pub fn isUuidV4Like(key: string): bool {
    if (key.len != 36) {
        return false;
    }
    let i: usize = 0;
    while (i < 36) {
        const c = key[i];
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (c != ('-' as u8)) {
                return false;
            }
        } else {
            if (!idempIsHex(c)) {
                return false;
            }
        }
        i += 1;
    }
    return true;
}

/// 64-bit wrapping multiplication using 32-bit split (Debug-mode overflow safe).
fn idempMulWrap64(a: u64, b: u64): u64 {
    const mask32: u64 = 0xFFFFFFFF as u64;
    const aLo = a & mask32;
    const aHi = (a >> (32 as u6)) & mask32;
    const bLo = b & mask32;
    const bHi = (b >> (32 as u6)) & mask32;

    const p0 = aLo * bLo;
    const p1 = (aLo * bHi) & mask32;
    const p2 = (aHi * bLo) & mask32;

    const lo = p0 & mask32;
    const hi = (((p0 >> (32 as u6)) & mask32) + p1 + p2) & mask32;
    return (hi << (32 as u6)) | lo;
}

fn idempFnvUpdate(h: u64, data: []const u8): u64 {
    const prime: u64 = ((0x00000100 as u64) << (32 as u6)) | (0x000001B3 as u64);
    let cur = h;
    let i: usize = 0;
    while (i < data.len) {
        cur = cur ^ (data[i] as u64);
        cur = idempMulWrap64(cur, prime);
        i += 1;
    }
    return cur;
}

fn idempFnvByte(h: u64, b: u8): u64 {
    const prime: u64 = ((0x00000100 as u64) << (32 as u6)) | (0x000001B3 as u64);
    return idempMulWrap64(h ^ (b as u64), prime);
}

/// Deterministic FNV-1a 64-bit fingerprint over `(method, path, body)` for detecting
/// parameter mismatch conflicts (HTTP 422) when reusing an `Idempotency-Key`.
pub fn fingerprintRequest(method: string, path: string, body: []const u8): u64 {
    // FNV-1a 64-bit offset basis 0xcbf29ce484222325 assembled from 32-bit halves.
    let h: u64 = ((0xCBF29CE4 as u64) << (32 as u6)) | (0x84222325 as u64);
    h = idempFnvUpdate(h, method);
    h = idempFnvByte(h, 0x1F as u8);
    h = idempFnvUpdate(h, path);
    h = idempFnvByte(h, 0x1F as u8);
    h = idempFnvUpdate(h, body);
    return h;
}

/// Evaluate idempotency state transition given stored entry metadata and incoming request fingerprint.
/// Returns `IDEMP_NEW` (0), `IDEMP_REPLAY` (1), `IDEMP_IN_FLIGHT` (2), or `IDEMP_CONFLICT` (3).
pub fn evaluateState(exists: bool, in_flight: bool, stored_fp: u64, req_fp: u64): i32 {
    if (!exists) {
        return IDEMP_NEW;
    }
    if (stored_fp != req_fp) {
        return IDEMP_CONFLICT;
    }
    if (in_flight) {
        return IDEMP_IN_FLIGHT;
    }
    return IDEMP_REPLAY;
}

export {
    IDEMP_NEW,
    IDEMP_REPLAY,
    IDEMP_IN_FLIGHT,
    IDEMP_CONFLICT,
    parseKeyHeader,
    formatKeyHeader,
    isValidKey,
    isUuidV4Like,
    fingerprintRequest,
    evaluateState,
};
