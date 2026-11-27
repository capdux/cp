// packages/@cap/idna-uts46/src/index.cp
// Zero-allocation UTS #46 Internationalized Domain Names (IDNA) in pure CP.
// Reuses @cap/icu-normalizer (normalizeNfc) and @cap/punycode (encodeLabel, decodeLabel, isAsciiOnly).
// Clean-room MIT implementation (Rust `idna` crate / UTS #46 & RFC 3492 parity).

import { normalizeNfc } from "../../icu_normalizer/src/index.cp";
import { encodeLabel, decodeLabel, isAsciiOnly } from "../../punycode/src/index.cp";

pub const IDNA_OK: i64 = 0;
pub const IDNA_ERR_EMPTY: i64 = -1;
pub const IDNA_ERR_EMPTY_LABEL: i64 = -2;
pub const IDNA_ERR_LABEL_TOO_LONG: i64 = -3;
pub const IDNA_ERR_DOMAIN_TOO_LONG: i64 = -4;
pub const IDNA_ERR_INVALID_HYPHEN: i64 = -5;
pub const IDNA_ERR_INVALID_CHAR: i64 = -6;
pub const IDNA_ERR_INVALID_PUNYCODE: i64 = -7;
pub const IDNA_ERR_BUFFER_TOO_SMALL: i64 = -8;

pub const IDNA_MAX_LABEL_LEN: usize = 63;
pub const IDNA_MAX_DOMAIN_LEN: usize = 253;

fn idnaToLowerAscii(ch: u8): u8 {
    if (ch >= ('A' as u8) && ch <= ('Z' as u8)) {
        return ch + (32 as u8);
    }
    return ch;
}

fn idnaIsLdhLower(ch: u8): bool {
    return (ch >= ('a' as u8) && ch <= ('z' as u8))
        || (ch >= ('0' as u8) && ch <= ('9' as u8))
        || ch == ('-' as u8);
}

fn idnaIsLdhAnyCase(ch: u8): bool {
    return (ch >= ('a' as u8) && ch <= ('z' as u8))
        || (ch >= ('A' as u8) && ch <= ('Z' as u8))
        || (ch >= ('0' as u8) && ch <= ('9' as u8))
        || ch == ('-' as u8);
}

/// Returns true if `label` starts with the RFC 3492 ACE prefix `"xn--"` (case-insensitive).
pub fn isPunycodeLabel(label: string): bool {
    if (label.len < 4) {
        return false;
    }
    return idnaToLowerAscii(label[0]) == ('x' as u8)
        && idnaToLowerAscii(label[1]) == ('n' as u8)
        && label[2] == ('-' as u8)
        && label[3] == ('-' as u8);
}

/// Maps UTS #46 Unicode label separators (`U+3002` `。`, `U+FF0E` `．`, `U+FF61` `｡`)
/// to ASCII `.` and lowercases ASCII letters into `out`.
/// Returns mapped byte count, or `IDNA_ERR_BUFFER_TOO_SMALL` if `out` is too small.
fn idnaMapDomain(domain: string, out: []u8): i64 {
    let pos: usize = 0;
    let out_pos: usize = 0;
    while (pos < domain.len) {
        if (out_pos >= out.len) {
            return IDNA_ERR_BUFFER_TOO_SMALL;
        }
        const b0 = domain[pos];
        // Check 3-byte UTF-8 Unicode full stops:
        // U+3002: E3 80 82, U+FF0E: EF BC 8E, U+FF61: EF BD A1
        if (pos + 2 < domain.len) {
            const b1 = domain[pos + 1];
            const b2 = domain[pos + 2];
            const is_3002 = b0 == (0xE3 as u8) && b1 == (0x80 as u8) && b2 == (0x82 as u8);
            const is_ff0e = b0 == (0xEF as u8) && b1 == (0xBC as u8) && b2 == (0x8E as u8);
            const is_ff61 = b0 == (0xEF as u8) && b1 == (0xBD as u8) && b2 == (0xA1 as u8);
            if (is_3002 || is_ff0e || is_ff61) {
                out[out_pos] = '.' as u8;
                out_pos = out_pos + 1;
                pos = pos + 3;
                continue;
            }
        }
        if (b0 < (0x80 as u8)) {
            out[out_pos] = idnaToLowerAscii(b0);
        } else {
            out[out_pos] = b0;
        }
        out_pos = out_pos + 1;
        pos = pos + 1;
    }
    return out_pos as i64;
}

