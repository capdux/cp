// Reference-hardened against servo/rust-url (MIT/Apache-2.0)
// Source: cp/lib/idna/idna.cp (Pure CP IDNA / UTS #46 Domain Conversion & RFC 1034/1123 Validation)
// Supports toAscii, toUnicode, isValidDomain, normalizeDomain, CheckHyphens, and 16-lane SIMD ASCII fast-paths.

const std = @import("std");
import {
    IdnaError,
    decodeUtf8Codepoints,
    punycodeEncode,
    punycodeDecode,
    simdIdnaIsAscii,
    simdIdnaIndexOfDot,
    simdIdnaToLowerAscii,
} from "./punycode.cp";

pub const MAX_LABEL_LEN: usize = 63;
pub const MAX_DOMAIN_LEN: usize = 253;

struct OutBuf {
    items: []u8 = [],
    len: usize = 0,

    fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.items.len > 0) {
            alloc.free(_.items);
            _.items = [];
            _.len = 0;
        }
    }

    fn ensureCap(*_, alloc: std.mem.Allocator, needed: usize): (void) ! IdnaError {
        if (needed <= _.items.len) {
            return;
        }
        let new_cap: usize = if (_.items.len == 0) 64 else _.items.len * 2;
        while (new_cap < needed) {
            new_cap *= 2;
        }
        if (_.items.len == 0) {
            _.items = alloc.alloc(u8, new_cap) catch return IdnaError.OutOfMemory;
        } else {
            _.items = alloc.realloc(_.items, new_cap) catch return IdnaError.OutOfMemory;
        }
    }

    fn appendByte(*_, alloc: std.mem.Allocator, b: u8): (void) ! IdnaError {
        _.ensureCap(alloc, _.len + 1)?;
        _.items[_.len] = b;
        _.len += 1;
    }

    fn appendSlice(*_, alloc: std.mem.Allocator, s: []const u8): (void) ! IdnaError {
        if (s.len == 0) {
            return;
        }
        _.ensureCap(alloc, _.len + s.len)?;
        @memcpy(_.items[_.len .. _.len + s.len], s);
        _.len += s.len;
    }

    fn toOwnedSlice(*_, alloc: std.mem.Allocator): ([]u8) ! IdnaError {
        const out = alloc.alloc(u8, _.len) catch return IdnaError.OutOfMemory;
        if (_.len > 0) {
            @memcpy(out, _.items[0.._.len]);
        }
        _.deinit(alloc);
        return out;
    }
}

fn idnaToAsciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn idnaIsValidAsciiLabelChar(c: u8): bool {
    if (c >= 'a' && c <= 'z') return true;
    if (c >= 'A' && c <= 'Z') return true;
    if (c >= '0' && c <= '9') return true;
    if (c == '-') return true;
    return false;
}

fn idnaHasXnPrefix(label: []const u8): bool {
    if (label.len < 4) {
        return false;
    }
    return idnaToAsciiLower(label[0]) == 'x'
        && idnaToAsciiLower(label[1]) == 'n'
        && label[2] == '-'
        && label[3] == '-';
}

fn idnaHasReservedHyphens(label: []const u8): bool {
    return label.len >= 4 && label[2] == '-' && label[3] == '-';
}

fn idnaHasNonAscii(s: []const u8): bool {
    return !simdIdnaIsAscii(s);
}

/// Checks if `domain` at `pos` starts with a recognized IDNA label separator:
/// `.` (0x2E), `\u{3002}` (E3 80 82), `\u{FF0E}` (EF BC 8E), `\u{FF61}` (EF BD A1).
/// Returns separator byte length (0 if not a separator).
fn separatorLenAt(domain: []const u8, pos: usize): usize {
    const b0 = domain[pos];
    if (b0 == '.') {
        return 1;
    }
    if (pos + 2 < domain.len) {
        const b1 = domain[pos + 1];
        const b2 = domain[pos + 2];
        if (b0 == 0xE3 && b1 == 0x80 && b2 == 0x82) return 3;
        if (b0 == 0xEF && b1 == 0xBC && b2 == 0x8E) return 3;
        if (b0 == 0xEF && b1 == 0xBD && b2 == 0xA1) return 3;
    }
    return 0;
}

