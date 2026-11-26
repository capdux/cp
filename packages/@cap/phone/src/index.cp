// @cap/phone — Zero-allocation International Phone Number Parser, Validator & Formatter
// Clean-room pure-CP implementation modeled after npm `libphonenumber-js` and Rust `phonenumber` (MIT License).
//
// Features:
// - 45 major countries/regions calling code & length metadata table (E.164 compliant)
// - Zero heap allocations: fixed stack buffers & caller-supplied `[]u8` formatting buffers
// - Supports `+`, `00` international prefix, `tel:` URI prefix, formatting punctuation cleanup,
//   default country completion, and national trunk leading-zero stripping (AU/GB/JP/KR/DE/FR/CN/etc.)
// - 4 output formats: E.164, International, National, RFC 3966 (`tel:`)

const MAX_E164_DIGITS: usize = 15;
const MAX_NATIONAL_LEN: usize = 16;

struct CountryMeta {
    iso2: string,
    country_code: u16,
    min_len: u8,
    max_len: u8,
    strip_leading_zero: bool,
    trunk_prefix: u8,
}

const COUNTRY_COUNT: usize = 45;

const COUNTRIES: [45]CountryMeta = [
    // North America (+1; US listed first as primary region for +1)
    CountryMeta{ iso2: "US", country_code: 1, min_len: 10, max_len: 10, strip_leading_zero: false, trunk_prefix: 0 },
    CountryMeta{ iso2: "CA", country_code: 1, min_len: 10, max_len: 10, strip_leading_zero: false, trunk_prefix: 0 },
    // East & Southeast Asia / Oceania
    CountryMeta{ iso2: "CN", country_code: 86, min_len: 10, max_len: 11, strip_leading_zero: true, trunk_prefix: 0 },
    CountryMeta{ iso2: "AU", country_code: 61, min_len: 9, max_len: 9, strip_leading_zero: true, trunk_prefix: '0' },
    CountryMeta{ iso2: "NZ", country_code: 64, min_len: 8, max_len: 10, strip_leading_zero: true, trunk_prefix: '0' },
    CountryMeta{ iso2: "JP", country_code: 81, min_len: 9, max_len: 10, strip_leading_zero: true, trunk_prefix: '0' },
    CountryMeta{ iso2: "KR", country_code: 82, min_len: 8, max_len: 10, strip_leading_zero: true, trunk_prefix: '0' },
    CountryMeta{ iso2: "SG", country_code: 65, min_len: 8, max_len: 8, strip_leading_zero: false, trunk_prefix: 0 },
    CountryMeta{ iso2: "HK", country_code: 852, min_len: 8, max_len: 8, strip_leading_zero: false, trunk_prefix: 0 },
    CountryMeta{ iso2: "TW", country_code: 886, min_len: 8, max_len: 9, strip_leading_zero: true, trunk_prefix: '0' },
    CountryMeta{ iso2: "MO", country_code: 853, min_len: 8, max_len: 8, strip_leading_zero: false, trunk_prefix: 0 },
    CountryMeta{ iso2: "MY", country_code: 60, min_len: 8, max_len: 10, strip_leading_zero: true, trunk_prefix: '0' },
    CountryMeta{ iso2: "TH", country_code: 66, min_len: 8, max_len: 9, strip_leading_zero: true, trunk_prefix: '0' },
    CountryMeta{ iso2: "VN", country_code: 84, min_len: 9, max_len: 10, strip_leading_zero: true, trunk_prefix: '0' },
    CountryMeta{ iso2: "ID", country_code: 62, min_len: 8, max_len: 12, strip_leading_zero: true, trunk_prefix: '0' },
    CountryMeta{ iso2: "PH", country_code: 63, min_len: 9, max_len: 10, strip_leading_zero: true, trunk_prefix: '0' },
    CountryMeta{ iso2: "IN", country_code: 91, min_len: 10, max_len: 10, strip_leading_zero: true, trunk_prefix: '0' },
    // Europe
    CountryMeta{ iso2: "GB", country_code: 44, min_len: 9, max_len: 10, strip_leading_zero: true, trunk_prefix: '0' },
    CountryMeta{ iso2: "DE", country_code: 49, min_len: 6, max_len: 12, strip_leading_zero: true, trunk_prefix: '0' },
    CountryMeta{ iso2: "FR", country_code: 33, min_len: 9, max_len: 9, strip_leading_zero: true, trunk_prefix: '0' },
    CountryMeta{ iso2: "IT", country_code: 39, min_len: 6, max_len: 11, strip_leading_zero: false, trunk_prefix: 0 },
    CountryMeta{ iso2: "ES", country_code: 34, min_len: 9, max_len: 9, strip_leading_zero: false, trunk_prefix: 0 },
    CountryMeta{ iso2: "NL", country_code: 31, min_len: 9, max_len: 9, strip_leading_zero: true, trunk_prefix: '0' },
    CountryMeta{ iso2: "CH", country_code: 41, min_len: 9, max_len: 9, strip_leading_zero: true, trunk_prefix: '0' },
    CountryMeta{ iso2: "SE", country_code: 46, min_len: 7, max_len: 10, strip_leading_zero: true, trunk_prefix: '0' },
    CountryMeta{ iso2: "IE", country_code: 353, min_len: 7, max_len: 9, strip_leading_zero: true, trunk_prefix: '0' },
    CountryMeta{ iso2: "PL", country_code: 48, min_len: 9, max_len: 9, strip_leading_zero: false, trunk_prefix: 0 },
    CountryMeta{ iso2: "PT", country_code: 351, min_len: 9, max_len: 9, strip_leading_zero: false, trunk_prefix: 0 },
    CountryMeta{ iso2: "BE", country_code: 32, min_len: 8, max_len: 9, strip_leading_zero: true, trunk_prefix: '0' },
    CountryMeta{ iso2: "AT", country_code: 43, min_len: 6, max_len: 13, strip_leading_zero: true, trunk_prefix: '0' },
    CountryMeta{ iso2: "NO", country_code: 47, min_len: 8, max_len: 8, strip_leading_zero: false, trunk_prefix: 0 },
    CountryMeta{ iso2: "DK", country_code: 45, min_len: 8, max_len: 8, strip_leading_zero: false, trunk_prefix: 0 },
    CountryMeta{ iso2: "FI", country_code: 358, min_len: 6, max_len: 11, strip_leading_zero: true, trunk_prefix: '0' },
    CountryMeta{ iso2: "GR", country_code: 30, min_len: 10, max_len: 10, strip_leading_zero: false, trunk_prefix: 0 },
    CountryMeta{ iso2: "RU", country_code: 7, min_len: 10, max_len: 10, strip_leading_zero: false, trunk_prefix: '8' },
    CountryMeta{ iso2: "TR", country_code: 90, min_len: 10, max_len: 10, strip_leading_zero: true, trunk_prefix: '0' },
    // Middle East & Africa
    CountryMeta{ iso2: "AE", country_code: 971, min_len: 8, max_len: 9, strip_leading_zero: true, trunk_prefix: '0' },
    CountryMeta{ iso2: "SA", country_code: 966, min_len: 9, max_len: 9, strip_leading_zero: true, trunk_prefix: '0' },
    CountryMeta{ iso2: "IL", country_code: 972, min_len: 8, max_len: 9, strip_leading_zero: true, trunk_prefix: '0' },
    CountryMeta{ iso2: "ZA", country_code: 27, min_len: 9, max_len: 9, strip_leading_zero: true, trunk_prefix: '0' },
    // Latin America
    CountryMeta{ iso2: "BR", country_code: 55, min_len: 10, max_len: 11, strip_leading_zero: true, trunk_prefix: '0' },
    CountryMeta{ iso2: "MX", country_code: 52, min_len: 10, max_len: 10, strip_leading_zero: false, trunk_prefix: 0 },
    CountryMeta{ iso2: "AR", country_code: 54, min_len: 10, max_len: 11, strip_leading_zero: true, trunk_prefix: '0' },
    CountryMeta{ iso2: "CL", country_code: 56, min_len: 9, max_len: 9, strip_leading_zero: false, trunk_prefix: 0 },
    CountryMeta{ iso2: "CO", country_code: 57, min_len: 10, max_len: 10, strip_leading_zero: false, trunk_prefix: 0 },
];

