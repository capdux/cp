import { dirnameOf } from "../../npm-run-path/src/index.cp";

pub const FIND_UP_MAX_DEPTH: usize = 64;

fn fupStrEq(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn fupTrimTrailingSlash(s: string): string {
    let end = s.len;
    while (end > 1 && s[end - 1] == 47) {
        end = end - 1;
    }
    return s[0..end];
}

fn fupPush(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p = p + 1;
        i = i + 1;
    }
    return p;
}

pub fn joinDirAndName(dir: string, name: string, out: []u8): usize {
    let d = fupTrimTrailingSlash(dir);
    let p: usize = 0;
    p = fupPush(out, p, d);
    if (d.len > 0 && !(d.len == 1 && d[0] == 47)) {
        if (p < out.len) { out[p] = 47; p = p + 1; }
    }
    p = fupPush(out, p, name);
    return p;
}

pub struct FindUpOptions {
    pub cwd: string,
    pub stop_at: string,
    pub allow_symlinks: bool,

    pub static fn defaults(cwd: string): FindUpOptions {
        return FindUpOptions {
            cwd: cwd,
            stop_at: "",
            allow_symlinks: true,
        };
    }
}

pub struct FindUpWalker {
    pub current: string,
    pub stop_at: string,
    pub done: bool,
    pub steps: usize,

    pub static fn new(opts: FindUpOptions): FindUpWalker {
        let cur = fupTrimTrailingSlash(opts.cwd);
        let stp = fupTrimTrailingSlash(opts.stop_at);
        return FindUpWalker {
            current: cur,
            stop_at: stp,
            done: cur.len == 0,
            steps: 0,
        };
    }

    pub fn nextDir(*_): string? {
        if (_.done || _.steps >= FIND_UP_MAX_DEPTH) {
            return null;
        }
        let dir = _.current;
        _.steps = _.steps + 1;

        if (_.stop_at.len > 0 && fupStrEq(dir, _.stop_at)) {
            _.done = true;
            return dir;
        }
        let parent = dirnameOf(dir, false);
        if (parent.len == 0 || fupStrEq(parent, dir)) {
            _.done = true;
        } else {
            _.current = parent;
        }
        return dir;
    }

    pub fn nextCandidate(*_, filename: string, out: []u8): usize? {
        let d_opt = _.nextDir();
        if (d_opt == null) { return null; }
        let d = d_opt ?? "";
        return joinDirAndName(d, filename, out);
    }
}

pub fn findUpInList(opts: FindUpOptions, filename: string, existing_paths: []string, out: []u8): usize? {
    let w = FindUpWalker.new(opts);
    let cand: [256]u8 = [0; 256];
    while (true) {
        let n_opt = w.nextCandidate(filename, cand[0..256]);
        if (n_opt == null) { break; }
        let n = n_opt ?? 0;
        let cand_str = cand[0..n] as string;
        let i: usize = 0;
        while (i < existing_paths.len) {
            if (fupStrEq(existing_paths[i], cand_str)) {
                return fupPush(out, 0, cand_str);
            }
            i = i + 1;
        }
    }
    return null;
}

pub fn findUpMultipleInList(opts: FindUpOptions, filenames: []string, existing_paths: []string, out: []u8): usize? {
    let w = FindUpWalker.new(opts);
    let cand: [256]u8 = [0; 256];
    while (true) {
        let d_opt = w.nextDir();
        if (d_opt == null) { break; }
        let d = d_opt ?? "";
        let fi: usize = 0;
        while (fi < filenames.len) {
            let n = joinDirAndName(d, filenames[fi], cand[0..256]);
            let cand_str = cand[0..n] as string;
            let ei: usize = 0;
            while (ei < existing_paths.len) {
                if (fupStrEq(existing_paths[ei], cand_str)) {
                    return fupPush(out, 0, cand_str);
                }
                ei = ei + 1;
            }
            fi = fi + 1;
        }
    }
    return null;
}
