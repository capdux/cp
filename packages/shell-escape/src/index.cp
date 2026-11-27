// @cap/shell-escape — Zero-allocation POSIX sh & Windows CommandLineToArgvW Argument Escaper
// Clean-room MIT implementation in pure CP.
// Accelerated with comptime 256-entry LUTs, 16-lane @Vector(16, u8) SIMD scanning, and @memcpy.

type SeVec16 = @Vector(16, u8);

fn seBuildSafePosixLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        if ((c >= 65 && c <= 90) || (c >= 97 && c <= 122) || (c >= 48 && c <= 57) ||
            c == 95 || c == 64 || c == 37 || c == 43 || c == 61 ||
            c == 58 || c == 44 || c == 46 || c == 47 || c == 45)
        {
            lut[c] = 1;
        }
        c += 1;
    }
    return lut;
}

fn seBuildEnvContLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        if ((c >= 65 && c <= 90) || (c >= 97 && c <= 122) || (c >= 48 && c <= 57) || c == 95) {
            lut[c] = 1;
        }
        c += 1;
    }
    return lut;
}

const SE_SAFE_POSIX_LUT: [256]u8 = seBuildSafePosixLut();
const SE_ENV_CONT_LUT: [256]u8 = seBuildEnvContLut();

fn seFindSingleQuote(arg: string, start: usize) -> usize {
    let i: usize = start;
    const len = arg.len;
    const v_sq: SeVec16 = @splat(39 as u8);
    while (i + 16 <= len) {
        const v: SeVec16 = arg[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_sq)) {
            let k: usize = 0;
            while (k < 16) {
                if (arg[i + k] == 39) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        if (arg[i] == 39) {
            return i;
        }
        i += 1;
    }
    return len;
}

/// Returns true if `arg` is non-empty and contains only safe POSIX shell characters
/// (`A-Za-z0-9_@%+=:,./-`) requiring no quoting.
pub fn isSafePosixWord(arg: string) -> bool {
    if (arg.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < arg.len) {
        if (SE_SAFE_POSIX_LUT[arg[i] as usize] == 0) {
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
        @memcpy(out[0..arg.len], arg[0..arg.len]);
        return arg.len;
    }

    // Count single quotes with 16-lane SIMD to compute exact output length
    let sq_count: usize = 0;
    let scan: usize = 0;
    while (scan < arg.len) {
        const next_sq = seFindSingleQuote(arg, scan);
        if (next_sq >= arg.len) { break; }
        sq_count += 1;
        scan = next_sq + 1;
    }
    const needed: usize = arg.len + 2 + sq_count * 3;
    if (needed > out.len) {
        return 0;
    }

    let w: usize = 0;
    out[w] = 39;
    w += 1;
    let i: usize = 0;
    while (i < arg.len) {
        const next_sq = seFindSingleQuote(arg, i);
        if (next_sq > i) {
            const seg_len = next_sq - i;
            @memcpy(out[w .. w + seg_len], arg[i..next_sq]);
            w += seg_len;
            i = next_sq;
            if (i >= arg.len) { break; }
        }
        out[w] = 39;     // '\''
        out[w + 1] = 92; // '\\'
        out[w + 2] = 39; // '\''
        out[w + 3] = 39; // '\''
        w += 4;
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
    const len = arg.len;
    const v_sp: SeVec16 = @splat(32 as u8);
    const v_tb: SeVec16 = @splat(9 as u8);
    const v_lf: SeVec16 = @splat(10 as u8);
    const v_cr: SeVec16 = @splat(13 as u8);
    const v_qt: SeVec16 = @splat(34 as u8);
    while (i + 16 <= len) {
        const v: SeVec16 = arg[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_sp) ||
            @reduce(.Or, v == v_tb) ||
            @reduce(.Or, v == v_lf) ||
            @reduce(.Or, v == v_cr) ||
            @reduce(.Or, v == v_qt))
        {
            return true;
        }
        i += 16;
    }
    while (i < len) {
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
        @memcpy(out[0..arg.len], arg[0..arg.len]);
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
        if (SE_ENV_CONT_LUT[name[i] as usize] == 0) {
            return false;
        }
        i += 1;
    }
    return true;
}
