// Source: cp/lib/otp/otp.cp (Pure CP RFC 4226 HOTP, RFC 6238 TOTP & otpauth:// URI Engine)
// Zero external C dependency implementation using pure CP and std.

const std = @import("std");

/// Unified error set for OTP generation, verification, Base32, and otpauth:// URI operations.
pub err OtpError {
    InvalidSecret,
    InvalidDigits,
    InvalidPeriod,
    InvalidCode,
    InvalidUri,
    UnsupportedAlgorithm,
    OutOfMemory,
}

/// Supported HMAC hash algorithms for HOTP (RFC 4226) and TOTP (RFC 6238).
pub enum OtpAlgorithm {
    Sha1,
    Sha256,
    Sha512,
}

/// Verification outcome returned by `verifyTotp`, including the matched time step for replay protection.
pub struct TotpVerifyResult {
    valid: bool = false,
    matched_step: u64 = 0,
    delta: i64 = 0,
}

/// Parsed or structured `otpauth://` Key URI payload.
pub struct OtpAuthUri {
    is_totp: bool = true,
    label: []const u8 = "",
    issuer: []const u8 = "",
    secret_b32: []const u8 = "",
    algo: OtpAlgorithm = OtpAlgorithm.Sha1,
    digits: u8 = 6,
    period: u64 = 30,
    counter: u64 = 0,

    /// Releases heap-allocated `label`, `issuer`, and `secret_b32` slices.
    fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.label.len > 0) {
            allocator.free(_.label);
        }
        if (_.issuer.len > 0) {
            allocator.free(_.issuer);
        }
        if (_.secret_b32.len > 0) {
            allocator.free(_.secret_b32);
        }
        _.label = "";
        _.issuer = "";
        _.secret_b32 = "";
    }
}

const BASE32_ALPHABET: []const u8 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

fn base32CharVal(c_raw: u8): u8 {
    let c = c_raw;
    if (c >= 'a' && c <= 'z') {
        c -= 32;
    }
    if (c >= 'A' && c <= 'Z') {
        return c - 'A';
    }
    if (c >= '2' && c <= '7') {
        return (c - '2') + 26;
    }
    return 0xFF;
}

