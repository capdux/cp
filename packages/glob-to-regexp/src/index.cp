// @cap/glob-to-regex — Zero-allocation Glob-to-RegExp Converter
// Clean-room MIT implementation in pure CP.
// Supports extended globs (?, [abc], [!abc], {a,b}), globstar (**, /**/), and anchoring (^...$).

pub struct GlobOptions {
    pub extended: bool,
    pub globstar: bool,
    pub anchored: bool,
}

fn pushByte(dst: []u8, pos: usize, b: u8) -> i64 {
    if (pos >= dst.len) {
        return -1;
    }
    dst[pos] = b;
    return (pos + 1) as i64;
}

fn pushSlice(dst: []u8, pos: usize, src: []const u8) -> i64 {
    if (pos + src.len > dst.len) {
        return -1;
    }
    let k: usize = 0;
    while (k < src.len) {
        dst[pos + k] = src[k];
        k += 1;
    }
    return (pos + src.len) as i64;
}

fn isEscapeMeta(b: u8) -> bool {
    // . + ^ $ ( ) |
    return b == 46 || b == 43 || b == 94 || b == 36 || b == 40 || b == 41 || b == 124;
}

/// Converts a POSIX/glob pattern into an ECMAScript/PCRE regular expression string in `dst`.
/// Returns bytes written, or `-1` if `dst` is too small.
pub fn globToRegex(glob: []const u8, opts: GlobOptions, dst: []u8) -> i64 {
    let out: usize = 0;
    let p: i64 = 0;

    if (opts.anchored) {
        p = pushByte(dst, out, 94); // '^'
        if (p < 0) {
            return -1;
        }
        out = p as usize;
    }

    let in_group = false;
    let in_class = false;
    let i: usize = 0;
    const n = glob.len;

    while (i < n) {
        const ch = glob[i];

        if (in_class) {
            if (ch == 93) {
                // ']'
                p = pushByte(dst, out, 93);
                if (p < 0) { return -1; }
                out = p as usize;
                in_class = false;
                i += 1;
            } else if (ch == 92 && i + 1 < n) {
                p = pushByte(dst, out, 92);
                if (p < 0) { return -1; }
                out = p as usize;
                p = pushByte(dst, out, glob[i + 1]);
                if (p < 0) { return -1; }
                out = p as usize;
                i += 2;
            } else {
                p = pushByte(dst, out, ch);
                if (p < 0) { return -1; }
                out = p as usize;
                i += 1;
            }
            continue;
        }

        // Backslash escape
        if (ch == 92) {
            if (i + 1 < n) {
                p = pushByte(dst, out, 92);
                if (p < 0) { return -1; }
                out = p as usize;
                p = pushByte(dst, out, glob[i + 1]);
                if (p < 0) { return -1; }
                out = p as usize;
                i += 2;
            } else {
                p = pushSlice(dst, out, "\\\\");
                if (p < 0) { return -1; }
                out = p as usize;
                i += 1;
            }
            continue;
        }

        // RegExp metacharacters to escape verbatim
        if (isEscapeMeta(ch)) {
            p = pushByte(dst, out, 92);
            if (p < 0) { return -1; }
            out = p as usize;
            p = pushByte(dst, out, ch);
            if (p < 0) { return -1; }
            out = p as usize;
            i += 1;
            continue;
        }

        // '?'
        if (ch == 63) {
            if (opts.extended) {
                p = pushByte(dst, out, 46); // '.'
            } else {
                p = pushSlice(dst, out, "\\?");
            }
            if (p < 0) { return -1; }
            out = p as usize;
            i += 1;
            continue;
        }

        // '['
        if (ch == 91) {
            if (opts.extended) {
                p = pushByte(dst, out, 91);
                if (p < 0) { return -1; }
                out = p as usize;
                in_class = true;
                i += 1;
                if (i < n && glob[i] == 33) {
                    // '[!' -> '[^'
                    p = pushByte(dst, out, 94);
                    if (p < 0) { return -1; }
                    out = p as usize;
                    i += 1;
                }
            } else {
                p = pushSlice(dst, out, "\\[");
                if (p < 0) { return -1; }
                out = p as usize;
                i += 1;
            }
            continue;
        }

        // ']' outside character class
        if (ch == 93) {
            p = pushSlice(dst, out, "\\]");
            if (p < 0) { return -1; }
            out = p as usize;
            i += 1;
            continue;
        }

        // '{'
        if (ch == 123) {
            if (opts.extended && !in_group) {
                p = pushByte(dst, out, 40); // '('
                in_group = true;
            } else {
                p = pushSlice(dst, out, "\\{");
            }
            if (p < 0) { return -1; }
            out = p as usize;
            i += 1;
            continue;
        }

        // '}'
        if (ch == 125) {
            if (opts.extended && in_group) {
                p = pushByte(dst, out, 41); // ')'
                in_group = false;
            } else {
                p = pushSlice(dst, out, "\\}");
            }
            if (p < 0) { return -1; }
            out = p as usize;
            i += 1;
            continue;
        }

        // ','
        if (ch == 44) {
            if (opts.extended && in_group) {
                p = pushByte(dst, out, 124); // '|'
            } else {
                p = pushByte(dst, out, 44);
            }
            if (p < 0) { return -1; }
            out = p as usize;
            i += 1;
            continue;
        }

        // '/' followed by '**/' in globstar mode
        if (ch == 47 && opts.globstar && i + 3 < n && glob[i + 1] == 42 && glob[i + 2] == 42 && glob[i + 3] == 47) {
            p = pushSlice(dst, out, "(?:/.+/|/)");
            if (p < 0) { return -1; }
            out = p as usize;
            i += 4;
            continue;
        }

        // '*'
        if (ch == 42) {
            let star_count: usize = 0;
            while (i + star_count < n && glob[i + star_count] == 42) {
                star_count += 1;
            }
            if (!opts.globstar) {
                p = pushSlice(dst, out, ".*");
                if (p < 0) { return -1; }
                out = p as usize;
                i += star_count;
            } else if (star_count >= 2) {
                if (i == 0 && i + star_count < n && glob[i + star_count] == 47) {
                    // Leading "**/" matches zero or more directories
                    p = pushSlice(dst, out, "(?:.+/)?");
                    if (p < 0) { return -1; }
                    out = p as usize;
                    i += star_count + 1;
                } else {
                    p = pushSlice(dst, out, ".*");
                    if (p < 0) { return -1; }
                    out = p as usize;
                    i += star_count;
                }
            } else {
                p = pushSlice(dst, out, "[^/]*");
                if (p < 0) { return -1; }
                out = p as usize;
                i += 1;
            }
            continue;
        }

        // Literal character
        p = pushByte(dst, out, ch);
        if (p < 0) { return -1; }
        out = p as usize;
        i += 1;
    }

    if (opts.anchored) {
        p = pushByte(dst, out, 36); // '$'
        if (p < 0) {
            return -1;
        }
        out = p as usize;
    }

    return out as i64;
}

/// Converts a glob pattern with default options (`extended=true, globstar=true, anchored=true`).
pub fn globToRegexDefault(glob: []const u8, dst: []u8) -> i64 {
    return globToRegex(
        glob,
        GlobOptions {
            extended: true,
            globstar: true,
            anchored: true,
        },
        dst,
    );
}
