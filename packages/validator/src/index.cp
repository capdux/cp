// @cap/validator — Zero-allocation schema & format validation engine for CP
// Clean-room MIT implementation inspired by npm `zod`, `validator`, `ajv` and Rust `validator`.

import { isDigit, isAlpha, isAlnum, isHexDigit, toLowerByte, eqIgnoreCase, strEq, isAscii, startsWith, endsWith } from "cap:ascii";
import { indexOfBytes } from "cap:bin";

export { isAscii, startsWith, endsWith };

const MAX_VALIDATION_ERRORS: usize = 32;

pub struct ValidationError {
    pub field: string,
    pub code: string,
    pub message: string,
}

// ---------------------------------------------------------------------------
// Low-level ASCII / character helpers
// ---------------------------------------------------------------------------

fn parseUintDec(s: string, start: usize, end: usize) -> i64 {
    if (start >= end || end > s.len) {
        return -1;
    }
    let val: i64 = 0;
    let i = start;
    while (i < end) {
        const c = s[i];
        if (!isDigit(c)) {
            return -1;
        }
        val = val * 10 + ((c - '0') as i64);
        if (val > 1000000000) {
            return -1;
        }
        i += 1;
    }
    return val;
}

// ---------------------------------------------------------------------------
// String & basic predicates
// ---------------------------------------------------------------------------