fn idnaValidateAsciiLabelLower(label: []const u8): i64 {
    if (label.len == 0) {
        return IDNA_ERR_EMPTY_LABEL;
    }
    if (label.len > IDNA_MAX_LABEL_LEN) {
        return IDNA_ERR_LABEL_TOO_LONG;
    }
    if (isPunycodeLabel(label) && label.len <= 4) {
        return IDNA_ERR_INVALID_PUNYCODE;
    }
    if (label[0] == ('-' as u8) || label[label.len - 1] == ('-' as u8)) {
        return IDNA_ERR_INVALID_HYPHEN;
    }
    let i: usize = 0;
    while (i < label.len) {
        if (!idnaIsLdhLower(label[i])) {
            return IDNA_ERR_INVALID_CHAR;
        }
        i = i + 1;
    }
    if (isPunycodeLabel(label)) {
        if (label.len <= 4) {
            return IDNA_ERR_INVALID_PUNYCODE;
        }
        let check_u8: [256]u8 = [0; 256];
        const decoded_len = decodeLabel(label[4..label.len], check_u8[0..256]);
        if (decoded_len <= (0 as i64)) {
            return IDNA_ERR_INVALID_PUNYCODE;
        }
    } else if (label.len >= 4 && label[2] == ('-' as u8) && label[3] == ('-' as u8)) {
        // UTS #46 CheckHyphens: positions 3 and 4 cannot both be '-' unless valid ACE "xn--"
        return IDNA_ERR_INVALID_HYPHEN;
    }
    return IDNA_OK;
}

fn idnaAppendBytes(out: []u8, pos: *usize, src: []const u8): bool {
    if (pos.* + src.len > out.len) {
        return false;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i = i + 1;
    }
    pos.* = pos.* + src.len;
    return true;
}

