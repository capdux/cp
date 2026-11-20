// Source: cp/lib/security/totp.cp (RFC 4226 HOTP, RFC 6238 TOTP & RFC 4648 Base32)
// lib/security/totp.cp
// RFC 4648 Base32 Codec, RFC 4226 HOTP & RFC 6238 TOTP Authentication Engine

import { constantTimeEql } from "./csrf.cp";
import { PercentEncoding } from "../url/percent.cp";
const std = @import("std");

const BASE32_ALPHABET: []const u8 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

pub enum OtpAlgorithm {
    Sha1,
    Sha256,
    Sha512,

    fn uriName(): string {
        switch (_) {
            .Sha1 => return "SHA1",
            .Sha256 => return "SHA256",
            .Sha512 => return "SHA512",
        }
    }
}

fn base32CharValue(c: u8): u8? {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a';
    if (c >= '2' && c <= '7') return (c - '2') + 26;
    return null;
}

fn isAsciiSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

pub struct Base32 {
    /// Encodes binary bytes to uppercase RFC 4648 Base32 with `=` padding.
    static fn encode(alloc: std.mem.Allocator, data: []const u8): ([]u8)! {
        return Base32.encodeImpl(alloc, data, true);
    }

    /// Encodes binary bytes to uppercase RFC 4648 Base32 without `=` padding.
    static fn encodeNoPad(alloc: std.mem.Allocator, data: []const u8): ([]u8)! {
        return Base32.encodeImpl(alloc, data, false);
    }

    static fn encodeImpl(alloc: std.mem.Allocator, data: []const u8, pad: bool): ([]u8)! {
        if (data.len == 0) {
            return alloc.alloc(u8, 0)?;
        }

        const full_groups = data.len / 5;
        const rem = data.len % 5;
        let out_len: usize = full_groups * 8;
        if (rem > 0) {
            if (pad) {
                out_len += 8;
            } else {
                const unpadded: usize = switch (rem) {
                    1 => 2,
                    2 => 4,
                    3 => 5,
                    4 => 7,
                    else => 0,
                };
                out_len += unpadded;
            }
        }

        let out = alloc.alloc(u8, out_len)?;
        let idx: usize = 0;
        let out_pos: usize = 0;
        let buffer: u32 = 0;
        let bits: u5 = 0;

        while (idx < data.len) {
            buffer = (buffer << 8) | u32(data[idx]);
            bits += 8;
            idx += 1;

            while (bits >= 5) {
                const shift: u5 = bits - 5;
                const val = (buffer >> shift) & 0x1F;
                out[out_pos] = BASE32_ALPHABET[usize(val)];
                out_pos += 1;
                bits -= 5;
            }
        }

        if (bits > 0) {
            const shift: u5 = 5 - bits;
            const val = (buffer << shift) & 0x1F;
            out[out_pos] = BASE32_ALPHABET[usize(val)];
            out_pos += 1;
        }

        if (pad) {
            while (out_pos < out_len) {
                out[out_pos] = '=';
                out_pos += 1;
            }
        }

        return out;
    }

    /// Decodes RFC 4648 Base32 (ignores ASCII whitespace, case-insensitive, optional `=` padding).
    static fn decode(alloc: std.mem.Allocator, encoded: []const u8): ([]u8)! {
        let data_chars: usize = 0;
        let pad_chars: usize = 0;
        let seen_pad = false;

        for (i of 0..encoded.len) {
            const c = encoded[i];
            if (isAsciiSpace(c)) continue;
            if (c == '=') {
                seen_pad = true;
                pad_chars += 1;
                continue;
            }
            if (seen_pad) {
                return err.InvalidBase32Padding;
            }
            if (base32CharValue(c) == null) {
                return err.InvalidBase32Character;
            }
            data_chars += 1;
        }

        if (data_chars == 0) {
            if (pad_chars > 0) return err.InvalidBase32Padding;
            return alloc.alloc(u8, 0)?;
        }

        const rem = data_chars % 8;
        if (rem == 1 || rem == 3 || rem == 6) {
            return err.InvalidBase32Length;
        }

        if (pad_chars > 0) {
            const expected_pad: usize = switch (rem) {
                0 => 0,
                2 => 6,
                4 => 4,
                5 => 3,
                7 => 1,
                else => 0,
            };
            if (pad_chars != expected_pad) {
                return err.InvalidBase32Padding;
            }
        }

        const out_len = (data_chars * 5) / 8;
        let out = alloc.alloc(u8, out_len)?;
        let out_pos: usize = 0;
        let buffer: u32 = 0;
        let bits: u5 = 0;

        for (i of 0..encoded.len) {
            const c = encoded[i];
            if (isAsciiSpace(c) || c == '=') continue;
            const val = base32CharValue(c).?;
            buffer = (buffer << 5) | u32(val);
            bits += 5;
            if (bits >= 8) {
                const shift: u5 = bits - 8;
                out[out_pos] = ((buffer >> shift) & 0xFF) as u8;
                out_pos += 1;
                bits -= 8;
            }
        }

        return out;
    }
}