fn foldUnicodeCase(cp: u32): u32 {
    if (cp >= 'A' && cp <= 'Z') {
        return cp + 32;
    }
    // Latin-1 Supplement uppercase letters: U+00C0..U+00D6 and U+00D8..U+00DE
    if ((cp >= 0x00C0 && cp <= 0x00D6) || (cp >= 0x00D8 && cp <= 0x00DE)) {
        return cp + 32;
    }
    return cp;
}

fn isDisallowedBidiOrControl(cp: u32): bool {
    // Explicit directional formatting characters (LRE, RLE, PDF, LRO, RLO, LRI, RLI, FSI, PDI, LRM, RLM)
    if (cp == 0x200E || cp == 0x200F) return true;
    if (cp >= 0x202A && cp <= 0x202E) return true;
    if (cp >= 0x2066 && cp <= 0x2069) return true;
    if (cp == 0x061C) return true;
    return false;
}

fn isRtlCodepoint(cp: u32): bool {
    if (cp >= 0x0590 && cp <= 0x08FF) return true;
    if (cp >= 0xFB1D && cp <= 0xFDFF) return true;
    if (cp >= 0xFE70 && cp <= 0xFEFF) return true;
    return false;
}

fn isLtrCodepoint(cp: u32): bool {
    if (cp >= 'a' && cp <= 'z') return true;
    if (cp >= 'A' && cp <= 'Z') return true;
    if (cp >= 0x00C0 && cp <= 0x024F) return true;
    return false;
}

/// Validates and case-folds a UTF-8 non-ASCII label, returning a newly allocated folded UTF-8 slice.
/// Enforces UTS #46 CheckHyphens (no leading/trailing `-` and no `--` in 3rd-4th positions).
fn prepareUnicodeLabel(alloc: std.mem.Allocator, label: []const u8): ([]u8) ! IdnaError {
    const cps = decodeUtf8Codepoints(alloc, label)?;
    defer alloc.free(cps);

    if (cps.len == 0) {
        return IdnaError.EmptyLabel;
    }

    if (cps[0] == '-' || cps[cps.len - 1] == '-') {
        return IdnaError.LeadingOrTrailingHyphen;
    }
    if (cps.len >= 4 && cps[2] == '-' && cps[3] == '-') {
        return IdnaError.ReservedHyphens;
    }

    let has_rtl = false;
    let has_ltr = false;
    let buf = OutBuf{};
    errdefer buf.deinit(alloc);

    let i: usize = 0;
    while (i < cps.len) : (i += 1) {
        const cp = foldUnicodeCase(cps[i]);
        if (isDisallowedBidiOrControl(cp)) {
            return IdnaError.DisallowedBidirection;
        }
        if (cp < 0x80) {
            if (!idnaIsValidAsciiLabelChar(cp as u8)) {
                return IdnaError.InvalidChar;
            }
        }
        if (isRtlCodepoint(cp)) has_rtl = true;
        if (isLtrCodepoint(cp)) has_ltr = true;

        // Encode folded codepoint to UTF-8
        if (cp < 0x80) {
            buf.appendByte(alloc, cp as u8)?;
        } else if (cp < 0x800) {
            buf.appendByte(alloc, (0xC0 | (cp >> 6)) as u8)?;
            buf.appendByte(alloc, (0x80 | (cp & 0x3F)) as u8)?;
        } else if (cp < 0x10000) {
            buf.appendByte(alloc, (0xE0 | (cp >> 12)) as u8)?;
            buf.appendByte(alloc, (0x80 | ((cp >> 6) & 0x3F)) as u8)?;
            buf.appendByte(alloc, (0x80 | (cp & 0x3F)) as u8)?;
        } else {
            buf.appendByte(alloc, (0xF0 | (cp >> 18)) as u8)?;
            buf.appendByte(alloc, (0x80 | ((cp >> 12) & 0x3F)) as u8)?;
            buf.appendByte(alloc, (0x80 | ((cp >> 6) & 0x3F)) as u8)?;
            buf.appendByte(alloc, (0x80 | (cp & 0x3F)) as u8)?;
        }
    }

    if (has_rtl && has_ltr) {
        return IdnaError.DisallowedBidirection;
    }

    return buf.toOwnedSlice(alloc);
}