fn toUpperAscii(c: u8): u8 {
    if (c >= 'a' && c <= 'z') return c - 32;
    return c;
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn eqIso2IgnoreCase(a: string, b: string): bool {
    if (a.len != 2 || b.len != 2) return false;
    return toUpperAscii(a[0]) == toUpperAscii(b[0]) &&
        toUpperAscii(a[1]) == toUpperAscii(b[1]);
}

fn findCountryIndexByIso2(iso2: string): i32 {
    if (iso2.len != 2) return -1;
    let i: usize = 0;
    while (i < COUNTRY_COUNT) : (i += 1) {
        if (eqIso2IgnoreCase(COUNTRIES[i].iso2, iso2)) {
            return i as i32;
        }
    }
    return -1;
}

fn findCountryIndexByCode(cc: u16): i32 {
    if (cc == 0) return -1;
    let i: usize = 0;
    while (i < COUNTRY_COUNT) : (i += 1) {
        if (COUNTRIES[i].country_code == cc) {
            return i as i32;
        }
    }
    return -1;
}

fn countryCodeDigitLen(cc: u16): usize {
    if (cc >= 100) return 3;
    if (cc >= 10) return 2;
    if (cc >= 1) return 1;
    return 0;
}

fn pushByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn pushSlice(out: []u8, pos: usize, src: []const u8): usize {
    let p = pos;
    let i: usize = 0;
    while (i < src.len && p < out.len) : (i += 1) {
        out[p] = src[i];
        p += 1;
    }
    return p;
}

fn pushU16(out: []u8, pos: usize, val: u16): usize {
    let tmp: [5]u8 = [0, 0, 0, 0, 0];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        return pushByte(out, pos, '0');
    }
    while (v > 0 && n < 5) {
        const rem = (v % 10) as u8;
        tmp[n] = (48 as u8) + rem;
        n += 1;
        v = v / 10;
    }
    let p = pos;
    let i = n;
    while (i > 0) {
        i -= 1;
        p = pushByte(out, p, tmp[i]);
    }
    return p;
}