fn pow10U32(digits: u8): u32! {
    return switch (digits) {
        1 => 10,
        2 => 100,
        3 => 1000,
        4 => 10000,
        5 => 100000,
        6 => 1000000,
        7 => 10000000,
        8 => 100000000,
        9 => 1000000000,
        else => return err.InvalidOtpDigits,
    };
}

fn dynamicTruncate(digest: []const u8): u32 {
    const offset = usize(digest[digest.len - 1] & 0x0F);
    const b0 = u32(digest[offset] & 0x7F) << 24;
    const b1 = u32(digest[offset + 1]) << 16;
    const b2 = u32(digest[offset + 2]) << 8;
    const b3 = u32(digest[offset + 3]);
    return b0 | b1 | b2 | b3;
}

pub struct Hotp {
    /// Generates an RFC 4226 / RFC 6238 HOTP integer value for `counter` and `digits` (1..=9).
    static fn generate(secret: []const u8, counter: u64, digits: u8, alg: OtpAlgorithm): u32! {
        const mod_val = pow10U32(digits)?;

        let msg: [8]u8 = undefined;
        std.mem.writeInt(u64, &msg, counter, .big);

        let bin_code: u32 = 0;
        switch (alg) {
            .Sha1 => {
                let mac: [20]u8 = undefined;
                std.crypto.auth.hmac.HmacSha1.create(&mac, &msg, secret);
                bin_code = dynamicTruncate(&mac);
            },
            .Sha256 => {
                let mac: [32]u8 = undefined;
                std.crypto.auth.hmac.sha2.HmacSha256.create(&mac, &msg, secret);
                bin_code = dynamicTruncate(&mac);
            },
            .Sha512 => {
                let mac: [64]u8 = undefined;
                std.crypto.auth.hmac.sha2.HmacSha512.create(&mac, &msg, secret);
                bin_code = dynamicTruncate(&mac);
            },
        }

        return bin_code % mod_val;
    }

    /// Formats a numeric OTP code into zero-padded ASCII digits inside `out`.
    static fn formatCode(code: u32, digits: u8, out: []u8): ([]const u8)! {
        const mod_val = pow10U32(digits)?;
        const d_usize = usize(digits);
        if (out.len < d_usize) return err.BufferTooSmall;

        let val = code % mod_val;
        let idx = d_usize;
        while (idx > 0) {
            idx -= 1;
            const digit: u8 = (val % 10) as u8;
            out[idx] = ('0' as u8) + digit;
            val /= 10;
        }
        return out[0..d_usize];
    }

    /// Generates and formats a zero-padded HOTP ASCII string directly into `out`.
    static fn generateCode(secret: []const u8, counter: u64, digits: u8, alg: OtpAlgorithm, out: []u8): ([]const u8)! {
        const code = Hotp.generate(secret, counter, digits, alg)?;
        return Hotp.formatCode(code, digits, out);
    }

    /// Verifies an HOTP ASCII code against `counter` using constant-time byte comparison.
    static fn verify(secret: []const u8, code: []const u8, counter: u64, digits: u8, alg: OtpAlgorithm): bool {
        if (digits < 1 || digits > 9) return false;
        if (code.len != usize(digits)) return false;

        let expected_buf: [10]u8 = undefined;
        const expected = Hotp.generateCode(secret, counter, digits, alg, &expected_buf) catch return false;
        return constantTimeEql(expected, code);
    }
}

