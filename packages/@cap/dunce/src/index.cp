// @cap/dunce — Zero-heap safe Windows verbatim UNC (`\\?\`) path simplifier in pure CP
// MIT License — Clean-room implementation inspired by Rust `dunce`

const MAX_LEGACY_PATH: usize = 259;

fn isAsciiAlpha(ch: u8): bool {
    return (ch >= 65 && ch <= 90) || (ch >= 97 && ch <= 122);
}

fn toUpper(ch: u8): u8 {
    if (ch >= 97 && ch <= 122) {
        return ch - 32;
    }
    return ch;
}

fn isIllegalWinChar(ch: u8): bool {
    if (ch < 0x20) {
        return true;
    }
    // < > : " | ? * /
    return ch == 60 || ch == 62 || ch == 58 || ch == 34 || ch == 124 || ch == 63 || ch == 42 || ch == 47;
}

/// Case-insensitively check whether `component` is a Windows reserved DOS device name:
/// `CON`, `PRN`, `AUX`, `NUL`, `COM1`..`COM9`, `LPT1`..`LPT9` (including with extensions like `CON.txt`).
pub fn isWindowsReservedName(component: string): bool {
    if (component.len == 0) {
        return false;
    }
    // Extract stem before first '.'
    let stem_len: usize = 0;
    while (stem_len < component.len && component[stem_len] != 46) {
        stem_len += 1;
    }
    // Trim trailing spaces on stem (Windows treats "CON " as CON)
    while (stem_len > 0 && component[stem_len - 1] == 32) {
        stem_len -= 1;
    }

    if (stem_len == 3) {
        const c0: u8 = toUpper(component[0]);
        const c1: u8 = toUpper(component[1]);
        const c2: u8 = toUpper(component[2]);
        if (c0 == 67 && c1 == 79 && c2 == 78) { return true; } // CON
        if (c0 == 80 && c1 == 82 && c2 == 78) { return true; } // PRN
        if (c0 == 65 && c1 == 85 && c2 == 88) { return true; } // AUX
        if (c0 == 78 && c1 == 85 && c2 == 76) { return true; } // NUL
        return false;
    }

    if (stem_len == 4) {
        const c0: u8 = toUpper(component[0]);
        const c1: u8 = toUpper(component[1]);
        const c2: u8 = toUpper(component[2]);
        const c3: u8 = component[3];
        if (c3 >= 49 && c3 <= 57) { // '1'..'9'
            if (c0 == 67 && c1 == 79 && c2 == 77) { return true; } // COM1..COM9
            if (c0 == 76 && c1 == 80 && c2 == 84) { return true; } // LPT1..LPT9
        }
    }
    return false;
}

/// Return true if `path` begins with the Windows verbatim prefix `\\?\`.
pub fn hasWindowsVerbatimPrefix(path: string): bool {
    return path.len >= 4 &&
        path[0] == 92 &&
        path[1] == 92 &&
        path[2] == 63 &&
        path[3] == 92;
}

/// Return true iff stripping the `\\?\` prefix from `path` is completely safe on Windows:
/// 1. Starts with `\\?\X:\` (ASCII drive letter)
/// 2. Stripped length <= 259 (`MAX_PATH - 1`)
/// 3. Contains no `/`, `.`, or `..` segments, nor illegal characters `< > : " | ? *`
/// 4. No path component is a Windows reserved device name or ends with space/dot
pub fn canStripVerbatimPrefix(path: string): bool {
    if (!hasWindowsVerbatimPrefix(path)) {
        return false;
    }
    if (path.len < 7) {
        return false;
    }
    // Must be \\?\X:\
    if (!isAsciiAlpha(path[4]) || path[5] != 58 || path[6] != 92) {
        return false;
    }
    // Stripped path length must fit within legacy MAX_PATH (259)
    if (path.len - 4 > MAX_LEGACY_PATH) {
        return false;
    }

    let i: usize = 7;
    while (i < path.len) {
        const seg_start: usize = i;
        while (i < path.len && path[i] != 92) {
            if (isIllegalWinChar(path[i])) {
                return false;
            }
            i += 1;
        }
        const seg_len: usize = i - seg_start;
        if (seg_len == 0) {
            // Double backslash inside path (e.g. \\?\C:\a\\b) is not canonical
            if (i < path.len) {
                return false;
            }
        } else {
            const seg: string = path[seg_start..i];
            // Reject "." and ".."
            if (seg_len == 1 && seg[0] == 46) {
                return false;
            }
            if (seg_len == 2 && seg[0] == 46 && seg[1] == 46) {
                return false;
            }
            // Reject trailing space or dot on any component
            const last: u8 = seg[seg_len - 1];
            if (last == 32 || last == 46) {
                return false;
            }
            // Reject Windows reserved device names
            if (isWindowsReservedName(seg)) {
                return false;
            }
        }
        if (i < path.len && path[i] == 92) {
            i += 1;
        }
    }

    return true;
}

/// Write the simplified Windows path into `out`:
/// strips `\\?\` iff `canStripVerbatimPrefix(path)` is true; otherwise copies `path` verbatim.
/// Returns the number of bytes written to `out`.
pub fn simplified(path: string, out: []u8): usize {
    if (path.len == 0 || out.len == 0) {
        return 0;
    }
    const src: string = if (canStripVerbatimPrefix(path)) path[4..path.len] else path;
    let w: usize = 0;
    while (w < src.len && w < out.len) {
        out[w] = src[w];
        w += 1;
    }
    return w;
}