/// Encodes raw bytes into uppercase RFC 4648 Base32 with `'='` padding.
/// Caller owns the returned slice and must free it with `allocator.free`.
pub fn base32Encode(allocator: std.mem.Allocator, raw: []const u8): OtpError![]u8 {
    if (raw.len == 0) {
        return allocator.alloc(u8, 0) catch return OtpError.OutOfMemory;
    }

    const out_len = ((raw.len + 4) / 5) * 8;
    let out = allocator.alloc(u8, out_len) catch return OtpError.OutOfMemory;

    let i: usize = 0;
    let out_idx: usize = 0;
    while (i < raw.len) {
        const rem = raw.len - i;
        const b0: u64 = raw[i] as u64;
        const b1: u64 = if (rem > 1) (raw[i + 1] as u64) else 0;
        const b2: u64 = if (rem > 2) (raw[i + 2] as u64) else 0;
        const b3: u64 = if (rem > 3) (raw[i + 3] as u64) else 0;
        const b4: u64 = if (rem > 4) (raw[i + 4] as u64) else 0;

        const block: u64 = (b0 << 32) | (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;

        out[out_idx] = BASE32_ALPHABET[((block >> 35) & 0x1F) as usize];
        out[out_idx + 1] = BASE32_ALPHABET[((block >> 30) & 0x1F) as usize];
        out[out_idx + 2] = if (rem > 1) BASE32_ALPHABET[((block >> 25) & 0x1F) as usize] else '=';
        out[out_idx + 3] = if (rem > 1) BASE32_ALPHABET[((block >> 20) & 0x1F) as usize] else '=';
        out[out_idx + 4] = if (rem > 2) BASE32_ALPHABET[((block >> 15) & 0x1F) as usize] else '=';
        out[out_idx + 5] = if (rem > 3) BASE32_ALPHABET[((block >> 10) & 0x1F) as usize] else '=';
        out[out_idx + 6] = if (rem > 3) BASE32_ALPHABET[((block >> 5) & 0x1F) as usize] else '=';
        out[out_idx + 7] = if (rem > 4) BASE32_ALPHABET[(block & 0x1F) as usize] else '=';

        i += 5;
        out_idx += 8;
    }

    return out;
}

/// Decodes RFC 4648 Base32 (case-insensitive, ignores spaces and hyphens, validates characters and padding).
/// Caller owns the returned slice and must free it with `allocator.free`.
pub fn base32Decode(allocator: std.mem.Allocator, b32: []const u8): OtpError![]u8 {
    let data_count: usize = 0;
    let pad_count: usize = 0;
    let seen_pad = false;

    let idx: usize = 0;
    while (idx < b32.len) : (idx += 1) {
        const c = b32[idx];
        if (c == ' ' || c == '-') {
            continue;
        }
        if (c == '=') {
            seen_pad = true;
            pad_count += 1;
            continue;
        }
        if (seen_pad) {
            return OtpError.InvalidSecret;
        }
        if (base32CharVal(c) == 0xFF) {
            return OtpError.InvalidSecret;
        }
        data_count += 1;
    }

    if (data_count == 0) {
        if (pad_count > 0) {
            return OtpError.InvalidSecret;
        }
        return allocator.alloc(u8, 0) catch return OtpError.OutOfMemory;
    }

    if (pad_count > 0) {
        const total = data_count + pad_count;
        if ((total % 8) != 0) {
            return OtpError.InvalidSecret;
        }
        if (pad_count != 1 && pad_count != 3 && pad_count != 4 && pad_count != 6) {
            return OtpError.InvalidSecret;
        }
    } else {
        const rem = data_count % 8;
        if (rem == 1 || rem == 3 || rem == 6) {
            return OtpError.InvalidSecret;
        }
    }

    const out_len = (data_count * 5) / 8;
    let out = allocator.alloc(u8, out_len) catch return OtpError.OutOfMemory;

    let buf: u64 = 0;
    let bits: u32 = 0;
    let out_idx: usize = 0;

    idx = 0;
    while (idx < b32.len) : (idx += 1) {
        const c = b32[idx];
        if (c == ' ' || c == '-' || c == '=') {
            continue;
        }
        const val = base32CharVal(c) as u64;
        buf = (buf << 5) | val;
        bits += 5;
        if (bits >= 8) {
            bits -= 8;
            out[out_idx] = ((buf >> (bits as u6)) & 0xFF) as u8;
            out_idx += 1;
        }
    }

    return out;
}

fn formatCodeToBuf(code: u32, digits: u8, buf: *[8]u8) {
    let val = code;
    let i: usize = digits as usize;
    while (i > 0) {
        i -= 1;
        buf.*[i] = ('0' as u8) + ((val % 10) as u8);
        val /= 10;
    }
}

fn constantTimeEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        diff |= a[i] ^ b[i];
    }
    return diff == 0;
}

/// Computes an RFC 4226 HOTP integer code (supports 6..8 digits).
pub fn hotp(secret: []const u8, counter: u64, digits: u8, algo: OtpAlgorithm): OtpError!u32 {
    if (secret.len == 0) {
        return OtpError.InvalidSecret;
    }
    if (digits < 6 || digits > 8) {
        return OtpError.InvalidDigits;
    }

    let msg: [8]u8 = [0; 8];
    let c = counter;
    let k: usize = 8;
    while (k > 0) {
        k -= 1;
        msg[k] = (c & 0xFF) as u8;
        c >>= 8;
    }

    let bin_code: u32 = 0;
    switch (algo) {
        OtpAlgorithm.Sha1 => {
            let mac: [20]u8 = [0; 20];
            std.crypto.auth.hmac.HmacSha1.create(&mac, &msg, secret);
            const offset = (mac[19] & 0x0F) as usize;
            bin_code = (((mac[offset] & 0x7F) as u32) << 24)
                | ((mac[offset + 1] as u32) << 16)
                | ((mac[offset + 2] as u32) << 8)
                | (mac[offset + 3] as u32);
        },
        OtpAlgorithm.Sha256 => {
            let mac: [32]u8 = [0; 32];
            std.crypto.auth.hmac.sha2.HmacSha256.create(&mac, &msg, secret);
            const offset = (mac[31] & 0x0F) as usize;
            bin_code = (((mac[offset] & 0x7F) as u32) << 24)
                | ((mac[offset + 1] as u32) << 16)
                | ((mac[offset + 2] as u32) << 8)
                | (mac[offset + 3] as u32);
        },
        OtpAlgorithm.Sha512 => {
            let mac: [64]u8 = [0; 64];
            std.crypto.auth.hmac.sha2.HmacSha512.create(&mac, &msg, secret);
            const offset = (mac[63] & 0x0F) as usize;
            bin_code = (((mac[offset] & 0x7F) as u32) << 24)
                | ((mac[offset + 1] as u32) << 16)
                | ((mac[offset + 2] as u32) << 8)
                | (mac[offset + 3] as u32);
        },
    }

    const mod: u32 = switch (digits) {
        6 => 1_000_000,
        7 => 10_000_000,
        8 => 100_000_000,
        else => return OtpError.InvalidDigits,
    };

    return bin_code % mod;
}

