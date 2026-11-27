// Source: cp/lib/paseto/types.cp (PASETO v4 Core Types, PAE, Base64URL, Footer & Claims)

const std = @import("std");

type Vec16u8 = @Vector(16, u8);

// bit 0: JSON whitespace, bit 1: JSON unquoted value stop char
fn makePasetoJsonLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    lut[' '] = 3;
    lut['\t'] = 3;
    lut['\n'] = 3;
    lut['\r'] = 3;
    lut[','] = 2;
    lut['}'] = 2;
    return lut;
}

const PASETO_JSON_LUT: [u8; 256] = makePasetoJsonLut();

fn findByteSimd(s: []const u8, start: usize, ch: u8): usize {
    let i = start;
    const v_ch: Vec16u8 = @splat(ch);
    while (i + 16 <= s.len) {
        const v: Vec16u8 = s[i .. i + 16][0..16].*;
        const mask: u16 = @bitCast(v == v_ch);
        if (mask != 0) {
            return i + @as(usize, @ctz(mask));
        }
        i += 16;
    }
    while (i < s.len && s[i] != ch) {
        i += 1;
    }
    return i;
}

fn findQuoteOrBackslashSimd(s: []const u8, start: usize): usize {
    let i = start;
    const v_q: Vec16u8 = @splat('"');
    const v_bs: Vec16u8 = @splat('\\');
    while (i + 16 <= s.len) {
        const v: Vec16u8 = s[i .. i + 16][0..16].*;
        const mask: u16 = @bitCast((v == v_q) | (v == v_bs));
        if (mask != 0) {
            return i + @as(usize, @ctz(mask));
        }
        i += 16;
    }
    while (i < s.len && s[i] != '"' && s[i] != '\\') {
        i += 1;
    }
    return i;
}

fn findValStopSimd(s: []const u8, start: usize): usize {
    let i = start;
    const v_comma: Vec16u8 = @splat(',');
    const v_rbrace: Vec16u8 = @splat('}');
    const v_sp: Vec16u8 = @splat(' ');
    const v_nl: Vec16u8 = @splat('\n');
    const v_cr: Vec16u8 = @splat('\r');
    const v_tb: Vec16u8 = @splat('\t');
    while (i + 16 <= s.len) {
        const v: Vec16u8 = s[i .. i + 16][0..16].*;
        const hit = (v == v_comma) | (v == v_rbrace) | (v == v_sp) | (v == v_nl) | (v == v_cr) | (v == v_tb);
        const mask: u16 = @bitCast(hit);
        if (mask != 0) {
            return i + @as(usize, @ctz(mask));
        }
        i += 16;
    }
    while (i < s.len && (PASETO_JSON_LUT[@as(usize, s[i])] & 2) == 0) {
        i += 1;
    }
    return i;
}

pub const PasetoError = error{
    InvalidPrefix,
    InvalidPurpose,
    InvalidFormat,
    InvalidBase64,
    AuthenticationFailed,
    InvalidSignature,
    InvalidKey,
    ExpiredToken,
    NotYetValid,
    ClaimMismatch,
    OutOfMemory,
};

pub const PasetoPurpose = enum {
    Local,
    Public,
};

pub const PasetoToken = struct {
    version: []const u8,
    purpose: PasetoPurpose,
    payload_b64: []const u8,
    footer_b64: []const u8,
};

/// Pre-Authentication Encoding (PAE) per PASETO v4 specification.
/// Encodes count as 64-bit little-endian, followed by each piece's 64-bit LE length and bytes.
pub fn paeEncode(alloc: std.mem.Allocator, pieces: []const []const u8): ([]u8) ! PasetoError {
    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);

    let len_buf: [8]u8 = undefined;
    const count_u64: u64 = @as(u64, @intCast(pieces.len)) & 0x7FFF_FFFF_FFFF_FFFF;
    std.mem.writeInt(u64, &len_buf, count_u64, .little);
    out.appendSlice(alloc, &len_buf) catch return PasetoError.OutOfMemory;

    for (piece of pieces) {
        const piece_u64: u64 = @as(u64, @intCast(piece.len)) & 0x7FFF_FFFF_FFFF_FFFF;
        std.mem.writeInt(u64, &len_buf, piece_u64, .little);
        out.appendSlice(alloc, &len_buf) catch return PasetoError.OutOfMemory;
        out.appendSlice(alloc, piece) catch return PasetoError.OutOfMemory;
    }

    return out.toOwnedSlice(alloc) catch return PasetoError.OutOfMemory;
}