fn validateAndLowerAsciiLabel(alloc: std.mem.Allocator, label: []const u8): ([]u8) ! IdnaError {
    if (label.len == 0) {
        return IdnaError.EmptyLabel;
    }
    if (label.len > MAX_LABEL_LEN) {
        return IdnaError.LabelTooLong;
    }
    if (label[0] == '-' || label[label.len - 1] == '-') {
        return IdnaError.LeadingOrTrailingHyphen;
    }

    let out = alloc.alloc(u8, label.len) catch return IdnaError.OutOfMemory;
    errdefer alloc.free(out);

    // 16-lane SIMD batch ASCII lowercase folding
    * = simdIdnaToLowerAscii(label, out);

    let i: usize = 0;
    while (i < out.len) : (i += 1) {
        if (!idnaIsValidAsciiLabelChar(out[i])) {
            return IdnaError.InvalidChar;
        }
    }

    if (idnaHasXnPrefix(out)) {
        if (out.len == 4) {
            return IdnaError.InvalidPunycode;
        }
        const decoded_chk = punycodeDecode(alloc, out[4..out.len])?;
        defer alloc.free(decoded_chk);
        if (!idnaHasNonAscii(decoded_chk)) {
            return IdnaError.InvalidPunycode;
        }
    } else if (idnaHasReservedHyphens(out)) {
        // UTS #46 CheckHyphens: disallow non-xn-- labels with '--' in positions 3-4
        return IdnaError.ReservedHyphens;
    }

    return out;
}

/// Converts an internationalized domain name to its ASCII (Punycode A-label) form.
/// Splits labels on `.`, `\u{3002}`, `\u{FF0E}`, and `\u{FF61}`.
/// Enforces RFC 1034/1123 + UTS #46 CheckHyphens (`<= 63` bytes/label, `<= 253` bytes total excluding optional trailing root dot).
pub fn toAscii(alloc: std.mem.Allocator, domain: []const u8): ([]u8) ! IdnaError {
    if (domain.len == 0) {
        return IdnaError.EmptyLabel;
    }

    let out = OutBuf{};
    errdefer out.deinit(alloc);

    // 16-lane SIMD pure-ASCII fast path (uses simdIdnaIndexOfDot + simdIdnaToLowerAscii)
    if (simdIdnaIsAscii(domain)) {
        let start: usize = 0;
        let has_trailing_dot = false;
        let label_count: usize = 0;

        while (start <= domain.len) {
            const rel_dot = simdIdnaIndexOfDot(domain[start..domain.len]);
            const end_pos = if (rel_dot != null) (start + rel_dot.?) else domain.len;
            const at_end = rel_dot == null;
            const label = domain[start..end_pos];

            if (label.len == 0) {
                if (at_end && label_count > 0) {
                    has_trailing_dot = true;
                    break;
                }
                return IdnaError.EmptyLabel;
            }

            if (label_count > 0) {
                out.appendByte(alloc, '.')?;
            }
            label_count += 1;

            const ascii_lbl = validateAndLowerAsciiLabel(alloc, label)?;
            defer alloc.free(ascii_lbl);
            out.appendSlice(alloc, ascii_lbl)?;

            if (at_end) {
                break;
            }
            start = end_pos + 1;
        }

        if (out.len > MAX_DOMAIN_LEN) {
            return IdnaError.DomainTooLong;
        }
        if (has_trailing_dot) {
            out.appendByte(alloc, '.')?;
        }
        return out.toOwnedSlice(alloc);
    }

    let label_start: usize = 0;
    let pos: usize = 0;
    let has_trailing_dot = false;
    let label_count: usize = 0;

    while (pos <= domain.len) {
        let sep_len: usize = 0;
        let at_end = (pos == domain.len);
        if (!at_end) {
            sep_len = separatorLenAt(domain, pos);
        }

        if (at_end || sep_len > 0) {
            const label = domain[label_start..pos];
            if (label.len == 0) {
                if (at_end && label_count > 0) {
                    has_trailing_dot = true;
                    break;
                }
                return IdnaError.EmptyLabel;
            }

            if (label_count > 0) {
                out.appendByte(alloc, '.')?;
            }
            label_count += 1;

            if (idnaHasNonAscii(label)) {
                const folded = prepareUnicodeLabel(alloc, label)?;
                defer alloc.free(folded);

                const encoded = punycodeEncode(alloc, folded)?;
                defer alloc.free(encoded);

                const full_len = 4 + encoded.len;
                if (full_len > MAX_LABEL_LEN) {
                    return IdnaError.LabelTooLong;
                }
                out.appendSlice(alloc, "xn--")?;
                out.appendSlice(alloc, encoded)?;
            } else {
                const ascii_lbl = validateAndLowerAsciiLabel(alloc, label)?;
                defer alloc.free(ascii_lbl);
                out.appendSlice(alloc, ascii_lbl)?;
            }

            if (at_end) {
                break;
            }
            pos += sep_len;
            label_start = pos;
        } else {
            pos += 1;
        }
    }

    if (out.len > MAX_DOMAIN_LEN) {
        return IdnaError.DomainTooLong;
    }
    if (has_trailing_dot) {
        out.appendByte(alloc, '.')?;
    }

    return out.toOwnedSlice(alloc);
}