/// Computes a zero-padded RFC 4226 HOTP string (`digits` ASCII characters, 6..8).
/// Caller owns the returned slice and must free it with `allocator.free`.
pub fn hotpString(
    allocator: std.mem.Allocator,
    secret: []const u8,
    counter: u64,
    digits: u8,
    algo: OtpAlgorithm,
): OtpError![]u8 {
    const code = hotp(secret, counter, digits, algo)?;
    let buf: [8]u8 = [0; 8];
    formatCodeToBuf(code, digits, &buf);
    const d = digits as usize;
    let out = allocator.alloc(u8, d) catch return OtpError.OutOfMemory;
    std.mem.copyForwards(u8, out, buf[0..d]);
    return out;
}

/// Computes an RFC 6238 TOTP integer code for the given `unix_sec` timestamp and `period`.
pub fn totp(secret: []const u8, unix_sec: u64, period: u64, digits: u8, algo: OtpAlgorithm): OtpError!u32 {
    if (period == 0) {
        return OtpError.InvalidPeriod;
    }
    const counter = unix_sec / period;
    return hotp(secret, counter, digits, algo);
}

/// Computes a zero-padded RFC 6238 TOTP string (`digits` ASCII characters, 6..8).
/// Caller owns the returned slice and must free it with `allocator.free`.
pub fn totpString(
    allocator: std.mem.Allocator,
    secret: []const u8,
    unix_sec: u64,
    period: u64,
    digits: u8,
    algo: OtpAlgorithm,
): OtpError![]u8 {
    if (period == 0) {
        return OtpError.InvalidPeriod;
    }
    const counter = unix_sec / period;
    return hotpString(allocator, secret, counter, digits, algo);
}

/// Verifies a TOTP code string using constant-time byte comparison within `±skew_steps` time steps.
/// Returns `TotpVerifyResult` with `matched_step` and `delta` for replay protection.
pub fn verifyTotp(
    secret: []const u8,
    code_str: []const u8,
    unix_sec: u64,
    period: u64,
    digits: u8,
    algo: OtpAlgorithm,
    skew_steps: u64,
): OtpError!TotpVerifyResult {
    if (secret.len == 0) {
        return OtpError.InvalidSecret;
    }
    if (period == 0) {
        return OtpError.InvalidPeriod;
    }
    if (digits < 6 || digits > 8) {
        return OtpError.InvalidDigits;
    }

    const d = digits as usize;
    if (code_str.len != d) {
        return OtpError.InvalidCode;
    }
    let ci: usize = 0;
    while (ci < d) : (ci += 1) {
        if (code_str[ci] < '0' || code_str[ci] > '9') {
            return OtpError.InvalidCode;
        }
    }

    const current_step = unix_sec / period;
    let buf: [8]u8 = [0; 8];

    // Check delta == 0 first
    const code0 = hotp(secret, current_step, digits, algo)?;
    formatCodeToBuf(code0, digits, &buf);
    if (constantTimeEql(code_str, buf[0..d])) {
        return TotpVerifyResult{
            valid: true,
            matched_step: current_step,
            delta: 0,
        };
    }

    let step_offset: u64 = 1;
    while (step_offset <= skew_steps) : (step_offset += 1) {
        // Check +step_offset
        if (current_step <= std.math.maxInt(u64) - step_offset) {
            const plus_step = current_step + step_offset;
            const code_plus = hotp(secret, plus_step, digits, algo)?;
            formatCodeToBuf(code_plus, digits, &buf);
            if (constantTimeEql(code_str, buf[0..d])) {
                return TotpVerifyResult{
                    valid: true,
                    matched_step: plus_step,
                    delta: step_offset as i64,
                };
            }
        }

        // Check -step_offset
        if (current_step >= step_offset) {
            const minus_step = current_step - step_offset;
            const code_minus = hotp(secret, minus_step, digits, algo)?;
            formatCodeToBuf(code_minus, digits, &buf);
            if (constantTimeEql(code_str, buf[0..d])) {
                return TotpVerifyResult{
                    valid: true,
                    matched_step: minus_step,
                    delta: -(step_offset as i64),
                };
            }
        }
    }

    return TotpVerifyResult{
        valid: false,
        matched_step: 0,
        delta: 0,
    };
}