/// Converts a Unicode/ASCII domain name into its canonical UTS #46 ASCII representation
/// (`xn--` Punycode for non-ASCII labels via `@cap/icu-normalizer::normalizeNfc` + `@cap/punycode::encodeLabel`).
/// Returns the number of bytes written to `out` on success, or a negative `IDNA_ERR_*` code.
pub fn domainToAscii(domain: string, out: []u8): i64 {
    if (domain.len == 0) {
        return IDNA_ERR_EMPTY;
    }

    let mapped_buf: [512]u8 = [0; 512];
    const mapped_len_i64 = idnaMapDomain(domain, mapped_buf[0..512]);
    if (mapped_len_i64 < (0 as i64)) {
        return IDNA_ERR_DOMAIN_TOO_LONG;
    }
    const mapped_len = mapped_len_i64 as usize;

    let out_pos: usize = 0;
    let pos: usize = 0;
    let has_trailing_dot = false;

    while (pos < mapped_len) {
        let dot = pos;
        while (dot < mapped_len && mapped_buf[dot] != ('.' as u8)) {
            dot = dot + 1;
        }
        const label = mapped_buf[pos..dot];
        if (label.len == 0) {
            return IDNA_ERR_EMPTY_LABEL;
        }

        if (isAsciiOnly(label)) {
            const v_err = idnaValidateAsciiLabelLower(label);
            if (v_err != IDNA_OK) {
                return v_err;
            }
            if (!idnaAppendBytes(out, &out_pos, label)) {
                return IDNA_ERR_BUFFER_TOO_SMALL;
            }
        } else {
            // UTS #46: normalize non-ASCII label to NFC via @cap/icu-normalizer
            let nfc_buf: [256]u8 = [0; 256];
            const nfc_len = normalizeNfc(label, nfc_buf[0..256]);
            if (nfc_len == 0) {
                return IDNA_ERR_EMPTY_LABEL;
            }
            if (nfc_buf[0] == ('-' as u8) || nfc_buf[nfc_len - 1] == ('-' as u8)) {
                return IDNA_ERR_INVALID_HYPHEN;
            }

            if (isAsciiOnly(nfc_buf[0..nfc_len])) {
                const v_err = idnaValidateAsciiLabelLower(nfc_buf[0..nfc_len]);
                if (v_err != IDNA_OK) {
                    return v_err;
                }
                if (!idnaAppendBytes(out, &out_pos, nfc_buf[0..nfc_len])) {
                    return IDNA_ERR_BUFFER_TOO_SMALL;
                }
            } else {
                let ace_buf: [128]u8 = [0; 128];
                ace_buf[0] = 'x' as u8;
                ace_buf[1] = 'n' as u8;
                ace_buf[2] = '-' as u8;
                ace_buf[3] = '-' as u8;
                const enc_len = encodeLabel(nfc_buf[0..nfc_len], ace_buf[4..128]);
                if (enc_len <= (0 as i64)) {
                    return IDNA_ERR_INVALID_PUNYCODE;
                }
                const total_ace = (4 as usize) + (enc_len as usize);
                if (total_ace > IDNA_MAX_LABEL_LEN) {
                    return IDNA_ERR_LABEL_TOO_LONG;
                }
                if (!idnaAppendBytes(out, &out_pos, ace_buf[0..total_ace])) {
                    return IDNA_ERR_BUFFER_TOO_SMALL;
                }
            }
        }

        if (dot < mapped_len) {
            if (dot + 1 == mapped_len) {
                // Single trailing root dot
                has_trailing_dot = true;
                if (!idnaAppendBytes(out, &out_pos, ".")) {
                    return IDNA_ERR_BUFFER_TOO_SMALL;
                }
                break;
            }
            if (!idnaAppendBytes(out, &out_pos, ".")) {
                return IDNA_ERR_BUFFER_TOO_SMALL;
            }
            pos = dot + 1;
        } else {
            break;
        }
    }

    let eff_len = out_pos;
    if (has_trailing_dot && eff_len > 0) {
        eff_len = eff_len - 1;
    }
    if (eff_len > IDNA_MAX_DOMAIN_LEN) {
        return IDNA_ERR_DOMAIN_TOO_LONG;
    }
    return out_pos as i64;
}

