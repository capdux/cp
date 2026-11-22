// Source: native-cp-stdlib (RFC 4226 / RFC 6238 TOTP & HOTP)

const std = @import("std");

type StrSlice = []const u8;
type ByteSlice = []u8;

export enum HashAlg {
    Sha1,
    Sha256,
    Sha512,
}

export enum OtpType {
    Totp,
    Hotp,
}

export struct OtpAuthUri {
    otp_type: OtpType,
    label: []const u8,
    issuer: []const u8,
    account: []const u8,
    secret: []const u8,
    algorithm: HashAlg,
    digits: u8,
    period: u64,
    counter: u64,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.label.len > 0) alloc.free(_.label);
        if (_.issuer.len > 0) alloc.free(_.issuer);
        if (_.account.len > 0) alloc.free(_.account);
        if (_.secret.len > 0) alloc.free(_.secret);
        _.label = "";
        _.issuer = "";
        _.account = "";
        _.secret = "";
    }
}

fn pow10(digits: u8): u32 {
    return switch (digits) {
        6 => 1000000,
        7 => 10000000,
        8 => 100000000,
        else => 1000000,
    };
}

fn dynamicTruncate(digest: []const u8): u32 {
    const last_idx: usize = digest.len - 1;
    const offset: usize = @intCast(digest[last_idx] & 0x0f);
    const idx1: usize = offset + 1;
    const idx2: usize = offset + 2;
    const idx3: usize = offset + 3;
    const b0: u32 = @intCast(digest[offset] & 0x7f);
    const b1: u32 = @intCast(digest[idx1]);
    const b2: u32 = @intCast(digest[idx2]);
    const b3: u32 = @intCast(digest[idx3]);
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

export fn hotpAtWithAlg(secret: []const u8, counter: u64, digits: u8, alg: HashAlg): u32 {
    let counter_bytes: [8]u8 = undefined;
    std.mem.writeInt(u64, &counter_bytes, counter, .big);

    const mod_val = pow10(digits);

    switch (alg) {
        .Sha1 => {
            let mac: [20]u8 = undefined;
            std.crypto.auth.hmac.HmacSha1.create(&mac, &counter_bytes, secret);
            return dynamicTruncate(&mac) % mod_val;
        },
        .Sha256 => {
            let mac: [32]u8 = undefined;
            std.crypto.auth.hmac.HmacSha256.create(&mac, &counter_bytes, secret);
            return dynamicTruncate(&mac) % mod_val;
        },
        .Sha512 => {
            let mac: [64]u8 = undefined;
            std.crypto.auth.hmac.HmacSha512.create(&mac, &counter_bytes, secret);
            return dynamicTruncate(&mac) % mod_val;
        },
    }
}

export fn hotpAt(secret: []const u8, counter: u64, digits: u8): u32 {
    return hotpAtWithAlg(secret, counter, digits, HashAlg.Sha1);
}

export fn totpAt(secret: []const u8, unix_sec: u64, period: u64, digits: u8, alg: HashAlg): u32 {
    const step: u64 = if (period == 0) 30 else period;
    const counter: u64 = unix_sec / step;
    return hotpAtWithAlg(secret, counter, digits, alg);
}

export fn formatCode(code: u32, digits: u8, out: []u8): []const u8 {
    const d: usize = if (digits >= 6 and digits <= 8) @intCast(digits) else 6;
    if (out.len < d) return "";
    let v: u32 = code;
    let i: usize = d;
    while (i > 0) {
        i = i - 1;
        const digit_u8: u8 = @intCast(v % 10);
        out[i] = @as(u8, '0') + digit_u8;
        v = v / 10;
    }
    return out[0..d];
}

export fn constantTimeEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) {
        diff = diff | (a[i] ^ b[i]);
        i = i + 1;
    }
    return diff == 0;
}

fn isAllDigits(s: []const u8): bool {
    for (ch of s) {
        if (ch < '0' or ch > '9') return false;
    }
    return true;
}

