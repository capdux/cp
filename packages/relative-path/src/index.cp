// @cap/relative-path — Portable platform-independent relative path manipulation (Clean-room MIT, zero-heap pure CP)
// Absorbed from crates.io: relative-path (MIT OR Apache-2.0)

import { bytesEq } from "cap:bin";

pub const REL_PATH_CAP: usize = 256;
const REL_MAX_SEGS: usize = 32;

fn rpIsSep(b: u8): bool {
    return b == '/' || b == '\\';
}

pub struct RelPath {
    pub buf: [256]u8;
    pub length: usize;

    pub static fn empty(): RelPath {
        return RelPath { buf: undefined, length: 0 };
    }

    pub static fn fromStr(s: string): RelPath {
        let out = RelPath.empty();
        let i: usize = 0;
        // Skip leading separators to guarantee a relative path
        while (i < s.len && rpIsSep(s[i])) {
            i = i + 1;
        }
        let prev_sep = false;
        while (i < s.len && out.length < REL_PATH_CAP) {
            let b = s[i];
            if (rpIsSep(b)) {
                if (!prev_sep) {
                    out.buf[out.length] = '/';
                    out.length = out.length + 1;
                    prev_sep = true;
                }
            } else {
                out.buf[out.length] = b;
                out.length = out.length + 1;
                prev_sep = false;
            }
            i = i + 1;
        }
        // Trim trailing '/'
        while (out.length > 0 && out.buf[out.length - 1] == '/') {
            out.length = out.length - 1;
        }
        return out;
    }

    pub fn len(): usize {
        return _.length;
    }

    pub fn isEmpty(): bool {
        return _.length == 0;
    }

    pub fn equals(other: string): bool {
        return bytesEq(_.buf[0.._.length], other);
    }

    pub fn copyTo(dst: []u8): usize {
        let n = _.length;
        if (dst.len < n) { n = dst.len; }
        let i: usize = 0;
        while (i < n) {
            dst[i] = _.buf[i];
            i = i + 1;
        }
        return n;
    }

    pub fn normalize(): RelPath {
        let seg_starts: [32]usize = undefined;
        let seg_lens: [32]usize = undefined;
        let seg_is_parent: [32]bool = undefined;
        let count: usize = 0;

        let i: usize = 0;
        while (i < _.length) {
            while (i < _.length && _.buf[i] == '/') { i = i + 1; }
            if (i >= _.length) { break; }
            let start = i;
            while (i < _.length && _.buf[i] != '/') { i = i + 1; }
            let slen = i - start;

            if (slen == 1 && _.buf[start] == '.') {
                continue;
            }
            if (slen == 2 && _.buf[start] == '.' && _.buf[start + 1] == '.') {
                if (count > 0 && !seg_is_parent[count - 1]) {
                    count = count - 1;
                } else if (count < REL_MAX_SEGS) {
                    seg_starts[count] = start;
                    seg_lens[count] = 2;
                    seg_is_parent[count] = true;
                    count = count + 1;
                }
                continue;
            }
            if (count < REL_MAX_SEGS) {
                seg_starts[count] = start;
                seg_lens[count] = slen;
                seg_is_parent[count] = false;
                count = count + 1;
            }
        }

        let out = RelPath.empty();
        let s: usize = 0;
        while (s < count) {
            if (s > 0 && out.length < REL_PATH_CAP) {
                out.buf[out.length] = '/';
                out.length = out.length + 1;
            }
            let k: usize = 0;
            while (k < seg_lens[s] && out.length < REL_PATH_CAP) {
                out.buf[out.length] = _.buf[seg_starts[s] + k];
                out.length = out.length + 1;
                k = k + 1;
            }
            s = s + 1;
        }
        return out;
    }

    pub fn join(other: string): RelPath {
        let rhs = RelPath.fromStr(other);
        if (_.length == 0) { return rhs; }
        if (rhs.length == 0) {
            let copy = RelPath.empty();
            let i: usize = 0;
            while (i < _.length) {
                copy.buf[i] = _.buf[i];
                i = i + 1;
            }
            copy.length = _.length;
            return copy;
        }
        let out = RelPath.empty();
        let i: usize = 0;
        while (i < _.length && out.length < REL_PATH_CAP) {
            out.buf[out.length] = _.buf[i];
            out.length = out.length + 1;
            i = i + 1;
        }
        if (out.length < REL_PATH_CAP) {
            out.buf[out.length] = '/';
            out.length = out.length + 1;
        }
        let j: usize = 0;
        while (j < rhs.length && out.length < REL_PATH_CAP) {
            out.buf[out.length] = rhs.buf[j];
            out.length = out.length + 1;
            j = j + 1;
        }
        return out;
    }