pub fn isAlphanumeric(s: string) -> bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        if (!isAlnum(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn isNumeric(s: string) -> bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    if (s[0] == '+' || s[0] == '-') {
        if (s.len == 1) {
            return false;
        }
        i = 1;
    }
    let saw_dot = false;
    let saw_digit = false;
    while (i < s.len) {
        const c = s[i];
        if (isDigit(c)) {
            saw_digit = true;
        } else if (c == '.' && !saw_dot) {
            saw_dot = true;
        } else {
            return false;
        }
        i += 1;
    }
    return saw_digit;
}

pub fn isHex(s: string) -> bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    if (s.len >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        if (s.len == 2) {
            return false;
        }
        i = 2;
    }
    while (i < s.len) {
        if (!isHexDigit(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn isBase64(s: string) -> bool {
    if (s.len == 0 || (s.len % 4) != 0) {
        return false;
    }
    let pad_count: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '=') {
            pad_count += 1;
            if (pad_count > 2 || i < s.len - 2) {
                return false;
            }
        } else {
            if (pad_count > 0) {
                return false;
            }
            const ok = isAlnum(c) || c == '+' || c == '/' || c == '-' || c == '_';
            if (!ok) {
                return false;
            }
        }
        i += 1;
    }
    return true;
}

pub fn isSlug(s: string) -> bool {
    if (s.len == 0) {
        return false;
    }
    if (s[0] == '-' || s[s.len - 1] == '-') {
        return false;
    }
    let prev_dash = false;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '-') {
            if (prev_dash) {
                return false;
            }
            prev_dash = true;
        } else if ((c >= 'a' && c <= 'z') || isDigit(c)) {
            prev_dash = false;
        } else {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn hasMinLen(s: string, min_len: usize) -> bool {
    return s.len >= min_len;
}

pub fn hasMaxLen(s: string, max_len: usize) -> bool {
    return s.len <= max_len;
}

pub fn hasLenBetween(s: string, min_len: usize, max_len: usize) -> bool {
    return s.len >= min_len && s.len <= max_len;
}

pub fn containsStr(s: string, needle: string) -> bool {
    return indexOfBytes(s, needle, 0) != null;
}

pub fn isOneOf(s: string, allowed: []string) -> bool {
    let i: usize = 0;
    while (i < allowed.len) {
        if (strEq(s, allowed[i])) {
            return true;
        }
        i += 1;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Network & Identity Validators (Email, URL, UUID, ULID, IPv4, IPv6, CIDR)
// ---------------------------------------------------------------------------

fn isValidDomainLabel(s: string, start: usize, end: usize) -> bool {
    if (end <= start) {
        return false;
    }
    const len = end - start;
    if (len > 63) {
        return false;
    }
    if (s[start] == '-' || s[end - 1] == '-') {
        return false;
    }
    let i = start;
    while (i < end) {
        const c = s[i];
        if (!isAlnum(c) && c != '-') {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isValidDomain(s: string, start: usize, end: usize) -> bool {
    if (end <= start) {
        return false;
    }
    const total = end - start;
    if (total > 253) {
        return false;
    }
    let label_start = start;
    let label_count: usize = 0;
    let last_label_start = start;
    let last_label_end = start;
    let i = start;
    while (i <= end) {
        if (i == end || s[i] == '.') {
            if (!isValidDomainLabel(s, label_start, i)) {
                return false;
            }
            label_count += 1;
            last_label_start = label_start;
            last_label_end = i;
            label_start = i + 1;
        }
        i += 1;
    }
    if (label_count < 2) {
        return false;
    }
    // TLD must be at least 2 chars and not all digits
    const tld_len = last_label_end - last_label_start;
    if (tld_len < 2) {
        return false;
    }
    let all_digits = true;
    let k = last_label_start;
    while (k < last_label_end) {
        if (!isDigit(s[k])) {
            all_digits = false;
            break;
        }
        k += 1;
    }
    return !all_digits;
}

pub fn isEmail(s: string) -> bool {
    if (s.len < 5 || s.len > 254) {
        return false;
    }
    let at_idx: usize = s.len;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '@') {
            if (at_idx != s.len) {
                return false; // multiple '@'
            }
            at_idx = i;
        }
        i += 1;
    }
    if (at_idx == 0 || at_idx == s.len || at_idx > 64 || at_idx + 3 >= s.len) {
        return false;
    }
    // Local part check
    if (s[0] == '.' || s[at_idx - 1] == '.') {
        return false;
    }
    let prev_dot = false;
    i = 0;
    while (i < at_idx) {
        const c = s[i];
        if (c == '.') {
            if (prev_dot) {
                return false;
            }
            prev_dot = true;
        } else {
            prev_dot = false;
            const ok = isAlnum(c) || c == '_' || c == '-' || c == '+' || c == '%' || c == '!' || c == '#';
            if (!ok) {
                return false;
            }
        }
        i += 1;
    }
    return isValidDomain(s, at_idx + 1, s.len);
}

pub fn isIpv4Range(s: string, start: usize, end: usize) -> bool {
    if (end <= start) {
        return false;
    }
    let octets: usize = 0;
    let seg_start = start;
    let i = start;
    while (i <= end) {
        if (i == end || s[i] == '.') {
            const seg_len = i - seg_start;
            if (seg_len == 0 || seg_len > 3) {
                return false;
            }
            if (seg_len > 1 && s[seg_start] == '0') {
                return false; // reject leading zeros
            }
            const val = parseUintDec(s, seg_start, i);
            if (val < 0 || val > 255) {
                return false;
            }
            octets += 1;
            seg_start = i + 1;
        }
        i += 1;
    }
    return octets == 4;
}

pub fn isIpv4(s: string) -> bool {
    return isIpv4Range(s, 0, s.len);
}

pub fn isIpv6Range(s: string, start: usize, end: usize) -> bool {
    if (end <= start) {
        return false;
    }
    const len = end - start;
    if (len < 2 || len > 45) {
        return false;
    }
    let groups: usize = 0;
    let has_double_colon = false;
    let i = start;

    if (len >= 2 && s[start] == ':' && s[start + 1] == ':') {
        has_double_colon = true;
        i = start + 2;
        if (i == end) {
            return true; // "::"
        }
    } else if (s[start] == ':') {
        return false;
    }

    while (i < end) {
        const g_start = i;
        while (i < end && s[i] != ':') {
            if (!isHexDigit(s[i])) {
                return false;
            }
            i += 1;
        }
        const g_len = i - g_start;
        if (g_len == 0 || g_len > 4) {
            return false;
        }
        groups += 1;
        if (groups > 8) {
            return false;
        }
        if (i == end) {
            break;
        }
        // s[i] == ':'
        i += 1;
        if (i == end) {
            return false; // trailing single ':'
        }
        if (s[i] == ':') {
            if (has_double_colon) {
                return false; // only one '::' allowed
            }
            has_double_colon = true;
            i += 1;
            if (i == end) {
                break; // trailing '::'
            }
        }
    }

    if (has_double_colon) {
        return groups < 8;
    }
    return groups == 8;
}

pub fn isIpv6(s: string) -> bool {
    return isIpv6Range(s, 0, s.len);
}

pub fn isCidr(s: string) -> bool {
    let slash: usize = s.len;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '/') {
            if (slash != s.len) {
                return false;
            }
            slash = i;
        }
        i += 1;
    }
    if (slash == 0 || slash >= s.len - 1) {
        return false;
    }
    const prefix = parseUintDec(s, slash + 1, s.len);
    if (prefix < 0) {
        return false;
    }
    if (isIpv4Range(s, 0, slash)) {
        return prefix <= 32;
    }
    if (isIpv6Range(s, 0, slash)) {
        return prefix <= 128;
    }
    return false;
}

pub fn isUrl(s: string) -> bool {
    if (s.len < 10 || s.len > 2048) {
        return false;
    }
    // Check scheme: http://, https://, ws://, wss://
    let host_start: usize = 0;
    if (s.len > 8 && eqIgnoreCase(s[0..8], "https://")) {
        host_start = 8;
    } else if (s.len > 7 && eqIgnoreCase(s[0..7], "http://")) {
        host_start = 7;
    } else if (s.len > 6 && eqIgnoreCase(s[0..6], "wss://")) {
        host_start = 6;
    } else if (s.len > 5 && eqIgnoreCase(s[0..5], "ws://")) {
        host_start = 5;
    } else {
        return false;
    }

    // Reject control chars or whitespace anywhere in URL
    let i: usize = host_start;
    while (i < s.len) {
        if (s[i] <= 32 || s[i] == 127) {
            return false;
        }
        i += 1;
    }

    // Find end of authority (first '/', '?', '#')
    let auth_end = host_start;
    while (auth_end < s.len && s[auth_end] != '/' && s[auth_end] != '?' && s[auth_end] != '#') {
        auth_end += 1;
    }
    if (auth_end <= host_start) {
        return false;
    }

    // Bracketed IPv6 host: [::1]:8080
    if (s[host_start] == '[') {
        let close_b = host_start + 1;
        while (close_b < auth_end && s[close_b] != ']') {
            close_b += 1;
        }
        if (close_b >= auth_end || !isIpv6Range(s, host_start + 1, close_b)) {
            return false;
        }
        if (close_b + 1 < auth_end) {
            if (s[close_b + 1] != ':') {
                return false;
            }
            const p = parseUintDec(s, close_b + 2, auth_end);
            return p >= 1 && p <= 65535;
        }
        return true;
    }

    // Split optional :port
    let host_end = auth_end;
    let colon = host_start;
    while (colon < auth_end) {
        if (s[colon] == ':') {
            host_end = colon;
            const p = parseUintDec(s, colon + 1, auth_end);
            if (p < 1 || p > 65535) {
                return false;
            }
            break;
        }
        colon += 1;
    }

    if (host_end <= host_start) {
        return false;
    }
    if (eqIgnoreCase(s[host_start..host_end], "localhost")) {
        return true;
    }
    if (isIpv4Range(s, host_start, host_end)) {
        return true;
    }
    return isValidDomain(s, host_start, host_end);
}

pub fn isUuid(s: string) -> bool {
    if (s.len != 36) {
        return false;
    }
    let i: usize = 0;
    while (i < 36) {
        const c = s[i];
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (c != '-') {
                return false;
            }
        } else {
            if (!isHexDigit(c)) {
                return false;
            }
        }
        i += 1;
    }
    return true;
}

fn isCrockfordBase32(c: u8) -> bool {
    const lc = toLowerByte(c);
    if (isDigit(lc)) {
        return true;
    }
    if (lc >= 'a' && lc <= 'z') {
        if (lc == 'i' || lc == 'l' || lc == 'o' || lc == 'u') {
            return false;
        }
        return true;
    }
    return false;
}

pub fn isUlid(s: string) -> bool {
    if (s.len != 26) {
        return false;
    }
    // First char must be '0'..'7' so 48-bit timestamp never overflows
    if (s[0] < '0' || s[0] > '7') {
        return false;
    }
    let i: usize = 0;
    while (i < 26) {
        if (!isCrockfordBase32(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

// ---------------------------------------------------------------------------
// SemVer, ISO-8601 Date/DateTime, Luhn Credit Card, E.164 Phone
// ---------------------------------------------------------------------------

pub fn isSemver(s: string) -> bool {
    if (s.len < 5) {
        return false;
    }
    // Strip optional +build and -prerelease to validate core MAJOR.MINOR.PATCH
    let core_end = s.len;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '-' || s[i] == '+') {
            core_end = i;
            break;
        }
        i += 1;
    }
    let seg_start: usize = 0;
    let parts: usize = 0;
    let j: usize = 0;
    while (j <= core_end) {
        if (j == core_end || s[j] == '.') {
            const seg_len = j - seg_start;
            if (seg_len == 0) {
                return false;
            }
            if (seg_len > 1 && s[seg_start] == '0') {
                return false; // no leading zeros in SemVer 2.0
            }
            if (parseUintDec(s, seg_start, j) < 0) {
                return false;
            }
            parts += 1;
            seg_start = j + 1;
        }
        j += 1;
    }
    if (parts != 3) {
        return false;
    }
    if (core_end < s.len) {
        if (core_end + 1 >= s.len) {
            return false; // empty prerelease or build metadata
        }
        let k = core_end + 1;
        while (k < s.len) {
            const c = s[k];
            if (!isAlnum(c) && c != '.' && c != '-' && c != '+') {
                return false;
            }
            k += 1;
        }
    }
    return true;
}

fn isLeapYear(y: i64) -> bool {
    return (@rem(y, 4) == 0 && @rem(y, 100) != 0) || @rem(y, 400) == 0;
}

fn daysInMonth(y: i64, m: i64) -> i64 {
    if (m == 2) {
        if (isLeapYear(y)) {
            return 29;
        }
        return 28;
    }
    if (m == 4 || m == 6 || m == 9 || m == 11) {
        return 30;
    }
    return 31;
}

pub fn isIsoDate(s: string) -> bool {
    if (s.len != 10) {
        return false;
    }
    if (s[4] != '-' || s[7] != '-') {
        return false;
    }
    const y = parseUintDec(s, 0, 4);
    const m = parseUintDec(s, 5, 7);
    const d = parseUintDec(s, 8, 10);
    if (y < 1 || m < 1 || m > 12 || d < 1) {
        return false;
    }
    return d <= daysInMonth(y, m);
}

pub fn isIsoDateTime(s: string) -> bool {
    if (s.len < 20) {
        return false;
    }
    if (!isIsoDate(s[0..10])) {
        return false;
    }
    if (s[10] != 'T' && s[10] != 't' && s[10] != ' ') {
        return false;
    }
    if (s[13] != ':' || s[16] != ':') {
        return false;
    }
    const hh = parseUintDec(s, 11, 13);
    const mm = parseUintDec(s, 14, 16);
    const ss = parseUintDec(s, 17, 19);
    if (hh < 0 || hh > 23 || mm < 0 || mm > 59 || ss < 0 || ss > 60) {
        return false;
    }
    let idx: usize = 19;
    if (idx < s.len && s[idx] == '.') {
        idx += 1;
        const frac_start = idx;
        while (idx < s.len && isDigit(s[idx])) {
            idx += 1;
        }
        if (idx == frac_start || idx - frac_start > 9) {
            return false;
        }
    }
    if (idx >= s.len) {
        return false;
    }
    if (s[idx] == 'Z' || s[idx] == 'z') {
        return idx + 1 == s.len;
    }
    if (s[idx] == '+' || s[idx] == '-') {
        if (idx + 6 != s.len || s[idx + 3] != ':') {
            return false;
        }
        const tzh = parseUintDec(s, idx + 1, idx + 3);
        const tzm = parseUintDec(s, idx + 4, idx + 6);
        return tzh >= 0 && tzh <= 23 && tzm >= 0 && tzm <= 59;
    }
    return false;
}

pub fn isLuhn(s: string) -> bool {
    let sum: i64 = 0;
    let digits: usize = 0;
    let double_next = false;
    let i = s.len;
    while (i > 0) {
        i -= 1;
        const c = s[i];
        if (c == ' ' || c == '-') {
            continue;
        }
        if (!isDigit(c)) {
            return false;
        }
        let d: i64 = (c - '0') as i64;
        if (double_next) {
            d = d * 2;
            if (d > 9) {
                d -= 9;
            }
        }
        sum += d;
        double_next = !double_next;
        digits += 1;
    }
    return digits >= 12 && digits <= 19 && @rem(sum, 10) == 0;
}

pub fn isE164Phone(s: string) -> bool {
    if (s.len < 8 || s.len > 16) {
        return false;
    }
    if (s[0] != '+' || s[1] < '1' || s[1] > '9') {
        return false;
    }
    let i: usize = 1;
    while (i < s.len) {
        if (!isDigit(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Numeric Validators
// ---------------------------------------------------------------------------

pub fn isPort(p: i64) -> bool {
    return p >= 1 && p <= 65535;
}

pub fn inRangeI64(v: i64, min_v: i64, max_v: i64) -> bool {
    return v >= min_v && v <= max_v;
}

pub fn inRangeF64(v: f64, min_v: f64, max_v: f64) -> bool {
    return v >= min_v && v <= max_v;
}

pub fn isMultipleOf(v: i64, step: i64) -> bool {
    if (step == 0) {
        return false;
    }
    return @rem(v, step) == 0;
}

// ---------------------------------------------------------------------------
// Structured Multi-Error Validator Collector (Zod / Rust validator style)
// ---------------------------------------------------------------------------

pub struct Validator {
    pub errors: [MAX_VALIDATION_ERRORS]ValidationError,
    pub count: usize,

    pub static fn init() -> Validator {
        let errs: [MAX_VALIDATION_ERRORS]ValidationError = undefined;
        return Validator{
            errors: errs,
            count: 0,
        };
    }

    pub fn addError(*_, field: string, code: string, message: string) {
        if (_.count < MAX_VALIDATION_ERRORS) {
            _.errors[_.count] = ValidationError{
                field: field,
                code: code,
                message: message,
            };
            _.count += 1;
        }
    }

    pub fn isValid() -> bool {
        return _.count == 0;
    }

    pub fn errorCount() -> usize {
        return _.count;
    }

    pub fn errorAt(idx: usize) -> ValidationError {
        return _.errors[idx];
    }

    pub fn required(*_, field: string, value: string) -> bool {
        if (value.len == 0) {
            _.addError(field, "required", "field must not be empty");
            return false;
        }
        return true;
    }

    pub fn minLen(*_, field: string, value: string, n: usize) -> bool {
        if (value.len < n) {
            _.addError(field, "min_length", "value is shorter than minimum length");
            return false;
        }
        return true;
    }

    pub fn maxLen(*_, field: string, value: string, n: usize) -> bool {
        if (value.len > n) {
            _.addError(field, "max_length", "value exceeds maximum length");
            return false;
        }
        return true;
    }

    pub fn email(*_, field: string, value: string) -> bool {
        if (!isEmail(value)) {
            _.addError(field, "invalid_email", "value must be a valid email address");
            return false;
        }
        return true;
    }

    pub fn url(*_, field: string, value: string) -> bool {
        if (!isUrl(value)) {
            _.addError(field, "invalid_url", "value must be a valid http/https/ws/wss URL");
            return false;
        }
        return true;
    }

    pub fn uuid(*_, field: string, value: string) -> bool {
        if (!isUuid(value)) {
            _.addError(field, "invalid_uuid", "value must be a valid UUID");
            return false;
        }
        return true;
    }

    pub fn ulid(*_, field: string, value: string) -> bool {
        if (!isUlid(value)) {
            _.addError(field, "invalid_ulid", "value must be a valid ULID");
            return false;
        }
        return true;
    }

    pub fn ipv4(*_, field: string, value: string) -> bool {
        if (!isIpv4(value)) {
            _.addError(field, "invalid_ipv4", "value must be a valid IPv4 address");
            return false;
        }
        return true;
    }

    pub fn ipv6(*_, field: string, value: string) -> bool {
        if (!isIpv6(value)) {
            _.addError(field, "invalid_ipv6", "value must be a valid IPv6 address");
            return false;
        }
        return true;
    }

    pub fn cidr(*_, field: string, value: string) -> bool {
        if (!isCidr(value)) {
            _.addError(field, "invalid_cidr", "value must be a valid IPv4/IPv6 CIDR block");
            return false;
        }
        return true;
    }

    pub fn semver(*_, field: string, value: string) -> bool {
        if (!isSemver(value)) {
            _.addError(field, "invalid_semver", "value must be a valid SemVer 2.0 string");
            return false;
        }
        return true;
    }

    pub fn isoDate(*_, field: string, value: string) -> bool {
        if (!isIsoDate(value)) {
            _.addError(field, "invalid_date", "value must be a valid YYYY-MM-DD date");
            return false;
        }
        return true;
    }

    pub fn isoDateTime(*_, field: string, value: string) -> bool {
        if (!isIsoDateTime(value)) {
            _.addError(field, "invalid_datetime", "value must be a valid ISO-8601 timestamp");
            return false;
        }
        return true;
    }

    pub fn luhn(*_, field: string, value: string) -> bool {
        if (!isLuhn(value)) {
            _.addError(field, "invalid_luhn", "card number failed Luhn checksum");
            return false;
        }
        return true;
    }

    pub fn phone(*_, field: string, value: string) -> bool {
        if (!isE164Phone(value)) {
            _.addError(field, "invalid_phone", "value must be a valid E.164 phone number");
            return false;
        }
        return true;
    }

    pub fn slug(*_, field: string, value: string) -> bool {
        if (!isSlug(value)) {
            _.addError(field, "invalid_slug", "value must be a lowercase kebab-case slug");
            return false;
        }
        return true;
    }

    pub fn oneOf(*_, field: string, value: string, allowed: []string) -> bool {
        if (!isOneOf(value, allowed)) {
            _.addError(field, "invalid_enum", "value is not in allowed set");
            return false;
        }
        return true;
    }

    pub fn rangeI64(*_, field: string, value: i64, min_v: i64, max_v: i64) -> bool {
        if (!inRangeI64(value, min_v, max_v)) {
            _.addError(field, "out_of_range", "integer value out of allowed range");
            return false;
        }
        return true;
    }

    pub fn rangeF64(*_, field: string, value: f64, min_v: f64, max_v: f64) -> bool {
        if (!inRangeF64(value, min_v, max_v)) {
            _.addError(field, "out_of_range", "float value out of allowed range");
            return false;
        }
        return true;
    }

    pub fn port(*_, field: string, value: i64) -> bool {
        if (!isPort(value)) {
            _.addError(field, "invalid_port", "port must be in range 1..65535");
            return false;
        }
        return true;
    }
}

export {
    ValidationError,
    Validator,
    isAscii,
    isAlphanumeric,
    isNumeric,
    isHex,
    isBase64,
    isSlug,
    hasMinLen,
    hasMaxLen,
    hasLenBetween,
    startsWith,
    endsWith,
    containsStr,
    isOneOf,
    isEmail,
    isIpv4,
    isIpv6,
    isCidr,
    isUrl,
    isUuid,
    isUlid,
    isSemver,
    isIsoDate,
    isIsoDateTime,
    isLuhn,
    isE164Phone,
    isPort,
    inRangeI64,
    inRangeF64,
    isMultipleOf,
};