/// Converts an ASCII or mixed domain name to its Unicode (U-label) form.
/// Decodes `xn--` (case-insensitive) labels via RFC 3492 Punycode.
pub fn toUnicode(alloc: std.mem.Allocator, domain: []const u8): ([]u8) ! IdnaError {
    if (domain.len == 0) {
        return IdnaError.EmptyLabel;
    }

    let out = OutBuf{};
    errdefer out.deinit(alloc);

    let ascii_len_check: usize = 0;
    let label_start: usize = 0;
    let pos: usize = 0;
    let has_trailing_dot = false;
    let label_count: usize = 0;

    while (pos <= domain.len) {
        let sep_len: usize = 0;
        let at_end = (pos == domain.len);
        if (!at_end) {
            sep_len = separatorLenAt(domain, pos);
        }

        if (at_end || sep_len > 0) {
            const label = domain[label_start..pos];
            if (label.len == 0) {
                if (at_end && label_count > 0) {
                    has_trailing_dot = true;
                    break;
                }
                return IdnaError.EmptyLabel;
            }

            if (label_count > 0) {
                out.appendByte(alloc, '.')?;
                ascii_len_check += 1;
            }
            label_count += 1;

            if (idnaHasNonAscii(label)) {
                const folded = prepareUnicodeLabel(alloc, label)?;
                defer alloc.free(folded);

                const encoded = punycodeEncode(alloc, folded)?;
                defer alloc.free(encoded);

                if (4 + encoded.len > MAX_LABEL_LEN) {
                    return IdnaError.LabelTooLong;
                }
                ascii_len_check += 4 + encoded.len;
                out.appendSlice(alloc, folded)?;
            } else if (idnaHasXnPrefix(label)) {
                if (label.len > MAX_LABEL_LEN) {
                    return IdnaError.LabelTooLong;
                }
                if (label.len == 4) {
                    return IdnaError.InvalidPunycode;
                }
                // Validate ASCII characters in xn-- label
                let j: usize = 0;
                while (j < label.len) : (j += 1) {
                    if (!idnaIsValidAsciiLabelChar(label[j])) {
                        return IdnaError.InvalidChar;
                    }
                }
                if (label[label.len - 1] == '-') {
                    return IdnaError.LeadingOrTrailingHyphen;
                }

                const decoded = punycodeDecode(alloc, label[4..label.len])?;
                defer alloc.free(decoded);

                if (!idnaHasNonAscii(decoded)) {
                    return IdnaError.InvalidPunycode;
                }
                const validated = prepareUnicodeLabel(alloc, decoded)?;
                defer alloc.free(validated);

                ascii_len_check += label.len;
                out.appendSlice(alloc, validated)?;
            } else {
                const ascii_lbl = validateAndLowerAsciiLabel(alloc, label)?;
                defer alloc.free(ascii_lbl);
                ascii_len_check += ascii_lbl.len;
                out.appendSlice(alloc, ascii_lbl)?;
            }

            if (at_end) {
                break;
            }
            pos += sep_len;
            label_start = pos;
        } else {
            pos += 1;
        }
    }

    if (ascii_len_check > MAX_DOMAIN_LEN) {
        return IdnaError.DomainTooLong;
    }
    if (has_trailing_dot) {
        out.appendByte(alloc, '.')?;
    }

    return out.toOwnedSlice(alloc);
}