    pub fn joinNormalized(other: string): RelPath {
        let joined = _.join(other);
        return joined.normalize();
    }

    pub fn parent(): RelPath? {
        if (_.length == 0) { return null; }
        let last_sep: usize = 0;
        let found = false;
        let i: usize = 0;
        while (i < _.length) {
            if (_.buf[i] == '/') {
                last_sep = i;
                found = true;
            }
            i = i + 1;
        }
        if (!found) {
            return RelPath.empty();
        }
        let out = RelPath.empty();
        let k: usize = 0;
        while (k < last_sep) {
            out.buf[k] = _.buf[k];
            k = k + 1;
        }
        out.length = last_sep;
        return out;
    }

    pub fn fileNameInto(dst: []u8): usize? {
        if (_.length == 0) { return null; }
        let start: usize = 0;
        let i: usize = 0;
        while (i < _.length) {
            if (_.buf[i] == '/') { start = i + 1; }
            i = i + 1;
        }
        let flen = _.length - start;
        if (flen == 0) { return null; }
        if (flen == 2 && _.buf[start] == '.' && _.buf[start + 1] == '.') { return null; }
        if (flen == 1 && _.buf[start] == '.') { return null; }
        let n = flen;
        if (dst.len < n) { n = dst.len; }
        let k: usize = 0;
        while (k < n) {
            dst[k] = _.buf[start + k];
            k = k + 1;
        }
        return n;
    }

    pub fn extensionInto(dst: []u8): usize? {
        if (_.length == 0) { return null; }
        let start: usize = 0;
        let i: usize = 0;
        while (i < _.length) {
            if (_.buf[i] == '/') { start = i + 1; }
            i = i + 1;
        }
        let flen = _.length - start;
        if (flen <= 1) { return null; }
        // Find last '.' strictly after start (ignore leading dot like ".gitignore")
        let dot_pos: usize = 0;
        let found = false;
        let j = start + 1;
        while (j < _.length) {
            if (_.buf[j] == '.') {
                dot_pos = j;
                found = true;
            }
            j = j + 1;
        }
        if (!found || dot_pos + 1 >= _.length) { return null; }
        let elen = _.length - (dot_pos + 1);
        let n = elen;
        if (dst.len < n) { n = dst.len; }
        let k: usize = 0;
        while (k < n) {
            dst[k] = _.buf[dot_pos + 1 + k];
            k = k + 1;
        }
        return n;
    }

    pub fn withExtension(ext: string): RelPath {
        if (_.length == 0) { return RelPath.empty(); }
        let start: usize = 0;
        let i: usize = 0;
        while (i < _.length) {
            if (_.buf[i] == '/') { start = i + 1; }
            i = i + 1;
        }
        let cut = _.length;
        let j = start + 1;
        while (j < _.length) {
            if (_.buf[j] == '.') { cut = j; }
            j = j + 1;
        }
        let out = RelPath.empty();
        let k: usize = 0;
        while (k < cut && out.length < REL_PATH_CAP) {
            out.buf[out.length] = _.buf[k];
            out.length = out.length + 1;
            k = k + 1;
        }
        if (ext.len > 0 && out.length < REL_PATH_CAP) {
            out.buf[out.length] = '.';
            out.length = out.length + 1;
            let m: usize = 0;
            while (m < ext.len && out.length < REL_PATH_CAP) {
                out.buf[out.length] = ext[m];
                out.length = out.length + 1;
                m = m + 1;
            }
        }
        return out;
    }

    pub fn hasPrefixPath(base: string): bool {
        let b = RelPath.fromStr(base).normalize();
        let a = _.normalize();
        if (b.length == 0) { return true; }
        if (a.length < b.length) { return false; }
        if (!bytesEq(a.buf[0..b.length], b.buf[0..b.length])) { return false; }
        if (a.length == b.length) { return true; }
        return a.buf[b.length] == '/';
    }

