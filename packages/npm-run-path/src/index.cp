pub const MAX_PATH_ENTRIES: usize = 32;

pub struct RunPathOptions {
    pub cwd: string,
    pub path_env: string,
    pub exec_path: string,
    pub prefer_local: bool,
    pub add_exec_path: bool,
    pub is_windows: bool,
    pub bin_suffix: string,

    pub static fn defaults(cwd: string, path_env: string): RunPathOptions {
        return RunPathOptions {
            cwd: cwd,
            path_env: path_env,
            exec_path: "",
            prefer_local: true,
            add_exec_path: false,
            is_windows: false,
            bin_suffix: "node_modules/.bin",
        };
    }

    pub static fn forCap(cwd: string, path_env: string): RunPathOptions {
        return RunPathOptions {
            cwd: cwd,
            path_env: path_env,
            exec_path: "",
            prefer_local: true,
            add_exec_path: false,
            is_windows: false,
            bin_suffix: "cap_modules/.bin",
        };
    }
}

fn nrpPushBytes(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p = p + 1;
        i = i + 1;
    }
    return p;
}

fn nrpPushChar(out: []u8, pos: usize, c: u8): usize {
    if (pos >= out.len) { return pos; }
    out[pos] = c;
    return pos + 1;
}

fn nrpIsSep(c: u8, is_windows: bool): bool {
    if (c == 47) { return true; } // '/'
    if (is_windows && c == 92) { return true; } // '\'
    return false;
}

pub fn dirnameOf(path: string, is_windows: bool): string {
    if (path.len == 0) { return ""; }
    let end = path.len;
    // Strip trailing slashes unless root
    while (end > 1 && nrpIsSep(path[end - 1], is_windows)) {
        end = end - 1;
    }
    if (end == 1 && nrpIsSep(path[0], is_windows)) {
        return path[0..1];
    }
    let i = end;
    while (i > 0) {
        i = i - 1;
        if (nrpIsSep(path[i], is_windows)) {
            if (i == 0) { return path[0..1]; }
            return path[0..i];
        }
    }
    return "";
}

pub fn countAncestorDirs(cwd: string, is_windows: bool): usize {
    if (cwd.len == 0) { return 0; }
    let cur = cwd;
    // Trim trailing slash (except root "/")
    while (cur.len > 1 && nrpIsSep(cur[cur.len - 1], is_windows)) {
        cur = cur[0..(cur.len - 1)];
    }
    let count: usize = 0;
    while (count < MAX_PATH_ENTRIES && cur.len > 0) {
        count = count + 1;
        let parent = dirnameOf(cur, is_windows);
        if (parent.len == 0 || parent.len == cur.len) {
            break;
        }
        cur = parent;
    }
    return count;
}

pub fn buildRunPath(opts: RunPathOptions, out: []u8): usize {
    let delim: u8 = if (opts.is_windows) { 59 } else { 58 }; // ';' or ':'
    let slash: u8 = if (opts.is_windows) { 92 } else { 47 }; // '\' or '/'
    let pos: usize = 0;
    let first = true;

    if (opts.prefer_local && opts.cwd.len > 0) {
        let cur = opts.cwd;
        while (cur.len > 1 && nrpIsSep(cur[cur.len - 1], opts.is_windows)) {
            cur = cur[0..(cur.len - 1)];
        }
        let depth: usize = 0;
        while (depth < MAX_PATH_ENTRIES && cur.len > 0) {
            if (!first) { pos = nrpPushChar(out, pos, delim); }
            first = false;
            pos = nrpPushBytes(out, pos, cur);
            if (!(cur.len == 1 && nrpIsSep(cur[0], opts.is_windows))) {
                pos = nrpPushChar(out, pos, slash);
            }
            // Write bin_suffix with platform slash normalization
            let si: usize = 0;
            while (si < opts.bin_suffix.len && pos < out.len) {
                let sc = opts.bin_suffix[si];
                if (sc == 47 && opts.is_windows) {
                    out[pos] = 92;
                } else {
                    out[pos] = sc;
                }
                pos = pos + 1;
                si = si + 1;
            }
            depth = depth + 1;
            let parent = dirnameOf(cur, opts.is_windows);
            if (parent.len == 0 || parent.len == cur.len) {
                break;
            }
            cur = parent;
        }
    }

    if (opts.add_exec_path && opts.exec_path.len > 0) {
        let exec_dir = dirnameOf(opts.exec_path, opts.is_windows);
        if (exec_dir.len > 0) {
            if (!first) { pos = nrpPushChar(out, pos, delim); }
            first = false;
            pos = nrpPushBytes(out, pos, exec_dir);
        }
    }

    if (opts.path_env.len > 0) {
        if (!first) { pos = nrpPushChar(out, pos, delim); }
        pos = nrpPushBytes(out, pos, opts.path_env);
    }

    return pos;
}

pub fn pathKey(is_windows: bool): string {
    if (is_windows) { return "Path"; }
    return "PATH";
}
