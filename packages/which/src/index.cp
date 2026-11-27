// @cap/which-bin — Zero-heap PATH executable candidate resolver (POSIX & Windows)
// Clean-room MIT CP implementation of npm `which` (200M+/wk)
// Reuses `@cap/normalize-path` for cross-platform separator normalization.

import { normalizePath } from "../../normalize-path/src/index.cp";

pub const WHICH_DEFAULT_WIN_PATHEXT: string = ".EXE;.CMD;.BAT;.COM";

fn whEqIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        let ca = if (a[i] >= 65 && a[i] <= 90) { a[i] + 32 } else { a[i] };
        let cb = if (b[i] >= 65 && b[i] <= 90) { b[i] + 32 } else { b[i] };
        if (ca != cb) { return false; }
        i = i + 1;
    }
    return true;
}

/// Returns true if `cmd` contains a path separator (`/` or `\`), meaning it should be resolved
/// directly rather than searched across `PATH`.
pub fn isExplicitPath(cmd: string): bool {
    let i: usize = 0;
    while (i < cmd.len) {
        if (cmd[i] == 47 || cmd[i] == 92) { return true; }
        i = i + 1;
    }
    return false;
}

/// Counts how many non-empty directory entries exist in `path_env` separated by `delim` (`:` on POSIX, `;` on Windows).
pub fn countPathDirs(path_env: string, delim: u8): usize {
    if (path_env.len == 0) { return 0; }
    let count: usize = 0;
    let start: usize = 0;
    let i: usize = 0;
    while (i <= path_env.len) {
        if (i == path_env.len || path_env[i] == delim) {
            if (i > start) {
                count = count + 1;
            }
            start = i + 1;
        }
        i = i + 1;
    }
    return count;
}

/// Extracts the `idx`-th non-empty directory from `path_env` (normalized via `@cap/normalize-path`),
/// writing into `out` and returning its byte length (0 if out of range).
pub fn pathDirAt(path_env: string, delim: u8, idx: usize, out: []u8): usize {
    let cur: usize = 0;
    let start: usize = 0;
    let i: usize = 0;
    while (i <= path_env.len) {
        if (i == path_env.len || path_env[i] == delim) {
            if (i > start) {
                if (cur == idx) {
                    let seg = path_env[start..i];
                    // Strip surrounding quotes (common on Windows PATH)
                    let s: usize = 0;
                    let e: usize = seg.len;
                    if (e >= 2 && seg[0] == 34 && seg[e - 1] == 34) {
                        s = 1;
                        e = e - 1;
                    }
                    if (s >= e) { return 0; }
                    return normalizePath(seg[s..e], true, out);
                }
                cur = cur + 1;
            }
            start = i + 1;
        }
        i = i + 1;
    }
    return 0;
}

/// Checks if `cmd` already ends with one of the extensions in `pathext` (e.g. `.EXE;.CMD;.BAT`).
pub fn hasPathExt(cmd: string, pathext: string): bool {
    let start: usize = 0;
    let i: usize = 0;
    while (i <= pathext.len) {
        if (i == pathext.len || pathext[i] == 59) { // ';'
            if (i > start) {
                let ext = pathext[start..i];
                if (cmd.len >= ext.len && whEqIgnoreCase(cmd[cmd.len - ext.len..cmd.len], ext)) {
                    return true;
                }
            }
            start = i + 1;
        }
        i = i + 1;
    }
    return false;
}

/// Builds a candidate executable path `dir + "/" + cmd + ext` into `out`.
pub fn formatCandidatePath(dir: string, cmd: string, ext: string, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < dir.len && pos < out.len) {
        out[pos] = dir[i];
        pos = pos + 1;
        i = i + 1;
    }
    if (dir.len > 0 && dir[dir.len - 1] != 47 && pos < out.len) {
        out[pos] = 47;
        pos = pos + 1;
    }
    let j: usize = 0;
    while (j < cmd.len && pos < out.len) {
        out[pos] = cmd[j];
        pos = pos + 1;
        j = j + 1;
    }
    let k: usize = 0;
    while (k < ext.len && pos < out.len) {
        out[pos] = ext[k];
        pos = pos + 1;
        k = k + 1;
    }
    return pos;
}

/// Zero-heap stateful candidate path iterator across `PATH` (and optional Windows `PATHEXT`).
pub struct WhichIterator {
    cmd: string,
    path_env: string,
    pathext: string,
    delim: u8,
    dir_idx: usize,
    dir_count: usize,
    ext_idx: usize,
    ext_count: usize,
    explicit_done: bool,

    pub static fn newPosix(cmd: string, path_env: string): WhichIterator {
        return WhichIterator {
            cmd: cmd,
            path_env: path_env,
            pathext: "",
            delim: 58, // ':'
            dir_idx: 0,
            dir_count: countPathDirs(path_env, 58),
            ext_idx: 0,
            ext_count: 1,
            explicit_done: false,
        };
    }

    pub static fn newWindows(cmd: string, path_env: string, pathext: string): WhichIterator {
        let pe = if (pathext.len == 0) { WHICH_DEFAULT_WIN_PATHEXT } else { pathext };
        let ec = if (hasPathExt(cmd, pe)) { 1 } else { countPathDirs(pe, 59) };
        return WhichIterator {
            cmd: cmd,
            path_env: path_env,
            pathext: if (hasPathExt(cmd, pe)) { "" } else { pe },
            delim: 59, // ';'
            dir_idx: 0,
            dir_count: countPathDirs(path_env, 59),
            ext_idx: 0,
            ext_count: if (ec == 0) { 1 } else { ec },
            explicit_done: false,
        };
    }

    /// Yields the next candidate executable path into `out`. Returns `0` when exhausted.
    pub fn next(*_, out: []u8): usize {
        if (_.cmd.len == 0) { return 0; }
        if (isExplicitPath(_.cmd)) {
            if (_.explicit_done) { return 0; }
            _.explicit_done = true;
            return normalizePath(_.cmd, true, out);
        }
        if (_.dir_idx >= _.dir_count) { return 0; }

        let dir_buf: [256]u8 = undefined;
        let dlen = pathDirAt(_.path_env, _.delim, _.dir_idx, dir_buf[0..256]);
        let ext_buf: [32]u8 = undefined;
        let elen: usize = 0;
        if (_.pathext.len > 0) {
            elen = pathDirAt(_.pathext, 59, _.ext_idx, ext_buf[0..32]);
        }

        _.ext_idx = _.ext_idx + 1;
        if (_.ext_idx >= _.ext_count) {
            _.ext_idx = 0;
            _.dir_idx = _.dir_idx + 1;
        }

        return formatCandidatePath(dir_buf[0..dlen], _.cmd, ext_buf[0..elen], out);
    }
}
