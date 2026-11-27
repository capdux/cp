// @cap/ajv-formats — Zero-allocation JSON Schema format validators
// Clean-room MIT implementation modeled on npm `ajv-formats`.
// Accelerated with comptime 256-entry bitmask LUT and 16-lane @Vector(16, u8) SIMD scanning.

type AjvfVec16 = @Vector(16, u8);

const AJVF_DIGIT: u8 = 1;
const AJVF_ALPHA: u8 = 2;
const AJVF_HEX: u8 = 4;
const AJVF_EMAIL_LOCAL: u8 = 8;
const AJVF_SCHEME: u8 = 16;
const AJVF_URI_SAFE: u8 = 32;

fn ajvfBuildCharLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) : (c += 1) {
        const b: u8 = c as u8;
        let f: u8 = 0;
        const is_d = b >= '0' && b <= '9';
        const is_a = (b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z');
        if (is_d) { f |= AJVF_DIGIT | AJVF_HEX | AJVF_EMAIL_LOCAL | AJVF_SCHEME; }
        if (is_a) { f |= AJVF_ALPHA | AJVF_EMAIL_LOCAL | AJVF_SCHEME; }
        if ((b >= 'a' && b <= 'f') || (b >= 'A' && b <= 'F')) { f |= AJVF_HEX; }
        if (b == '.' || b == '!' || b == '#' || b == '$' || b == '%' || b == '&' || b == '\''
            || b == '*' || b == '+' || b == '-' || b == '/' || b == '=' || b == '?' || b == '^'
            || b == '_' || b == '`' || b == '{' || b == '|' || b == '}' || b == '~')
        {
            f |= AJVF_EMAIL_LOCAL;
        }
        if (b == '+' || b == '-' || b == '.') {
            f |= AJVF_SCHEME;
        }
        if (b > 0x20 && b < 0x7F
            && b != '<' && b != '>' && b != '"' && b != '{' && b != '}'
            && b != '|' && b != '\\' && b != '^' && b != '`')
        {
            f |= AJVF_URI_SAFE;
        }
        lut[c] = f;
    }
    return lut;
}

const AJVF_CHAR_LUT: [256]u8 = ajvfBuildCharLut();

fn ajvfIsDigit(b: u8): bool {
    return (AJVF_CHAR_LUT[b as usize] & AJVF_DIGIT) != 0;
}

fn ajvfIsAlpha(b: u8): bool {
    return (AJVF_CHAR_LUT[b as usize] & AJVF_ALPHA) != 0;
}

fn ajvfIsAlphaNum(b: u8): bool {
    return (AJVF_CHAR_LUT[b as usize] & (AJVF_DIGIT | AJVF_ALPHA)) != 0;
}

fn ajvfIsHex(b: u8): bool {
    return (AJVF_CHAR_LUT[b as usize] & AJVF_HEX) != 0;
}

fn ajvfParse2Digits(s: string, offset: usize): i32 {
    if (offset + 1 >= s.len) return -1;
    const a = s[offset];
    const b = s[offset + 1];
    if (!ajvfIsDigit(a) || !ajvfIsDigit(b)) return -1;
    return i32(a - '0') * 10 + i32(b - '0');
}

fn ajvfParse4Digits(s: string, offset: usize): i32 {
    const hi = ajvfParse2Digits(s, offset);
    const lo = ajvfParse2Digits(s, offset + 2);
    if (hi < 0 || lo < 0) return -1;
    return hi * 100 + lo;
}

