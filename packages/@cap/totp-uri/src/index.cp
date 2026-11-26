// packages/@cap/totp-uri/src/index.cp
// Zero-allocation Google Authenticator Key URI Format (otpauth://) parser & builder.
// MIT clean-room pure CP implementation.

pub const OTP_TOTP: u8 = 1;
pub const OTP_HOTP: u8 = 2;

pub const ALG_SHA1: u8 = 1;
pub const ALG_SHA256: u8 = 2;
pub const ALG_SHA512: u8 = 3;

/// Parsed `otpauth://totp/...` or `otpauth://hotp/...` configuration.
pub struct OtpAuthUri {
    pub kind: u8,
    pub label: string,
    pub secret: string,
    pub issuer: string,
    pub algorithm: u8,
    pub digits: u8,
    pub period: u32,
    pub counter: u64,
    pub valid: bool,
}

fn otpToLower(c: u8): u8 {
    if (c >= ('A' as u8) && c <= ('Z' as u8)) {
        return c + (32 as u8);
    }
    return c;
}

fn otpEqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (otpToLower(a[i]) != otpToLower(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn otpIsUnreserved(c: u8): bool {
    if ((c >= ('a' as u8) && c <= ('z' as u8)) || (c >= ('A' as u8) && c <= ('Z' as u8)) || (c >= ('0' as u8) && c <= ('9' as u8))) {
        return true;
    }
    return c == ('-' as u8) || c == ('.' as u8) || c == ('_' as u8) || c == ('~' as u8);
}

fn otpHexUpper(n: u8): u8 {
    if (n < (10 as u8)) {
        return ('0' as u8) + n;
    }
    return ('A' as u8) + (n - (10 as u8));
}

fn otpParseU64(s: []const u8, out: *u64): bool {
    if (s.len == 0) {
        return false;
    }
    let acc: u64 = 0 as u64;
    const maxDiv10: u64 = 1844674407370955161 as u64;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < ('0' as u8) || c > ('9' as u8)) {
            return false;
        }
        const d = (c - ('0' as u8)) as u64;
        if (acc > maxDiv10 || (acc == maxDiv10 && d > (5 as u64))) {
            return false;
        }
        acc = acc * (10 as u64) + d;
        i += 1;
    }
    out.* = acc;
    return true;
}

fn otpWriteRaw(out: []u8, pos: *usize, src: []const u8): bool {
    if (pos.* + src.len > out.len) {
        return false;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos.*] = src[i];
        pos.* += 1;
        i += 1;
    }
    return true;
}

fn otpWritePctEncoded(out: []u8, pos: *usize, src: []const u8, allow_label_chars: bool): bool {
    let i: usize = 0;
    while (i < src.len) {
        const c = src[i];
        const keep = otpIsUnreserved(c) || (allow_label_chars && (c == (':' as u8) || c == ('@' as u8)));
        if (keep) {
            if (pos.* + 1 > out.len) {
                return false;
            }
            out[pos.*] = c;
            pos.* += 1;
        } else {
            if (pos.* + 3 > out.len) {
                return false;
            }
            out[pos.*] = '%' as u8;
            out[pos.* + 1] = otpHexUpper((c >> (4 as u3)) & (0x0F as u8));
            out[pos.* + 2] = otpHexUpper(c & (0x0F as u8));
            pos.* += 3;
        }
        i += 1;
    }
    return true;
}

fn otpWriteU64(out: []u8, pos: *usize, val: u64): bool {
    if (val == (0 as u64)) {
        return otpWriteRaw(out, pos, "0");
    }
    let rev: [24]u8 = undefined;
    let n: usize = 0;
    let cur = val;
    while (cur > (0 as u64)) {
        rev[n] = ('0' as u8) + ((cur % (10 as u64)) as u8);
        n += 1;
        cur = cur / (10 as u64);
    }
    if (pos.* + n > out.len) {
        return false;
    }
    let i: usize = 0;
    while (i < n) {
        out[pos.*] = rev[n - 1 - i];
        pos.* += 1;
        i += 1;
    }
    return true;
}