export fn verifyHotp(
    secret: []const u8,
    counter: u64,
    code: []const u8,
    digits: u8,
    lookahead: u64,
    alg: HashAlg,
): u64? {
    const d: u8 = if (digits >= 6 and digits <= 8) digits else 6;
    const d_usize: usize = @intCast(d);
    if (code.len != d_usize or !isAllDigits(code)) return null;

    let expected_buf: [8]u8 = undefined;
    let matched_next: u64? = null;
    let offset: u64 = 0;
    while (offset <= lookahead) {
        if (counter > std.math.maxInt(u64) - offset) break;
        const c: u64 = counter + offset;
        const expected = hotpAtWithAlg(secret, c, d, alg);
        const expected_str = formatCode(expected, d, &expected_buf);
        if (constantTimeEq(code, expected_str)) {
            if (matched_next == null) {
                matched_next = c +% 1;
            }
        }
        offset = offset + 1;
    }
    return matched_next;
}

export fn verifyTotp(
    secret: []const u8,
    unix_sec: u64,
    code: []const u8,
    period: u64,
    digits: u8,
    window: u64,
    alg: HashAlg,
): i64? {
    const d: u8 = if (digits >= 6 and digits <= 8) digits else 6;
    const d_usize: usize = @intCast(d);
    if (code.len != d_usize or !isAllDigits(code)) return null;

    const step: u64 = if (period == 0) 30 else period;
    const base_counter: u64 = unix_sec / step;

    let expected_buf: [8]u8 = undefined;
    let best_delta: i64? = null;
    let best_abs: u64 = std.math.maxInt(u64);

    let neg_off: u64 = 1;
    while (neg_off <= window) {
        if (base_counter >= neg_off) {
            const c: u64 = base_counter - neg_off;
            const expected = hotpAtWithAlg(secret, c, d, alg);
            const expected_str = formatCode(expected, d, &expected_buf);
            if (constantTimeEq(code, expected_str)) {
                if (neg_off < best_abs) {
                    best_abs = neg_off;
                    const signed_off: i64 = @intCast(neg_off);
                    best_delta = -signed_off;
                }
            }
        }
        neg_off = neg_off + 1;
    }

    let pos_off: u64 = 0;
    while (pos_off <= window) {
        if (base_counter <= std.math.maxInt(u64) - pos_off) {
            const c: u64 = base_counter + pos_off;
            const expected = hotpAtWithAlg(secret, c, d, alg);
            const expected_str = formatCode(expected, d, &expected_buf);
            if (constantTimeEq(code, expected_str)) {
                if (pos_off <= best_abs) {
                    best_abs = pos_off;
                    const signed_off: i64 = @intCast(pos_off);
                    best_delta = signed_off;
                }
            }
        }
        pos_off = pos_off + 1;
    }

    return best_delta;
}

const B32_ALPHABET: []const u8 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

export fn base32Encode(alloc: std.mem.Allocator, data: []const u8, pad: bool): ByteSlice? {
    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);

    if (data.len == 0) {
        return out.toOwnedSlice(alloc) catch return null;
    }

    let buffer: u32 = 0;
    let bits_left: u5 = 0;

    for (byte of data) {
        buffer = (buffer << 8) | @as(u32, byte);
        bits_left = bits_left + 8;
        while (bits_left >= 5) {
            const shift: u5 = bits_left - 5;
            const idx: usize = @intCast((buffer >> shift) & 0x1f);
            out.append(alloc, B32_ALPHABET[idx]) catch {
                out.deinit(alloc);
                return null;
            };
            bits_left = bits_left - 5;
        }
    }

    if (bits_left > 0) {
        const shift: u5 = 5 - bits_left;
        const idx: usize = @intCast((buffer << shift) & 0x1f);
        out.append(alloc, B32_ALPHABET[idx]) catch {
            out.deinit(alloc);
            return null;
        };
    }

    if (pad) {
        while (out.items.len % 8 != 0) {
            out.append(alloc, '=') catch {
                out.deinit(alloc);
                return null;
            };
        }
    }

    return out.toOwnedSlice(alloc) catch {
        out.deinit(alloc);
        return null;
    };
}

fn base32CharVal(ch: u8): u8? {
    if (ch >= 'A' and ch <= 'Z') return ch - 'A';
    if (ch >= 'a' and ch <= 'z') return ch - 'a';
    if (ch >= '2' and ch <= '7') return (ch - '2') + 26;
    return null;
}

