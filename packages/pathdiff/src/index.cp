// pathdiff — Zero-heap relative path computation in pure CP
// Clean-room MIT/Apache-2.0 parity with Rust `pathdiff` crate.

import { isAlpha, toLowerByte, strEq } from "cap:ascii";
import { writeStr } from "cap:fmt";
import { isValidUtf8 } from "cap:utf8";

pub const PATHDIFF_MAX_SEGS: usize = 64;

fn pdIsSep(b: u8): bool {
    return b == 47 || b == 92; // '/' or '\\'
}

struct SegList {
    segs: [64]string,
    count: usize,
    is_abs: bool,
    drive: u8, // 0 if none, otherwise lowercase ASCII drive letter ('a'..'z')
}

fn pdSplitNormalized(path: string): SegList {
    let arr: [64]string = [""; 64];
    let drive: u8 = 0;
    let i: usize = 0;
    let is_abs = false;

    // Detect Windows drive prefix (e.g. "C:/foo" or "C:\\foo")
    if (path.len >= 2 && isAlpha(path[0]) && path[1] == 58) { // ':'
        drive = toLowerByte(path[0]);
        i = 2;
        if (i < path.len && pdIsSep(path[i])) {
            is_abs = true;
        }
    } else if (path.len > 0 && pdIsSep(path[0])) {
        is_abs = true;
    }

    let cnt: usize = 0;
    while (i < path.len) {
        while (i < path.len && pdIsSep(path[i])) {
            i += 1;
        }
        if (i >= path.len) {
            break;
        }
        const start = i;
        while (i < path.len && !pdIsSep(path[i])) {
            i += 1;
        }
        const seg = path[start..i];
        if (strEq(seg, ".")) {
            continue;
        } else if (strEq(seg, "..")) {
            if (cnt > 0 && !strEq(arr[cnt - 1], "..")) {
                cnt -= 1;
            } else if (!is_abs && cnt < PATHDIFF_MAX_SEGS) {
                arr[cnt] = "..";
                cnt += 1;
            }
        } else {
            if (cnt < PATHDIFF_MAX_SEGS) {
                arr[cnt] = seg;
                cnt += 1;
            }
        }
    }
    return SegList {
        segs: arr,
        count: cnt,
        is_abs: is_abs,
        drive: drive,
    };
}

/// Returns `true` if `path` and `base` share compatible root semantics (both absolute with
/// the same drive prefix, or both relative with the same drive prefix).
pub fn hasSameRoot(path: string, base: string): bool {
    const t = pdSplitNormalized(path);
    const b = pdSplitNormalized(base);
    return t.is_abs == b.is_abs && t.drive == b.drive;
}

/// Normalizes `.`, `..`, backslashes, and redundant slashes into `out`.
/// Returns bytes written, or `null` if `out` is too small.
pub fn normalizePath(path: string, out: []u8): usize? {
    const s = pdSplitNormalized(path);
    let pos: usize = 0;
    if (s.drive != 0) {
        if (pos + 2 > out.len) { return null; }
        out[pos] = s.drive - 32; // Uppercase drive letter
        out[pos + 1] = 58;       // ':'
        pos += 2;
    }
    if (s.is_abs) {
        if (pos + 1 > out.len) { return null; }
        out[pos] = 47; // '/'
        pos += 1;
    }
    let idx: usize = 0;
    while (idx < s.count) {
        if (idx > 0) {
            if (pos + 1 > out.len) { return null; }
            out[pos] = 47;
            pos += 1;
        }
        const seg = s.segs[idx];
        if (pos + seg.len > out.len) { return null; }
        pos = writeStr(seg, out, pos);
        idx += 1;
    }
    if (pos == 0) {
        if (out.len < 1) { return null; }
        out[0] = 46; // '.'
        return 1;
    }
    return pos;
}

/// Constructs a relative path from `base` to `target` and writes it into `out`.
/// Normalizes `.`, `..`, redundant separators, and backslashes before diffing.
/// Returns `"."` when paths are equivalent, or `null` when roots are incompatible
/// (e.g. absolute vs relative, or differing drive letters) or `out` is too small.
pub fn diffPaths(target: string, base: string, out: []u8): usize? {
    const t = pdSplitNormalized(target);
    const b = pdSplitNormalized(base);
    if (t.is_abs != b.is_abs || t.drive != b.drive) {
        return null;
    }

    let common: usize = 0;
    while (common < t.count && common < b.count && strEq(t.segs[common], b.segs[common])) {
        common += 1;
    }

    // If base still has unresolved leading ".." beyond common prefix, cannot reliably step out
    let bi = common;
    while (bi < b.count) {
        if (strEq(b.segs[bi], "..")) {
            return null;
        }
        bi += 1;
    }

    let pos: usize = 0;
    let first = true;

    const up = b.count - common;
    let ui: usize = 0;
    while (ui < up) {
        if (!first) {
            if (pos + 1 > out.len) { return null; }
            out[pos] = 47;
            pos += 1;
        }
        first = false;
        if (pos + 2 > out.len) { return null; }
        pos = writeStr("..", out, pos);
        ui += 1;
    }

    let ti = common;
    while (ti < t.count) {
        if (!first) {
            if (pos + 1 > out.len) { return null; }
            out[pos] = 47;
            pos += 1;
        }
        first = false;
        const seg = t.segs[ti];
        if (pos + seg.len > out.len) { return null; }
        pos = writeStr(seg, out, pos);
        ti += 1;
    }

    if (first) {
        if (out.len < 1) { return null; }
        out[0] = 46; // '.'
        return 1;
    }
    return pos;
}

/// UTF-8 validated variant of `diffPaths` (Rust `pathdiff::diff_utf8_paths` parity).
/// Returns `null` if either `path` or `base` is not valid UTF-8, roots differ, or `out` is too small.
pub fn diffUtf8Paths(path: string, base: string, out: []u8): usize? {
    if (!isValidUtf8(path) || !isValidUtf8(base)) {
        return null;
    }
    return diffPaths(path, base, out);
}

export {
    PATHDIFF_MAX_SEGS,
    hasSameRoot,
    normalizePath,
    diffPaths,
    diffUtf8Paths,
};