pub struct TotpConfig {
    period_sec: u64 = 30;
    digits: u8 = 6;
    alg: OtpAlgorithm = .Sha1;

    static fn initDefault(): TotpConfig {
        return TotpConfig{
            period_sec: 30,
            digits: 6,
            alg: .Sha1,
        };
    }
}

fn getWallTimeSec(): u64 {
    let ts: std.os.linux.timespec = undefined;
    * = std.os.linux.clock_gettime(std.os.linux.CLOCK.REALTIME, &ts);
    if (ts.sec <= 0) return 0;
    return ts.sec as u64;
}

fn isValidBase32Token(s: []const u8): bool {
    if (s.len == 0) return false;
    for (i of 0..s.len) {
        const c = s[i];
        if (c == '=') continue;
        if (base32CharValue(c) == null) return false;
    }
    return true;
}

fn formatDecimalU64(val_in: u64, buf: []u8): []const u8 {
    if (val_in == 0) {
        buf[0] = '0' as u8;
        return buf[0..1];
    }
    let tmp: [24]u8 = undefined;
    let n: usize = 0;
    let v = val_in;
    while (v > 0) {
        tmp[n] = ('0' as u8) + ((v % 10) as u8);
        n += 1;
        v /= 10;
    }
    let pos: usize = 0;
    while (pos < n && pos < buf.len) {
        buf[pos] = tmp[n - 1 - pos];
        pos += 1;
    }
    return buf[0..pos];
}

fn hasControlChars(s: []const u8): bool {
    for (i of 0..s.len) {
        const c = s[i];
        if (c == '\r' || c == '\n' || c == 0) return true;
    }
    return false;
}

pub struct Totp {
    static fn timeStep(unix_sec: u64, period_sec: u64): u64! {
        if (period_sec == 0) return err.InvalidTotpPeriod;
        return unix_sec / period_sec;
    }

    /// Generates an RFC 6238 TOTP numeric code at `unix_sec`.
    static fn generateAt(secret: []const u8, unix_sec: u64, cfg: TotpConfig): u32! {
        const step = Totp.timeStep(unix_sec, cfg.period_sec)?;
        return Hotp.generate(secret, step, cfg.digits, cfg.alg);
    }

    /// Generates and formats a zero-padded TOTP ASCII code at `unix_sec` into `out`.
    static fn generateCodeAt(secret: []const u8, unix_sec: u64, cfg: TotpConfig, out: []u8): ([]const u8)! {
        const code = Totp.generateAt(secret, unix_sec, cfg)?;
        return Hotp.formatCode(code, cfg.digits, out);
    }

    /// Generates a TOTP numeric code at the current system wall clock time.
    static fn generate(secret: []const u8, cfg: TotpConfig): u32! {
        return Totp.generateAt(secret, getWallTimeSec(), cfg);
    }

    /// Verifies a TOTP ASCII code at `unix_sec` within +/- `skew_steps` using constant-time comparison.
    static fn verifyAt(secret: []const u8, code: []const u8, unix_sec: u64, skew_steps: u32, cfg: TotpConfig): bool {
        if (cfg.period_sec == 0) return false;
        if (cfg.digits < 1 || cfg.digits > 9) return false;
        if (code.len != usize(cfg.digits)) return false;

        const base_step = unix_sec / cfg.period_sec;
        const skew = u64(skew_steps);
        const start_step = if (base_step > skew) base_step - skew else 0;
        const max_u64 = std.math.maxInt(u64);
        const end_step = if (max_u64 - base_step < skew) max_u64 else base_step + skew;

        let matched: u8 = 0;
        let step = start_step;
        while (true) {
            let expected_buf: [10]u8 = undefined;
            const expected = Hotp.generateCode(secret, step, cfg.digits, cfg.alg, &expected_buf) catch return false;
            if (constantTimeEql(expected, code)) {
                matched |= 1;
            }
            if (step == end_step) break;
            step += 1;
        }

        return matched != 0;
    }