    pub fn stripPrefix(base: string): RelPath? {
        let b = RelPath.fromStr(base).normalize();
        let a = _.normalize();
        if (b.length == 0) { return a; }
        if (a.length < b.length) { return null; }
        if (!bytesEq(a.buf[0..b.length], b.buf[0..b.length])) { return null; }
        if (a.length == b.length) { return RelPath.empty(); }
        if (a.buf[b.length] != '/') { return null; }
        let out = RelPath.empty();
        let j = b.length + 1;
        while (j < a.length && out.length < REL_PATH_CAP) {
            out.buf[out.length] = a.buf[j];
            out.length = out.length + 1;
            j = j + 1;
        }
        return out;
    }

    pub fn toLogicalPath(base: string): RelPath {
        let b = RelPath.fromStr(base);
        let combined = b.join("");
        if (_.length > 0) {
            let out = RelPath.empty();
            let i: usize = 0;
            while (i < b.length && out.length < REL_PATH_CAP) {
                out.buf[out.length] = b.buf[i];
                out.length = out.length + 1;
                i = i + 1;
            }
            if (b.length > 0 && out.length < REL_PATH_CAP) {
                out.buf[out.length] = '/';
                out.length = out.length + 1;
            }
            let j: usize = 0;
            while (j < _.length && out.length < REL_PATH_CAP) {
                out.buf[out.length] = _.buf[j];
                out.length = out.length + 1;
                j = j + 1;
            }
            combined = out;
        }
        return combined.normalize();
    }
}

pub fn normalizeRelPath(path: string): RelPath {
    return RelPath.fromStr(path).normalize();
}

pub fn joinRelPath(base: string, rel: string): RelPath {
    return RelPath.fromStr(base).joinNormalized(rel);
}

pub fn relativeTo(from_path: string, to_path: string): RelPath {
    let a = RelPath.fromStr(from_path).normalize();
    let b = RelPath.fromStr(to_path).normalize();

    let a_starts: [32]usize = undefined;
    let a_lens: [32]usize = undefined;
    let a_cnt: usize = 0;
    let i: usize = 0;
    while (i < a.length && a_cnt < REL_MAX_SEGS) {
        while (i < a.length && a.buf[i] == '/') { i = i + 1; }
        if (i >= a.length) { break; }
        let s = i;
        while (i < a.length && a.buf[i] != '/') { i = i + 1; }
        a_starts[a_cnt] = s;
        a_lens[a_cnt] = i - s;
        a_cnt = a_cnt + 1;
    }

    let b_starts: [32]usize = undefined;
    let b_lens: [32]usize = undefined;
    let b_cnt: usize = 0;
    let j: usize = 0;
    while (j < b.length && b_cnt < REL_MAX_SEGS) {
        while (j < b.length && b.buf[j] == '/') { j = j + 1; }
        if (j >= b.length) { break; }
        let s = j;
        while (j < b.length && b.buf[j] != '/') { j = j + 1; }
        b_starts[b_cnt] = s;
        b_lens[b_cnt] = j - s;
        b_cnt = b_cnt + 1;
    }

    // Find common prefix segment count
    let common: usize = 0;
    while (common < a_cnt && common < b_cnt) {
        if (!bytesEq(
            a.buf[a_starts[common]..(a_starts[common] + a_lens[common])],
            b.buf[b_starts[common]..(b_starts[common] + b_lens[common])]
        )) {
            break;
        }
        common = common + 1;
    }

    let out = RelPath.empty();
    let up = common;
    while (up < a_cnt) {
        if (out.length > 0 && out.length < REL_PATH_CAP) {
            out.buf[out.length] = '/';
            out.length = out.length + 1;
        }
        if (out.length + 2 <= REL_PATH_CAP) {
            out.buf[out.length] = '.';
            out.buf[out.length + 1] = '.';
            out.length = out.length + 2;
        }
        up = up + 1;
    }

    let dn = common;
    while (dn < b_cnt) {
        if (out.length > 0 && out.length < REL_PATH_CAP) {
            out.buf[out.length] = '/';
            out.length = out.length + 1;
        }
        let k: usize = 0;
        while (k < b_lens[dn] && out.length < REL_PATH_CAP) {
            out.buf[out.length] = b.buf[b_starts[dn] + k];
            out.length = out.length + 1;
            k = k + 1;
        }
        dn = dn + 1;
    }
    return out;
}
