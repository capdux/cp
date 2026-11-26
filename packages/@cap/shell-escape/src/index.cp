// @cap/shell-escape — Zero-allocation POSIX sh & Windows CommandLineToArgvW Argument Escaper
// Clean-room MIT implementation in pure CP.

fn isSafePosixByte(b: u8) -> bool {
    if (b >= 65 && b <= 90) { return true; }  // A-Z
    if (b >= 97 && b <= 122) { return true; } // a-z
    if (b >= 48 && b <= 57) { return true; }  // 0-9
    // _ @ % + = : , . / -
    if (b == 95 || b == 64 || b == 37 || b == 43 || b == 61) { return true; }
    if (b == 58 || b == 44 || b == 46 || b == 47 || b == 45) { return true; }
    return false;
}

/// Returns true if `arg` is non-empty and contains only safe POSIX shell characters
/// (`A-Za-z0-9_@%+=:,./-`) requiring no quoting.
pub fn isSafePosixWord(arg: string) -> bool {
    if (arg.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < arg.len) {
        if (!isSafePosixByte(arg[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Escapes `arg` for safe POSIX sh interpolation into `out`.
/// Empty string becomes `''`; safe words pass through verbatim; other strings are single-quoted
/// with internal `'` replaced by `'\''`. Returns bytes written, or `0` if `out` is too small.
pub fn escapePosix(arg: string, out: []u8) -> usize {
    if (arg.len == 0) {
        if (out.len < 2) {
            return 0;
        }
        out[0] = 39; // '\''
        out[1] = 39;
        return 2;
    }

    if (isSafePosixWord(arg)) {
        if (arg.len > out.len) {
            return 0;
        }
        let i: usize = 0;
        while (i < arg.len) {
            out[i] = arg[i];
            i += 1;
        }
        return arg.len;
    }

    let needed: usize = 2;
    let i: usize = 0;
    while (i < arg.len) {
        if (arg[i] == 39) {
            needed += 4; // '\''
        } else {
            needed += 1;
        }
        i += 1;
    }
    if (needed > out.len) {
        return 0;
    }

    let w: usize = 0;
    out[w] = 39;
    w += 1;
    i = 0;
    while (i < arg.len) {
        if (arg[i] == 39) {
            out[w] = 39;     // '\''
            out[w + 1] = 92; // '\\'
            out[w + 2] = 39; // '\''
            out[w + 3] = 39; // '\''
            w += 4;
        } else {
            out[w] = arg[i];
            w += 1;
        }
        i += 1;
    }
    out[w] = 39;
    w += 1;
    return w;
}

fn needsWindowsQuotes(arg: string) -> bool {
    if (arg.len == 0) {
        return true;
    }
    let i: usize = 0;
    while (i < arg.len) {
        const b = arg[i];
        if (b == 32 || b == 9 || b == 10 || b == 13 || b == 34) {
            return true;
        }
        i += 1;
    }
    return false;
}

/// Escapes `arg` following Microsoft's `CommandLineToArgvW` quoting rules:
/// wraps empty strings or arguments with whitespace/quotes in `"..."`, doubling backslashes
/// that immediately precede `"` or the closing quote. Returns bytes written, or `0` on overflow.
pub fn escapeWindows(arg: string, out: []u8) -> usize {
    if (arg.len == 0) {
        if (out.len < 2) {
            return 0;
        }
        out[0] = 34; // '"'
        out[1] = 34;
        return 2;
    }

    if (!needsWindowsQuotes(arg)) {
        if (arg.len > out.len) {
            return 0;
        }
        let i: usize = 0;
        while (i < arg.len) {
            out[i] = arg[i];
            i += 1;
        }
        return arg.len;
    }

    let w: usize = 0;
    if (w >= out.len) { return 0; }
    out[w] = 34; // opening '"'
    w += 1;

    let bs_count: usize = 0;
    let i: usize = 0;
    while (i < arg.len) {
        const ch = arg[i];
        if (ch == 92) {
            // '\\'
            bs_count += 1;
        } else if (ch == 34) {
            // '"' -> emit 2*bs_count + 1 backslashes then '"'
            const total_bs = bs_count * 2 + 1;
            if (w + total_bs + 1 > out.len) { return 0; }
            let k: usize = 0;
            while (k < total_bs) {
                out[w] = 92;
                w += 1;
                k += 1;
            }
            out[w] = 34;
            w += 1;
            bs_count = 0;
        } else {
            if (w + bs_count + 1 > out.len) { return 0; }
            let k: usize = 0;
            while (k < bs_count) {
                out[w] = 92;
                w += 1;
                k += 1;
            }
            out[w] = ch;
            w += 1;
            bs_count = 0;
        }
        i += 1;
    }

    // Trailing backslashes before closing quote must be doubled
    const tail_bs = bs_count * 2;
    if (w + tail_bs + 1 > out.len) { return 0; }
    let k: usize = 0;
    while (k < tail_bs) {
        out[w] = 92;
        w += 1;
        k += 1;
    }
    out[w] = 34; // closing '"'
    w += 1;
    return w;
}

/// Joins an argument slice into a POSIX sh command string separated by spaces,
/// escaping each argument with `escapePosix`. Returns bytes written, or `0` on empty/overflow.
pub fn joinPosixCommand(args: []const string, out: []u8) -> usize {
    if (args.len == 0) {
        return 0;
    }
    let w: usize = 0;
    let i: usize = 0;
    while (i < args.len) {
        if (i > 0) {
            if (w >= out.len) {
                return 0;
            }
            out[w] = 32; // ' '
            w += 1;
        }
        const n = escapePosix(args[i], out[w..out.len]);
        if (n == 0) {
            return 0;
        }
        w += n;
        i += 1;
    }
    return w;
}

/// Validates a POSIX environment variable name (`[A-Za-z_][A-Za-z0-9_]*`).
pub fn isValidEnvName(name: string) -> bool {
    if (name.len == 0) {
        return false;
    }
    const c0 = name[0];
    const start_ok = (c0 >= 65 && c0 <= 90) || (c0 >= 97 && c0 <= 122) || c0 == 95;
    if (!start_ok) {
        return false;
    }
    let i: usize = 1;
    while (i < name.len) {
        const ch = name[i];
        const ok = (ch >= 65 && ch <= 90) || (ch >= 97 && ch <= 122) || (ch >= 48 && ch <= 57) || ch == 95;
        if (!ok) {
            return false;
        }
        i += 1;
    }
    return true;
}