fn pushGroupRange(out: []u8, pos: usize, buf: [16]u8, start: usize, end: usize): usize {
    let p = pos;
    let i = start;
    while (i < end && p < out.len) : (i += 1) {
        out[p] = buf[i];
        p += 1;
    }
    return p;
}

/// Write grouped national digits into `out` using separator `sep` (e.g. `' '` or `'-'`).
fn writeGroupedNational(out: []u8, pos: usize, cc: u16, buf: [16]u8, len: usize, sep: u8): usize {
    let p = pos;
    if (len == 0) return p;

    // North America (+1): 3-3-4
    if (cc == 1 && len == 10) {
        p = pushGroupRange(out, p, buf, 0, 3);
        p = pushByte(out, p, sep);
        p = pushGroupRange(out, p, buf, 3, 6);
        p = pushByte(out, p, sep);
        p = pushGroupRange(out, p, buf, 6, 10);
        return p;
    }

    // China (+86): mobile 11-digit 3-4-4, 10-digit 2-4-4
    if (cc == 86 && len == 11) {
        p = pushGroupRange(out, p, buf, 0, 3);
        p = pushByte(out, p, sep);
        p = pushGroupRange(out, p, buf, 3, 7);
        p = pushByte(out, p, sep);
        p = pushGroupRange(out, p, buf, 7, 11);
        return p;
    }
    if (cc == 86 && len == 10) {
        p = pushGroupRange(out, p, buf, 0, 2);
        p = pushByte(out, p, sep);
        p = pushGroupRange(out, p, buf, 2, 6);
        p = pushByte(out, p, sep);
        p = pushGroupRange(out, p, buf, 6, 10);
        return p;
    }

    // Australia (+61): 3-3-3
    if (cc == 61 && len == 9) {
        p = pushGroupRange(out, p, buf, 0, 3);
        p = pushByte(out, p, sep);
        p = pushGroupRange(out, p, buf, 3, 6);
        p = pushByte(out, p, sep);
        p = pushGroupRange(out, p, buf, 6, 9);
        return p;
    }

    // UK (+44): 4-6 for 10 digits
    if (cc == 44 && len == 10) {
        p = pushGroupRange(out, p, buf, 0, 4);
        p = pushByte(out, p, sep);
        p = pushGroupRange(out, p, buf, 4, 10);
        return p;
    }

    // Japan (+81): 2-4-4 for 10-digit mobile (90/80/70)
    if (cc == 81 && len == 10) {
        p = pushGroupRange(out, p, buf, 0, 2);
        p = pushByte(out, p, sep);
        p = pushGroupRange(out, p, buf, 2, 6);
        p = pushByte(out, p, sep);
        p = pushGroupRange(out, p, buf, 6, 10);
        return p;
    }

    // Korea (+82): 2-4-4 for 10 digits
    if (cc == 82 && len == 10) {
        p = pushGroupRange(out, p, buf, 0, 2);
        p = pushByte(out, p, sep);
        p = pushGroupRange(out, p, buf, 2, 6);
        p = pushByte(out, p, sep);
        p = pushGroupRange(out, p, buf, 6, 10);
        return p;
    }

    // Singapore (+65), Hong Kong (+852), Macau (+853): 4-4 for 8 digits
    if ((cc == 65 || cc == 852 || cc == 853) && len == 8) {
        p = pushGroupRange(out, p, buf, 0, 4);
        p = pushByte(out, p, sep);
        p = pushGroupRange(out, p, buf, 4, 8);
        return p;
    }

    // France (+33): 1-2-2-2-2
    if (cc == 33 && len == 9) {
        p = pushGroupRange(out, p, buf, 0, 1);
        p = pushByte(out, p, sep);
        p = pushGroupRange(out, p, buf, 1, 3);
        p = pushByte(out, p, sep);
        p = pushGroupRange(out, p, buf, 3, 5);
        p = pushByte(out, p, sep);
        p = pushGroupRange(out, p, buf, 5, 7);
        p = pushByte(out, p, sep);
        p = pushGroupRange(out, p, buf, 7, 9);
        return p;
    }

    // Generic fallback grouping by length
    if (len == 10) {
        p = pushGroupRange(out, p, buf, 0, 3);
        p = pushByte(out, p, sep);
        p = pushGroupRange(out, p, buf, 3, 6);
        p = pushByte(out, p, sep);
        p = pushGroupRange(out, p, buf, 6, 10);
        return p;
    }
    if (len == 9) {
        p = pushGroupRange(out, p, buf, 0, 3);
        p = pushByte(out, p, sep);
        p = pushGroupRange(out, p, buf, 3, 6);
        p = pushByte(out, p, sep);
        p = pushGroupRange(out, p, buf, 6, 9);
        return p;
    }
    if (len == 8) {
        p = pushGroupRange(out, p, buf, 0, 4);
        p = pushByte(out, p, sep);
        p = pushGroupRange(out, p, buf, 4, 8);
        return p;
    }
    return pushGroupRange(out, p, buf, 0, len);
}

