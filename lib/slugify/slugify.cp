// Source: Absorbed from /data/dev/bun/src/string/immutable.zig & Rust `slug`/`deunicode` crates
// Pure-CP zero-allocation URL-safe slug generator, Unicode Latin transliterator, validator, and word-boundary truncator.

/// Configuration options for `slugifyWithOptions` (modeled after Rust `slug` / `slugify` crates).
pub struct SlugOptions {
    /// Word separator byte (default `'-'`; e.g. `'_'` for snake_case slugs).
    separator: u8 = '-',
    /// Fold ASCII letters to lowercase (default `true`).
    lowercase: bool = true,
    /// Maximum output length in bytes; `0` means unlimited. When `> 0`, trims at separator word boundaries.
    max_len: usize = 0,
    /// Expand common symbols (`&` -> `and`, `@` -> `at`, `+` -> `plus`, `%` -> `percent`).
    expand_symbols: bool = false,
}

type SlugVec16 = @Vector(16, u8);

fn makeSlugLowerLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if (i >= 'A' && i <= 'Z') {
            lut[i] = @as(u8, @intCast(i + 32));
        } else {
            lut[i] = @as(u8, @intCast(i));
        }
    }
    return lut;
}

fn makeSlugClassLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    // bit 0: isAlnumLower ([a-z0-9])
    // bit 1: isAlnumAny ([A-Za-z0-9])
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        let flags: u8 = 0;
        if ((i >= 'a' && i <= 'z') || (i >= '0' && i <= '9')) {
            flags |= 3;
        } else if (i >= 'A' && i <= 'Z') {
            flags |= 2;
        }
        lut[i] = flags;
    }
    return lut;
}

const SLUG_LOWER_LUT: [u8; 256] = makeSlugLowerLut();
const SLUG_CLASS_LUT: [u8; 256] = makeSlugClassLut();

fn isAlnumLower(b: u8): bool {
    return (SLUG_CLASS_LUT[usize(b)] & 1) != 0;
}

fn isAlnumAny(b: u8): bool {
    return (SLUG_CLASS_LUT[usize(b)] & 2) != 0;
}

fn toLowerAscii(b: u8): u8 {
    return SLUG_LOWER_LUT[usize(b)];
}

struct DecodedChar {
    cp: u32,
    len: usize,
}

/// Decodes one UTF-8 codepoint starting at `input[pos]`.
/// Invalid byte sequences yield `cp = 0xFFFD, len = 1`.
fn decodeUtf8(input: []const u8, pos: usize): DecodedChar {
    const b0 = input[pos];
    if (b0 < 0x80) {
        return DecodedChar{ cp: b0 as u32, len: 1 };
    }
    if ((b0 & 0xE0) == 0xC0 && pos + 1 < input.len) {
        const b1 = input[pos + 1];
        if ((b1 & 0xC0) == 0x80) {
            const cp = (((b0 & 0x1F) as u32) << 6) | ((b1 & 0x3F) as u32);
            if (cp >= 0x80) return DecodedChar{ cp: cp, len: 2 };
        }
        return DecodedChar{ cp: 0xFFFD, len: 1 };
    }
    if ((b0 & 0xF0) == 0xE0 && pos + 2 < input.len) {
        const b1 = input[pos + 1];
        const b2 = input[pos + 2];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80) {
            const cp = (((b0 & 0x0F) as u32) << 12) | (((b1 & 0x3F) as u32) << 6) | ((b2 & 0x3F) as u32);
            if (cp >= 0x800) return DecodedChar{ cp: cp, len: 3 };
        }
        return DecodedChar{ cp: 0xFFFD, len: 1 };
    }
    if ((b0 & 0xF8) == 0xF0 && pos + 3 < input.len) {
        const b1 = input[pos + 1];
        const b2 = input[pos + 2];
        const b3 = input[pos + 3];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80) {
            const cp = (((b0 & 0x07) as u32) << 18)
                | (((b1 & 0x3F) as u32) << 12)
                | (((b2 & 0x3F) as u32) << 6)
                | ((b3 & 0x3F) as u32);
            if (cp >= 0x10000 && cp <= 0x10FFFF) return DecodedChar{ cp: cp, len: 4 };
        }
        return DecodedChar{ cp: 0xFFFD, len: 1 };
    }
    return DecodedChar{ cp: 0xFFFD, len: 1 };
}