/// Converts an ASCII/IDNA domain name (`xn--` A-labels) back into canonical UTF-8 U-labels
/// via `@cap/punycode::decodeLabel` and `@cap/icu-normalizer::normalizeNfc`.
/// Returns bytes written to `out` on success, or a negative `IDNA_ERR_*` code.
pub fn domainToUnicode(domain: string, out: []u8): i64 {
    if (domain.len == 0) {
        return IDNA_ERR_EMPTY;
    }

    let mapped_buf: [512]u8 = [0; 512];
    const mapped_len_i64 = idnaMapDomain(domain, mapped_buf[0..512]);
    if (mapped_len_i64 < (0 as i64)) {
        return IDNA_ERR_DOMAIN_TOO_LONG;
    }
    const mapped_len = mapped_len_i64 as usize;

    let out_pos: usize = 0;
    let pos: usize = 0;

    while (pos < mapped_len) {
        let dot = pos;
        while (dot < mapped_len && mapped_buf[dot] != ('.' as u8)) {
            dot = dot + 1;
        }
        const label = mapped_buf[pos..dot];
        if (label.len == 0) {
            return IDNA_ERR_EMPTY_LABEL;
        }

        if (isPunycodeLabel(label)) {
            if (label.len <= 4 || label.len > IDNA_MAX_LABEL_LEN) {
                return IDNA_ERR_INVALID_PUNYCODE;
            }
            let u_buf: [256]u8 = [0; 256];
            const dec_len = decodeLabel(label[4..label.len], u_buf[0..256]);
            if (dec_len <= (0 as i64)) {
                return IDNA_ERR_INVALID_PUNYCODE;
            }
            let nfc_buf: [256]u8 = [0; 256];
            const nfc_len = normalizeNfc(u_buf[0..(dec_len as usize)], nfc_buf[0..256]);
            if (nfc_len == 0) {
                return IDNA_ERR_INVALID_PUNYCODE;
            }
            if (!idnaAppendBytes(out, &out_pos, nfc_buf[0..nfc_len])) {
                return IDNA_ERR_BUFFER_TOO_SMALL;
            }
        } else if (isAsciiOnly(label)) {
            const v_err = idnaValidateAsciiLabelLower(label);
            if (v_err != IDNA_OK) {
                return v_err;
            }
            if (!idnaAppendBytes(out, &out_pos, label)) {
                return IDNA_ERR_BUFFER_TOO_SMALL;
            }
        } else {
            let nfc_buf: [256]u8 = [0; 256];
            const nfc_len = normalizeNfc(label, nfc_buf[0..256]);
            if (nfc_len == 0) {
                return IDNA_ERR_EMPTY_LABEL;
            }
            if (!idnaAppendBytes(out, &out_pos, nfc_buf[0..nfc_len])) {
                return IDNA_ERR_BUFFER_TOO_SMALL;
            }
        }

        if (dot < mapped_len) {
            if (dot + 1 == mapped_len) {
                if (!idnaAppendBytes(out, &out_pos, ".")) {
                    return IDNA_ERR_BUFFER_TOO_SMALL;
                }
                break;
            }
            if (!idnaAppendBytes(out, &out_pos, ".")) {
                return IDNA_ERR_BUFFER_TOO_SMALL;
            }
            pos = dot + 1;
        } else {
            break;
        }
    }

    return out_pos as i64;
}

/// Returns true if `domain` is a valid UTS #46 / RFC 1035 ASCII IDNA domain name
/// (all ASCII LDH labels, 1..=63 bytes per label, <= 253 total bytes, valid `xn--` Punycode).
pub fn isValidIdnaAscii(domain: string): bool {
    if (domain.len == 0 || !isAsciiOnly(domain)) {
        return false;
    }
    let eff_len = domain.len;
    if (domain[domain.len - 1] == ('.' as u8)) {
        if (domain.len == 1) {
            return false;
        }
        eff_len = domain.len - 1;
    }
    if (eff_len > IDNA_MAX_DOMAIN_LEN) {
        return false;
    }

    let pos: usize = 0;
    while (pos < eff_len) {
        let dot = pos;
        while (dot < eff_len && domain[dot] != ('.' as u8)) {
            dot = dot + 1;
        }
        const label = domain[pos..dot];
        if (label.len == 0 || label.len > IDNA_MAX_LABEL_LEN) {
            return false;
        }
        if (label[0] == ('-' as u8) || label[label.len - 1] == ('-' as u8)) {
            return false;
        }
        let i: usize = 0;
        while (i < label.len) {
            if (!idnaIsLdhAnyCase(label[i])) {
                return false;
            }
            i = i + 1;
        }
        if (isPunycodeLabel(label)) {
            if (label.len <= 4) {
                return false;
            }
            let tmp: [256]u8 = [0; 256];
            if (decodeLabel(label[4..label.len], tmp[0..256]) <= (0 as i64)) {
                return false;
            }
        } else if (label.len >= 4 && label[2] == ('-' as u8) && label[3] == ('-' as u8)) {
            return false;
        }
        pos = dot + 1;
    }
    return true;
}