export fn base32Decode(alloc: std.mem.Allocator, input: []const u8): ByteSlice? {
    let out = std.ArrayList(u8).empty;
    let buffer: u32 = 0;
    let bits_left: u5 = 0;

    for (ch of input) {
        if (ch == ' ' or ch == '\t' or ch == '\r' or ch == '\n' or ch == '=') {
            continue;
        }
        const val = base32CharVal(ch) orelse {
            out.deinit(alloc);
            return null;
        };
        buffer = (buffer << 5) | @as(u32, val);
        bits_left = bits_left + 5;
        if (bits_left >= 8) {
            const shift: u5 = bits_left - 8;
            const byte_val: u8 = @intCast((buffer >> shift) & 0xff);
            out.append(alloc, byte_val) catch {
                out.deinit(alloc);
                return null;
            };
            bits_left = bits_left - 8;
        }
    }

    return out.toOwnedSlice(alloc) catch {
        out.deinit(alloc);
        return null;
    };
}

export fn generateSecret(alloc: std.mem.Allocator, num_bytes: usize): ByteSlice? {
    let threaded = std.Io.Threaded.init(alloc, .{});
    defer threaded.deinit();
    const io = threaded.io();
    const buf = alloc.alloc(u8, num_bytes) catch return null;
    io.random(buf);
    return buf;
}

fn isUnreservedUriChar(c: u8): bool {
    if (c >= 'A' and c <= 'Z') return true;
    if (c >= 'a' and c <= 'z') return true;
    if (c >= '0' and c <= '9') return true;
    return c == '-' or c == '_' or c == '.' or c == '~';
}

fn hexDigit(v: u8): u8 {
    return if (v < 10) @as(u8, '0') + v else @as(u8, 'A') + (v - 10);
}

fn fromHexDigit(c: u8): u8? {
    if (c >= '0' and c <= '9') return c - '0';
    if (c >= 'A' and c <= 'F') return (c - 'A') + 10;
    if (c >= 'a' and c <= 'f') return (c - 'a') + 10;
    return null;
}

fn urlEncode(alloc: std.mem.Allocator, input: []const u8): ByteSlice? {
    let out = std.ArrayList(u8).empty;
    for (c of input) {
        if (isUnreservedUriChar(c)) {
            out.append(alloc, c) catch {
                out.deinit(alloc);
                return null;
            };
        } else {
            out.append(alloc, '%') catch {
                out.deinit(alloc);
                return null;
            };
            out.append(alloc, hexDigit(c >> 4)) catch {
                out.deinit(alloc);
                return null;
            };
            out.append(alloc, hexDigit(c & 0x0f)) catch {
                out.deinit(alloc);
                return null;
            };
        }
    }
    return out.toOwnedSlice(alloc) catch {
        out.deinit(alloc);
        return null;
    };
}

fn urlDecode(alloc: std.mem.Allocator, input: []const u8): ByteSlice? {
    let out = std.ArrayList(u8).empty;
    let i: usize = 0;
    while (i < input.len) {
        if (input[i] == '%' and i + 2 < input.len) {
            const hi = fromHexDigit(input[i + 1]);
            const lo = fromHexDigit(input[i + 2]);
            if (hi != null and lo != null) {
                const byte_val: u8 = (hi.? << 4) | lo.?;
                out.append(alloc, byte_val) catch {
                    out.deinit(alloc);
                    return null;
                };
                i = i + 3;
                continue;
            }
        } else if (input[i] == '+') {
            out.append(alloc, ' ') catch {
                out.deinit(alloc);
                return null;
            };
            i = i + 1;
            continue;
        }
        out.append(alloc, input[i]) catch {
            out.deinit(alloc);
            return null;
        };
        i = i + 1;
    }
    return out.toOwnedSlice(alloc) catch {
        out.deinit(alloc);
        return null;
    };
}

fn asciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        let ca = a[i];
        let cb = b[i];
        if (ca >= 'A' and ca <= 'Z') ca = ca + 32;
        if (cb >= 'A' and cb <= 'Z') cb = cb + 32;
        if (ca != cb) return false;
        i = i + 1;
    }
    return true;
}

