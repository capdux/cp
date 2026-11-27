import { strEq } from "cap:ascii";
import { writeStr } from "cap:fmt";
import { FindUpOptions, FindUpWalker, joinDirAndName } from "../../find-up/src/index.cp";
import { dirnameOf } from "../../npm-run-path/src/index.cp";

fn rfExists(path: string, files: []string): bool {
    let i: usize = 0;
    while (i < files.len) {
        if (strEq(files[i], path)) { return true; }
        i = i + 1;
    }
    return false;
}

pub fn isRelativeSpecifier(spec: string): bool {
    if (spec.len == 0) { return false; }
    if (spec.len == 1 && spec[0] == 46) { return true; } // "."
    if (spec.len >= 2 && spec[0] == 46 && spec[1] == 47) { return true; } // "./"
    if (spec.len == 2 && spec[0] == 46 && spec[1] == 46) { return true; } // ".."
    if (spec.len >= 3 && spec[0] == 46 && spec[1] == 46 && spec[2] == 47) { return true; } // "../"
    return false;
}

pub fn isScopedSpecifier(spec: string): bool {
    return spec.len >= 3 && spec[0] == 64; // '@'
}

fn rfProbeBase(base: string, files: []string, out: []u8): usize? {
    let exts: [6]string = ["", ".cp", ".ts", ".js", "/index.cp", "/index.js"];
    let cand: [256]u8 = undefined;
    let i: usize = 0;
    while (i < 6) {
        let p = writeStr(base, cand[0..256], 0);
        p = writeStr(exts[i], cand[0..256], p);
        let s = cand[0..p] as string;
        if (rfExists(s, files)) {
            return writeStr(s, out, 0);
        }
        i = i + 1;
    }
    return null;
}

fn rfResolveRelativeBase(from_dir: string, rel: string, out: []u8): usize {
    let cur = from_dir;
    let rest = rel;
    while (true) {
        if (rest.len >= 2 && rest[0] == 46 && rest[1] == 47) { // "./"
            rest = rest[2..rest.len];
        } else if (rest.len >= 3 && rest[0] == 46 && rest[1] == 46 && rest[2] == 47) { // "../"
            cur = dirnameOf(cur, false);
            rest = rest[3..rest.len];
        } else if (strEq(rest, ".")) {
            rest = "";
            break;
        } else if (strEq(rest, "..")) {
            cur = dirnameOf(cur, false);
            rest = "";
            break;
        } else {
            break;
        }
    }
    if (rest.len == 0) {
        return writeStr(cur, out, 0);
    }
    return joinDirAndName(cur, rest, out);
}

pub fn resolveFrom(from_dir: string, module_id: string, existing_files: []string, out: []u8): usize? {
    if (from_dir.len == 0 || module_id.len == 0) { return null; }

    // Absolute specifier
    if (module_id[0] == 47) {
        return rfProbeBase(module_id, existing_files, out);
    }

    // Relative specifier (./ or ../)
    if (isRelativeSpecifier(module_id)) {
        let base_buf: [256]u8 = undefined;
        let bn = rfResolveRelativeBase(from_dir, module_id, base_buf[0..256]);
        return rfProbeBase(base_buf[0..bn] as string, existing_files, out);
    }

    // Bare or scoped package: walk ancestor node_modules & cap_modules
    let w = FindUpWalker.new(FindUpOptions.defaults(from_dir));
    let mod_dirs: [2]string = ["node_modules", "cap_modules"];
    let dir_buf: [256]u8 = undefined;
    let pkg_buf: [256]u8 = undefined;
    while (true) {
        let d_opt = w.nextDir();
        if (d_opt == null) { break; }
        let d = d_opt ?? "";
        let mi: usize = 0;
        while (mi < 2) {
            let dn = joinDirAndName(d, mod_dirs[mi], dir_buf[0..256]);
            let pn = joinDirAndName(dir_buf[0..dn] as string, module_id, pkg_buf[0..256]);
            let hit = rfProbeBase(pkg_buf[0..pn] as string, existing_files, out);
            if (hit != null) {
                return hit;
            }
            mi = mi + 1;
        }
    }
    return null;
}

pub const RESOLVE_OK: i32 = 0;
pub const RESOLVE_ERR_EMPTY_INPUT: i32 = 1;
pub const RESOLVE_ERR_NOT_FOUND: i32 = 2;
pub const RESOLVE_ERR_BUFFER_TOO_SMALL: i32 = 3;

