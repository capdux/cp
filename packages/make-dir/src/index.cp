pub const MAKEDIR_DEFAULT_MODE: u32 = 511; // 0o777
pub const MAKEDIR_MAX_SEGMENTS: usize = 32;

pub struct MakeDirOptions {
    pub mode: u32,
    pub is_windows: bool,

    pub static fn defaults(): MakeDirOptions {
        return MakeDirOptions {
            mode: MAKEDIR_DEFAULT_MODE,
            is_windows: false,
        };
    }
}

pub fn isValidDirPath(path: string, is_windows: bool): bool {
    if (path.len == 0) { return false; }
    let i: usize = 0;
    while (i < path.len) {
        let c = path[i];
        if (c == 0) { return false; } // NUL byte forbidden everywhere
        if (is_windows) {
            // Forbidden on Windows: < > " | ? *
            if (c == 60 || c == 62 || c == 34 || c == 124 || c == 63 || c == 42) {
                return false;
            }
            // ':' only allowed at drive position index 1 (e.g. C:\...)
            if (c == 58 && i != 1) {
                return false;
            }
        }
        i = i + 1;
    }
    return true;
}

fn mkdStrEq(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn mkdExists(p: string, existing: []string): bool {
    let i: usize = 0;
    while (i < existing.len) {
        if (mkdStrEq(existing[i], p)) { return true; }
        i = i + 1;
    }
    return false;
}

pub struct MakeDirPlan {
    pub valid: bool,
    pub mode: u32,
    pub dirs: [32]string,
    pub count: usize,

    pub static fn empty(valid: bool, mode: u32): MakeDirPlan {
        let arr: [32]string = [""; 32];
        return MakeDirPlan {
            valid: valid,
            mode: mode,
            dirs: arr,
            count: 0,
        };
    }
}

pub fn planMakeDir(target: string, existing_dirs: []string, opts: MakeDirOptions): MakeDirPlan {
    if (!isValidDirPath(target, opts.is_windows)) {
        return MakeDirPlan.empty(false, opts.mode);
    }
    let end = target.len;
    while (end > 1 && target[end - 1] == 47) {
        end = end - 1;
    }
    let clean = target[0..end];
    let plan = MakeDirPlan.empty(true, opts.mode);

    if (clean.len == 1 && clean[0] == 47) {
        return plan; // root '/' always exists
    }

    let i: usize = if (clean[0] == 47) { 1 } else { 0 };
    while (i <= clean.len) {
        if (i == clean.len || clean[i] == 47) {
            if (i > 0 && !(i == 1 && clean[0] == 47)) {
                let prefix = clean[0..i];
                if (!mkdExists(prefix, existing_dirs)) {
                    if (plan.count < MAKEDIR_MAX_SEGMENTS) {
                        plan.dirs[plan.count] = prefix;
                        plan.count = plan.count + 1;
                    }
                }
            }
        }
        i = i + 1;
    }
    return plan;
}

pub fn applyUmask(mode: u32, umask: u32): u32 {
    return mode & (511 ^ (umask & 511));
}

pub const DIR_PATH_OK: i32 = 0;
pub const DIR_PATH_ERR_EMPTY: i32 = 1;
pub const DIR_PATH_ERR_NUL_BYTE: i32 = 2;
pub const DIR_PATH_ERR_INVALID_CHAR: i32 = 3;
pub const DIR_PATH_ERR_INVALID_COLON: i32 = 4;

pub struct DirPathValidation {
    pub valid: bool,
    pub err_code: i32,
    pub bad_index: usize,
}

/// Validates a directory path and returns structured diagnostics (`err_code` and `bad_index`).
pub fn validateDirPath(path: string, is_windows: bool): DirPathValidation {
    if (path.len == 0) {
        return DirPathValidation { valid: false, err_code: DIR_PATH_ERR_EMPTY, bad_index: 0 };
    }
    let i: usize = 0;
    while (i < path.len) {
        const c = path[i];
        if (c == 0) {
            return DirPathValidation { valid: false, err_code: DIR_PATH_ERR_NUL_BYTE, bad_index: i };
        }
        if (is_windows) {
            if (c == 60 || c == 62 || c == 34 || c == 124 || c == 63 || c == 42) {
                return DirPathValidation { valid: false, err_code: DIR_PATH_ERR_INVALID_CHAR, bad_index: i };
            }
            if (c == 58 && i != 1) {
                return DirPathValidation { valid: false, err_code: DIR_PATH_ERR_INVALID_COLON, bad_index: i };
            }
        }
        i = i + 1;
    }
    return DirPathValidation { valid: true, err_code: DIR_PATH_OK, bad_index: 0 };
}

fn mkdIsAlpha(c: u8): bool {
    return (c >= 65 && c <= 90) || (c >= 97 && c <= 122);
}

/// Normalizes directory path separators (`\` -> `/` on Windows), collapses consecutive `/`,
/// and strips trailing `/` while preserving root `/` and Windows drive root `C:/`.
pub fn normalizeDirPath(path: string, is_windows: bool, out: []u8): usize {
    if (path.len == 0 || out.len == 0) {
        return 0;
    }
    let w: usize = 0;
    let i: usize = 0;
    let prev_slash = false;
    while (i < path.len && w < out.len) {
        let c = path[i];
        if (is_windows && c == 92) {
            c = 47;
        }
        if (c == 47) {
            if (!prev_slash) {
                out[w] = 47;
                w = w + 1;
                prev_slash = true;
            }
        } else {
            out[w] = c;
            w = w + 1;
            prev_slash = false;
        }
        i = i + 1;
    }
    if (w > 1 && out[w - 1] == 47) {
        const is_drive_root = is_windows && w == 3 && mkdIsAlpha(out[0]) && out[1] == 58;
        if (!is_drive_root) {
            w = w - 1;
        }
    }
    return w;
}

/// Alias for `planMakeDir`: plans missing ancestor directories to create in top-down order.
pub fn makeDirPlan(target: string, existing_dirs: []string, opts: MakeDirOptions): MakeDirPlan {
    return planMakeDir(target, existing_dirs, opts);
}

/// Zero-allocation iterator yielding each ancestor directory prefix from shallowest to deepest.
pub struct ParentDirIter {
    pub path: string,
    pub cursor: usize,
    pub done: bool,

    pub fn next(*_): string? {
        if (_.done || _.path.len == 0) {
            _.done = true;
            return null;
        }
        while (_.cursor <= _.path.len) {
            const idx = _.cursor;
            _.cursor = _.cursor + 1;
            if (idx == _.path.len || _.path[idx] == 47) {
                if (idx > 0 && !(idx == 1 && _.path[0] == 47)) {
                    if (idx == _.path.len) {
                        _.done = true;
                    }
                    return _.path[0..idx];
                }
            }
        }
        _.done = true;
        return null;
    }
}

/// Creates a `ParentDirIter` over all directory prefixes of `path` (stripping trailing slashes).
pub fn iterParentDirs(path: string): ParentDirIter {
    let end = path.len;
    while (end > 1 && path[end - 1] == 47) {
        end = end - 1;
    }
    const clean = path[0..end];
    if (clean.len == 0 || (clean.len == 1 && clean[0] == 47)) {
        return ParentDirIter { path: clean, cursor: clean.len, done: true };
    }
    const start: usize = if (clean[0] == 47) { 1 } else { 0 };
    return ParentDirIter { path: clean, cursor: start, done: false };
}

/// Collects all parent/target directory prefixes of `path` into `out` and returns the count written.
pub fn collectParentDirs(path: string, out: []string): usize {
    let it = iterParentDirs(path);
    let count: usize = 0;
    while (count < out.len) {
        const next_opt = it.next();
        if (next_opt == null) {
            break;
        }
        out[count] = next_opt ?? "";
        count = count + 1;
    }
    return count;
}