    /// Verifies a TOTP ASCII code against the current system wall clock time.
    static fn verify(secret: []const u8, code: []const u8, skew_steps: u32, cfg: TotpConfig): bool {
        return Totp.verifyAt(secret, code, getWallTimeSec(), skew_steps, cfg);
    }

    /// Builds a standard `otpauth://totp/...` provisioning URI.
    static fn buildOtpAuthUri(
        alloc: std.mem.Allocator,
        issuer: []const u8,
        account: []const u8,
        secret_b32: []const u8,
        cfg: TotpConfig,
    ): ([]u8)! {
        if (account.len == 0) return err.InvalidAccount;
        if (cfg.period_sec == 0) return err.InvalidTotpPeriod;
        if (cfg.digits < 1 || cfg.digits > 9) return err.InvalidOtpDigits;
        if (hasControlChars(issuer) || hasControlChars(account)) return err.InvalidOtpAuthInput;
        if (!isValidBase32Token(secret_b32)) return err.InvalidBase32Secret;

        const enc_account = PercentEncoding.encodeComponent(account, alloc)?;
        defer alloc.free(enc_account);

        let enc_issuer: []const u8 = "";
        let owns_issuer = false;
        if (issuer.len > 0) {
            enc_issuer = PercentEncoding.encodeComponent(issuer, alloc)?;
            owns_issuer = true;
        }
        defer {
            if (owns_issuer) alloc.free(enc_issuer);
        }

        let digits_buf: [4]u8 = undefined;
        const digits_str = formatDecimalU64(u64(cfg.digits), &digits_buf);

        let period_buf: [32]u8 = undefined;
        const period_str = formatDecimalU64(cfg.period_sec, &period_buf);

        const alg_str = cfg.alg.uriName();
        const prefix = "otpauth://totp/";
        const sep_colon: usize = if (owns_issuer) 1 else 0;
        const issuer_label_len: usize = if (owns_issuer) enc_issuer.len else 0;
        const issuer_param_len: usize = if (owns_issuer) ("&issuer=".len + enc_issuer.len) else 0;

        const total_len = prefix.len
            + issuer_label_len
            + sep_colon
            + enc_account.len
            + "?secret=".len
            + secret_b32.len
            + issuer_param_len
            + "&algorithm=".len
            + alg_str.len
            + "&digits=".len
            + digits_str.len
            + "&period=".len
            + period_str.len;

        let out = alloc.alloc(u8, total_len)?;
        let pos: usize = 0;

        @memcpy(out[pos .. pos + prefix.len], prefix);
        pos += prefix.len;

        if (owns_issuer) {
            @memcpy(out[pos .. pos + enc_issuer.len], enc_issuer);
            pos += enc_issuer.len;
            out[pos] = ':';
            pos += 1;
        }

        @memcpy(out[pos .. pos + enc_account.len], enc_account);
        pos += enc_account.len;

        const q_secret = "?secret=";
        @memcpy(out[pos .. pos + q_secret.len], q_secret);
        pos += q_secret.len;

        @memcpy(out[pos .. pos + secret_b32.len], secret_b32);
        pos += secret_b32.len;

        if (owns_issuer) {
            const q_issuer = "&issuer=";
            @memcpy(out[pos .. pos + q_issuer.len], q_issuer);
            pos += q_issuer.len;
            @memcpy(out[pos .. pos + enc_issuer.len], enc_issuer);
            pos += enc_issuer.len;
        }

        const q_alg = "&algorithm=";
        @memcpy(out[pos .. pos + q_alg.len], q_alg);
        pos += q_alg.len;
        @memcpy(out[pos .. pos + alg_str.len], alg_str);
        pos += alg_str.len;

        const q_digits = "&digits=";
        @memcpy(out[pos .. pos + q_digits.len], q_digits);
        pos += q_digits.len;
        @memcpy(out[pos .. pos + digits_str.len], digits_str);
        pos += digits_str.len;

        const q_period = "&period=";
        @memcpy(out[pos .. pos + q_period.len], q_period);
        pos += q_period.len;
        @memcpy(out[pos .. pos + period_str.len], period_str);
        pos += period_str.len;

        return out;
    }
}
