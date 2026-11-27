// @cap/raw-body — Pure CP zero-allocation HTTP request body byte-limit enforcer,
// Content-Length validator, streaming chunk accumulator & RFC 3629 UTF-8 validator.
// Clean-room MIT implementation inspired by npm `raw-body` v3.

import { isSpace, isDigit, trim, eqIgnoreCase } from "cap:ascii";

pub const RAW_OK: u16 = 0;
pub const RAW_ERR_TOO_LARGE: u16 = 413;
pub const RAW_ERR_LENGTH_MISMATCH: u16 = 400;
pub const RAW_ERR_INVALID_UTF8: u16 = 400;
pub const RAW_ERR_ABORTED: u16 = 499;

fn rwbIsCont(b: u8): bool {
    return (b & 0xC0) == 0x80;
}

fn rwbUnitMultiplier(unit: string): u64 {
    if (unit.len == 0 || eqIgnoreCase(unit, "b")) {
        return 1 as u64;
    }
    if (eqIgnoreCase(unit, "k") || eqIgnoreCase(unit, "kb")) {
        return 1024 as u64;
    }
    if (eqIgnoreCase(unit, "m") || eqIgnoreCase(unit, "mb")) {
        return 1048576 as u64;
    }
    if (eqIgnoreCase(unit, "g") || eqIgnoreCase(unit, "gb")) {
        return 1073741824 as u64;
    }
    if (eqIgnoreCase(unit, "t") || eqIgnoreCase(unit, "tb")) {
        return 1099511627776 as u64;
    }
    return 0 as u64;
}

/// Strict RFC 3629 UTF-8 validator (rejects overlong sequences, UTF-16 surrogates,
/// codepoints > U+10FFFF, and truncated multi-byte sequences).
pub fn isValidUtf8(data: []const u8): bool {
    let i: usize = 0;
    while (i < data.len) {
        let b0 = data[i];
        if (b0 < 0x80) {
            i = i + 1;
            continue;
        }
        if (b0 >= 0xC2 && b0 <= 0xDF) {
            if (i + 1 >= data.len || !rwbIsCont(data[i + 1])) {
                return false;
            }
            i = i + 2;
            continue;
        }
        if (b0 == 0xE0) {
            if (i + 2 >= data.len) { return false; }
            let b1 = data[i + 1];
            let b2 = data[i + 2];
            if (b1 < 0xA0 || b1 > 0xBF || !rwbIsCont(b2)) { return false; }
            i = i + 3;
            continue;
        }
        if ((b0 >= 0xE1 && b0 <= 0xEC) || b0 == 0xEE || b0 == 0xEF) {
            if (i + 2 >= data.len) { return false; }
            if (!rwbIsCont(data[i + 1]) || !rwbIsCont(data[i + 2])) { return false; }
            i = i + 3;
            continue;
        }
        if (b0 == 0xED) {
            if (i + 2 >= data.len) { return false; }
            let b1 = data[i + 1];
            let b2 = data[i + 2];
            if (b1 < 0x80 || b1 > 0x9F || !rwbIsCont(b2)) { return false; }
            i = i + 3;
            continue;
        }
        if (b0 == 0xF0) {
            if (i + 3 >= data.len) { return false; }
            let b1 = data[i + 1];
            if (b1 < 0x90 || b1 > 0xBF || !rwbIsCont(data[i + 2]) || !rwbIsCont(data[i + 3])) {
                return false;
            }
            i = i + 4;
            continue;
        }
        if (b0 >= 0xF1 && b0 <= 0xF3) {
            if (i + 3 >= data.len) { return false; }
            if (!rwbIsCont(data[i + 1]) || !rwbIsCont(data[i + 2]) || !rwbIsCont(data[i + 3])) {
                return false;
            }
            i = i + 4;
            continue;
        }
        if (b0 == 0xF4) {
            if (i + 3 >= data.len) { return false; }
            let b1 = data[i + 1];
            if (b1 < 0x80 || b1 > 0x8F || !rwbIsCont(data[i + 2]) || !rwbIsCont(data[i + 3])) {
                return false;
            }
            i = i + 4;
            continue;
        }
        return false;
    }
    return true;
}

/// Parses human-readable byte limits such as `"100kb"`, `"1mb"`, `"512b"`, `"1.5kb"`, or `"1024"`.
/// Returns `null` on empty, negative, overflowing, or unrecognized specifications.
pub fn parseByteLimit(spec: string): usize? {
    const s = trim(spec);
    if (s.len == 0) {
        return null;
    }

    let i: usize = 0;
    const end: usize = s.len;
    let int_part: u64 = 0;
    let digits: usize = 0;
    while (i < end && isDigit(s[i])) {
        let d = (s[i] - 48) as u64;
        if (int_part > ((0xFFFFFFFFFFFFFFFF as u64) - d) / (10 as u64)) {
            return null;
        }
        int_part = int_part * (10 as u64) + d;
        digits = digits + 1;
        i = i + 1;
    }

    let frac_num: u64 = 0;
    let frac_den: u64 = 1;
    let has_dot = false;
    if (i < end && s[i] == 0x2E) { // '.'
        has_dot = true;
        i = i + 1;
        let frac_digits: usize = 0;
        while (i < end && isDigit(s[i])) {
            if (frac_digits < 6) {
                frac_num = frac_num * (10 as u64) + ((s[i] - 48) as u64);
                frac_den = frac_den * (10 as u64);
            }
            frac_digits = frac_digits + 1;
            i = i + 1;
        }
        if (digits == 0 && frac_digits == 0) {
            return null;
        }
    } else if (digits == 0) {
        return null;
    }

    while (i < end && isSpace(s[i])) {
        i = i + 1;
    }
    let unit = s[i..end];
    let mult: u64 = rwbUnitMultiplier(unit);
    if (mult == 0) {
        return null;
    }

    if (int_part > (0xFFFFFFFFFFFFFFFF as u64) / mult) {
        return null;
    }
    let base_bytes: u64 = int_part * mult;
    let frac_bytes: u64 = if (has_dot && frac_num > 0) {
        (frac_num * mult) / frac_den
    } else {
        0 as u64
    };

    if (base_bytes > (0xFFFFFFFFFFFFFFFF as u64) - frac_bytes) {
        return null;
    }
    return (base_bytes + frac_bytes) as usize;
}

