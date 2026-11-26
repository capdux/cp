import { FindUpOptions, findUpInList, findUpMultipleInList } from "../../find-up/src/index.cp";
import { dirnameOf } from "../../npm-run-path/src/index.cp";

fn pkdPush(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p = p + 1;
        i = i + 1;
    }
    return p;
}

pub fn packageDirectoryInList(cwd: string, existing_files: []string, out: []u8): usize? {
    let opts = FindUpOptions.defaults(cwd);
    let found_buf: [256]u8 = [0; 256];
    let n_opt = findUpInList(opts, "package.json", existing_files, found_buf[0..256]);
    if (n_opt == null) { return null; }
    let n = n_opt ?? 0;
    let dir = dirnameOf(found_buf[0..n] as string, false);
    return pkdPush(out, 0, dir);
}

pub fn capPackageDirectoryInList(cwd: string, existing_files: []string, out: []u8): usize? {
    let opts = FindUpOptions.defaults(cwd);
    let manifests: [2]string = ["cap.json", "package.json"];
    let found_buf: [256]u8 = [0; 256];
    let n_opt = findUpMultipleInList(opts, manifests[0..2], existing_files, found_buf[0..256]);
    if (n_opt == null) { return null; }
    let n = n_opt ?? 0;
    let dir = dirnameOf(found_buf[0..n] as string, false);
    return pkdPush(out, 0, dir);
}

pub fn packageDirectoryBounded(cwd: string, stop_at: string, existing_files: []string, out: []u8): usize? {
    let opts = FindUpOptions.defaults(cwd);
    opts.stop_at = stop_at;
    let found_buf: [256]u8 = [0; 256];
    let n_opt = findUpInList(opts, "package.json", existing_files, found_buf[0..256]);
    if (n_opt == null) { return null; }
    let n = n_opt ?? 0;
    let dir = dirnameOf(found_buf[0..n] as string, false);
    return pkdPush(out, 0, dir);
}