fn isUriUnreservedOrColon(c: u8): bool {
    if (c >= 'A' && c <= 'Z') return true;
    if (c >= 'a' && c <= 'z') return true;
    if (c >= '0' && c <= '9') return true;
    return c == '-' || c == '_' || c == '.' || c == '~' || c == ':';
}

const HEX_UPPER: []const u8 = "0123456789ABCDEF";

fn percentEncodedLen(s: []const u8): usize {
    let n: usize = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (isUriUnreservedOrColon(s[i])) {
            n += 1;
        } else {
            n += 3;
        }
    }
    return n;
}

fn writePercentEncoded(dst: []u8, pos_in: usize, s: []const u8): usize {
    let pos = pos_in;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (isUriUnreservedOrColon(c)) {
            dst[pos] = c;
            pos += 1;
        } else {
            dst[pos] = '%';
            dst[pos + 1] = HEX_UPPER[(c >> 4) as usize];
            dst[pos + 2] = HEX_UPPER[(c & 0x0F) as usize];
            pos += 3;
        }
    }
    return pos;
}

fn hexVal(c: u8): u8 {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return (c - 'A') + 10;
    if (c >= 'a' && c <= 'f') return (c - 'a') + 10;
    return 0xFF;
}

fn percentDecodeAlloc(allocator: std.mem.Allocator, s: []const u8): OtpError![]const u8 {
    if (s.len == 0) {
        return "";
    }
    let out_len: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '%') {
            if (i + 2 >= s.len || hexVal(s[i + 1]) == 0xFF || hexVal(s[i + 2]) == 0xFF) {
                return OtpError.InvalidUri;
            }
            out_len += 1;
            i += 3;
        } else {
            out_len += 1;
            i += 1;
        }
    }

    let out = allocator.alloc(u8, out_len) catch return OtpError.OutOfMemory;
    let out_idx: usize = 0;
    i = 0;
    while (i < s.len) {
        if (s[i] == '%') {
            const hi = hexVal(s[i + 1]);
            const lo = hexVal(s[i + 2]);
            out[out_idx] = (hi << 4) | lo;
            out_idx += 1;
            i += 3;
        } else {
            out[out_idx] = s[i];
            out_idx += 1;
            i += 1;
        }
    }
    return out;
}

fn decimalU64Len(v_in: u64): usize {
    if (v_in == 0) return 1;
    let v = v_in;
    let n: usize = 0;
    while (v > 0) : (v /= 10) {
        n += 1;
    }
    return n;
}

fn writeDecimalU64(dst: []u8, pos_in: usize, v_in: u64): usize {
    const len = decimalU64Len(v_in);
    let v = v_in;
    let idx = pos_in + len;
    if (v == 0) {
        dst[pos_in] = '0';
        return pos_in + 1;
    }
    while (v > 0) {
        idx -= 1;
        dst[idx] = ('0' as u8) + ((v % 10) as u8);
        v /= 10;
    }
    return pos_in + len;
}