/// Zero-allocation Latin-1 Supplement (U+00C0..U+00FF) & Latin Extended-A (U+0100..U+017F)
/// ASCII transliteration (modeled after Rust `deunicode`).
/// Returns `""` if the codepoint has no Latin ASCII transliteration.
pub fn deunicodeChar(cp: u32): []const u8 {
    // Latin-1 Supplement (U+00C0..U+00FF)
    if (cp >= 0x00C0 && cp <= 0x00C5) return "A";
    if (cp == 0x00C6) return "AE";
    if (cp == 0x00C7) return "C";
    if (cp >= 0x00C8 && cp <= 0x00CB) return "E";
    if (cp >= 0x00CC && cp <= 0x00CF) return "I";
    if (cp == 0x00D0) return "D";
    if (cp == 0x00D1) return "N";
    if ((cp >= 0x00D2 && cp <= 0x00D6) || cp == 0x00D8) return "O";
    if (cp >= 0x00D9 && cp <= 0x00DC) return "U";
    if (cp == 0x00DD) return "Y";
    if (cp == 0x00DE) return "Th";
    if (cp == 0x00DF) return "ss";
    if (cp >= 0x00E0 && cp <= 0x00E5) return "a";
    if (cp == 0x00E6) return "ae";
    if (cp == 0x00E7) return "c";
    if (cp >= 0x00E8 && cp <= 0x00EB) return "e";
    if (cp >= 0x00EC && cp <= 0x00EF) return "i";
    if (cp == 0x00F0) return "d";
    if (cp == 0x00F1) return "n";
    if ((cp >= 0x00F2 && cp <= 0x00F6) || cp == 0x00F8) return "o";
    if (cp >= 0x00F9 && cp <= 0x00FC) return "u";
    if (cp == 0x00FD || cp == 0x00FF) return "y";
    if (cp == 0x00FE) return "th";

    // Latin Extended-A (U+0100..U+017F)
    if (cp == 0x0100 || cp == 0x0102 || cp == 0x0104) return "A";
    if (cp == 0x0101 || cp == 0x0103 || cp == 0x0105) return "a";
    if (cp == 0x0106 || cp == 0x0108 || cp == 0x010A || cp == 0x010C) return "C";
    if (cp == 0x0107 || cp == 0x0109 || cp == 0x010B || cp == 0x010D) return "c";
    if (cp == 0x010E || cp == 0x0110) return "D";
    if (cp == 0x010F || cp == 0x0111) return "d";
    if (cp == 0x0112 || cp == 0x0114 || cp == 0x0116 || cp == 0x0118 || cp == 0x011A) return "E";
    if (cp == 0x0113 || cp == 0x0115 || cp == 0x0117 || cp == 0x0119 || cp == 0x011B) return "e";
    if (cp == 0x011C || cp == 0x011E || cp == 0x0120 || cp == 0x0122) return "G";
    if (cp == 0x011D || cp == 0x011F || cp == 0x0121 || cp == 0x0123) return "g";
    if (cp == 0x0124 || cp == 0x0126) return "H";
    if (cp == 0x0125 || cp == 0x0127) return "h";
    if (cp == 0x0128 || cp == 0x012A || cp == 0x012C || cp == 0x012E || cp == 0x0130) return "I";
    if (cp == 0x0129 || cp == 0x012B || cp == 0x012D || cp == 0x012F || cp == 0x0131) return "i";
    if (cp == 0x0132) return "IJ";
    if (cp == 0x0133) return "ij";
    if (cp == 0x0134) return "J";
    if (cp == 0x0135) return "j";
    if (cp == 0x0136) return "K";
    if (cp == 0x0137 || cp == 0x0138) return "k";
    if (cp == 0x0139 || cp == 0x013B || cp == 0x013D || cp == 0x013F || cp == 0x0141) return "L";
    if (cp == 0x013A || cp == 0x013C || cp == 0x013E || cp == 0x0140 || cp == 0x0142) return "l";
    if (cp == 0x0143 || cp == 0x0145 || cp == 0x0147 || cp == 0x014A) return "N";
    if (cp == 0x0144 || cp == 0x0146 || cp == 0x0148 || cp == 0x0149 || cp == 0x014B) return "n";
    if (cp == 0x014C || cp == 0x014E || cp == 0x0150) return "O";
    if (cp == 0x014D || cp == 0x014F || cp == 0x0151) return "o";
    if (cp == 0x0152) return "OE";
    if (cp == 0x0153) return "oe";
    if (cp == 0x0154 || cp == 0x0156 || cp == 0x0158) return "R";
    if (cp == 0x0155 || cp == 0x0157 || cp == 0x0159) return "r";
    if (cp == 0x015A || cp == 0x015C || cp == 0x015E || cp == 0x0160) return "S";
    if (cp == 0x015B || cp == 0x015D || cp == 0x015F || cp == 0x0161) return "s";
    if (cp == 0x0162 || cp == 0x0164 || cp == 0x0166) return "T";
    if (cp == 0x0163 || cp == 0x0165 || cp == 0x0167) return "t";
    if (cp == 0x0168 || cp == 0x016A || cp == 0x016C || cp == 0x016E || cp == 0x0170 || cp == 0x0172) return "U";
    if (cp == 0x0169 || cp == 0x016B || cp == 0x016D || cp == 0x016F || cp == 0x0171 || cp == 0x0173) return "u";
    if (cp == 0x0174) return "W";
    if (cp == 0x0175) return "w";
    if (cp == 0x0176 || cp == 0x0178) return "Y";
    if (cp == 0x0177) return "y";
    if (cp == 0x0179 || cp == 0x017B || cp == 0x017D) return "Z";
    if (cp == 0x017A || cp == 0x017C || cp == 0x017E) return "z";
    if (cp == 0x017F) return "s";

    // Common typographic punctuation & symbols
    if (cp == 0x2013 || cp == 0x2014) return "-";
    if (cp == 0x2026) return "...";
    if (cp == 0x00A9) return "(c)";
    if (cp == 0x00AE) return "(r)";
    if (cp == 0x2122) return "tm";

    return "";
}

