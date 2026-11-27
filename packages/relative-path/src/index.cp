// @cap/relative-path — Portable platform-independent relative path manipulation (Clean-room MIT, zero-heap pure CP)
// Absorbed from crates.io: relative-path (MIT OR Apache-2.0)

import { bytesEq } from "cap:bin";

pub const REL_PATH_CAP: usize = 256;
const REL_MAX_SEGS: usize = 32;

type RpVec16 = @Vector(16, u8);

fn rpBuildSepLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    lut['/' as usize] = 1;
    lut['\\' as usize] = 1;
    return lut;
}

const RP_SEP_LUT: [256]u8 = rpBuildSepLut();

fn rpIsSep(b: u8): bool {
    return RP_SEP_LUT[b as usize] != 0;
}

fn rpFindSep(s: string, start: usize): usize {
    let i: usize = start;
    const len = s.len;
    const v_fwd: RpVec16 = @splat('/' as u8);
    const v_bwd: RpVec16 = @splat('\\' as u8);
    while (i + 16 <= len) {
        const v: RpVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_fwd) || @reduce(.Or, v == v_bwd)) {
            let k: usize = 0;
            while (k < 16) {
                if (RP_SEP_LUT[s[i + k] as usize] != 0) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len && RP_SEP_LUT[s[i] as usize] == 0) {
        i += 1;
    }
    return i;
}