fn ajvfIsLeapYear(year: u32): bool {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

fn ajvfDaysInMonth(year: u32, month: u32): u32 {
    if (month == 2) {
        if (ajvfIsLeapYear(year)) return 29;
        return 28;
    }
    if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
    return 31;
}

/// Validate RFC 3339 full-date (`YYYY-MM-DD`) including leap-year February 29 rules.
pub fn isDate(s: string): bool {
    if (s.len != 10) return false;
    if (s[4] != '-' || s[7] != '-') return false;
    const year_i = ajvfParse4Digits(s, 0);
    const month_i = ajvfParse2Digits(s, 5);
    const day_i = ajvfParse2Digits(s, 8);
    if (year_i < 0 || month_i < 1 || month_i > 12 || day_i < 1) return false;
    const max_day = ajvfDaysInMonth(u32(year_i), u32(month_i));
    return u32(day_i) <= max_day;
}

/// Validate RFC 3339 full-time (`HH:MM:SS(.frac)?(Z|±HH:MM)`) including leap second check.
pub fn isTime(s: string): bool {
    if (s.len < 9) return false;
    if (s[2] != ':' || s[5] != ':') return false;
    const hh = ajvfParse2Digits(s, 0);
    const mm = ajvfParse2Digits(s, 3);
    const ss = ajvfParse2Digits(s, 6);
    if (hh < 0 || hh > 23 || mm < 0 || mm > 59 || ss < 0 || ss > 60) return false;

    let pos: usize = 8;
    // Optional fractional seconds
    if (pos < s.len && s[pos] == '.') {
        pos += 1;
        const frac_start = pos;
        while (pos < s.len && ajvfIsDigit(s[pos])) : (pos += 1) {}
        if (pos == frac_start) return false;
    }

    // Required timezone offset: Z/z or (+|-)HH:MM
    if (pos >= s.len) return false;
    let tz_offset_min: i32 = 0;
    const tz_ch = s[pos];
    if (tz_ch == 'Z' || tz_ch == 'z') {
        if (pos + 1 != s.len) return false;
    } else if (tz_ch == '+' || tz_ch == '-') {
        if (pos + 6 != s.len) return false;
        if (s[pos + 3] != ':') return false;
        const tz_h = ajvfParse2Digits(s, pos + 1);
        const tz_m = ajvfParse2Digits(s, pos + 4);
        if (tz_h < 0 || tz_h > 23 || tz_m < 0 || tz_m > 59) return false;
        tz_offset_min = tz_h * 60 + tz_m;
        if (tz_ch == '-') tz_offset_min = -tz_offset_min;
    } else {
        return false;
    }

    // Leap second validation: UTC time must be 23:59:60
    if (ss == 60) {
        let utc_min = hh * 60 + mm - tz_offset_min;
        while (utc_min < 0) utc_min += 1440;
        while (utc_min >= 1440) utc_min -= 1440;
        if (utc_min != 23 * 60 + 59) return false;
    }
    return true;
}

/// Validate RFC 3339 date-time (`YYYY-MM-DD(T|t| )HH:MM:SS...`).
pub fn isDateTime(s: string): bool {
    if (s.len < 20) return false;
    const sep = s[10];
    if (sep != 'T' && sep != 't' && sep != ' ') return false;
    if (!isDate(s[0..10])) return false;
    return isTime(s[11..s.len]);
}

/// Validate RFC 1034 / 1123 hostname (label <= 63, total <= 253, no leading/trailing hyphen).
pub fn isHostname(s: string): bool {
    if (s.len == 0) return false;
    let end = s.len;
    if (s[end - 1] == '.') {
        end -= 1;
    }
    if (end == 0 || end > 253) return false;

    let label_len: usize = 0;
    let i: usize = 0;
    while (i < end) : (i += 1) {
        const b = s[i];
        if (b == '.') {
            if (label_len == 0 || label_len > 63) return false;
            if (s[i - 1] == '-') return false;
            label_len = 0;
        } else {
            if (label_len == 0) {
                if (!ajvfIsAlphaNum(b)) return false;
            } else {
                if (!ajvfIsAlphaNum(b) && b != '-') return false;
            }
            label_len += 1;
            if (label_len > 63) return false;
        }
    }
    if (label_len == 0 || s[end - 1] == '-') return false;
    return true;
}

fn ajvfIsEmailLocalChar(b: u8): bool {
    return (AJVF_CHAR_LUT[b as usize] & AJVF_EMAIL_LOCAL) != 0;
}

/// Validate email address (`local@domain`).
pub fn isEmail(s: string): bool {
    if (s.len < 3 || s.len > 254) return false;
    let at_pos: usize = s.len;
    let at_count: usize = 0;
    let i: usize = 0;
    const v_at: AjvfVec16 = @splat('@' as u8);
    while (i + 16 <= s.len) {
        const v: AjvfVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_at)) {
            let k: usize = 0;
            while (k < 16) : (k += 1) {
                if (s[i + k] == '@') {
                    at_pos = i + k;
                    at_count += 1;
                }
            }
        }
        i += 16;
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == '@') {
            at_pos = i;
            at_count += 1;
        }
    }
    if (at_count != 1) return false;
    if (at_pos == 0 || at_pos > 64) return false;
    if (at_pos + 1 >= s.len) return false;

    const local = s[0..at_pos];
    const domain = s[at_pos + 1..s.len];

    if (local[0] == '.' || local[local.len - 1] == '.') return false;
    let prev_dot = false;
    let k: usize = 0;
    while (k < local.len) : (k += 1) {
        const b = local[k];
        if (!ajvfIsEmailLocalChar(b)) return false;
        if (b == '.') {
            if (prev_dot) return false;
            prev_dot = true;
        } else {
            prev_dot = false;
        }
    }
    return isHostname(domain);
}

