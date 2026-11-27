// pathdiff — Zero-heap relative path computation in pure CP
// Clean-room MIT/Apache-2.0 parity with Rust `pathdiff` crate.
// Accelerated with 16-lane @Vector(16, u8) SIMD separator scanning, comptime 256-entry LUTs, @memcpy, and undefined stack buffers.

import { strEq } from "cap:ascii";
import { isValidUtf8 } from "cap:utf8";

pub const PATHDIFF_MAX_SEGS: usize = 64;

type PdVec16 = @Vector(16, u8);

fn pdBuildSepLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    lut[47] = 1; // '/'
    lut[92] = 1; // '\\'
    return lut;
}

fn pdBuildDriveLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 65;
    while (c <= 90) {
        lut[c] = (c + 32) as u8;
        c += 1;
    }
    c = 97;
    while (c <= 122) {
        lut[c] = c as u8;
        c += 1;
    }
    return lut;
}

const PD_SEP_LUT: [256]u8 = pdBuildSepLut();
const PD_DRIVE_LUT: [256]u8 = pdBuildDriveLut();

fn pdIsSep(b: u8): bool {
    return PD_SEP_LUT[b as usize] != 0;
}

fn pdFindNextSep(path: string, start: usize): usize {
    let i = start;
    const len = path.len;
    const v_slash: PdVec16 = @splat(47);
    const v_bs: PdVec16 = @splat(92);
    while (i + 16 <= len) {
        const v: PdVec16 = path[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_slash) || @reduce(.Or, v == v_bs)) {
            let k: usize = 0;
            while (k < 16) {
                if (PD_SEP_LUT[path[i + k] as usize] != 0) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len && PD_SEP_LUT[path[i] as usize] == 0) {
        i += 1;
    }
    return i;
}

struct SegList {
    segs: [64]string,
    count: usize,
    is_abs: bool,
    drive: u8, // 0 if none, otherwise lowercase ASCII drive letter ('a'..'z')
}

fn pdSplitNormalized(path: string): SegList {
    let arr: [64]string = undefined;
    let drive: u8 = 0;
    let i: usize = 0;
    let is_abs = false;

    // Detect Windows drive prefix (e.g. "C:/foo" or "C:\\foo")
    if (path.len >= 2 && path[1] == 58) { // ':'
        const drv = PD_DRIVE_LUT[path[0] as usize];
        if (drv != 0) {
            drive = drv;
            i = 2;
            if (i < path.len && pdIsSep(path[i])) {
                is_abs = true;
            }
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
        i = pdFindNextSep(path, i);
        const seg = path[start..i];
        if (seg.len == 1 && seg[0] == 46) {
            continue;
        } else if (seg.len == 2 && seg[0] == 46 && seg[1] == 46) {
            if (cnt > 0 && !(arr[cnt - 1].len == 2 && arr[cnt - 1][0] == 46 && arr[cnt - 1][1] == 46)) {
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
        if (seg.len > 0) {
            @memcpy(out[pos .. pos + seg.len], seg);
            pos += seg.len;
        }
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
        if (b.segs[bi].len == 2 && b.segs[bi][0] == 46 && b.segs[bi][1] == 46) {
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
        out[pos] = 46;
        out[pos + 1] = 46;
        pos += 2;
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
        if (seg.len > 0) {
            @memcpy(out[pos .. pos + seg.len], seg);
            pos += seg.len;
        }
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