/// Validate an RFC 4648 Base32 secret (`A-Z`, `2-7`, case-insensitive, optional trailing `=`).
pub fn isValidBase32Secret(secret: string): bool {
    if (secret.len == 0) {
        return false;
    }
    let seenPad = false;
    let dataChars: usize = 0;
    let i: usize = 0;
    while (i < secret.len) {
        const c = secret[i];
        if (c == ('=' as u8)) {
            seenPad = true;
        } else {
            if (seenPad) {
                return false;
            }
            const isUpper = c >= ('A' as u8) && c <= ('Z' as u8);
            const isLower = c >= ('a' as u8) && c <= ('z' as u8);
            const isBase32Digit = c >= ('2' as u8) && c <= ('7' as u8);
            if (!isUpper && !isLower && !isBase32Digit) {
                return false;
            }
            dataChars += 1;
        }
        i += 1;
    }
    return dataChars > 0;
}

/// Strictly parse an `otpauth://totp/...` or `otpauth://hotp/...` URI.
/// Defaults: `algorithm = ALG_SHA1`, `digits = 6`, `period = 30`.
/// Requires valid Base32 `secret`; `hotp` additionally requires `counter`.
pub fn parseOtpAuthUri(uri: string): OtpAuthUri {
    let res = OtpAuthUri{
        kind: 0 as u8,
        label: "",
        secret: "",
        issuer: "",
        algorithm: ALG_SHA1,
        digits: 6 as u8,
        period: 30 as u32,
        counter: 0 as u64,
        valid: false,
    };

    // Scheme: "otpauth://" (10 bytes).
    if (uri.len < 16 || !otpEqIgnoreCase(uri[0..10], "otpauth://")) {
        return res;
    }

    // Extract OTP type before '/'.
    let slash: usize = 10;
    while (slash < uri.len && uri[slash] != ('/' as u8)) {
        slash += 1;
    }
    if (slash >= uri.len) {
        return res;
    }
    const kindStr = uri[10..slash];
    if (otpEqIgnoreCase(kindStr, "totp")) {
        res.kind = OTP_TOTP;
    } else if (otpEqIgnoreCase(kindStr, "hotp")) {
        res.kind = OTP_HOTP;
    } else {
        return res;
    }

    // Extract label up to '?'.
    const labelStart = slash + 1;
    let qmark = labelStart;
    while (qmark < uri.len && uri[qmark] != ('?' as u8)) {
        qmark += 1;
    }
    if (qmark == labelStart || qmark >= uri.len) {
        return res;
    }
    res.label = uri[labelStart..qmark];

    // Parse query parameters.
    let hasSecret = false;
    let hasCounter = false;
    let pos = qmark + 1;
    while (pos < uri.len) {
        let amp = pos;
        while (amp < uri.len && uri[amp] != ('&' as u8)) {
            amp += 1;
        }
        const pair = uri[pos..amp];
        if (amp < uri.len) {
            pos = amp + 1;
        } else {
            pos = uri.len;
        }

        if (pair.len == 0) {
            continue;
        }
        let eq: usize = 0;
        while (eq < pair.len && pair[eq] != ('=' as u8)) {
            eq += 1;
        }
        if (eq >= pair.len) {
            continue;
        }

        const key = pair[0..eq];
        const val = pair[(eq + 1)..pair.len];

        if (otpEqIgnoreCase(key, "secret")) {
            if (!isValidBase32Secret(val)) {
                return res;
            }
            res.secret = val;
            hasSecret = true;
        } else if (otpEqIgnoreCase(key, "issuer")) {
            res.issuer = val;
        } else if (otpEqIgnoreCase(key, "algorithm")) {
            if (otpEqIgnoreCase(val, "SHA1")) {
                res.algorithm = ALG_SHA1;
            } else if (otpEqIgnoreCase(val, "SHA256")) {
                res.algorithm = ALG_SHA256;
            } else if (otpEqIgnoreCase(val, "SHA512")) {
                res.algorithm = ALG_SHA512;
            } else {
                return res;
            }
        } else if (otpEqIgnoreCase(key, "digits")) {
            let dVal: u64 = 0 as u64;
            if (!otpParseU64(val, &dVal)) {
                return res;
            }
            if (dVal != (6 as u64) && dVal != (8 as u64)) {
                return res;
            }
            res.digits = dVal as u8;
        } else if (otpEqIgnoreCase(key, "period")) {
            let pVal: u64 = 0 as u64;
            if (!otpParseU64(val, &pVal) || pVal == (0 as u64) || pVal > (4294967295 as u64)) {
                return res;
            }
            res.period = pVal as u32;
        } else if (otpEqIgnoreCase(key, "counter")) {
            let cVal: u64 = 0 as u64;
            if (!otpParseU64(val, &cVal)) {
                return res;
            }
            res.counter = cVal;
            hasCounter = true;
        }
    }

    if (!hasSecret) {
        return res;
    }
    if (res.kind == OTP_HOTP && !hasCounter) {
        return res;
    }

    res.valid = true;
    return res;
}

