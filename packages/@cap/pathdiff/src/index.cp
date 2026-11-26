pub const PATHDIFF_MAX_SEGS: usize = 32;

fn pdStrEq(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn pdPush(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p = p + 1;
        i = i + 1;
    }
    return p;
}

struct SegList {
    segs: [32]string,
    count: usize,
    is_abs: bool,
}

fn pdSplitNormalized(path: string): SegList {
    let arr: [32]string = [""; 32];
    let is_abs = path.len > 0 && path[0] == 47;
    let cnt: usize = 0;
    let i: usize = 0;
    while (i < path.len) {
        while (i < path.len && path[i] == 47) { i = i + 1; }
        if (i >= path.len) { break; }
        let start = i;
        while (i < path.len && path[i] != 47) { i = i + 1; }
        let seg = path[start..i];
        if (pdStrEq(seg, ".")) {
            continue;
        } else if (pdStrEq(seg, "..")) {
            if (cnt > 0 && !pdStrEq(arr[cnt - 1], "..")) {
                cnt = cnt - 1;
            } else if (!is_abs && cnt < PATHDIFF_MAX_SEGS) {
                arr[cnt] = "..";
                cnt = cnt + 1;
            }
        } else {
            if (cnt < PATHDIFF_MAX_SEGS) {
                arr[cnt] = seg;
                cnt = cnt + 1;
            }
        }
    }
    return SegList { segs: arr, count: cnt, is_abs: is_abs };
}

pub fn diffPaths(target: string, base: string, out: []u8): usize? {
    let t = pdSplitNormalized(target);
    let b = pdSplitNormalized(base);
    if (t.is_abs != b.is_abs) {
        return null;
    }

    let common: usize = 0;
    while (common < t.count && common < b.count && pdStrEq(t.segs[common], b.segs[common])) {
        common = common + 1;
    }

    let pos: usize = 0;
    let first = true;

    let up = b.count - common;
    let ui: usize = 0;
    while (ui < up) {
        if (!first) { pos = pdPush(out, pos, "/"); }
        first = false;
        pos = pdPush(out, pos, "..");
        ui = ui + 1;
    }

    let ti = common;
    while (ti < t.count) {
        if (!first) { pos = pdPush(out, pos, "/"); }
        first = false;
        pos = pdPush(out, pos, t.segs[ti]);
        ti = ti + 1;
    }

    if (first) {
        return pdPush(out, 0, ".");
    }
    return pos;
}