/// Transliterates UTF-8 Latin diacritics into plain ASCII into `out`, preserving ASCII spacing & punctuation.
/// Returns `null` if `out` buffer is too small.
pub fn deunicode(input: []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    let i: usize = 0;
    const hi_mask: SlugVec16 = @splat(0x80 as u8);
    while (i < input.len) {
        if (i + 16 <= input.len && pos + 16 <= out.len) {
            const v: SlugVec16 = input[i .. i + 16][0..16].*;
            if (@reduce(.Or, v & hi_mask) == 0) {
                @memcpy(out[pos .. pos + 16], input[i .. i + 16]);
                pos += 16;
                i += 16;
                continue;
            }
        }
        const b = input[i];
        if (b < 0x80) {
            if (pos >= out.len) return null;
            out[pos] = b;
            pos += 1;
            i += 1;
        } else {
            const decoded = decodeUtf8(input, i);
            const mapped = deunicodeChar(decoded.cp);
            if (mapped.len > 0) {
                if (pos + mapped.len > out.len) return null;
                @memcpy(out[pos .. pos + mapped.len], mapped);
                pos += mapped.len;
            }
            i += decoded.len;
        }
    }
    return out[0..pos];
}

fn pushTokenBytes(
    bytes: []const u8,
    sep: u8,
    lowercase: bool,
    out: []u8,
    pos: *usize,
    pending_sep: *bool,
): bool {
    let k: usize = 0;
    while (k < bytes.len) {
        const raw = bytes[k];
        const c = if (lowercase) toLowerAscii(raw) else raw;
        if (isAlnumAny(c)) {
            if (pending_sep.* && pos.* > 0) {
                if (pos.* >= out.len) return false;
                out[pos.*] = sep;
                pos.* += 1;
            }
            pending_sep.* = false;
            if (pos.* >= out.len) return false;
            out[pos.*] = c;
            pos.* += 1;
        } else if (c != '\'') {
            if (pos.* > 0) pending_sep.* = true;
        }
        k += 1;
    }
    return true;
}