fn writeSlice(dst: []u8, pos_in: usize, s: []const u8): usize {
    std.mem.copyForwards(u8, dst[pos_in..(pos_in + s.len)], s);
    return pos_in + s.len;
}

fn algoToString(algo: OtpAlgorithm): []const u8 {
    return switch (algo) {
        OtpAlgorithm.Sha1 => "SHA1",
        OtpAlgorithm.Sha256 => "SHA256",
        OtpAlgorithm.Sha512 => "SHA512",
    };
}

fn eqlAsciiIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        let ca = a[i];
        let cb = b[i];
        if (ca >= 'a' && ca <= 'z') ca -= 32;
        if (cb >= 'a' && cb <= 'z') cb -= 32;
        if (ca != cb) return false;
    }
    return true;
}

fn parseU64Strict(s: []const u8): u64? {
    if (s.len == 0) return null;
    let val: u64 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        const digit = (c - '0') as u64;
        if (val > (std.math.maxInt(u64) - digit) / 10) return null;
        val = val * 10 + digit;
    }
    return val;
}

/// Formats an `OtpAuthUri` struct into a standard `otpauth://` URI string.
/// Caller owns the returned slice and must free it with `allocator.free`.
pub fn formatOtpAuthUri(allocator: std.mem.Allocator, uri_info: OtpAuthUri): OtpError![]u8 {
    if (uri_info.secret_b32.len == 0) {
        return OtpError.InvalidSecret;
    }
    if (uri_info.digits < 6 || uri_info.digits > 8) {
        return OtpError.InvalidDigits;
    }
    if (uri_info.is_totp && uri_info.period == 0) {
        return OtpError.InvalidPeriod;
    }

    const decoded_check = base32Decode(allocator, uri_info.secret_b32)?;
    allocator.free(decoded_check);

    const kind_str: []const u8 = if (uri_info.is_totp) "totp" else "hotp";
    const algo_str = algoToString(uri_info.algo);
    const digits_str_len = decimalU64Len(uri_info.digits as u64);
    const tail_key: []const u8 = if (uri_info.is_totp) "&period=" else "&counter=";
    const tail_val: u64 = if (uri_info.is_totp) uri_info.period else uri_info.counter;
    const tail_val_len = decimalU64Len(tail_val);

    let total_len: usize = "otpauth://".len + kind_str.len + 1 + percentEncodedLen(uri_info.label);
    total_len += "?secret=".len + uri_info.secret_b32.len;
    if (uri_info.issuer.len > 0) {
        total_len += "&issuer=".len + percentEncodedLen(uri_info.issuer);
    }
    total_len += "&algorithm=".len + algo_str.len;
    total_len += "&digits=".len + digits_str_len;
    total_len += tail_key.len + tail_val_len;

    let out = allocator.alloc(u8, total_len) catch return OtpError.OutOfMemory;
    let pos: usize = 0;
    pos = writeSlice(out, pos, "otpauth://");
    pos = writeSlice(out, pos, kind_str);
    out[pos] = '/';
    pos += 1;
    pos = writePercentEncoded(out, pos, uri_info.label);
    pos = writeSlice(out, pos, "?secret=");
    pos = writeSlice(out, pos, uri_info.secret_b32);
    if (uri_info.issuer.len > 0) {
        pos = writeSlice(out, pos, "&issuer=");
        pos = writePercentEncoded(out, pos, uri_info.issuer);
    }
    pos = writeSlice(out, pos, "&algorithm=");
    pos = writeSlice(out, pos, algo_str);
    pos = writeSlice(out, pos, "&digits=");
    pos = writeDecimalU64(out, pos, uri_info.digits as u64);
    pos = writeSlice(out, pos, tail_key);
    pos = writeDecimalU64(out, pos, tail_val);

    return out;
}

