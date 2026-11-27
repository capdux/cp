// @cap/commondir — Zero-heap deepest common parent directory calculator in pure CP
// Clean-room MIT implementation of npm `commondir` (#976, 57.1M/wk).
// Supports raw zero-copy prefix computation, POSIX path normalization (`.`/`..`/`//`),
// and `commonDirWithBase(base, files, out)` matching Node `commondir(basedir, relfiles)`.

import { writeStr } from "cap:fmt";

pub const COMMONDIR_MAX_PATH: usize = 1024;
const COMMONDIR_MAX_SEGS: usize = 128;

fn cmdIsSep(b: u8): bool {
    return b == 47 || b == 92; // '/' or '\'
}

/// Normalizes a POSIX path into `out`: converts `\` to `/`, collapses duplicate `//`,
/// resolves `.` and `..` segments, and strips trailing `/` (preserving root `/`).
/// Returns bytes written into `out`.
pub fn normalizePosixPath(path: string, out: []u8): usize {
    if (path.len == 0 || out.len == 0) { return 0; }
    let is_abs = cmdIsSep(path[0]);

    let seg_starts: [128]usize = undefined;
    let seg_lens: [128]usize = undefined;
    let seg_count: usize = 0;

    let i: usize = 0;
    while (i < path.len) {
        while (i < path.len && cmdIsSep(path[i])) {
            i += 1;
        }
        if (i >= path.len) { break; }
        let start = i;
        while (i < path.len && !cmdIsSep(path[i])) {
            i += 1;
        }
        let slen = i - start;

        // Skip "."
        if (slen == 1 && path[start] == 46) {
            continue;
        }
        // Handle ".."
        if (slen == 2 && path[start] == 46 && path[start + 1] == 46) {
            if (seg_count > 0) {
                let prev_s = seg_starts[seg_count - 1];
                let prev_l = seg_lens[seg_count - 1];
                let prev_is_dotdot = prev_l == 2 && path[prev_s] == 46 && path[prev_s + 1] == 46;
                if (!prev_is_dotdot) {
                    seg_count -= 1;
                    continue;
                }
            }
            if (is_abs) {
                // Cannot traverse above root "/"
                continue;
            }
            if (seg_count < COMMONDIR_MAX_SEGS) {
                seg_starts[seg_count] = start;
                seg_lens[seg_count] = 2;
                seg_count += 1;
            }
            continue;
        }

        if (seg_count < COMMONDIR_MAX_SEGS) {
            seg_starts[seg_count] = start;
            seg_lens[seg_count] = slen;
            seg_count += 1;
        }
    }

    let w: usize = 0;
    if (is_abs) {
        if (w < out.len) {
            out[w] = 47;
            w += 1;
        }
    }
    let k: usize = 0;
    while (k < seg_count) {
        if (k > 0 && w < out.len) {
            out[w] = 47;
            w += 1;
        }
        let s = seg_starts[k];
        let l = seg_lens[k];
        w = writeStr(path[s..(s + l)], out, w);
        k += 1;
    }

    if (w == 0 && !is_abs) {
        out[0] = 46; // "."
        return 1;
    }
    return w;
}

/// Resolves `rel` against `base` (POSIX semantics) and writes the normalized path into `out`.
/// If `rel` is absolute, `base` is ignored and `rel` is normalized directly.
pub fn resolvePosixPath(base: string, rel: string, out: []u8): usize {
    if (rel.len > 0 && cmdIsSep(rel[0])) {
        return normalizePosixPath(rel, out);
    }
    if (base.len == 0) {
        return normalizePosixPath(rel, out);
    }
    if (rel.len == 0) {
        return normalizePosixPath(base, out);
    }
    let joined: [1024]u8 = undefined;
    let j: usize = writeStr(base, joined[0..1024], 0);
    if (j < 1024 && (j == 0 || !cmdIsSep(joined[j - 1]))) {
        joined[j] = 47;
        j += 1;
    }
    j = writeStr(rel, joined[0..1024], j);
    return normalizePosixPath(joined[0..j], out);
}