/// Full-featured zero-allocation slug generator with custom separator, case preservation,
/// Unicode Latin transliteration, optional symbol expansion, and word-boundary truncation.
pub fn slugifyWithOptions(input: []const u8, opts: SlugOptions, out: []u8): ([]const u8)? {
    const sep = opts.separator;
    let pos: usize = 0;
    let pending_sep = false;

    // When max_len > 0 we render into `out` up to its capacity, then word-boundary trim.
    let i: usize = 0;
    const v_a: SlugVec16 = @splat(97 as u8);
    const v_z: SlugVec16 = @splat(122 as u8);
    const v_A: SlugVec16 = @splat(65 as u8);
    const v_Z: SlugVec16 = @splat(90 as u8);
    const v_0: SlugVec16 = @splat(48 as u8);
    const v_9: SlugVec16 = @splat(57 as u8);
    const v_case: SlugVec16 = @splat(0x20 as u8);
    const v_zero: SlugVec16 = @splat(0 as u8);
    const v_one: SlugVec16 = @splat(1 as u8);

    while (i < input.len) {
        if (!pending_sep && i + 16 <= input.len && pos + 16 <= out.len) {
            const v: SlugVec16 = input[i .. i + 16][0..16].*;
            const is_lo = (v >= v_a) & (v <= v_z);
            const is_up = (v >= v_A) & (v <= v_Z);
            const is_dg = (v >= v_0) & (v <= v_9);
            const is_alnum = is_lo | is_up | is_dg;
            const bad = @select(u8, is_alnum, v_zero, v_one);
            if (@reduce(.Or, bad) == 0) {
                if (opts.lowercase) {
                    const lowered: [16]u8 = @select(u8, is_up, v | v_case, v);
                    @memcpy(out[pos .. pos + 16], lowered[0..16]);
                } else {
                    @memcpy(out[pos .. pos + 16], input[i .. i + 16]);
                }
                pos += 16;
                i += 16;
                continue;
            }
        }
        const b = input[i];
        if (b < 0x80) {
            if (opts.expand_symbols) {
                if (b == '&') {
                    if (pos > 0) pending_sep = true;
                    if (!pushTokenBytes("and", sep, opts.lowercase, out, &pos, &pending_sep)) return null;
                    pending_sep = true;
                    i += 1;
                    continue;
                }
                if (b == '@') {
                    if (pos > 0) pending_sep = true;
                    if (!pushTokenBytes("at", sep, opts.lowercase, out, &pos, &pending_sep)) return null;
                    pending_sep = true;
                    i += 1;
                    continue;
                }
                if (b == '+') {
                    if (pos > 0) pending_sep = true;
                    if (!pushTokenBytes("plus", sep, opts.lowercase, out, &pos, &pending_sep)) return null;
                    pending_sep = true;
                    i += 1;
                    continue;
                }
                if (b == '%') {
                    if (pos > 0) pending_sep = true;
                    if (!pushTokenBytes("percent", sep, opts.lowercase, out, &pos, &pending_sep)) return null;
                    pending_sep = true;
                    i += 1;
                    continue;
                }
            }
            const c = if (opts.lowercase) toLowerAscii(b) else b;
            if (isAlnumAny(c)) {
                if (pending_sep && pos > 0) {
                    if (pos >= out.len) return null;
                    out[pos] = sep;
                    pos += 1;
                }
                pending_sep = false;
                if (pos >= out.len) return null;
                out[pos] = c;
                pos += 1;
            } else if (c != '\'') {
                // Apostrophes (e.g. "don't") are dropped without inserting a separator
                if (pos > 0) pending_sep = true;
            }
            i += 1;
        } else {
            const decoded = decodeUtf8(input, i);
            // Smart/curly apostrophes U+2018 / U+2019 are dropped like ASCII '\''
            if (decoded.cp == 0x2018 || decoded.cp == 0x2019) {
                i += decoded.len;
                continue;
            }
            const mapped = deunicodeChar(decoded.cp);
            if (mapped.len > 0) {
                if (!pushTokenBytes(mapped, sep, opts.lowercase, out, &pos, &pending_sep)) return null;
            } else {
                if (pos > 0) pending_sep = true;
            }
            i += decoded.len;
        }
    }

    if (opts.max_len > 0 && pos > opts.max_len) {
        let cut = opts.max_len;
        let back = cut;
        while (back > 0 && out[back] != sep) {
            back -= 1;
        }
        if (back > 0 && out[back] == sep) {
            cut = back;
        }
        while (cut > 0 && out[cut - 1] == sep) {
            cut -= 1;
        }
        pos = cut;
    }

    return out[0..pos];
}