fn validateNationalRules(cc: u16, buf: [16]u8, len: usize): bool {
    if (len == 0) return false;

    // Reject all-zero numbers
    let all_zero = true;
    let i: usize = 0;
    while (i < len) : (i += 1) {
        if (buf[i] != '0') {
            all_zero = false;
            break;
        }
    }
    if (all_zero) return false;

    // NANP (+1): NXX-NXX-XXXX where N in [2..9]
    if (cc == 1) {
        if (len != 10) return false;
        return buf[0] >= '2' && buf[3] >= '2';
    }

    // China (+86): 11-digit mobile starts with 1[3-9], or 10-11 digit landline starts with [2-9]
    if (cc == 86) {
        if (buf[0] == '0') return false;
        if (buf[0] == '1') {
            if (len != 11) return false;
            return buf[1] >= '3' && buf[1] <= '9';
        }
        return true;
    }

    // Australia (+61): national significant number starts with [2,3,4,7,8]
    if (cc == 61) {
        const d0 = buf[0];
        return d0 == '2' || d0 == '3' || d0 == '4' || d0 == '7' || d0 == '8';
    }

    // UK (+44): national significant number never starts with '0'
    if (cc == 44) {
        return buf[0] != '0';
    }

    // Japan (+81): national significant number never starts with '0'
    if (cc == 81) {
        return buf[0] != '0';
    }

    // Singapore (+65): 8-digit starting with 3, 6, 8, or 9
    if (cc == 65) {
        const d0 = buf[0];
        return d0 == '3' || d0 == '6' || d0 == '8' || d0 == '9';
    }

    // Hong Kong (+852): 8-digit starting with 2, 3, 5, 6, 7, 8, or 9
    if (cc == 852) {
        const d0 = buf[0];
        return d0 == '2' || d0 == '3' || d0 == '5' || d0 == '6' || d0 == '7' || d0 == '8' || d0 == '9';
    }

    return true;
}

