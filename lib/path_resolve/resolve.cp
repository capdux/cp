// Source: Absorbed from /data/dev/bun/src/paths/resolve_path.cp | Synced: 2026-11-22T06:27:04Z
// Pure-CP zero-allocation POSIX path resolution utilities: normalizePosix,
// joinNormalized, relativePosix, longestCommonPath, isParentOrEqual, isPathInside.

pub enum ParentRelation: u8 {
    parent = 0,
    equal = 1,
    unrelated = 2,
}

/// Determine whether `parent` is a directory ancestor of `child`, equal to `child`, or unrelated.
/// Strips trailing slashes before comparing.
pub fn isParentOrEqual(parent_in: []const u8, child_in: []const u8): ParentRelation {
    let parent = stripTrailingSlashes(parent_in);
    let child = stripTrailingSlashes(child_in);

    if (parent.len == 0 || child.len == 0) return .unrelated;
    if (strEq(parent, "/")) {
        if (strEq(child, "/")) return .equal;
        if (child[0] == '/') return .parent;
        return .unrelated;
    }

    if (child.len < parent.len) return .unrelated;
    if (!strEq(child[0..parent.len], parent)) return .unrelated;
    if (child.len == parent.len) return .equal;
    if (child[parent.len] == '/') return .parent;
    return .unrelated;
}

/// Return true if `child` is strictly inside or equal to `parent` (after POSIX normalization).
pub fn isPathInside(parent: []const u8, child: []const u8): bool {
    let p_buf: [512]u8 = [0; 512];
    let c_buf: [512]u8 = [0; 512];
    const p_norm = normalizePosix(parent, p_buf[0..]) ?? return false;
    const c_norm = normalizePosix(child, c_buf[0..]) ?? return false;
    const rel = isParentOrEqual(p_norm, c_norm);
    return rel == .parent || rel == .equal;
}

/// Compute the longest common directory prefix across `paths` (returned as slice of `paths[0]`).
/// Returns `null` if `paths` is empty or no common directory prefix exists.
pub fn longestCommonPath(paths: []const []const u8): ([]const u8)? {
    if (paths.len == 0) return null;
    if (paths.len == 1) return paths[0];

    let min_len = paths[0].len;
    let k: usize = 1;
    while (k < paths.len) : (k += 1) {
        if (paths[k].len < min_len) min_len = paths[k].len;
    }
    if (min_len == 0) return null;

    let last_sep: usize? = null;
    let i: usize = 0;
    while (i < min_len) : (i += 1) {
        const c = paths[0][i];
        let j: usize = 1;
        let all_eq = true;
        while (j < paths.len) : (j += 1) {
            if (paths[j][i] != c) {
                all_eq = false;
                break;
            }
        }
        if (!all_eq) break;
        if (c == '/') last_sep = i;
    }

    // Check if the entire min_len matches and every path either ends there or has '/' next
    if (i == min_len) {
        let all_boundary = true;
        let j: usize = 0;
        while (j < paths.len) : (j += 1) {
            if (paths[j].len > min_len && paths[j][min_len] != '/') {
                all_boundary = false;
                break;
            }
        }
        if (all_boundary) return paths[0][0..min_len];
    }

    if (sep := last_sep) {
        if (sep == 0) return paths[0][0..1]; // "/"
        return paths[0][0..sep];
    }
    return null;
}

/// Normalize a POSIX path into `out`, collapsing `//`, `.`, and resolving `..`.
pub fn normalizePosix(input: []const u8, out: []u8): ([]const u8)? {
    if (input.len == 0) {
        if (out.len < 1) return null;
        out[0] = '.';
        return out[0..1];
    }

    const is_abs = input[0] == '/';
    let segs: [64][]const u8 = [""; 64];
    let seg_len: usize = 0;

    let pos: usize = 0;
    while (pos < input.len) {
        while (pos < input.len && input[pos] == '/') : (pos += 1) {}
        if (pos >= input.len) break;
        const start = pos;
        while (pos < input.len && input[pos] != '/') : (pos += 1) {}
        const seg = input[start..pos];

        if (strEq(seg, ".")) {
            continue;
        } else if (strEq(seg, "..")) {
            if (seg_len > 0 && !strEq(segs[seg_len - 1], "..")) {
                seg_len -= 1;
            } else if (!is_abs) {
                if (seg_len >= 64) return null;
                segs[seg_len] = "..";
                seg_len += 1;
            }
        } else {
            if (seg_len >= 64) return null;
            segs[seg_len] = seg;
            seg_len += 1;
        }
    }

    let p: usize = 0;
    if (is_abs) {
        if (p >= out.len) return null;
        out[p] = '/';
        p += 1;
    }

    let i: usize = 0;
    while (i < seg_len) : (i += 1) {
        if (i > 0) {
            if (p >= out.len) return null;
            out[p] = '/';
            p += 1;
        }
        const s = segs[i];
        if (p + s.len > out.len) return null;
        let k: usize = 0;
        while (k < s.len) : (k += 1) {
            out[p] = s[k];
            p += 1;
        }
    }

    if (p == 0) {
        if (out.len < 1) return null;
        out[0] = '.';
        return out[0..1];
    }
    return out[0..p];
}

