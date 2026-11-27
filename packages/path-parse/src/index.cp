// @cap/path-parse — Zero-allocation POSIX & Win32 path.parse() in pure CP
// Clean-room MIT implementation of npm `path-parse` (#238, 196M/wk).
// Accelerated with 16-lane @Vector(16, u8) SIMD reverse separator & dot scanning.

type PprVec16 = @Vector(16, u8);

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

fn pprRfindByteSimd(s: string, lo: usize, hi: usize, target: u8): usize {
    let i = hi;
    const vt: PprVec16 = @splat(target);
    while (i >= lo + 16) {
        const blk = i - 16;
        const v: PprVec16 = s[blk .. blk + 16][0..16].*;
        if (@reduce(.Or, v == vt)) {
            break;
        }
        i = blk;
    }
    while (i > lo) {
        i -= 1;
        if (s[i] == target) {
            return i;
        }
    }
    return hi;
}

fn pprRfindWinSepSimd(s: string, lo: usize, hi: usize): usize {
    let i = hi;
    const v_fwd: PprVec16 = @splat(47 as u8);
    const v_bwd: PprVec16 = @splat(92 as u8);
    while (i >= lo + 16) {
        const blk = i - 16;
        const v: PprVec16 = s[blk .. blk + 16][0..16].*;
        if (@reduce(.Or, v == v_fwd) || @reduce(.Or, v == v_bwd)) {
            break;
        }
        i = blk;
    }
    while (i > lo) {
        i -= 1;
        if (pprIsWinSep(s[i])) {
            return i;
        }
    }
    return hi;
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
    const dot = pprRfindByteSimd(base, 0, base.len, 46 as u8);
    if (dot == base.len || dot == 0) {
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

    // Find last '/' before `end` via 16-lane SIMD
    const last_slash = pprRfindByteSimd(path, 0, end, 47 as u8);

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

    const last_sep = pprRfindWinSepSimd(path, root_end, end);

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