/// Parsed international phone number (stack-allocated, zero heap usage).
pub struct PhoneNumber {
    national_buf: [16]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
    national_len: u8 = 0,
    country_code: u16 = 0,
    iso2: string = "",
    is_valid: bool = false,
    is_possible: bool = false,

    /// Construct an empty invalid PhoneNumber.
    pub static fn invalid(): PhoneNumber {
        return PhoneNumber{
            national_buf: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
            national_len: 0,
            country_code: 0,
            iso2: "",
            is_valid: false,
            is_possible: false,
        };
    }

    /// Format as strict E.164 (e.g. `"+61412345678"`). Writes into caller buffer `out`.
    pub fn formatE164(out: []u8): string {
        if (!this.is_possible || this.country_code == 0 || this.national_len == 0) {
            return "";
        }
        let pos: usize = 0;
        pos = pushByte(out, pos, '+');
        pos = pushU16(out, pos, this.country_code);
        pos = pushGroupRange(out, pos, this.national_buf, 0, this.national_len as usize);
        return out[0..pos];
    }

    /// Format as human-readable international format (e.g. `"+61 412 345 678"`).
    pub fn formatInternational(out: []u8): string {
        if (!this.is_possible || this.country_code == 0 || this.national_len == 0) {
            return "";
        }
        let pos: usize = 0;
        pos = pushByte(out, pos, '+');
        pos = pushU16(out, pos, this.country_code);
        pos = pushByte(out, pos, ' ');
        pos = writeGroupedNational(out, pos, this.country_code, this.national_buf, this.national_len as usize, ' ');
        return out[0..pos];
    }

    /// Format as domestic national format (e.g. `"(415) 555-2671"`, `"0412 345 678"`, `"090-1234-5678"`).
    pub fn formatNational(out: []u8): string {
        if (!this.is_possible || this.country_code == 0 || this.national_len == 0) {
            return "";
        }
        const len = this.national_len as usize;
        let pos: usize = 0;

        // NANP (+1): (NXX) NXX-XXXX
        if (this.country_code == 1 && len == 10) {
            pos = pushByte(out, pos, '(');
            pos = pushGroupRange(out, pos, this.national_buf, 0, 3);
            pos = pushSlice(out, pos, ") ");
            pos = pushGroupRange(out, pos, this.national_buf, 3, 6);
            pos = pushByte(out, pos, '-');
            pos = pushGroupRange(out, pos, this.national_buf, 6, 10);
            return out[0..pos];
        }

        // Japan (+81): 090-1234-5678
        if (this.country_code == 81 && len == 10) {
            pos = pushByte(out, pos, '0');
            pos = pushGroupRange(out, pos, this.national_buf, 0, 2);
            pos = pushByte(out, pos, '-');
            pos = pushGroupRange(out, pos, this.national_buf, 2, 6);
            pos = pushByte(out, pos, '-');
            pos = pushGroupRange(out, pos, this.national_buf, 6, 10);
            return out[0..pos];
        }

        // Korea (+82): 010-1234-5678
        if (this.country_code == 82 && len == 10) {
            pos = pushByte(out, pos, '0');
            pos = pushGroupRange(out, pos, this.national_buf, 0, 2);
            pos = pushByte(out, pos, '-');
            pos = pushGroupRange(out, pos, this.national_buf, 2, 6);
            pos = pushByte(out, pos, '-');
            pos = pushGroupRange(out, pos, this.national_buf, 6, 10);
            return out[0..pos];
        }

        // Look up trunk prefix if any
        const idx = findCountryIndexByIso2(this.iso2);
        if (idx >= 0) {
            const trunk = COUNTRIES[idx as usize].trunk_prefix;
            if (trunk != 0) {
                pos = pushByte(out, pos, trunk);
            }
        }
        pos = writeGroupedNational(out, pos, this.country_code, this.national_buf, len, ' ');
        return out[0..pos];
    }