pub fn encodeBase64Url(alloc: std.mem.Allocator, data: []const u8): ([]u8) ! PasetoError {
    const encoded_len = std.base64.url_safe_no_pad.Encoder.calcSize(data.len);
    const out = alloc.alloc(u8, encoded_len) catch return PasetoError.OutOfMemory;
    const encoded = std.base64.url_safe_no_pad.Encoder.encode(out, data);
    if (encoded.len != encoded_len) {
        return PasetoError.InvalidFormat;
    }
    return out;
}

pub fn decodeBase64Url(alloc: std.mem.Allocator, encoded: []const u8): ([]u8) ! PasetoError {
    const decoded_len = std.base64.url_safe_no_pad.Decoder.calcSizeForSlice(encoded) catch {
        return PasetoError.InvalidBase64;
    };
    const out = alloc.alloc(u8, decoded_len) catch return PasetoError.OutOfMemory;
    errdefer alloc.free(out);
    std.base64.url_safe_no_pad.Decoder.decode(out, encoded) catch {
        return PasetoError.InvalidBase64;
    };
    return out;
}

/// Parses a PASETO token string into its structural components without allocating.
pub fn parseToken(token: []const u8): PasetoToken ! PasetoError {
    let it = std.mem.splitScalar(u8, token, '.');
    const ver = it.next() orelse return PasetoError.InvalidFormat;
    const purp_str = it.next() orelse return PasetoError.InvalidFormat;
    const payload_b64 = it.next() orelse return PasetoError.InvalidFormat;
    const footer_b64 = it.next() orelse "";
    if (it.next() != null) {
        return PasetoError.InvalidFormat;
    }

    if (!std.mem.eql(u8, ver, "v4")) {
        return PasetoError.InvalidPrefix;
    }

    let purpose: PasetoPurpose = undefined;
    if (std.mem.eql(u8, purp_str, "local")) {
        purpose = PasetoPurpose.Local;
    } else if (std.mem.eql(u8, purp_str, "public")) {
        purpose = PasetoPurpose.Public;
    } else {
        return PasetoError.InvalidPurpose;
    }

    if (payload_b64.len == 0) {
        return PasetoError.InvalidFormat;
    }

    return PasetoToken{
        .version = ver,
        .purpose = purpose,
        .payload_b64 = payload_b64,
        .footer_b64 = footer_b64,
    };
}

/// Extracts and Base64URL-decodes the optional footer from a PASETO v4 token.
pub fn extractFooter(alloc: std.mem.Allocator, token: []const u8): ([]u8) ! PasetoError {
    const parsed = parseToken(token) ?;
    if (parsed.footer_b64.len == 0) {
        return alloc.alloc(u8, 0) catch return PasetoError.OutOfMemory;
    }
    return decodeBase64Url(alloc, parsed.footer_b64);
}

fn isLeapYear(year: i64): bool {
    if (@mod(year, 400) == 0) {
        return true;
    }
    if (@mod(year, 100) == 0) {
        return false;
    }
    return @mod(year, 4) == 0;
}

fn daysInMonth(year: i64, month: i64): i64 {
    if (month == 2) {
        if (isLeapYear(year)) {
            return 29;
        }
        return 28;
    }
    if (month == 4 or month == 6 or month == 9 or month == 11) {
        return 30;
    }
    return 31;
}

fn parseTwoDigits(s: []const u8): i64? {
    if (s.len < 2 or s[0] < '0' or s[0] > '9' or s[1] < '0' or s[1] > '9') {
        return null;
    }
    return @as(i64, s[0] - '0') * 10 + @as(i64, s[1] - '0');
}

fn parseFourDigits(s: []const u8): i64? {
    if (s.len < 4) {
        return null;
    }
    const hi = parseTwoDigits(s[0..2]) orelse return null;
    const lo = parseTwoDigits(s[2..4]) orelse return null;
    return hi * 100 + lo;
}