/// Validate IPv4 address (`0.0.0.0` .. `255.255.255.255`, no leading zeros).
pub fn isIpv4(s: string): bool {
    if (s.len < 7 || s.len > 15) return false;
    let parts: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (parts >= 4) return false;
        const start = i;
        let val: u32 = 0;
        let digits: usize = 0;
        while (i < s.len && s[i] != '.') : (i += 1) {
            if (!ajvfIsDigit(s[i])) return false;
            if (digits == 1 && s[start] == '0') return false;
            val = val * 10 + u32(s[i] - '0');
            digits += 1;
            if (digits > 3) return false;
        }
        if (digits == 0 || val > 255) return false;
        parts += 1;
        if (i < s.len) {
            if (s[i] != '.') return false;
            i += 1;
            if (i == s.len) return false;
        }
    }
    return parts == 4;
}

/// Validate IPv6 address (RFC 4291 / 5952, supports `::` compression and embedded IPv4).
pub fn isIpv6(s: string): bool {
    if (s.len < 2 || s.len > 45) return false;

    let groups: usize = 0;
    let has_double_colon = false;
    let i: usize = 0;

    if (s[0] == ':') {
        if (s.len < 2 || s[1] != ':') return false;
        has_double_colon = true;
        i = 2;
        if (i == s.len) return true;
    }

    while (i < s.len) {
        if (groups >= 8) return false;

        // Scan token until next ':' or end
        const start = i;
        let has_dot = false;
        while (i < s.len && s[i] != ':') : (i += 1) {
            if (s[i] == '.') has_dot = true;
        }
        const tok_len = i - start;

        if (has_dot) {
            // Embedded IPv4 must be at the very end and counts as 2 groups
            if (i != s.len) return false;
            if (!isIpv4(s[start..s.len])) return false;
            groups += 2;
            break;
        }

        if (tok_len == 0 || tok_len > 4) return false;
        let k = start;
        while (k < i) : (k += 1) {
            if (!ajvfIsHex(s[k])) return false;
        }
        groups += 1;

        if (i < s.len) {
            // Current char is ':'
            i += 1;
            if (i == s.len) return false; // trailing single ':'
            if (s[i] == ':') {
                if (has_double_colon) return false;
                has_double_colon = true;
                i += 1;
                if (i == s.len) break;
                if (s[i] == ':') return false; // ':::'
            }
        }
    }

    if (has_double_colon) {
        return groups < 8;
    }
    return groups == 8;
}

/// Validate UUID (`8-4-4-4-12` hex digits).
pub fn isUuid(s: string): bool {
    if (s.len != 36) return false;
    let i: usize = 0;
    while (i < 36) : (i += 1) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (s[i] != '-') return false;
        } else {
            if (!ajvfIsHex(s[i])) return false;
        }
    }
    return true;
}

fn ajvfIsSchemeChar(b: u8): bool {
    return (AJVF_CHAR_LUT[b as usize] & AJVF_SCHEME) != 0;
}

