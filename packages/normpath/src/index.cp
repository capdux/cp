// @cap/normpath — Zero-heap cross-platform lexical path normalization
// Clean-room pure CP counterpart to crates.io:normpath
// Accelerated with 16-lane @Vector(16, u8) SIMD separator scanning, comptime 256-entry LUTs, and @memcpy.

type NpVec16 = @Vector(16, u8);

const NP_MAX_SEGS: usize = 256;
const NP_WIN_TMP_CAP: usize = 4096;

fn npBuildUpperLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        if (c >= 0x61 && c <= 0x7A) {
            lut[c] = (c - 32) as u8;
        } else {
            lut[c] = c as u8;
        }
        c += 1;
    }
    return lut;
}

fn npBuildCharLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    // bit0: ASCII alpha, bit1: Windows sep ('/' or '\')
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        if ((c >= 0x41 && c <= 0x5A) || (c >= 0x61 && c <= 0x7A)) {
            lut[c] = 1;
        }
        c += 1;
    }
    lut[0x2F] = 2;
    lut[0x5C] = 2;
    return lut;
}

const NP_UPPER_LUT: [256]u8 = npBuildUpperLut();
const NP_CHAR_LUT: [256]u8 = npBuildCharLut();

fn npIsAsciiAlpha(b: u8): bool {
    return (NP_CHAR_LUT[b as usize] & 1) != 0;
}

fn npToUpper(b: u8): u8 {
    return NP_UPPER_LUT[b as usize];
}

fn npIsWinSep(b: u8): bool {
    return (NP_CHAR_LUT[b as usize] & 2) != 0;
}

fn npFindSlash(s: string, start: usize): usize {
    let i: usize = start;
    const v_sl: NpVec16 = @splat(0x2F as u8);
    while (i + 16 <= s.len) {
        const v: NpVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_sl)) {
            break;
        }
        i += 16;
    }
    while (i < s.len && s[i] != 0x2F) {
        i += 1;
    }
    return i;
}

fn npFindWinSep(s: string, start: usize): usize {
    let i: usize = start;
    const v_sl: NpVec16 = @splat(0x2F as u8);
    const v_bs: NpVec16 = @splat(0x5C as u8);
    while (i + 16 <= s.len) {
        const v: NpVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, (v == v_sl) | (v == v_bs))) {
            break;
        }
        i += 16;
    }
    while (i < s.len && !npIsWinSep(s[i])) {
        i += 1;
    }
    return i;
}

fn npWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn npWriteSlice(out: []u8, pos: usize, src: string): usize {
    if (src.len == 0 || pos >= out.len) {
        return pos;
    }
    const avail = out.len - pos;
    const n = if (src.len <= avail) src.len else avail;
    @memcpy(out[pos .. pos + n], src[0..n]);
    return pos + n;
}

fn npSegEqDot(src: string, start: usize, len: usize): bool {
    return len == 1 && src[start] == 0x2E;
}

fn npSegEqDotDot(src: string, start: usize, len: usize): bool {
    return len == 2 && src[start] == 0x2E && src[start + 1] == 0x2E;
}