    /// Format as RFC 3966 URI (e.g. `"tel:+61-412-345-678"`).
    pub fn formatRfc3966(out: []u8): string {
        if (!this.is_possible || this.country_code == 0 || this.national_len == 0) {
            return "";
        }
        let pos: usize = 0;
        pos = pushSlice(out, pos, "tel:+");
        pos = pushU16(out, pos, this.country_code);
        pos = pushByte(out, pos, '-');
        pos = writeGroupedNational(out, pos, this.country_code, this.national_buf, this.national_len as usize, '-');
        return out[0..pos];
    }
}

/// Return the number of built-in countries/regions in the metadata table.
pub fn supportedCountryCount(): usize {
    return COUNTRY_COUNT;
}

/// Look up international calling code by ISO 3166-1 alpha-2 code (case-insensitive). Returns `0` if unknown.
pub fn countryCodeFromIso2(iso2: string): u16 {
    const idx = findCountryIndexByIso2(iso2);
    if (idx < 0) return 0;
    return COUNTRIES[idx as usize].country_code;
}

/// Look up primary ISO 3166-1 alpha-2 code by international calling code. Returns `""` if unknown.
pub fn iso2FromCountryCode(cc: u16): string {
    const idx = findCountryIndexByCode(cc);
    if (idx < 0) return "";
    return COUNTRIES[idx as usize].iso2;
}

struct CcMatch {
    idx: i32,
    cc_len: usize,
}

/// Match a 1-, 2-, or 3-digit calling code prefix from `digits[0..n]`.
fn matchCallingCodePrefix(digits: [24]u8, n: usize): CcMatch {
    if (n == 0) return CcMatch{ idx: -1, cc_len: 0 };

    // 1-digit codes (e.g. +1, +7)
    const d1 = (digits[0] - (48 as u8)) as u16;
    const idx1 = findCountryIndexByCode(d1);
    if (idx1 >= 0) {
        return CcMatch{ idx: idx1, cc_len: 1 };
    }

    // 2-digit codes (e.g. +86, +61, +44, +81, +65)
    if (n >= 2) {
        const d2 = d1 * 10 + ((digits[1] - (48 as u8)) as u16);
        const idx2 = findCountryIndexByCode(d2);
        if (idx2 >= 0) {
            return CcMatch{ idx: idx2, cc_len: 2 };
        }
    }

    // 3-digit codes (e.g. +852, +886, +853, +971, +353)
    if (n >= 3) {
        const d3 = d1 * 100 + ((digits[1] - (48 as u8)) as u16) * 10 + ((digits[2] - (48 as u8)) as u16);
        const idx3 = findCountryIndexByCode(d3);
        if (idx3 >= 0) {
            return CcMatch{ idx: idx3, cc_len: 3 };
        }
    }

    return CcMatch{ idx: -1, cc_len: 0 };
}