/// Validate RFC 3986 absolute URI (requires valid scheme and legal URI characters).
pub fn isUri(s: string): bool {
    if (s.len < 2) return false;
    if (!ajvfIsAlpha(s[0])) return false;
    let pos: usize = 1;
    while (pos < s.len && ajvfIsSchemeChar(s[pos])) : (pos += 1) {}
    if (pos >= s.len || s[pos] != ':') return false;
    pos += 1;

    // Validate percent-encodings and forbidden characters (16-lane SIMD fast path)
    let i = pos;
    const v_sp: AjvfVec16 = @splat(0x20 as u8);
    const v_lb: AjvfVec16 = @splat(0x7B as u8);
    const v_pct: AjvfVec16 = @splat('%' as u8);
    const v_qt: AjvfVec16 = @splat('"' as u8);
    const v_lt: AjvfVec16 = @splat('<' as u8);
    const v_gt: AjvfVec16 = @splat('>' as u8);
    const v_bs: AjvfVec16 = @splat('\\' as u8);
    const v_ct: AjvfVec16 = @splat('^' as u8);
    const v_bt: AjvfVec16 = @splat('`' as u8);
    while (i < s.len) {
        while (i + 16 <= s.len) {
            const v: AjvfVec16 = s[i .. i + 16][0..16].*;
            if (@reduce(.Or, v <= v_sp) ||
                @reduce(.Or, v >= v_lb) ||
                @reduce(.Or, v == v_pct) ||
                @reduce(.Or, v == v_qt) ||
                @reduce(.Or, v == v_lt) ||
                @reduce(.Or, v == v_gt) ||
                @reduce(.Or, v == v_bs) ||
                @reduce(.Or, v == v_ct) ||
                @reduce(.Or, v == v_bt))
            {
                break;
            }
            i += 16;
        }
        if (i >= s.len) break;
        const b = s[i];
        if ((AJVF_CHAR_LUT[b as usize] & AJVF_URI_SAFE) == 0) return false;
        if (b == '%') {
            if (i + 2 >= s.len || !ajvfIsHex(s[i + 1]) || !ajvfIsHex(s[i + 2])) return false;
            i += 3;
            continue;
        }
        i += 1;
    }

    // If authority is present (//...), verify bracket matching & port digits
    if (pos + 1 < s.len && s[pos] == '/' && s[pos + 1] == '/') {
        let auth_start = pos + 2;
        let auth_end = auth_start;
        while (auth_end < s.len && s[auth_end] != '/' && s[auth_end] != '?' && s[auth_end] != '#') : (auth_end += 1) {}
        const auth = s[auth_start..auth_end];
        let host_start: usize = 0;
        let k: usize = 0;
        while (k < auth.len) : (k += 1) {
            if (auth[k] == '@') host_start = k + 1;
        }
        const hp = auth[host_start..auth.len];
        if (hp.len > 0 && hp[0] == '[') {
            let close: usize = hp.len;
            let j: usize = 1;
            while (j < hp.len) : (j += 1) {
                if (hp[j] == ']') {
                    close = j;
                    break;
                }
            }
            if (close >= hp.len) return false;
            if (close + 1 < hp.len) {
                if (hp[close + 1] != ':') return false;
                let p = close + 2;
                while (p < hp.len) : (p += 1) {
                    if (!ajvfIsDigit(hp[p])) return false;
                }
            }
        } else {
            let j: usize = 0;
            while (j < hp.len) : (j += 1) {
                if (hp[j] == '[' || hp[j] == ']') return false;
                if (hp[j] == ':') {
                    let p = j + 1;
                    while (p < hp.len) : (p += 1) {
                        if (!ajvfIsDigit(hp[p])) return false;
                    }
                    break;
                }
            }
        }
    }
    return true;
}

/// Validate RFC 6901 JSON Pointer (`""` or `/...` with only `~0`/`~1` escapes).
pub fn isJsonPointer(s: string): bool {
    if (s.len == 0) return true;
    if (s[0] != '/') return false;
    let i: usize = 1;
    const v_tilde: AjvfVec16 = @splat('~' as u8);
    while (i < s.len) {
        while (i + 16 <= s.len) {
            const v: AjvfVec16 = s[i .. i + 16][0..16].*;
            if (@reduce(.Or, v == v_tilde)) {
                break;
            }
            i += 16;
        }
        if (i >= s.len) break;
        if (s[i] == '~') {
            if (i + 1 >= s.len) return false;
            const next = s[i + 1];
            if (next != '0' && next != '1') return false;
            i += 2;
            continue;
        }
        i += 1;
    }
    return true;
}

fn ajvfStrEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/// Dispatch format validation by JSON Schema format name.
pub fn validateFormat(format_name: string, value: string): bool {
    if (ajvfStrEq(format_name, "date")) return isDate(value);
    if (ajvfStrEq(format_name, "time")) return isTime(value);
    if (ajvfStrEq(format_name, "date-time")) return isDateTime(value);
    if (ajvfStrEq(format_name, "email")) return isEmail(value);
    if (ajvfStrEq(format_name, "hostname")) return isHostname(value);
    if (ajvfStrEq(format_name, "ipv4")) return isIpv4(value);
    if (ajvfStrEq(format_name, "ipv6")) return isIpv6(value);
    if (ajvfStrEq(format_name, "uuid")) return isUuid(value);
    if (ajvfStrEq(format_name, "uri")) return isUri(value);
    if (ajvfStrEq(format_name, "json-pointer")) return isJsonPointer(value);
    return false;
}