/// Computes the longest common directory prefix of two paths, returning a slice of `a`.
pub fn commonDirTwo(a: string, b: string): string {
    if (a.len == 0 || b.len == 0) {
        return "";
    }
    let min_len = a.len;
    if (b.len < min_len) { min_len = b.len; }

    let i: usize = 0;
    let last_sep: usize = 0;
    let saw_sep = false;

    while (i < min_len) {
        const ca = if (cmdIsSep(a[i])) (47 as u8) else a[i];
        const cb = if (cmdIsSep(b[i])) (47 as u8) else b[i];
        if (ca != cb) {
            break;
        }
        if (ca == 47) {
            last_sep = i;
            saw_sep = true;
        }
        i += 1;
    }

    // Exact match or one ends at a directory boundary of the other
    if (i == min_len) {
        if (a.len == b.len) {
            // Strip trailing slash unless root "/"
            if (i > 1 && cmdIsSep(a[i - 1])) {
                return a[0..(i - 1)];
            }
            return a[0..i];
        }
        if (a.len > min_len && cmdIsSep(a[min_len])) {
            if (min_len > 1 && cmdIsSep(a[min_len - 1])) {
                return a[0..(min_len - 1)];
            }
            return a[0..min_len];
        }
        if (b.len > min_len && cmdIsSep(b[min_len])) {
            if (min_len > 1 && cmdIsSep(b[min_len - 1])) {
                return a[0..(min_len - 1)];
            }
            return a[0..min_len];
        }
    }

    if (!saw_sep) {
        return "";
    }
    if (last_sep == 0) {
        return a[0..1]; // root "/"
    }
    return a[0..last_sep];
}

/// Computes the deepest common parent directory across all `paths` (zero-copy slice).
pub fn commonDir(paths: []const string): string {
    if (paths.len == 0) {
        return "";
    }
    let acc = paths[0];
    if (paths.len == 1) {
        if (acc.len > 1 && cmdIsSep(acc[acc.len - 1])) {
            return acc[0..(acc.len - 1)];
        }
        return acc;
    }
    let i: usize = 1;
    while (i < paths.len) {
        acc = commonDirTwo(acc, paths[i]);
        if (acc.len == 0) {
            return "";
        }
        i += 1;
    }
    return acc;
}

/// Normalizes two POSIX paths and writes their deepest common parent directory into `out`.
pub fn commonDirTwoNormalized(a: string, b: string, out: []u8): usize {
    let na: [1024]u8 = undefined;
    let nb: [1024]u8 = undefined;
    let la = normalizePosixPath(a, na[0..1024]);
    let lb = normalizePosixPath(b, nb[0..1024]);
    let res = commonDirTwo(na[0..la], nb[0..lb]);
    return writeStr(res, out, 0);
}

/// Normalizes all POSIX `paths` (resolving `.`, `..`, `//`) and writes their deepest common
/// parent directory into `out`, matching Node `commondir(files)`.
pub fn commonDirNormalized(paths: []const string, out: []u8): usize {
    if (paths.len == 0 || out.len == 0) { return 0; }
    let acc_buf: [1024]u8 = undefined;
    let cur_buf: [1024]u8 = undefined;
    let acc_len = normalizePosixPath(paths[0], acc_buf[0..1024]);
    let i: usize = 1;
    while (i < paths.len) {
        let cur_len = normalizePosixPath(paths[i], cur_buf[0..1024]);
        let next = commonDirTwo(acc_buf[0..acc_len], cur_buf[0..cur_len]);
        acc_len = next.len;
        if (acc_len == 0) { return 0; }
        i += 1;
    }
    return writeStr(acc_buf[0..acc_len], out, 0);
}

/// Resolves each relative path in `files` against `base` and computes their deepest common
/// parent directory into `out`, matching Node `commondir(basedir, relfiles)`.
pub fn commonDirWithBase(base: string, files: []const string, out: []u8): usize {
    if (files.len == 0 || out.len == 0) { return 0; }
    let acc_buf: [1024]u8 = undefined;
    let cur_buf: [1024]u8 = undefined;
    let acc_len = resolvePosixPath(base, files[0], acc_buf[0..1024]);
    let i: usize = 1;
    while (i < files.len) {
        let cur_len = resolvePosixPath(base, files[i], cur_buf[0..1024]);
        let next = commonDirTwo(acc_buf[0..acc_len], cur_buf[0..cur_len]);
        acc_len = next.len;
        if (acc_len == 0) { return 0; }
        i += 1;
    }
    return writeStr(acc_buf[0..acc_len], out, 0);
}

export {
    COMMONDIR_MAX_PATH,
    normalizePosixPath,
    resolvePosixPath,
    commonDirTwo,
    commonDir,
    commonDirTwoNormalized,
    commonDirNormalized,
    commonDirWithBase,
};
