// packages/@cap/locate-path/src/index.cp
// Zero-allocation locate-path v7 compatible candidate path locator for CP.
// Clean-room MIT pure CP implementation reusing `@cap/find-up`.

import { joinDirAndName } from "../../find-up/src/index.cp";

pub const ENTRY_ANY: u8 = 0;
pub const ENTRY_FILE: u8 = 1;
pub const ENTRY_DIR: u8 = 2;

const LCP_MAX_PATH: usize = 512;

/// Options controlling how candidate paths are resolved and filtered.
pub struct LocateOptions {
    pub cwd: string,
    pub entry_type: u8,
    pub allow_symlinks: bool,
    pub preserve_order: bool,

    pub static fn defaults(cwd: string): LocateOptions {
        return LocateOptions {
            cwd: cwd,
            entry_type: ENTRY_FILE,
            allow_symlinks: true,
            preserve_order: true,
        };
    }
}

/// Virtual filesystem entry descriptor used for zero-allocation path location.
pub struct FsEntry {
    pub path: string,
    pub kind: u8,
    pub is_symlink: bool,
}

fn lcpStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn lcpTrimTrailingSlash(s: string): string {
    let end: usize = s.len;
    while (end > 1 && s[end - 1] == ('/' as u8)) {
        end -= 1;
    }
    return s[0..end];
}

fn lcpCopyStr(dst: []u8, src: string): usize {
    let i: usize = 0;
    let limit: usize = src.len;
    if (limit > dst.len) {
        limit = dst.len;
    }
    while (i < limit) {
        dst[i] = src[i];
        i += 1;
    }
    return limit;
}

/// Returns true if `p` is an absolute POSIX, Windows drive, or UNC path.
pub fn isAbsolutePath(p: string): bool {
    if (p.len == 0) {
        return false;
    }
    if (p[0] == ('/' as u8)) {
        return true;
    }
    if (p.len >= 2 && p[0] == ('\\' as u8) && p[1] == ('\\' as u8)) {
        return true;
    }
    if (p.len >= 3) {
        let c0: u8 = p[0];
        let is_alpha: bool = (c0 >= ('A' as u8) && c0 <= ('Z' as u8))
            || (c0 >= ('a' as u8) && c0 <= ('z' as u8));
        if (is_alpha && p[1] == (':' as u8) && (p[2] == ('/' as u8) || p[2] == ('\\' as u8))) {
            return true;
        }
    }
    return false;
}

/// Resolve `cand` against `cwd` into `out`, reusing `joinDirAndName` from `@cap/find-up`.
fn lcpResolveCandidate(cwd: string, cand: string, out: []u8): usize {
    if (cand.len == 0) {
        return 0;
    }
    if (isAbsolutePath(cand) || cwd.len == 0) {
        let trimmed: string = lcpTrimTrailingSlash(cand);
        return lcpCopyStr(out, trimmed);
    }

    let rel: string = cand;
    while (rel.len >= 2 && rel[0] == ('.' as u8) && rel[1] == ('/' as u8)) {
        rel = rel[2..rel.len];
    }

    let n: usize = joinDirAndName(cwd, rel, out);
    while (n > 1 && out[n - 1] == ('/' as u8)) {
        n -= 1;
    }
    return n;
}

fn lcpEntryMatches(entry: FsEntry, target: string, opts: LocateOptions): bool {
    if (!opts.allow_symlinks && entry.is_symlink) {
        return false;
    }
    if (opts.entry_type != ENTRY_ANY && entry.kind != opts.entry_type) {
        return false;
    }
    let norm_entry: string = lcpTrimTrailingSlash(entry.path);
    let norm_target: string = lcpTrimTrailingSlash(target);
    return lcpStrEq(norm_entry, norm_target);
}

/// Locate the first matching candidate path in `entries` and write the resolved path into `out`.
/// Returns the number of bytes written to `out`, or `null` if no candidate matched.
pub fn locatePathInEntries(
    candidates: []string,
    entries: []FsEntry,
    opts: LocateOptions,
    out: []u8
): usize? {
    let buf: [512]u8 = undefined;

    if (opts.preserve_order) {
        let ci: usize = 0;
        while (ci < candidates.len) {
            let n: usize = lcpResolveCandidate(opts.cwd, candidates[ci], buf[0..LCP_MAX_PATH]);
            if (n > 0) {
                let resolved: string = buf[0..n];
                let ei: usize = 0;
                while (ei < entries.len) {
                    if (lcpEntryMatches(entries[ei], resolved, opts)) {
                        return lcpCopyStr(out, resolved);
                    }
                    ei += 1;
                }
            }
            ci += 1;
        }
        return null;
    }

    // When `preserve_order` is false, prefer the earliest matching entry in `entries`.
    let ei: usize = 0;
    while (ei < entries.len) {
        let entry: FsEntry = entries[ei];
        let allowed: bool = (opts.allow_symlinks || !entry.is_symlink)
            && (opts.entry_type == ENTRY_ANY || entry.kind == opts.entry_type);
        if (allowed) {
            let ci: usize = 0;
            while (ci < candidates.len) {
                let n: usize = lcpResolveCandidate(opts.cwd, candidates[ci], buf[0..LCP_MAX_PATH]);
                if (n > 0) {
                    let resolved: string = buf[0..n];
                    if (lcpEntryMatches(entry, resolved, opts)) {
                        return lcpCopyStr(out, resolved);
                    }
                }
                ci += 1;
            }
        }
        ei += 1;
    }
    return null;
}

/// Locate all matching candidates in `entries`, writing matched candidate indices into `matched_indices`.
/// Returns the total number of matched candidates.
pub fn locateAllPathsInEntries(
    candidates: []string,
    entries: []FsEntry,
    opts: LocateOptions,
    matched_indices: []usize
): usize {
    let buf: [512]u8 = undefined;
    let count: usize = 0;

    let ci: usize = 0;
    while (ci < candidates.len) {
        let n: usize = lcpResolveCandidate(opts.cwd, candidates[ci], buf[0..LCP_MAX_PATH]);
        if (n > 0) {
            let resolved: string = buf[0..n];
            let ei: usize = 0;
            let hit: bool = false;
            while (ei < entries.len) {
                if (lcpEntryMatches(entries[ei], resolved, opts)) {
                    hit = true;
                    break;
                }
                ei += 1;
            }
            if (hit) {
                if (count < matched_indices.len) {
                    matched_indices[count] = ci;
                }
                count += 1;
            }
        }
        ci += 1;
    }

    return count;
}