/// Parse an international or national phone number string into a `PhoneNumber`.
/// Supports `+`, `00` international prefix, `tel:` URI prefix, spaces/hyphens/parentheses/dots,
/// default country completion via `default_iso2`, and domestic leading-zero stripping.
pub fn parsePhoneNumber(raw: string, default_iso2: string): PhoneNumber {
    if (raw.len == 0) return PhoneNumber.invalid();

    // Trim leading/trailing ASCII whitespace
    let start: usize = 0;
    let end: usize = raw.len;
    while (start < end && (raw[start] == ' ' || raw[start] == '\t' || raw[start] == '\r' || raw[start] == '\n')) {
        start += 1;
    }
    while (end > start && (raw[end - 1] == ' ' || raw[end - 1] == '\t' || raw[end - 1] == '\r' || raw[end - 1] == '\n')) {
        end -= 1;
    }
    if (start >= end) return PhoneNumber.invalid();

    // Optional RFC 3966 `tel:` prefix (case-insensitive)
    if (end - start >= 4 &&
        toUpperAscii(raw[start]) == 'T' &&
        toUpperAscii(raw[start + 1]) == 'E' &&
        toUpperAscii(raw[start + 2]) == 'L' &&
        raw[start + 3] == ':')
    {
        start += 4;
        while (start < end && raw[start] == ' ') {
            start += 1;
        }
    }
    if (start >= end) return PhoneNumber.invalid();

    let has_plus = false;
    let digits: [24]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    let dlen: usize = 0;
    let paren_depth: i32 = 0;

    let i = start;
    while (i < end) : (i += 1) {
        const c = raw[i];
        if (c == '+') {
            if (has_plus || dlen > 0) return PhoneNumber.invalid();
            has_plus = true;
        } else if (isDigit(c)) {
            if (dlen >= 24) return PhoneNumber.invalid();
            digits[dlen] = c;
            dlen += 1;
        } else if (c == ' ' || c == '-' || c == '.' || c == '/' || c == '\t') {
            // Allowed visual separator
        } else if (c == '(') {
            paren_depth += 1;
        } else if (c == ')') {
            paren_depth -= 1;
            if (paren_depth < 0) return PhoneNumber.invalid();
        } else {
            // Illegal character
            return PhoneNumber.invalid();
        }
    }

    if (paren_depth != 0 || dlen == 0) {
        return PhoneNumber.invalid();
    }

    // Determine whether input is international (`+` or `00` prefix)
    let is_intl = has_plus;
    let digit_offset: usize = 0;
    if (!has_plus && dlen >= 6 && digits[0] == '0' && digits[1] == '0') {
        is_intl = true;
        digit_offset = 2;
    }

    let meta_idx: i32 = -1;
    let nat_start: usize = 0;

    if (is_intl) {
        // Copy shifted digits for calling code matching
        let shifted: [24]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        const rem_len = dlen - digit_offset;
        if (rem_len < 4) return PhoneNumber.invalid();
        let k: usize = 0;
        while (k < rem_len) : (k += 1) {
            shifted[k] = digits[digit_offset + k];
        }

        const m = matchCallingCodePrefix(shifted, rem_len);
        meta_idx = m.idx;
        const cc_len = m.cc_len;
        if (meta_idx < 0) return PhoneNumber.invalid();

        // If default_iso2 shares the same calling code (e.g. CA for +1), preserve default_iso2
        if (default_iso2.len == 2) {
            const def_idx = findCountryIndexByIso2(default_iso2);
            if (def_idx >= 0 && COUNTRIES[def_idx as usize].country_code == COUNTRIES[meta_idx as usize].country_code) {
                meta_idx = def_idx;
            }
        }

        nat_start = digit_offset + cc_len;

        // Strip optional redundant trunk '0' in international format (e.g. +44 (0) 7700 900123 or +61 0412 345 678)
        const meta = COUNTRIES[meta_idx as usize];
        if (meta.strip_leading_zero && nat_start < dlen && digits[nat_start] == '0') {
            nat_start += 1;
        }
    } else {
        // Domestic number: requires valid default_iso2
        meta_idx = findCountryIndexByIso2(default_iso2);
        if (meta_idx < 0) return PhoneNumber.invalid();

        const meta = COUNTRIES[meta_idx as usize];
        nat_start = 0;

        if (meta.strip_leading_zero && dlen > 1 && digits[0] == '0') {
            nat_start = 1;
        } else if (meta.country_code == 1 && dlen == 11 && digits[0] == '1') {
            // NANP 11-digit domestic dial: 1-415-555-2671
            nat_start = 1;
        } else if (meta.country_code == 7 && dlen == 11 && digits[0] == '8') {
            // Russia 11-digit domestic trunk '8'
            nat_start = 1;
        }
    }

    const meta = COUNTRIES[meta_idx as usize];
    if (nat_start >= dlen) return PhoneNumber.invalid();
    const nat_len = dlen - nat_start;
    if (nat_len > MAX_NATIONAL_LEN) return PhoneNumber.invalid();

    const cc_digits = countryCodeDigitLen(meta.country_code);
    const total_e164_len = cc_digits + nat_len;

    let res = PhoneNumber.invalid();
    res.country_code = meta.country_code;
    res.iso2 = meta.iso2;
    res.national_len = nat_len as u8;

    let j: usize = 0;
    while (j < nat_len) : (j += 1) {
        res.national_buf[j] = digits[nat_start + j];
    }

    const len_ok = nat_len >= (meta.min_len as usize) &&
        nat_len <= (meta.max_len as usize) &&
        total_e164_len <= MAX_E164_DIGITS;

    res.is_possible = len_ok;
    res.is_valid = len_ok && validateNationalRules(meta.country_code, res.national_buf, nat_len);
    return res;
}

/// Convenience predicate returning `true` iff `raw` parses as a valid phone number.
pub fn isValidPhone(raw: string, default_iso2: string): bool {
    const p = parsePhoneNumber(raw, default_iso2);
    return p.is_valid;
}