fn npNormalizePosixTwo(p1: string, p2: string, hasP2: bool, out: []u8): usize {
    const isAbs = p1.len > 0 && p1[0] == 0x2F;

    let segSrc: [256]u8 = undefined;
    let segStart: [256]usize = undefined;
    let segLen: [256]usize = undefined;
    let segCount: usize = 0;
    let parentCount: usize = 0;

    let pass: u8 = 0;
    while (pass < 2) {
        if (pass == 1 && !hasP2) {
            break;
        }
        const cur = if (pass == 0) p1 else p2;
        let i: usize = 0;
        while (i < cur.len) {
            while (i < cur.len && cur[i] == 0x2F) {
                i += 1;
            }
            if (i >= cur.len) {
                break;
            }
            const start = i;
            i = npFindSlash(cur, i);
            const len = i - start;
            if (npSegEqDot(cur, start, len)) {
                continue;
            }
            if (npSegEqDotDot(cur, start, len)) {
                if (segCount > 0) {
                    segCount -= 1;
                } else if (!isAbs) {
                    parentCount += 1;
                }
                continue;
            }
            if (segCount < NP_MAX_SEGS) {
                segSrc[segCount] = pass;
                segStart[segCount] = start;
                segLen[segCount] = len;
                segCount += 1;
            }
        }
        pass += 1;
    }

    let pos: usize = 0;
    if (isAbs) {
        pos = npWriteByte(out, pos, 0x2F);
        let k: usize = 0;
        while (k < segCount) {
            if (k > 0) {
                pos = npWriteByte(out, pos, 0x2F);
            }
            const src = if (segSrc[k] == 0) p1 else p2;
            pos = npWriteSlice(out, pos, src[segStart[k]..segStart[k] + segLen[k]]);
            k += 1;
        }
        return pos;
    }

    if (parentCount == 0 && segCount == 0) {
        return npWriteByte(out, pos, 0x2E);
    }

    let wroteAny = false;
    let pc: usize = 0;
    while (pc < parentCount) {
        if (wroteAny) {
            pos = npWriteByte(out, pos, 0x2F);
        }
        pos = npWriteSlice(out, pos, "..");
        wroteAny = true;
        pc += 1;
    }

    let k: usize = 0;
    while (k < segCount) {
        if (wroteAny) {
            pos = npWriteByte(out, pos, 0x2F);
        }
        const src = if (segSrc[k] == 0) p1 else p2;
        pos = npWriteSlice(out, pos, src[segStart[k]..segStart[k] + segLen[k]]);
        wroteAny = true;
        k += 1;
    }

    return pos;
}

/// Lexically normalize a POSIX path into `out`, returning bytes written.
pub fn normalizePosix(path: string, out: []u8): usize {
    return npNormalizePosixTwo(path, "", false, out);
}

/// Join `base` and `rel` using POSIX semantics and lexically normalize into `out`.
pub fn joinNormalized(base: string, rel: string, out: []u8): usize {
    if (rel.len > 0 && rel[0] == 0x2F) {
        return normalizePosix(rel, out);
    }
    if (base.len == 0) {
        return normalizePosix(rel, out);
    }
    if (rel.len == 0) {
        return normalizePosix(base, out);
    }
    return npNormalizePosixTwo(base, rel, true, out);
}

/// Returns true if `path` starts with Windows verbatim prefix `\\?\`.
pub fn isVerbatimWindows(path: string): bool {
    return path.len >= 4 && path[0] == 0x5C && path[1] == 0x5C && path[2] == 0x3F && path[3] == 0x5C;
}

/// Strip leading `\\?\` verbatim prefix if present (zero-copy slice).
pub fn stripVerbatimPrefix(path: string): string {
    if (isVerbatimWindows(path)) {
        return path[4..path.len];
    }
    return path;
}

fn npHasUncTag(path: string, off: usize): bool {
    if (path.len < off + 4) {
        return false;
    }
    const c0 = npToUpper(path[off]);
    const c1 = npToUpper(path[off + 1]);
    const c2 = npToUpper(path[off + 2]);
    return c0 == 0x55 && c1 == 0x4E && c2 == 0x43 && npIsWinSep(path[off + 3]);
}