fn rpFindSlash(buf: []const u8, start: usize): usize {
    let i: usize = start;
    const len = buf.len;
    const v_sl: RpVec16 = @splat('/' as u8);
    while (i + 16 <= len) {
        const v: RpVec16 = buf[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_sl)) {
            let k: usize = 0;
            while (k < 16) {
                if (buf[i + k] == '/') {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len && buf[i] != '/') {
        i += 1;
    }
    return i;
}

fn rpRfindByte(buf: []const u8, target: u8): usize? {
    let i: usize = buf.len;
    const vt: RpVec16 = @splat(target);
    while (i >= 16) {
        const base = i - 16;
        const v: RpVec16 = buf[base .. base + 16][0..16].*;
        if (@reduce(.Or, v == vt)) {
            let k: usize = 16;
            while (k > 0) {
                k -= 1;
                if (buf[base + k] == target) {
                    return base + k;
                }
            }
        }
        i = base;
    }
    while (i > 0) {
        i -= 1;
        if (buf[i] == target) {
            return i;
        }
    }
    return null;
}

fn rpAppendSlice(out_buf: *[256]u8, out_len: *usize, src: []const u8) {
    if (out_len.* >= REL_PATH_CAP || src.len == 0) { return; }
    const avail = REL_PATH_CAP - out_len.*;
    const n = if (src.len < avail) src.len else avail;
    @memcpy(out_buf.*[out_len.* .. out_len.* + n], src[0..n]);
    out_len.* = out_len.* + n;
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
        while (i < s.len && out.length < REL_PATH_CAP) {
            const next_sep = rpFindSep(s, i);
            if (next_sep > i) {
                rpAppendSlice(&out.buf, &out.length, s[i..next_sep]);
                i = next_sep;
            }
            if (i < s.len && rpIsSep(s[i])) {
                while (i < s.len && rpIsSep(s[i])) {
                    i = i + 1;
                }
                if (i < s.len && out.length < REL_PATH_CAP) {
                    out.buf[out.length] = '/';
                    out.length = out.length + 1;
                }
            }
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
        if (n > 0) {
            @memcpy(dst[0..n], _.buf[0..n]);
        }
        return n;
    }

    pub fn normalize(): RelPath {
        let seg_starts: [32]usize = undefined;
        let seg_lens: [32]usize = undefined;
        let seg_is_parent: [32]bool = undefined;
        let count: usize = 0;

        const slice = _.buf[0.._.length];
        let i: usize = 0;
        while (i < _.length) {
            while (i < _.length && _.buf[i] == '/') { i = i + 1; }
            if (i >= _.length) { break; }
            let start = i;
            i = rpFindSlash(slice, start);
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
            rpAppendSlice(&out.buf, &out.length, _.buf[seg_starts[s] .. seg_starts[s] + seg_lens[s]]);
            s = s + 1;
        }
        return out;
    }

    pub fn join(other: string): RelPath {
        let rhs = RelPath.fromStr(other);
        if (_.length == 0) { return rhs; }
        if (rhs.length == 0) {
            let copy = RelPath.empty();
            @memcpy(copy.buf[0.._.length], _.buf[0.._.length]);
            copy.length = _.length;
            return copy;
        }
        let out = RelPath.empty();
        rpAppendSlice(&out.buf, &out.length, _.buf[0.._.length]);
        if (out.length < REL_PATH_CAP) {
            out.buf[out.length] = '/';
            out.length = out.length + 1;
        }
        rpAppendSlice(&out.buf, &out.length, rhs.buf[0..rhs.length]);
        return out;
    }

    pub fn joinNormalized(other: string): RelPath {
        let joined = _.join(other);
        return joined.normalize();
    }

    pub fn parent(): RelPath? {
        if (_.length == 0) { return null; }
        const last_sep = rpRfindByte(_.buf[0.._.length], '/') ?? return RelPath.empty();
        let out = RelPath.empty();
        if (last_sep > 0) {
            @memcpy(out.buf[0..last_sep], _.buf[0..last_sep]);
        }
        out.length = last_sep;
        return out;
    }

    pub fn fileNameInto(dst: []u8): usize? {
        if (_.length == 0) { return null; }
        let start: usize = 0;
        if (sep := rpRfindByte(_.buf[0.._.length], '/')) {
            start = sep + 1;
        }
        let flen = _.length - start;
        if (flen == 0) { return null; }
        if (flen == 2 && _.buf[start] == '.' && _.buf[start + 1] == '.') { return null; }
        if (flen == 1 && _.buf[start] == '.') { return null; }
        let n = flen;
        if (dst.len < n) { n = dst.len; }
        if (n > 0) {
            @memcpy(dst[0..n], _.buf[start .. start + n]);
        }
        return n;
    }

    pub fn extensionInto(dst: []u8): usize? {
        if (_.length == 0) { return null; }
        let start: usize = 0;
        if (sep := rpRfindByte(_.buf[0.._.length], '/')) {
            start = sep + 1;
        }
        let flen = _.length - start;
        if (flen <= 1) { return null; }
        // Find last '.' strictly after start (ignore leading dot like ".gitignore")
        const rel_dot = rpRfindByte(_.buf[(start + 1).._.length], '.') ?? return null;
        const dot_pos = start + 1 + rel_dot;
        if (dot_pos + 1 >= _.length) { return null; }
        let elen = _.length - (dot_pos + 1);
        let n = elen;
        if (dst.len < n) { n = dst.len; }
        if (n > 0) {
            @memcpy(dst[0..n], _.buf[(dot_pos + 1)..(dot_pos + 1 + n)]);
        }
        return n;
    }

    pub fn withExtension(ext: string): RelPath {
        if (_.length == 0) { return RelPath.empty(); }
        let start: usize = 0;
        if (sep := rpRfindByte(_.buf[0.._.length], '/')) {
            start = sep + 1;
        }
        let cut = _.length;
        if (start + 1 < _.length) {
            if (rel_dot := rpRfindByte(_.buf[(start + 1).._.length], '.')) {
                cut = start + 1 + rel_dot;
            }
        }
        let out = RelPath.empty();
        rpAppendSlice(&out.buf, &out.length, _.buf[0..cut]);
        if (ext.len > 0 && out.length < REL_PATH_CAP) {
            out.buf[out.length] = '.';
            out.length = out.length + 1;
            rpAppendSlice(&out.buf, &out.length, ext);
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
        rpAppendSlice(&out.buf, &out.length, a.buf[(b.length + 1)..a.length]);
        return out;
    }

    pub fn toLogicalPath(base: string): RelPath {
        let b = RelPath.fromStr(base);
        let combined = b.join("");
        if (_.length > 0) {
            let out = RelPath.empty();
            rpAppendSlice(&out.buf, &out.length, b.buf[0..b.length]);
            if (b.length > 0 && out.length < REL_PATH_CAP) {
                out.buf[out.length] = '/';
                out.length = out.length + 1;
            }
            rpAppendSlice(&out.buf, &out.length, _.buf[0.._.length]);
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
        i = rpFindSlash(a.buf[0..a.length], s);
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
        j = rpFindSlash(b.buf[0..b.length], s);
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
        rpAppendSlice(&out.buf, &out.length, b.buf[b_starts[dn] .. b_starts[dn] + b_lens[dn]]);
        dn = dn + 1;
    }
    return out;
}