/// Format an `OtpAuthUri` configuration into a standard `otpauth://` URI in `out`.
/// Percent-encodes `label` and `issuer`. Returns bytes written, or `0` on invalid config or overflow.
pub fn formatOtpAuthUri(cfg: OtpAuthUri, out: []u8): usize {
    if ((cfg.kind != OTP_TOTP && cfg.kind != OTP_HOTP) || cfg.label.len == 0 || !isValidBase32Secret(cfg.secret)) {
        return 0;
    }

    let pos: usize = 0;
    if (cfg.kind == OTP_TOTP) {
        if (!otpWriteRaw(out, &pos, "otpauth://totp/")) {
            return 0;
        }
    } else {
        if (!otpWriteRaw(out, &pos, "otpauth://hotp/")) {
            return 0;
        }
    }

    if (!otpWritePctEncoded(out, &pos, cfg.label, true)) {
        return 0;
    }
    if (!otpWriteRaw(out, &pos, "?secret=")) {
        return 0;
    }
    if (!otpWriteRaw(out, &pos, cfg.secret)) {
        return 0;
    }

    if (cfg.issuer.len > 0) {
        if (!otpWriteRaw(out, &pos, "&issuer=")) {
            return 0;
        }
        if (!otpWritePctEncoded(out, &pos, cfg.issuer, false)) {
            return 0;
        }
    }

    if (!otpWriteRaw(out, &pos, "&algorithm=")) {
        return 0;
    }
    if (cfg.algorithm == ALG_SHA256) {
        if (!otpWriteRaw(out, &pos, "SHA256")) {
            return 0;
        }
    } else if (cfg.algorithm == ALG_SHA512) {
        if (!otpWriteRaw(out, &pos, "SHA512")) {
            return 0;
        }
    } else {
        if (!otpWriteRaw(out, &pos, "SHA1")) {
            return 0;
        }
    }

    if (!otpWriteRaw(out, &pos, "&digits=")) {
        return 0;
    }
    if (cfg.digits == (8 as u8)) {
        if (!otpWriteRaw(out, &pos, "8")) {
            return 0;
        }
    } else {
        if (!otpWriteRaw(out, &pos, "6")) {
            return 0;
        }
    }

    if (cfg.kind == OTP_TOTP) {
        let per = cfg.period;
        if (per == (0 as u32)) {
            per = 30 as u32;
        }
        if (!otpWriteRaw(out, &pos, "&period=")) {
            return 0;
        }
        if (!otpWriteU64(out, &pos, per as u64)) {
            return 0;
        }
    } else {
        if (!otpWriteRaw(out, &pos, "&counter=")) {
            return 0;
        }
        if (!otpWriteU64(out, &pos, cfg.counter)) {
            return 0;
        }
    }

    return pos;
}

/// Zero-copy split components of an `otpauth` label (`Issuer:account`).
pub struct LabelParts {
    pub issuer: string,
    pub account: string,
}

/// Split an `otpauth` label on the first `:` into `LabelParts{ issuer, account }`.
/// When `:` is absent, `issuer` is `""` and `account` is `label`.
pub fn splitLabel(label: string): LabelParts {
    let i: usize = 0;
    while (i < label.len) {
        if (label[i] == (':' as u8)) {
            return LabelParts{
                issuer: label[0..i],
                account: label[(i + 1)..label.len],
            };
        }
        i += 1;
    }
    return LabelParts{
        issuer: "",
        account: label,
    };
}

export {
    OTP_TOTP,
    OTP_HOTP,
    ALG_SHA1,
    ALG_SHA256,
    ALG_SHA512,
    OtpAuthUri,
    LabelParts,
    isValidBase32Secret,
    parseOtpAuthUri,
    formatOtpAuthUri,
    splitLabel,
};
