// @cap/path-parse — Zero-allocation POSIX & Win32 path.parse() in pure CP
// Clean-room MIT implementation of npm `path-parse` (#238, 196M/wk).

pub struct ParsedPath {
    pub root: string,
    pub dir: string,
    pub base: string,
    pub ext: string,
    pub name: string,
}

fn pprIsWinSep(b: u8): bool {
    return b == 47 || b == 92; // '/' or '\'
}

fn pprIsAlpha(b: u8): bool {
    return (b >= 65 && b <= 90) || (b >= 97 && b <= 122);
}

struct PprNameExt {
    name: string,
    ext: string,
}

fn pprSplitBaseExt(base: string): PprNameExt {
    if (base.len == 0) {
        return PprNameExt { name: "", ext: "" };
    }
    // Find last '.' after index 0 (dotfiles like ".gitignore" have empty ext)
    let dot: usize = 0;
    let found = false;
    let i: usize = base.len;
    while (i > 0) {
        i -= 1;
        if (base[i] == 46) {
            if (i > 0) {
                dot = i;
                found = true;
            }
            break;
        }
    }
    if (!found) {
        return PprNameExt { name: base, ext: "" };
    }
    // Special case: ".." has name="..", ext=""
    if (base.len == 2 && base[0] == 46 && base[1] == 46) {
        return PprNameExt { name: base, ext: "" };
    }
    return PprNameExt { name: base[0..dot], ext: base[dot..base.len] };
}

/// Parses a POSIX path into `{ root, dir, base, ext, name }` with zero allocation.
pub fn parsePosix(path: string): ParsedPath {
    if (path.len == 0) {
        return ParsedPath { root: "", dir: "", base: "", ext: "", name: "" };
    }
    const is_abs = path[0] == 47;
    let root = "";
    if (is_abs) {
        root = "/";
    }

    // Trim trailing slashes (except root itself)
    let end = path.len;
    while (end > 1 && path[end - 1] == 47) {
        end -= 1;
    }
    if (end == 1 && path[0] == 47) {
        return ParsedPath { root: "/", dir: "/", base: "", ext: "", name: "" };
    }

    // Find last '/' before `end`
    let last_slash: usize = end;
    let i = end;
    while (i > 0) {
        i -= 1;
        if (path[i] == 47) {
            last_slash = i;
            break;
        }
    }

    let dir = "";
    let base = "";
    if (last_slash == end) {
        base = path[0..end];
    } else {
        base = path[(last_slash + 1)..end];
        if (last_slash == 0) {
            dir = "/";
        } else {
            let d_end = last_slash;
            while (d_end > 1 && path[d_end - 1] == 47) {
                d_end -= 1;
            }
            dir = path[0..d_end];
        }
    }

    const parts = pprSplitBaseExt(base);
    return ParsedPath {
        root: root,
        dir: dir,
        base: base,
        ext: parts.ext,
        name: parts.name,
    };
}

/// Parses a Win32 path (supporting `C:\`, `C:`, `\` and `/`) into `{ root, dir, base, ext, name }`.
pub fn parseWin32(path: string): ParsedPath {
    if (path.len == 0) {
        return ParsedPath { root: "", dir: "", base: "", ext: "", name: "" };
    }
    let root_end: usize = 0;
    if (path.len >= 2 && pprIsAlpha(path[0]) && path[1] == 58) { // 'C:'
        root_end = 2;
        if (path.len >= 3 && pprIsWinSep(path[2])) {
            root_end = 3;
        }
    } else if (pprIsWinSep(path[0])) {
        root_end = 1;
    }
    const root = path[0..root_end];

    let end = path.len;
    while (end > root_end && pprIsWinSep(path[end - 1])) {
        end -= 1;
    }
    if (end == root_end) {
        return ParsedPath { root: root, dir: root, base: "", ext: "", name: "" };
    }

    let last_sep: usize = end;
    let i = end;
    while (i > root_end) {
        i -= 1;
        if (pprIsWinSep(path[i])) {
            last_sep = i;
            break;
        }
    }

    let dir = "";
    let base = "";
    if (last_sep == end) {
        dir = root;
        base = path[root_end..end];
    } else {
        base = path[(last_sep + 1)..end];
        let d_end = last_sep;
        while (d_end > root_end && pprIsWinSep(path[d_end - 1])) {
            d_end -= 1;
        }
        dir = path[0..d_end];
    }

    const parts = pprSplitBaseExt(base);
    return ParsedPath {
        root: root,
        dir: dir,
        base: base,
        ext: parts.ext,
        name: parts.name,
    };
}

export {
    ParsedPath,
    parsePosix,
    parseWin32,
};