/// Parses either a decimal Unix timestamp string or an ISO-8601 UTC timestamp ("YYYY-MM-DDTHH:MM:SSZ").
fn parseTimestampUnix(s: []const u8): i64? {
    if (s.len == 0) {
        return null;
    }
    // Try integer unix timestamp first
    const ts_opt: i64? = std.fmt.parseInt(i64, s, 10) catch null;
    if (ts_opt != null) {
        return ts_opt.?;
    }

    // ISO-8601: YYYY-MM-DDTHH:MM:SSZ (minimum 20 chars)
    if (s.len < 20 or s[4] != '-' or s[7] != '-' or (s[10] != 'T' and s[10] != 't') or s[13] != ':' or s[16] != ':') {
        return null;
    }
    const year = parseFourDigits(s[0..4]) orelse return null;
    const month = parseTwoDigits(s[5..7]) orelse return null;
    const day = parseTwoDigits(s[8..10]) orelse return null;
    const hour = parseTwoDigits(s[11..13]) orelse return null;
    const min = parseTwoDigits(s[14..16]) orelse return null;
    const sec = parseTwoDigits(s[17..19]) orelse return null;

    if (month < 1 or month > 12 or day < 1 or day > 31 or hour > 23 or min > 59 or sec > 59) {
        return null;
    }

    let days: i64 = 0;
    if (year >= 1970) {
        let y: i64 = 1970;
        while (y < year) {
            days += if (isLeapYear(y)) 366 else 365;
            y += 1;
        }
    } else {
        let y: i64 = year;
        while (y < 1970) {
            days -= if (isLeapYear(y)) 366 else 365;
            y += 1;
        }
    }

    let m: i64 = 1;
    while (m < month) {
        days += daysInMonth(year, m);
        m += 1;
    }
    days += (day - 1);

    return days * 86400 + hour * 3600 + min * 60 + sec;
}

/// Extracts a JSON string or raw number value for a top-level key in a JSON object.
fn findJsonField(json: []const u8, key: []const u8): ([]const u8)? {
    let i: usize = 0;
    while (i < json.len) {
        i = findByteSimd(json, i, '"');
        if (i >= json.len) {
            return null;
        }
        i += 1;
        const key_start = i;
        while (i < json.len) {
            i = findQuoteOrBackslashSimd(json, i);
            if (i >= json.len) {
                return null;
            }
            if (json[i] == '\\') {
                if (i + 1 < json.len) {
                    i += 2;
                } else {
                    i += 1;
                }
            } else {
                break;
            }
        }
        if (i >= json.len) {
            return null;
        }
        const found_key = json[key_start..i];
        i += 1; // skip closing quote

        while (i < json.len and (PASETO_JSON_LUT[@as(usize, json[i])] & 1) != 0) {
            i += 1;
        }
        if (i < json.len and json[i] == ':') {
            i += 1;
            while (i < json.len and (PASETO_JSON_LUT[@as(usize, json[i])] & 1) != 0) {
                i += 1;
            }
            if (i >= json.len) {
                return null;
            }
            if (json[i] == '"') {
                i += 1;
                const val_start = i;
                while (i < json.len) {
                    i = findQuoteOrBackslashSimd(json, i);
                    if (i >= json.len) {
                        return null;
                    }
                    if (json[i] == '\\') {
                        if (i + 1 < json.len) {
                            i += 2;
                        } else {
                            i += 1;
                        }
                    } else {
                        break;
                    }
                }
                if (i >= json.len) {
                    return null;
                }
                const val = json[val_start..i];
                i += 1;
                if (std.mem.eql(u8, found_key, key)) {
                    return val;
                }
            } else {
                const val_start = i;
                i = findValStopSimd(json, i);
                const val = json[val_start..i];
                if (std.mem.eql(u8, found_key, key)) {
                    return val;
                }
            }
        }
    }
    return null;
}

/// Validates standard PASETO claims (`exp`, `nbf`, `iss`, `aud`) in a JSON payload.
pub fn validateClaims(
    json_payload: []const u8,
    now_iso_or_unix: []const u8,
    issuer_opt: ([]const u8)?,
    audience_opt: ([]const u8)?,
): void ! PasetoError {
    const now_unix = parseTimestampUnix(now_iso_or_unix) orelse return PasetoError.InvalidFormat;

    const exp_opt = findJsonField(json_payload, "exp");
    if (exp_opt != null) {
        const exp_unix = parseTimestampUnix(exp_opt.?) orelse return PasetoError.InvalidFormat;
        if (now_unix >= exp_unix) {
            return PasetoError.ExpiredToken;
        }
    }

    const nbf_opt = findJsonField(json_payload, "nbf");
    if (nbf_opt != null) {
        const nbf_unix = parseTimestampUnix(nbf_opt.?) orelse return PasetoError.InvalidFormat;
        if (now_unix < nbf_unix) {
            return PasetoError.NotYetValid;
        }
    }

    if (issuer_opt != null) {
        const actual_iss = findJsonField(json_payload, "iss") orelse return PasetoError.ClaimMismatch;
        if (!std.mem.eql(u8, actual_iss, issuer_opt.?)) {
            return PasetoError.ClaimMismatch;
        }
    }

    if (audience_opt != null) {
        const actual_aud = findJsonField(json_payload, "aud") orelse return PasetoError.ClaimMismatch;
        if (!std.mem.eql(u8, actual_aud, audience_opt.?)) {
            return PasetoError.ClaimMismatch;
        }
    }
}
