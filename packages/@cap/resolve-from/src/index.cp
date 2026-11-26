import { FindUpOptions, FindUpWalker, joinDirAndName } from "../../find-up/src/index.cp";
import { dirnameOf } from "../../npm-run-path/src/index.cp";

fn rfStrEq(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn rfPush(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p = p + 1;
        i = i + 1;
    }
    return p;
}

fn rfExists(path: string, files: []string): bool {
    let i: usize = 0;
    while (i < files.len) {
        if (rfStrEq(files[i], path)) { return true; }
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
    let cand: [256]u8 = [0; 256];
    let i: usize = 0;
    while (i < 6) {
        let p = rfPush(cand[0..256], 0, base);
        p = rfPush(cand[0..256], p, exts[i]);
        let s = cand[0..p] as string;
        if (rfExists(s, files)) {
            return rfPush(out, 0, s);
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
        } else if (rfStrEq(rest, ".")) {
            rest = "";
            break;
        } else if (rfStrEq(rest, "..")) {
            cur = dirnameOf(cur, false);
            rest = "";
            break;
        } else {
            break;
        }
    }
    if (rest.len == 0) {
        return rfPush(out, 0, cur);
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
        let base_buf: [256]u8 = [0; 256];
        let bn = rfResolveRelativeBase(from_dir, module_id, base_buf[0..256]);
        return rfProbeBase(base_buf[0..bn] as string, existing_files, out);
    }

    // Bare or scoped package: walk ancestor node_modules & cap_modules
    let w = FindUpWalker.new(FindUpOptions.defaults(from_dir));
    let mod_dirs: [2]string = ["node_modules", "cap_modules"];
    let dir_buf: [256]u8 = [0; 256];
    let pkg_buf: [256]u8 = [0; 256];
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