/// Lexically normalize a Windows path (drive, UNC, verbatim `\\?\`, relative) into `out`.
pub fn normalizeWindows(path: string, out: []u8): usize {
    let pos: usize = 0;
    let idx: usize = 0;
    let isRooted = false;
    let needSepBeforeBody = false;

    if (isVerbatimWindows(path)) {
        pos = npWriteSlice(out, pos, "\\\\?\\");
        idx = 4;
        if (npHasUncTag(path, idx)) {
            pos = npWriteSlice(out, pos, "UNC\\");
            idx += 4;
            while (idx < path.len && npIsWinSep(path[idx])) {
                idx += 1;
            }
            const srvStart = idx;
            while (idx < path.len && !npIsWinSep(path[idx])) {
                idx += 1;
            }
            pos = npWriteSlice(out, pos, path[srvStart..idx]);
            while (idx < path.len && npIsWinSep(path[idx])) {
                idx += 1;
            }
            const shrStart = idx;
            while (idx < path.len && !npIsWinSep(path[idx])) {
                idx += 1;
            }
            if (idx > shrStart) {
                pos = npWriteByte(out, pos, 0x5C);
                pos = npWriteSlice(out, pos, path[shrStart..idx]);
            }
            isRooted = true;
            needSepBeforeBody = true;
        } else if (idx + 2 <= path.len && npIsAsciiAlpha(path[idx]) && path[idx + 1] == 0x3A) {
            pos = npWriteByte(out, pos, npToUpper(path[idx]));
            pos = npWriteByte(out, pos, 0x3A);
            idx += 2;
            if (idx < path.len && npIsWinSep(path[idx])) {
                pos = npWriteByte(out, pos, 0x5C);
                isRooted = true;
                while (idx < path.len && npIsWinSep(path[idx])) {
                    idx += 1;
                }
            }
        } else {
            isRooted = true;
        }
    } else if (path.len >= 2 && npIsWinSep(path[0]) && npIsWinSep(path[1])) {
        // UNC path: \\server\share\...
        idx = 2;
        while (idx < path.len && npIsWinSep(path[idx])) {
            idx += 1;
        }
        const srvStart = idx;
        while (idx < path.len && !npIsWinSep(path[idx])) {
            idx += 1;
        }
        pos = npWriteSlice(out, pos, "\\\\");
        pos = npWriteSlice(out, pos, path[srvStart..idx]);
        while (idx < path.len && npIsWinSep(path[idx])) {
            idx += 1;
        }
        const shrStart = idx;
        while (idx < path.len && !npIsWinSep(path[idx])) {
            idx += 1;
        }
        if (idx > shrStart) {
            pos = npWriteByte(out, pos, 0x5C);
            pos = npWriteSlice(out, pos, path[shrStart..idx]);
        }
        isRooted = true;
        needSepBeforeBody = true;
    } else if (path.len >= 2 && npIsAsciiAlpha(path[0]) && path[1] == 0x3A) {
        // Drive letter: C:\... or C:rel
        pos = npWriteByte(out, pos, npToUpper(path[0]));
        pos = npWriteByte(out, pos, 0x3A);
        idx = 2;
        if (idx < path.len && npIsWinSep(path[idx])) {
            pos = npWriteByte(out, pos, 0x5C);
            isRooted = true;
            while (idx < path.len && npIsWinSep(path[idx])) {
                idx += 1;
            }
        }
    } else if (path.len >= 1 && npIsWinSep(path[0])) {
        pos = npWriteByte(out, pos, 0x5C);
        isRooted = true;
        idx = 1;
    }

    let segStart: [256]usize = undefined;
    let segLen: [256]usize = undefined;
    let segCount: usize = 0;
    let parentCount: usize = 0;

    while (idx < path.len) {
        while (idx < path.len && npIsWinSep(path[idx])) {
            idx += 1;
        }
        if (idx >= path.len) {
            break;
        }
        const start = idx;
        idx = npFindWinSep(path, idx);
        const len = idx - start;
        if (npSegEqDot(path, start, len)) {
            continue;
        }
        if (npSegEqDotDot(path, start, len)) {
            if (segCount > 0) {
                segCount -= 1;
            } else if (!isRooted) {
                parentCount += 1;
            }
            continue;
        }
        if (segCount < NP_MAX_SEGS) {
            segStart[segCount] = start;
            segLen[segCount] = len;
            segCount += 1;
        }
    }

    if (isRooted) {
        if (segCount == 0) {
            return pos;
        }
        if (needSepBeforeBody) {
            pos = npWriteByte(out, pos, 0x5C);
        }
        let k: usize = 0;
        while (k < segCount) {
            if (k > 0) {
                pos = npWriteByte(out, pos, 0x5C);
            }
            pos = npWriteSlice(out, pos, path[segStart[k]..segStart[k] + segLen[k]]);
            k += 1;
        }
        return pos;
    }

    if (parentCount == 0 && segCount == 0) {
        return npWriteByte(out, pos, 0x2E);
    }

    let wroteAny = false;
    let pc: usize = 0;
    while (pc < parentCount) {
        if (wroteAny) {
            pos = npWriteByte(out, pos, 0x5C);
        }
        pos = npWriteSlice(out, pos, "..");
        wroteAny = true;
        pc += 1;
    }

    let k: usize = 0;
    while (k < segCount) {
        if (wroteAny) {
            pos = npWriteByte(out, pos, 0x5C);
        }
        pos = npWriteSlice(out, pos, path[segStart[k]..segStart[k] + segLen[k]]);
        wroteAny = true;
        k += 1;
    }

    return pos;
}