fn parseAlg(s: []const u8): HashAlg? {
    if (asciiEqlIgnoreCase(s, "SHA1")) return HashAlg.Sha1;
    if (asciiEqlIgnoreCase(s, "SHA256")) return HashAlg.Sha256;
    if (asciiEqlIgnoreCase(s, "SHA512")) return HashAlg.Sha512;
    return null;
}

fn algToStr(alg: HashAlg): []const u8 {
    return switch (alg) {
        .Sha1 => "SHA1",
        .Sha256 => "SHA256",
        .Sha512 => "SHA512",
    };
}

export fn buildOtpAuthUri(alloc: std.mem.Allocator, uri: OtpAuthUri): ByteSlice? {
    let out = std.ArrayList(u8).empty;

    const type_str: []const u8 = if (uri.otp_type == OtpType.Totp) "totp" else "hotp";
    out.appendSlice(alloc, "otpauth://") catch {
        out.deinit(alloc);
        return null;
    };
    out.appendSlice(alloc, type_str) catch {
        out.deinit(alloc);
        return null;
    };
    out.append(alloc, '/') catch {
        out.deinit(alloc);
        return null;
    };

    if (uri.issuer.len > 0) {
        const enc_issuer = urlEncode(alloc, uri.issuer) orelse {
            out.deinit(alloc);
            return null;
        };
        defer alloc.free(enc_issuer);
        out.appendSlice(alloc, enc_issuer) catch {
            out.deinit(alloc);
            return null;
        };
        out.append(alloc, ':') catch {
            out.deinit(alloc);
            return null;
        };
    }

    const enc_account = urlEncode(alloc, uri.account) orelse {
        out.deinit(alloc);
        return null;
    };
    defer alloc.free(enc_account);
    out.appendSlice(alloc, enc_account) catch {
        out.deinit(alloc);
        return null;
    };

    out.appendSlice(alloc, "?secret=") catch {
        out.deinit(alloc);
        return null;
    };
    out.appendSlice(alloc, uri.secret) catch {
        out.deinit(alloc);
        return null;
    };

    if (uri.issuer.len > 0) {
        const enc_issuer2 = urlEncode(alloc, uri.issuer) orelse {
            out.deinit(alloc);
            return null;
        };
        defer alloc.free(enc_issuer2);
        out.appendSlice(alloc, "&issuer=") catch {
            out.deinit(alloc);
            return null;
        };
        out.appendSlice(alloc, enc_issuer2) catch {
            out.deinit(alloc);
            return null;
        };
    }

    out.appendSlice(alloc, "&algorithm=") catch {
        out.deinit(alloc);
        return null;
    };
    out.appendSlice(alloc, algToStr(uri.algorithm)) catch {
        out.deinit(alloc);
        return null;
    };

    let num_buf: [32]u8 = undefined;
    const digits_val: u8 = if (uri.digits >= 6 and uri.digits <= 8) uri.digits else 6;
    const digits_str = std.fmt.bufPrint(&num_buf, "&digits={d}", .{digits_val}) catch {
        out.deinit(alloc);
        return null;
    };
    out.appendSlice(alloc, digits_str) catch {
        out.deinit(alloc);
        return null;
    };

    if (uri.otp_type == OtpType.Totp) {
        const period_val: u64 = if (uri.period == 0) 30 else uri.period;
        const period_str = std.fmt.bufPrint(&num_buf, "&period={d}", .{period_val}) catch {
            out.deinit(alloc);
            return null;
        };
        out.appendSlice(alloc, period_str) catch {
            out.deinit(alloc);
            return null;
        };
    } else {
        const counter_str = std.fmt.bufPrint(&num_buf, "&counter={d}", .{uri.counter}) catch {
            out.deinit(alloc);
            return null;
        };
        out.appendSlice(alloc, counter_str) catch {
            out.deinit(alloc);
            return null;
        };
    }

    return out.toOwnedSlice(alloc) catch {
        out.deinit(alloc);
        return null;
    };
}

