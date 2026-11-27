// @cap/module-details-from-path — Zero-heap node_modules / cap_modules path parser
// Clean-room CP implementation inspired by npm:module-details-from-path (watson/module-details-from-path)

pub struct ModuleDetails {
    name: string,
    basedir: string,
    subpath: string,
    found: bool,
}

fn mdfpIsSep(ch: u8): bool {
    return ch == '/' || ch == '\\';
}

fn mdfpMatchDir(s: string, pos: usize, dir: string): bool {
    if (pos > 0 && !mdfpIsSep(s[pos - 1])) {
        return false;
    }
    if (pos + dir.len >= s.len) {
        return false;
    }
    if (!mdfpIsSep(s[pos + dir.len])) {
        return false;
    }
    let i: usize = 0;
    while (i < dir.len) : (i += 1) {
        if (s[pos + i] != dir[i]) {
            return false;
        }
    }
    return true;
}

fn mdfpSkipSeps(s: string, start: usize): usize {
    let i = start;
    while (i < s.len && mdfpIsSep(s[i])) : (i += 1) {}
    return i;
}

fn mdfpFindSep(s: string, start: usize): usize {
    let i = start;
    while (i < s.len && !mdfpIsSep(s[i])) : (i += 1) {}
    return i;
}

fn mdfpEmpty(): ModuleDetails {
    return ModuleDetails{
        name: "",
        basedir: "",
        subpath: "",
        found: false,
    };
}

/// Extract package name (including `@scope/name`), package `basedir`, and relative `subpath`
/// from the innermost `node_modules/` or `cap_modules/` segment in `file_path`.
/// Supports both POSIX `/` and Windows `\` path separators with zero heap allocation.
pub fn parseModulePath(file_path: string): ModuleDetails {
    const nm = "node_modules";
    const cm = "cap_modules";

    let found_dir = false;
    let pkg_start: usize = 0;

    let i: usize = 0;
    while (i < file_path.len) : (i += 1) {
        if (mdfpMatchDir(file_path, i, nm)) {
            const after = mdfpSkipSeps(file_path, i + nm.len);
            if (after < file_path.len) {
                found_dir = true;
                pkg_start = after;
            }
        } else if (mdfpMatchDir(file_path, i, cm)) {
            const after = mdfpSkipSeps(file_path, i + cm.len);
            if (after < file_path.len) {
                found_dir = true;
                pkg_start = after;
            }
        }
    }

    if (!found_dir) {
        return mdfpEmpty();
    }

    const seg1_end = mdfpFindSep(file_path, pkg_start);
    if (seg1_end == pkg_start) {
        return mdfpEmpty();
    }

    let pkg_end = seg1_end;
    if (file_path[pkg_start] == '@') {
        if (seg1_end >= file_path.len) {
            return mdfpEmpty();
        }
        const seg2_start = mdfpSkipSeps(file_path, seg1_end);
        if (seg2_start >= file_path.len) {
            return mdfpEmpty();
        }
        const seg2_end = mdfpFindSep(file_path, seg2_start);
        if (seg2_end == seg2_start) {
            return mdfpEmpty();
        }
        pkg_end = seg2_end;
    }

    let subpath: string = "";
    if (pkg_end < file_path.len) {
        const sub_start = mdfpSkipSeps(file_path, pkg_end);
        if (sub_start < file_path.len) {
            subpath = file_path[sub_start..file_path.len];
        }
    }

    return ModuleDetails{
        name: file_path[pkg_start..pkg_end],
        basedir: file_path[0..pkg_end],
        subpath: subpath,
        found: true,
    };
}