fn npWinIsAbsolute(path: string): bool {
    if (path.len == 0) {
        return false;
    }
    if (isVerbatimWindows(path)) {
        return true;
    }
    if (npIsWinSep(path[0])) {
        return true;
    }
    if (path.len >= 2 && npIsAsciiAlpha(path[0]) && path[1] == 0x3A) {
        return true;
    }
    return false;
}

/// Join `base` and `rel` using Windows semantics and lexically normalize into `out`.
pub fn joinNormalizedWindows(base: string, rel: string, out: []u8): usize {
    if (npWinIsAbsolute(rel)) {
        return normalizeWindows(rel, out);
    }
    if (base.len == 0) {
        return normalizeWindows(rel, out);
    }
    if (rel.len == 0) {
        return normalizeWindows(base, out);
    }
    let tmp: [4096]u8 = undefined;
    let tpos: usize = 0;
    tpos = npWriteSlice(tmp[0..], tpos, base);
    if (tpos > 0 && tpos < NP_WIN_TMP_CAP && !npIsWinSep(tmp[tpos - 1])) {
        tpos = npWriteByte(tmp[0..], tpos, 0x5C);
    }
    tpos = npWriteSlice(tmp[0..], tpos, rel);
    return normalizeWindows(tmp[0..tpos], out);
}

/// Returns true if `path` contains any `..` path component (splitting on `/` or `\`).
pub fn hasParentTraversal(path: string): bool {
    let i: usize = 0;
    while (i < path.len) {
        while (i < path.len && npIsWinSep(path[i])) {
            i += 1;
        }
        if (i >= path.len) {
            break;
        }
        const start = i;
        i = npFindWinSep(path, i);
        if (npSegEqDotDot(path, start, i - start)) {
            return true;
        }
    }
    return false;
}

/// Returns true if a relative POSIX path escapes above its starting root after lexical normalization.
pub fn escapesRoot(path: string): bool {
    if (path.len > 0 && path[0] == 0x2F) {
        return false;
    }
    let depth: usize = 0;
    let i: usize = 0;
    while (i < path.len) {
        while (i < path.len && path[i] == 0x2F) {
            i += 1;
        }
        if (i >= path.len) {
            break;
        }
        const start = i;
        i = npFindSlash(path, i);
        const len = i - start;
        if (npSegEqDot(path, start, len)) {
            continue;
        }
        if (npSegEqDotDot(path, start, len)) {
            if (depth == 0) {
                return true;
            }
            depth -= 1;
        } else {
            depth += 1;
        }
    }
    return false;
}

/// Returns true if `path` is already in canonical POSIX normalized form.
pub fn isNormalized(path: string): bool {
    if (path.len == 0) {
        return false;
    }
    if (path.len == 1) {
        return path[0] == 0x2F || path[0] == 0x2E || path[0] != 0x00;
    }

    const isAbs = path[0] == 0x2F;
    if (path[path.len - 1] == 0x2F) {
        return false;
    }

    let seenNormal = false;
    let i: usize = if (isAbs) 1 else 0;
    while (i < path.len) {
        if (path[i] == 0x2F) {
            return false;
        }
        const start = i;
        i = npFindSlash(path, i);
        const len = i - start;
        if (npSegEqDot(path, start, len)) {
            return false;
        }
        if (npSegEqDotDot(path, start, len)) {
            if (isAbs || seenNormal) {
                return false;
            }
        } else {
            seenNormal = true;
        }
        if (i < path.len && path[i] == 0x2F) {
            i += 1;
            if (i < path.len && path[i] == 0x2F) {
                return false;
            }
        }
    }
    return true;
}