/// Join `base` and `rel` (if `rel` is absolute, `rel` wins) and normalize into `out`.
pub fn joinNormalized(base: []const u8, rel: []const u8, out: []u8): ([]const u8)? {
    if (rel.len > 0 && rel[0] == '/') {
        return normalizePosix(rel, out);
    }
    if (base.len == 0) {
        return normalizePosix(rel, out);
    }
    let tmp: [1024]u8 = [0; 1024];
    if (base.len + 1 + rel.len > 1024) return null;
    let p: usize = 0;
    let i: usize = 0;
    while (i < base.len) : (i += 1) {
        tmp[p] = base[i];
        p += 1;
    }
    tmp[p] = '/';
    p += 1;
    i = 0;
    while (i < rel.len) : (i += 1) {
        tmp[p] = rel[i];
        p += 1;
    }
    return normalizePosix(tmp[0..p], out);
}

/// Compute POSIX relative path from `from_in` to `to_in`, writing into `out`.
pub fn relativePosix(from_in: []const u8, to_in: []const u8, out: []u8): ([]const u8)? {
    let f_buf: [512]u8 = [0; 512];
    let t_buf: [512]u8 = [0; 512];
    const from_norm = normalizePosix(from_in, f_buf[0..]) ?? return null;
    const to_norm = normalizePosix(to_in, t_buf[0..]) ?? return null;

    let f_segs: [64][]const u8 = [""; 64];
    let t_segs: [64][]const u8 = [""; 64];
    const f_len = splitSegments(from_norm, f_segs[0..]);
    const t_len = splitSegments(to_norm, t_segs[0..]);

    let common: usize = 0;
    while (common < f_len && common < t_len && strEq(f_segs[common], t_segs[common])) : (common += 1) {}

    let p: usize = 0;
    let up = common;
    while (up < f_len) : (up += 1) {
        if (p > 0) {
            if (p >= out.len) return null;
            out[p] = '/';
            p += 1;
        }
        if (p + 2 > out.len) return null;
        out[p] = '.';
        out[p + 1] = '.';
        p += 2;
    }

    let down = common;
    while (down < t_len) : (down += 1) {
        if (p > 0) {
            if (p >= out.len) return null;
            out[p] = '/';
            p += 1;
        }
        const s = t_segs[down];
        if (p + s.len > out.len) return null;
        let k: usize = 0;
        while (k < s.len) : (k += 1) {
            out[p] = s[k];
            p += 1;
        }
    }

    if (p == 0) {
        if (out.len < 1) return null;
        out[0] = '.';
        return out[0..1];
    }
    return out[0..p];
}

// ---------- Helpers ----------

fn splitSegments(path: []const u8, dst: [][]const u8): usize {
    let n: usize = 0;
    let pos: usize = 0;
    while (pos < path.len && n < dst.len) {
        while (pos < path.len && path[pos] == '/') : (pos += 1) {}
        if (pos >= path.len) break;
        const start = pos;
        while (pos < path.len && path[pos] != '/') : (pos += 1) {}
        const seg = path[start..pos];
        if (!strEq(seg, ".")) {
            dst[n] = seg;
            n += 1;
        }
    }
    return n;
}

fn stripTrailingSlashes(s: []const u8): []const u8 {
    let end = s.len;
    while (end > 1 && s[end - 1] == '/') : (end -= 1) {}
    return s[0..end];
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}
