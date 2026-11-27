// dunce — Zero-heap safe Windows verbatim UNC (`\\?\`) path simplifier in pure CP
// MIT License — Clean-room implementation inspired by Rust `dunce`

import { isAlpha, eqIgnoreCase, findByte, startsWith } from "cap:ascii";
import { writeStr } from "cap:fmt";

pub const MAX_LEGACY_PATH: usize = 259;

pub fn isIllegalWinChar(ch: u8): bool {
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

/// Checks whether a single path component is safe under legacy Win32 path rules:
/// non-empty, not `.`/`..`, no illegal chars, no trailing space/dot, not a DOS reserved name.
pub fn isSafeLegacyComponent(component: string): bool {
    if (component.len == 0) {
        return false;
    }
    if (component.len == 1 && component[0] == 46) {
        return false;
    }
    if (component.len == 2 && component[0] == 46 && component[1] == 46) {
        return false;
    }
    const last: u8 = component[component.len - 1];
    if (last == 32 || last == 46) {
        return false;
    }
    let i: usize = 0;
    while (i < component.len) : (i += 1) {
        if (isIllegalWinChar(component[i]) || component[i] == 92) {
            return false;
        }
    }
    return !isWindowsReservedName(component);
}

/// Return true if `path` begins with the Windows verbatim prefix `\\?\`.
pub fn hasWindowsVerbatimPrefix(path: string): bool {
    return startsWith(path, "\\\\?\\");
}

/// Return true if `path` begins with the Windows verbatim UNC prefix `\\?\UNC\` (case-insensitive `UNC`).
pub fn hasWindowsVerbatimUncPrefix(path: string): bool {
    if (path.len < 8 || !hasWindowsVerbatimPrefix(path)) {
        return false;
    }
    return eqIgnoreCase(path[4..7], "UNC") && path[7] == 92;
}

fn validateTrailingComponents(path: string, start_idx: usize): bool {
    let i: usize = start_idx;
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
            if (!isSafeLegacyComponent(seg)) {
                return false;
            }
        }
        if (i < path.len && path[i] == 92) {
            i += 1;
        }
    }
    return true;
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
    return validateTrailingComponents(path, 7);
}

/// Return true iff a verbatim UNC path `\\?\UNC\server\share[\...]` can be safely
/// simplified to `\\server\share[\...]`:
/// 1. Starts with `\\?\UNC\` (case-insensitive `UNC`)
/// 2. Has non-empty valid `server` and `share` components
/// 3. Simplified length (`path.len - 6`) <= 259 (`MAX_LEGACY_PATH`)
/// 4. Every component passes legacy Win32 safety checks
pub fn canSimplifyUncPath(path: string): bool {
    if (!hasWindowsVerbatimUncPrefix(path)) {
        return false;
    }
    // Simplified length: replace "\\?\UNC\" (8 bytes) with "\\" (2 bytes) => len - 6
    if (path.len - 6 > MAX_LEGACY_PATH) {
        return false;
    }
    // Must have non-empty server component followed by '\'
    let i: usize = 8;
    const srv_start = i;
    while (i < path.len && path[i] != 92) : (i += 1) {}
    if (i == srv_start || i >= path.len) {
        return false;
    }
    if (!isSafeLegacyComponent(path[srv_start..i])) {
        return false;
    }
    // Advance past '\' into share component
    i += 1;
    const share_start = i;
    while (i < path.len && path[i] != 92) : (i += 1) {}
    if (i == share_start) {
        return false;
    }
    if (!isSafeLegacyComponent(path[share_start..i])) {
        return false;
    }
    if (i == path.len) {
        return true;
    }
    // Validate remaining components after `\\?\UNC\server\share\`
    return validateTrailingComponents(path, i + 1);
}

/// Zero-copy slice simplification for drive verbatim paths (`\\?\C:\...` -> `C:\...`).
/// Returns `path` unchanged for non-verbatim, unsafe, or UNC paths.
pub fn simplifiedSlice(path: string): string {
    if (canStripVerbatimPrefix(path)) {
        return path[4..path.len];
    }
    return path;
}

/// Returns the exact byte length produced by `simplified(path, out)`.
pub fn simplifiedLen(path: string): usize {
    return simplifiedSlice(path).len;
}

/// Write the simplified Windows path into `out`:
/// strips `\\?\` iff `canStripVerbatimPrefix(path)` is true; otherwise copies `path` verbatim.
/// Returns the number of bytes written to `out`.
pub fn simplified(path: string, out: []u8): usize {
    if (path.len == 0 || out.len == 0) {
        return 0;
    }
    const src: string = simplifiedSlice(path);
    return writeStr(src, out, 0);
}

/// Simplifies a verbatim UNC path `\\?\UNC\server\share...` into `\\server\share...` when safe;
/// otherwise copies `path` verbatim. Returns bytes written.
pub fn simplifiedUnc(path: string, out: []u8): usize {
    if (path.len == 0 || out.len == 0) {
        return 0;
    }
    if (canSimplifyUncPath(path)) {
        let p = writeStr("\\\\", out, 0);
        p = writeStr(path[8..path.len], out, p);
        return p;
    }
    return writeStr(path, out, 0);
}

/// Full simplification supporting both drive verbatim paths (`\\?\C:\...` -> `C:\...`)
/// and UNC verbatim paths (`\\?\UNC\server\share...` -> `\\server\share...`).
pub fn simplifiedWithUnc(path: string, out: []u8): usize {
    if (path.len == 0 || out.len == 0) {
        return 0;
    }
    if (canStripVerbatimPrefix(path)) {
        return writeStr(path[4..path.len], out, 0);
    }
    if (canSimplifyUncPath(path)) {
        let p = writeStr("\\\\", out, 0);
        p = writeStr(path[8..path.len], out, p);
        return p;
    }
    return writeStr(path, out, 0);
}