/// Configuration for `RawBodyReader`.
pub struct RawBodyOptions {
    pub limit: usize,
    pub expected_len: usize,
    pub has_expected_len: bool,
    pub validate_utf8: bool,

    pub static fn defaultOpts(): RawBodyOptions {
        return RawBodyOptions {
            limit: 102400, // 100kb
            expected_len: 0,
            has_expected_len: false,
            validate_utf8: false
        };
    }

    pub static fn withLimit(limit: usize): RawBodyOptions {
        return RawBodyOptions {
            limit: limit,
            expected_len: 0,
            has_expected_len: false,
            validate_utf8: false
        };
    }
}

/// Final completion result returned by `RawBodyReader.finish`.
pub struct RawBodyResult {
    pub status: u16,
    pub bytes_written: usize,

    pub fn isOk(): bool {
        return _.status == RAW_OK;
    }
}

/// Zero-allocation streaming HTTP request body accumulator & validator.
/// Writes incoming chunks into caller-provided `out: []u8`.
pub struct RawBodyReader {
    pub opts: RawBodyOptions,
    pub received: usize,
    pub failed_status: u16,
    pub finished: bool,

    pub static fn new(opts: RawBodyOptions): RawBodyReader {
        return RawBodyReader {
            opts: opts,
            received: 0,
            failed_status: RAW_OK,
            finished: false
        };
    }

    /// Pre-flight check on declared `Content-Length` against configured `limit`.
    pub fn checkDeclaredLength(): u16 {
        if (_.opts.has_expected_len && _.opts.limit > 0 && _.opts.expected_len > _.opts.limit) {
            return RAW_ERR_TOO_LARGE;
        }
        return RAW_OK;
    }

    /// Appends a streaming `chunk` into `out`, enforcing `limit` and `out.len` bounds.
    pub fn pushChunk(*_, chunk: []const u8, out: []u8): u16 {
        if (_.failed_status != RAW_OK) {
            return _.failed_status;
        }
        if (_.finished) {
            return RAW_ERR_ABORTED;
        }
        if (_.opts.has_expected_len && _.opts.limit > 0 && _.opts.expected_len > _.opts.limit) {
            _.failed_status = RAW_ERR_TOO_LARGE;
            return RAW_ERR_TOO_LARGE;
        }
        if (_.opts.limit > 0) {
            if (chunk.len > _.opts.limit || _.received > _.opts.limit - chunk.len) {
                _.failed_status = RAW_ERR_TOO_LARGE;
                return RAW_ERR_TOO_LARGE;
            }
        }
        if (chunk.len > out.len || _.received > out.len - chunk.len) {
            _.failed_status = RAW_ERR_TOO_LARGE;
            return RAW_ERR_TOO_LARGE;
        }
        if (chunk.len > 0) {
            @memcpy(out[_.received .. (_.received + chunk.len)], chunk);
        }
        _.received = _.received + chunk.len;
        return RAW_OK;
    }

    /// Marks the stream as aborted by client disconnect (`RAW_ERR_ABORTED = 499`).
    pub fn abort(*_): u16 {
        _.failed_status = RAW_ERR_ABORTED;
        _.finished = true;
        return RAW_ERR_ABORTED;
    }

    /// Finalizes the stream: verifies `received == expected_len` (when enabled)
    /// and validates UTF-8 encoding (when `validate_utf8` is true).
    pub fn finish(*_, out: []const u8): RawBodyResult {
        _.finished = true;
        if (_.failed_status != RAW_OK) {
            return RawBodyResult {
                status: _.failed_status,
                bytes_written: _.received
            };
        }
        if (_.opts.has_expected_len && _.received != _.opts.expected_len) {
            _.failed_status = RAW_ERR_LENGTH_MISMATCH;
            return RawBodyResult {
                status: RAW_ERR_LENGTH_MISMATCH,
                bytes_written: _.received
            };
        }
        if (_.opts.validate_utf8) {
            let n = if (_.received <= out.len) { _.received } else { out.len };
            if (!isValidUtf8(out[0..n])) {
                _.failed_status = RAW_ERR_INVALID_UTF8;
                return RawBodyResult {
                    status: RAW_ERR_INVALID_UTF8,
                    bytes_written: _.received
                };
            }
        }
        return RawBodyResult {
            status: RAW_OK,
            bytes_written: _.received
        };
    }

    pub fn receivedBytes(): usize {
        return _.received;
    }
}

export {
    RAW_OK,
    RAW_ERR_TOO_LARGE,
    RAW_ERR_LENGTH_MISMATCH,
    RAW_ERR_INVALID_UTF8,
    RAW_ERR_ABORTED,
    RawBodyOptions,
    RawBodyResult,
    RawBodyReader,
    parseByteLimit,
    isValidUtf8
};
