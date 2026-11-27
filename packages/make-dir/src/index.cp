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