export fn parseOtpAuthUri(alloc: std.mem.Allocator, uri_str: []const u8): OtpAuthUri? {
    const prefix = "otpauth://";
    if (!std.mem.startsWith(u8, uri_str, prefix)) return null;
    const rest = uri_str[prefix.len..];

    const slash_idx = std.mem.indexOfScalar(u8, rest, '/') orelse return null;
    const type_part = rest[0..slash_idx];
    let otp_type: OtpType = OtpType.Totp;
    if (asciiEqlIgnoreCase(type_part, "totp")) {
        otp_type = OtpType.Totp;
    } else if (asciiEqlIgnoreCase(type_part, "hotp")) {
        otp_type = OtpType.Hotp;
    } else {
        return null;
    }

    const after_type = rest[(slash_idx + 1)..];
    const q_idx = std.mem.indexOfScalar(u8, after_type, '?') orelse return null;
    const raw_label = after_type[0..q_idx];
    const query = after_type[(q_idx + 1)..];

    const decoded_label = urlDecode(alloc, raw_label) orelse return null;

    let label_issuer: []const u8 = "";
    let label_account: []const u8 = decoded_label;
    if (colon_idx := std.mem.indexOfScalar(u8, decoded_label, ':')) {
        label_issuer = decoded_label[0..colon_idx];
        let acc = decoded_label[(colon_idx + 1)..];
        if (acc.len > 0 and acc[0] == ' ') {
            acc = acc[1..];
        }
        label_account = acc;
    }

    let secret_owned: []const u8 = "";
    let issuer_param: []const u8 = "";
    let alg: HashAlg = HashAlg.Sha1;
    let digits: u8 = 6;
    let period: u64 = 30;
    let counter: u64 = 0;

    let param_iter = std.mem.splitScalar(u8, query, '&');
    while (param := param_iter.next()) {
        if (param.len == 0) continue;
        const eq_idx = std.mem.indexOfScalar(u8, param, '=') orelse continue;
        const key = param[0..eq_idx];
        const raw_val = param[(eq_idx + 1)..];
        const val = urlDecode(alloc, raw_val) orelse {
            if (secret_owned.len > 0) alloc.free(secret_owned);
            if (issuer_param.len > 0) alloc.free(issuer_param);
            alloc.free(decoded_label);
            return null;
        };

        if (asciiEqlIgnoreCase(key, "secret")) {
            if (secret_owned.len > 0) alloc.free(secret_owned);
            secret_owned = val;
        } else if (asciiEqlIgnoreCase(key, "issuer")) {
            if (issuer_param.len > 0) alloc.free(issuer_param);
            issuer_param = val;
        } else if (asciiEqlIgnoreCase(key, "algorithm")) {
            if (parsed_alg := parseAlg(val)) {
                alg = parsed_alg;
            }
            alloc.free(val);
        } else if (asciiEqlIgnoreCase(key, "digits")) {
            const parsed_d = std.fmt.parseInt(u8, val, 10) catch 6;
            if (parsed_d >= 6 and parsed_d <= 8) {
                digits = parsed_d;
            }
            alloc.free(val);
        } else if (asciiEqlIgnoreCase(key, "period")) {
            const parsed_p = std.fmt.parseInt(u64, val, 10) catch 30;
            if (parsed_p > 0) {
                period = parsed_p;
            }
            alloc.free(val);
        } else if (asciiEqlIgnoreCase(key, "counter")) {
            counter = std.fmt.parseInt(u64, val, 10) catch 0;
            alloc.free(val);
        } else {
            alloc.free(val);
        }
    }

    if (secret_owned.len == 0) {
        if (issuer_param.len > 0) alloc.free(issuer_param);
        alloc.free(decoded_label);
        return null;
    }

    let final_issuer: []const u8 = "";
    if (issuer_param.len > 0) {
        final_issuer = issuer_param;
    } else if (label_issuer.len > 0) {
        final_issuer = alloc.dupe(u8, label_issuer) catch {
            alloc.free(secret_owned);
            alloc.free(decoded_label);
            return null;
        };
    }

    const final_account = alloc.dupe(u8, label_account) catch {
        if (final_issuer.len > 0) alloc.free(final_issuer);
        alloc.free(secret_owned);
        alloc.free(decoded_label);
        return null;
    };

    return OtpAuthUri{
        .otp_type = otp_type,
        .label = decoded_label,
        .issuer = final_issuer,
        .account = final_account,
        .secret = secret_owned,
        .algorithm = alg,
        .digits = digits,
        .period = period,
        .counter = counter,
    };
}