/// Validates whether `domain` is a compliant IDNA / RFC 1034 / RFC 1123 domain name.
/// Performs zero heap allocations on the caller's behalf using a fixed stack buffer.
pub fn isValidDomain(domain: []const u8): bool {
    if (domain.len == 0 || domain.len > 1024) {
        return false;
    }
    let stack_buf: [4096]u8 = [0; 4096];
    let fba = std.heap.FixedBufferAllocator.init(&stack_buf);
    const alloc = fba.allocator();
    const res = toAscii(alloc, domain) catch return false;
    alloc.free(res);
    return true;
}

/// Normalizes a domain name: folds ASCII/Latin-1 case to lowercase, converts IDNA dot
/// separators (`\u{3002}`, `\u{FF0E}`, `\u{FF61}`) to `.`, and validates all labels.
pub fn normalizeDomain(alloc: std.mem.Allocator, domain: []const u8): ([]u8) ! IdnaError {
    if (domain.len == 0) {
        return IdnaError.EmptyLabel;
    }

    if (simdIdnaIsAscii(domain)) {
        return toAscii(alloc, domain);
    }

    let out = OutBuf{};
    errdefer out.deinit(alloc);

    let ascii_len_check: usize = 0;
    let label_start: usize = 0;
    let pos: usize = 0;
    let has_trailing_dot = false;
    let label_count: usize = 0;

    while (pos <= domain.len) {
        let sep_len: usize = 0;
        let at_end = (pos == domain.len);
        if (!at_end) {
            sep_len = separatorLenAt(domain, pos);
        }

        if (at_end || sep_len > 0) {
            const label = domain[label_start..pos];
            if (label.len == 0) {
                if (at_end && label_count > 0) {
                    has_trailing_dot = true;
                    break;
                }
                return IdnaError.EmptyLabel;
            }

            if (label_count > 0) {
                out.appendByte(alloc, '.')?;
                ascii_len_check += 1;
            }
            label_count += 1;

            if (idnaHasNonAscii(label)) {
                const folded = prepareUnicodeLabel(alloc, label)?;
                defer alloc.free(folded);

                const encoded = punycodeEncode(alloc, folded)?;
                defer alloc.free(encoded);

                if (4 + encoded.len > MAX_LABEL_LEN) {
                    return IdnaError.LabelTooLong;
                }
                ascii_len_check += 4 + encoded.len;
                out.appendSlice(alloc, folded)?;
            } else {
                const ascii_lbl = validateAndLowerAsciiLabel(alloc, label)?;
                defer alloc.free(ascii_lbl);
                ascii_len_check += ascii_lbl.len;
                out.appendSlice(alloc, ascii_lbl)?;
            }

            if (at_end) {
                break;
            }
            pos += sep_len;
            label_start = pos;
        } else {
            pos += 1;
        }
    }

    if (ascii_len_check > MAX_DOMAIN_LEN) {
        return IdnaError.DomainTooLong;
    }
    if (has_trailing_dot) {
        out.appendByte(alloc, '.')?;
    }

    return out.toOwnedSlice(alloc);
}
