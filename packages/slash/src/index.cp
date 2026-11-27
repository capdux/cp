// packages/@cap/slash/src/index.cp
// Zero-allocation Windows backslash <-> POSIX slash path converter (npm `slash` parity).
// MIT clean-room pure CP implementation.

/// Returns true if `path` begins with the Windows extended-length prefix `\\?\`.
pub fn isExtendedLengthPath(path: string): bool {
    if (path.len < 4) {
        return false;
    }
    return path[0] == ('\\' as u8) && path[1] == ('\\' as u8) && path[2] == ('?' as u8) && path[3] == ('\\' as u8);
}

/// Convert Windows backslash (`\`) path separators to POSIX forward slashes (`/`) into `out`.
/// Strictly preserves Windows extended-length paths starting with `\\?\` unchanged.
/// Returns bytes written into `out` (capped at `out.len`).
pub fn slash(path: string, out: []u8): usize {
    let n = path.len;
    if (n > out.len) {
        n = out.len;
    }
    if (n == 0) {
        return 0;
    }

    if (isExtendedLengthPath(path)) {
        let i: usize = 0;
        while (i < n) {
            out[i] = path[i];
            i += 1;
        }
        return n;
    }

    let i: usize = 0;
    while (i < n) {
        const c = path[i];
        if (c == ('\\' as u8)) {
            out[i] = '/' as u8;
        } else {
            out[i] = c;
        }
        i += 1;
    }
    return n;
}

/// Convert POSIX forward slashes (`/`) to Windows backslashes (`\`) into `out`.
/// Preserves Windows extended-length paths (`\\?\`) unchanged.
/// Returns bytes written into `out` (capped at `out.len`).
pub fn toBackslash(path: string, out: []u8): usize {
    let n = path.len;
    if (n > out.len) {
        n = out.len;
    }
    if (n == 0) {
        return 0;
    }

    if (isExtendedLengthPath(path)) {
        let i: usize = 0;
        while (i < n) {
            out[i] = path[i];
            i += 1;
        }
        return n;
    }

    let i: usize = 0;
    while (i < n) {
        const c = path[i];
        if (c == ('/' as u8)) {
            out[i] = '\\' as u8;
        } else {
            out[i] = c;
        }
        i += 1;
    }
    return n;
}

/// Count total path separators (`/` and `\`) in `path`.
pub fn countSeparators(path: string): usize {
    let count: usize = 0;
    let i: usize = 0;
    while (i < path.len) {
        const c = path[i];
        if (c == ('/' as u8) || c == ('\\' as u8)) {
            count += 1;
        }
        i += 1;
    }
    return count;
}

export {
    slash,
    isExtendedLengthPath,
    toBackslash,
    countSeparators,
};