pub struct ResolveResult {
    pub found: bool,
    pub len: usize,
    pub err_code: i32,
}

pub struct ResolveOptions {
    pub extensions: [8]string,
    pub ext_count: usize,
    pub include_node_modules: bool,
    pub include_cap_modules: bool,

    pub static fn defaults(): ResolveOptions {
        let exts: [8]string = ["", ".cp", ".ts", ".js", ".json", "/index.cp", "/index.ts", "/index.js"];
        return ResolveOptions {
            extensions: exts,
            ext_count: 8,
            include_node_modules: true,
            include_cap_modules: true,
        };
    }
}

/// Returns true if `spec` is a bare or scoped package specifier (not empty, not relative, not absolute).
pub fn isBareSpecifier(spec: string): bool {
    if (spec.len == 0) { return false; }
    if (spec[0] == 47) { return false; }
    if (isRelativeSpecifier(spec)) { return false; }
    return true;
}

/// Silent variant matching npm `resolveFrom.silent(fromDirectory, moduleId)`:
/// returns `null` when the module cannot be resolved instead of signaling an error.
pub fn resolveFromSilent(from_dir: string, module_id: string, existing_files: []string, out: []u8): usize? {
    return resolveFrom(from_dir, module_id, existing_files, out);
}

fn rfProbeBaseOpts(base: string, files: []string, opts: ResolveOptions, out: []u8): ResolveResult {
    let cand: [256]u8 = undefined;
    let i: usize = 0;
    const limit: usize = if (opts.ext_count <= 8) opts.ext_count else 8;
    while (i < limit) {
        let p = writeStr(base, cand[0..256], 0);
        p = writeStr(opts.extensions[i], cand[0..256], p);
        let s = cand[0..p] as string;
        if (rfExists(s, files)) {
            if (s.len > out.len) {
                return ResolveResult { found: false, len: 0, err_code: RESOLVE_ERR_BUFFER_TOO_SMALL };
            }
            const written = writeStr(s, out, 0);
            return ResolveResult { found: true, len: written, err_code: RESOLVE_OK };
        }
        i = i + 1;
    }
    return ResolveResult { found: false, len: 0, err_code: RESOLVE_ERR_NOT_FOUND };
}

/// Resolve a module specifier from `from_dir` with configurable extension probing and module directories,
/// returning a structured `ResolveResult` with error codes.
pub fn resolveFromWithOptions(from_dir: string, module_id: string, existing_files: []string, opts: ResolveOptions, out: []u8): ResolveResult {
    if (from_dir.len == 0 || module_id.len == 0) {
        return ResolveResult { found: false, len: 0, err_code: RESOLVE_ERR_EMPTY_INPUT };
    }

    if (module_id[0] == 47) {
        return rfProbeBaseOpts(module_id, existing_files, opts, out);
    }

    if (isRelativeSpecifier(module_id)) {
        let base_buf: [256]u8 = undefined;
        let bn = rfResolveRelativeBase(from_dir, module_id, base_buf[0..256]);
        return rfProbeBaseOpts(base_buf[0..bn] as string, existing_files, opts, out);
    }

    let w = FindUpWalker.new(FindUpOptions.defaults(from_dir));
    let dir_buf: [256]u8 = undefined;
    let pkg_buf: [256]u8 = undefined;
    while (true) {
        let d_opt = w.nextDir();
        if (d_opt == null) { break; }
        let d = d_opt ?? "";
        if (opts.include_node_modules) {
            let dn = joinDirAndName(d, "node_modules", dir_buf[0..256]);
            let pn = joinDirAndName(dir_buf[0..dn] as string, module_id, pkg_buf[0..256]);
            let res = rfProbeBaseOpts(pkg_buf[0..pn] as string, existing_files, opts, out);
            if (res.found || res.err_code == RESOLVE_ERR_BUFFER_TOO_SMALL) {
                return res;
            }
        }
        if (opts.include_cap_modules) {
            let dn = joinDirAndName(d, "cap_modules", dir_buf[0..256]);
            let pn = joinDirAndName(dir_buf[0..dn] as string, module_id, pkg_buf[0..256]);
            let res = rfProbeBaseOpts(pkg_buf[0..pn] as string, existing_files, opts, out);
            if (res.found || res.err_code == RESOLVE_ERR_BUFFER_TOO_SMALL) {
                return res;
            }
        }
    }
    return ResolveResult { found: false, len: 0, err_code: RESOLVE_ERR_NOT_FOUND };
}