/// Generate a clean URL-safe ASCII slug using separator `sep` (typically `'-'` or `'_'`).
/// Lowercases ASCII letters, transliterates Latin Unicode diacritics, collapses consecutive
/// non-alnum runs into a single `sep`, and strips leading/trailing separators.
pub fn slugifyWithSep(input: []const u8, sep: u8, out: []u8): ([]const u8)? {
    return slugifyWithOptions(input, SlugOptions{ separator: sep }, out);
}

/// Generate a standard hyphenated URL slug (`'-'` separator).
pub fn slugify(input: []const u8, out: []u8): ([]const u8)? {
    return slugifyWithOptions(input, SlugOptions{}, out);
}

/// Generate a hyphenated URL slug capped to `max_len` bytes, trimming at a `-` word boundary when possible.
pub fn slugifyBounded(input: []const u8, max_len: usize, out: []u8): ([]const u8)? {
    if (max_len == 0) return out[0..0];
    let tmp: [1024]u8 = undefined;
    const full = slugifyWithOptions(input, SlugOptions{ max_len: max_len }, tmp[0..]) ?? return null;
    if (full.len > out.len) return null;
    @memcpy(out[0..full.len], full);
    return out[0..full.len];
}

/// Check if `s` is already a valid canonical slug for separator `sep` (`[a-z0-9]+(<sep>[a-z0-9]+)*`).
pub fn isValidSlugWithSep(s: []const u8, sep: u8): bool {
    if (s.len == 0) return false;
    if (s[0] == sep || s[s.len - 1] == sep) return false;
    let prev_sep = false;
    let i: usize = 0;
    const v_a: SlugVec16 = @splat(97 as u8);
    const v_z: SlugVec16 = @splat(122 as u8);
    const v_0: SlugVec16 = @splat(48 as u8);
    const v_9: SlugVec16 = @splat(57 as u8);
    const v_zero: SlugVec16 = @splat(0 as u8);
    const v_one: SlugVec16 = @splat(1 as u8);
    while (i < s.len) {
        if (!prev_sep && i + 16 <= s.len) {
            const v: SlugVec16 = s[i .. i + 16][0..16].*;
            const ok = ((v >= v_a) & (v <= v_z)) | ((v >= v_0) & (v <= v_9));
            if (@reduce(.Or, @select(u8, ok, v_zero, v_one)) == 0) {
                i += 16;
                continue;
            }
        }
        const c = s[i];
        if (c == sep) {
            if (prev_sep) return false;
            prev_sep = true;
        } else if (isAlnumLower(c)) {
            prev_sep = false;
        } else {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Check if `s` is already a valid canonical hyphenated slug (`[a-z0-9]+(-[a-z0-9]+)*`).
pub fn isValidSlug(s: []const u8): bool {
    return isValidSlugWithSep(s, '-');
}
