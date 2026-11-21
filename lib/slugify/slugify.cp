// Source: Absorbed from /data/dev/bun/src/string/immutable.zig (URL Slug Generator & Validator)
// Pure-CP zero-allocation URL-safe slug generator, validator, and word-boundary truncator.

fn isAlnumLower(b: u8): bool {
    return (b >= 'a' && b <= 'z') || (b >= '0' && b <= '9');
}

fn toLowerAscii(b: u8): u8 {
    if (b >= 'A' && b <= 'Z') return b + 32;
    return b;
}

/// Generate a clean URL-safe ASCII slug using separator `sep` (typically `'-'` or `'_'`).
/// Lowercases ASCII letters, collapses consecutive non-alnum runs into a single `sep`,
/// and strips leading/trailing separators.
export fn slugifyWithSep(input: []const u8, sep: u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    let pending_sep = false;
    let i: usize = 0;
    while (i < input.len) {
        const c = toLowerAscii(input[i]);
        if (isAlnumLower(c)) {
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
            // Apostrophes (e.g. "don't") are dropped without inserting a separator; other punctuation separates
            if (pos > 0) pending_sep = true;
        }
        i += 1;
    }
    return out[0..pos];
}

/// Generate a standard hyphenated URL slug (`'-'` separator).
export fn slugify(input: []const u8, out: []u8): ([]const u8)? {
    return slugifyWithSep(input, '-', out);
}

/// Generate a hyphenated URL slug capped to `max_len` bytes, trimming at a `-` word boundary when possible.
export fn slugifyBounded(input: []const u8, max_len: usize, out: []u8): ([]const u8)? {
    if (max_len == 0) return out[0..0];
    let tmp: [512]u8 = [0; 512];
    const full = slugifyWithSep(input, '-', tmp[0..]) ?? return null;
    let cut = full.len;
    if (cut > max_len) {
        cut = max_len;
        // Prefer breaking at last '-' if one exists in the second half of the budget
        let back = cut;
        while (back > 0 && full[back] != '-') {
            back -= 1;
        }
        if (back > 0 && full[back] == '-') {
            cut = back;
        }
        while (cut > 0 && full[cut - 1] == '-') {
            cut -= 1;
        }
    }
    if (cut > out.len) return null;
    let i: usize = 0;
    while (i < cut) {
        out[i] = full[i];
        i += 1;
    }
    return out[0..cut];
}

/// Check if `s` is already a valid canonical hyphenated slug (`[a-z0-9]+(-[a-z0-9]+)*`).
export fn isValidSlug(s: []const u8): bool {
    if (s.len == 0) return false;
    if (s[0] == '-' || s[s.len - 1] == '-') return false;
    let prev_dash = false;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '-') {
            if (prev_dash) return false;
            prev_dash = true;
        } else if (isAlnumLower(c)) {
            prev_dash = false;
        } else {
            return false;
        }
        i += 1;
    }
    return true;
}