/// Parses an `otpauth://` URI string into an `OtpAuthUri` struct.
/// Caller must call `result.deinit(allocator)` when finished.
pub fn parseOtpAuthUri(allocator: std.mem.Allocator, uri: []const u8): OtpError!OtpAuthUri {
    const prefix = "otpauth://";
    if (!std.mem.startsWith(u8, uri, prefix)) {
        return OtpError.InvalidUri;
    }

    const rest = uri[prefix.len..];
    const slash_pos = std.mem.indexOfScalar(u8, rest, '/') orelse return OtpError.InvalidUri;
    const kind = rest[0..slash_pos];

    let is_totp = true;
    if (std.mem.eql(u8, kind, "totp")) {
        is_totp = true;
    } else if (std.mem.eql(u8, kind, "hotp")) {
        is_totp = false;
    } else {
        return OtpError.InvalidUri;
    }

    const after_kind = rest[(slash_pos + 1)..];
    const q_pos = std.mem.indexOfScalar(u8, after_kind, '?') orelse return OtpError.InvalidUri;
    const raw_label = after_kind[0..q_pos];
    const query = after_kind[(q_pos + 1)..];

    let label = percentDecodeAlloc(allocator, raw_label)?;
    errdefer if (label.len > 0) allocator.free(label);

    let issuer: []const u8 = "";
    errdefer if (issuer.len > 0) allocator.free(issuer);

    let secret_b32: []const u8 = "";
    errdefer if (secret_b32.len > 0) allocator.free(secret_b32);

    let algo = OtpAlgorithm.Sha1;
    let digits: u8 = 6;
    let period: u64 = 30;
    let counter: u64 = 0;

    let q_idx: usize = 0;
    while (q_idx <= query.len) {
        const next_amp = std.mem.indexOfScalarPos(u8, query, q_idx, '&') orelse query.len;
        const pair = query[q_idx..next_amp];
        if (pair.len > 0) {
            const eq_pos = std.mem.indexOfScalar(u8, pair, '=') orelse return OtpError.InvalidUri;
            const k = pair[0..eq_pos];
            const v = pair[(eq_pos + 1)..];

            if (std.mem.eql(u8, k, "secret")) {
                if (v.len == 0) {
                    return OtpError.InvalidSecret;
                }
                const decoded_sec = base32Decode(allocator, v)?;
                if (decoded_sec.len == 0) {
                    allocator.free(decoded_sec);
                    return OtpError.InvalidSecret;
                }
                allocator.free(decoded_sec);

                if (secret_b32.len > 0) {
                    allocator.free(secret_b32);
                }
                let sec_copy = allocator.alloc(u8, v.len) catch return OtpError.OutOfMemory;
                std.mem.copyForwards(u8, sec_copy, v);
                secret_b32 = sec_copy;
            } else if (std.mem.eql(u8, k, "issuer")) {
                if (issuer.len > 0) {
                    allocator.free(issuer);
                }
                issuer = percentDecodeAlloc(allocator, v)?;
            } else if (std.mem.eql(u8, k, "algorithm")) {
                if (eqlAsciiIgnoreCase(v, "SHA1")) {
                    algo = OtpAlgorithm.Sha1;
                } else if (eqlAsciiIgnoreCase(v, "SHA256")) {
                    algo = OtpAlgorithm.Sha256;
                } else if (eqlAsciiIgnoreCase(v, "SHA512")) {
                    algo = OtpAlgorithm.Sha512;
                } else {
                    return OtpError.UnsupportedAlgorithm;
                }
            } else if (std.mem.eql(u8, k, "digits")) {
                const d_val = parseU64Strict(v) orelse return OtpError.InvalidDigits;
                if (d_val < 6 || d_val > 8) {
                    return OtpError.InvalidDigits;
                }
                digits = d_val as u8;
            } else if (std.mem.eql(u8, k, "period")) {
                const p_val = parseU64Strict(v) orelse return OtpError.InvalidPeriod;
                if (p_val == 0) {
                    return OtpError.InvalidPeriod;
                }
                period = p_val;
            } else if (std.mem.eql(u8, k, "counter")) {
                const c_val = parseU64Strict(v) orelse return OtpError.InvalidUri;
                counter = c_val;
            }
        }
        if (next_amp == query.len) {
            break;
        }
        q_idx = next_amp + 1;
    }

    if (secret_b32.len == 0) {
        return OtpError.InvalidSecret;
    }

    return OtpAuthUri{
        is_totp: is_totp,
        label: label,
        issuer: issuer,
        secret_b32: secret_b32,
        algo: algo,
        digits: digits,
        period: period,
        counter: counter,
    };
}
