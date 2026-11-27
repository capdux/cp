// @cap/dunce — Zero-heap safe Windows verbatim UNC (`\\?\`) path simplifier in pure CP
// MIT License — Clean-room implementation inspired by Rust `dunce`

import { isAlpha, eqIgnoreCase, findByte, startsWith } from "cap:ascii";
import { writeStr } from "cap:fmt";

const MAX_LEGACY_PATH: usize = 259;

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
    let stem_len: usize = findByte(component, 46, 0) ?? component.len;
    // Trim trailing spaces on stem (Windows treats "CON " as CON)
    while (stem_len > 0 && component[stem_len - 1] == 32) {
        stem_len -= 1;
    }
    const stem: string = component[0..stem_len];

    if (stem_len == 3) {
        return eqIgnoreCase(stem, "CON")
            || eqIgnoreCase(stem, "PRN")
            || eqIgnoreCase(stem, "AUX")
            || eqIgnoreCase(stem, "NUL");
    }

    if (stem_len == 4 && stem[3] >= 49 && stem[3] <= 57) { // '1'..'9'
        const p3: string = stem[0..3];
        return eqIgnoreCase(p3, "COM") || eqIgnoreCase(p3, "LPT");
    }
    return false;
}

/// Return true if `path` begins with the Windows verbatim prefix `\\?\`.
pub fn hasWindowsVerbatimPrefix(path: string): bool {
    return startsWith(path, "\\\\?\\");
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
    if (!isAlpha(path[4]) || path[5] != 58 || path[6] != 92) {
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
    return writeStr(src, out, 0);
}
