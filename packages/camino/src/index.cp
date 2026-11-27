// @cap/camino — Zero-heap strictly validated UTF-8 path engine (crates.io camino equivalent)
// Clean-room MIT implementation for CP.
// Accelerated with 16-lane @Vector(16, u8) SIMD slash scanning and @memcpy bulk copies.

import { isValidUtf8 } from "../../utf8parse/src/index.cp";
import { bytesEq } from "cap:bin";

type CamVec16 = @Vector(16, u8);

pub const CAMINO_MAX_PATH: usize = 512;

struct CamSpan {
    start: usize,
    end: usize,
    found: bool,
}

struct CamCompStep {
    start: usize,
    end: usize,
    next_pos: usize,
    is_root: bool,
    found: bool,
}

fn camFindNextSlash(data: []const u8, start: usize, len: usize): usize {
    let i: usize = start;
    const v_sl: CamVec16 = @splat('/' as u8);
    while (i + 16 <= len) {
        const v: CamVec16 = data[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_sl)) {
            let k: usize = 0;
            while (k < 16) {
                if (data[i + k] == '/') {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len && data[i] != '/') {
        i += 1;
    }
    return i;
}

fn camFindLastSlash(data: []const u8, trimmed: usize): usize? {
    let i: usize = trimmed;
    const v_sl: CamVec16 = @splat('/' as u8);
    while (i >= 16) {
        const base = i - 16;
        const v: CamVec16 = data[base .. i][0..16].*;
        if (@reduce(.Or, v == v_sl)) {
            let k: usize = 16;
            while (k > 0) {
                k -= 1;
                if (data[base + k] == '/') {
                    return base + k;
                }
            }
        }
        i = base;
    }
    while (i > 0) {
        i -= 1;
        if (data[i] == '/') {
            return i;
        }
    }
    return null;
}

fn camTrimTrailingSlashes(data: []const u8, len: usize): usize {
    if (len == 0) {
        return 0;
    }
    let end: usize = len;
    while (end > 1 && data[end - 1] == '/') {
        end -= 1;
    }
    return end;
}

fn camSliceEq(a: []const u8, a_start: usize, a_end: usize, b: []const u8, b_start: usize, b_end: usize): bool {
    return bytesEq(a[a_start..a_end], b[b_start..b_end]);
}

fn camIsDot(data: []const u8, start: usize, end: usize): bool {
    return (end - start) == 1 && data[start] == '.';
}

fn camIsDotDot(data: []const u8, start: usize, end: usize): bool {
    return (end - start) == 2 && data[start] == '.' && data[start + 1] == '.';
}

fn camFileNameSpan(data: []const u8, len: usize): CamSpan {
    if (len == 0) {
        return CamSpan { start: 0, end: 0, found: false };
    }
    const trimmed = camTrimTrailingSlashes(data, len);
    if (trimmed == 1 && data[0] == '/') {
        return CamSpan { start: 0, end: 0, found: false };
    }
    let start: usize = 0;
    const slash_opt = camFindLastSlash(data, trimmed);
    if (slash_opt != null) {
        start = (slash_opt ?? 0) + 1;
    }
    if (start >= trimmed || camIsDot(data, start, trimmed) || camIsDotDot(data, start, trimmed)) {
        return CamSpan { start: 0, end: 0, found: false };
    }
    return CamSpan { start: start, end: trimmed, found: true };
}

fn camFindLastDot(data: []const u8, start: usize, end: usize): usize? {
    let i: usize = end;
    while (i > start) {
        i -= 1;
        if (data[i] == '.') {
            if (i == start) {
                return null;
            }
            return i;
        }
    }
    return null;
}

fn camCopyToDst(data: []const u8, start: usize, end: usize, dst: []u8): usize? {
    const n = end - start;
    if (dst.len < n) {
        return null;
    }
    if (n > 0) {
        @memcpy(dst[0..n], data[start..end]);
    }
    return n;
}

fn camNextComponent(data: []const u8, len: usize, cursor: usize): CamCompStep {
    if (cursor >= len) {
        return CamCompStep { start: 0, end: 0, next_pos: len, is_root: false, found: false };
    }
    if (cursor == 0 && data[0] == '/') {
        let p: usize = 1;
        while (p < len && data[p] == '/') {
            p += 1;
        }
        return CamCompStep { start: 0, end: 1, next_pos: p, is_root: true, found: true };
    }
    let pos: usize = cursor;
    while (pos < len) {
        while (pos < len && data[pos] == '/') {
            pos += 1;
        }
        if (pos >= len) {
            return CamCompStep { start: 0, end: 0, next_pos: len, is_root: false, found: false };
        }
        const seg_start = pos;
        pos = camFindNextSlash(data, pos, len);
        const seg_end = pos;
        if (camIsDot(data, seg_start, seg_end)) {
            continue;
        }
        return CamCompStep { start: seg_start, end: seg_end, next_pos: pos, is_root: false, found: true };
    }
    return CamCompStep { start: 0, end: 0, next_pos: len, is_root: false, found: false };
}

fn camParentLength(data: []const u8, len: usize): usize? {
    if (len == 0) {
        return null;
    }
    const trimmed = camTrimTrailingSlashes(data, len);
    if (trimmed == 1 && data[0] == '/') {
        return null;
    }
    const slash_opt = camFindLastSlash(data, trimmed);
    if (slash_opt == null) {
        return 0;
    }
    const slash_idx = slash_opt ?? 0;
    if (slash_idx == 0) {
        return 1;
    }
    return camTrimTrailingSlashes(data, slash_idx);
}

pub struct Utf8PathBuf {
    pub buf: [512]u8,
    pub length: usize,

    pub static fn fromStr(s: string): Utf8PathBuf {
        let out = Utf8PathBuf { buf: undefined, length: 0 };
        const n = if (s.len > CAMINO_MAX_PATH) CAMINO_MAX_PATH else s.len;
        if (n > 0) {
            @memcpy(out.buf[0..n], s[0..n]);
        }
        out.length = n;
        return out;
    }

    pub static fn fromBytes(b: []u8): Utf8PathBuf? {
        if (b.len > CAMINO_MAX_PATH) {
            return null;
        }
        if (!isValidUtf8(b)) {
            return null;
        }
        let out = Utf8PathBuf { buf: undefined, length: b.len };
        if (b.len > 0) {
            @memcpy(out.buf[0..b.len], b[0..b.len]);
        }
        return out;
    }

    pub fn isAbsolute(): bool {
        return _.length > 0 && _.buf[0] == '/';
    }

    pub fn isRelative(): bool {
        return !_.isAbsolute();
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

    pub fn push(*_, path: string) {
        if (path.len > 0 && path[0] == '/') {
            const n = if (path.len > CAMINO_MAX_PATH) CAMINO_MAX_PATH else path.len;
            @memcpy(_.buf[0..n], path[0..n]);
            _.length = n;
            return;
        }
        if (path.len == 0) {
            return;
        }
        if (_.length > 0 && _.buf[_.length - 1] != '/') {
            if (_.length < CAMINO_MAX_PATH) {
                _.buf[_.length] = '/';
                _.length += 1;
            }
        }
        const avail = CAMINO_MAX_PATH - _.length;
        const take = if (path.len < avail) path.len else avail;
        if (take > 0) {
            @memcpy(_.buf[_.length .. _.length + take], path[0..take]);
            _.length += take;
        }
    }

    pub fn pop(*_): bool {
        const parent_len_opt = camParentLength(_.buf[0.._.length], _.length);
        if (parent_len_opt == null) {
            return false;
        }
        _.length = parent_len_opt ?? 0;
        return true;
    }

    pub fn parent(): Utf8PathBuf? {
        const parent_len_opt = camParentLength(_.buf[0.._.length], _.length);
        if (parent_len_opt == null) {
            return null;
        }
        const plen = parent_len_opt ?? 0;
        let out = Utf8PathBuf { buf: undefined, length: plen };
        if (plen > 0) {
            @memcpy(out.buf[0..plen], _.buf[0..plen]);
        }
        return out;
    }

    pub fn fileNameInto(dst: []u8): usize? {
        const span = camFileNameSpan(_.buf[0.._.length], _.length);
        if (!span.found) {
            return null;
        }
        return camCopyToDst(_.buf[0.._.length], span.start, span.end, dst);
    }

    pub fn fileStemInto(dst: []u8): usize? {
        const span = camFileNameSpan(_.buf[0.._.length], _.length);
        if (!span.found) {
            return null;
        }
        const dot_opt = camFindLastDot(_.buf[0.._.length], span.start, span.end);
        const stem_end = dot_opt ?? span.end;
        return camCopyToDst(_.buf[0.._.length], span.start, stem_end, dst);
    }

    pub fn extensionInto(dst: []u8): usize? {
        const span = camFileNameSpan(_.buf[0.._.length], _.length);
        if (!span.found) {
            return null;
        }
        const dot_opt = camFindLastDot(_.buf[0.._.length], span.start, span.end);
        if (dot_opt == null) {
            return null;
        }
        const dot_idx = dot_opt ?? span.end;
        return camCopyToDst(_.buf[0.._.length], dot_idx + 1, span.end, dst);
    }

    pub fn setFileName(*_, name: string) {
        const span = camFileNameSpan(_.buf[0.._.length], _.length);
        if (span.found) {
            * = _.pop();
        }
        _.push(name);
    }

    pub fn setExtension(*_, ext: string): bool {
        const span = camFileNameSpan(_.buf[0.._.length], _.length);
        if (!span.found) {
            return false;
        }
        const dot_opt = camFindLastDot(_.buf[0.._.length], span.start, span.end);
        const stem_end = dot_opt ?? span.end;
        if (ext.len == 0) {
            _.length = stem_end;
            return true;
        }
        if (stem_end + 1 + ext.len > CAMINO_MAX_PATH) {
            return false;
        }
        _.length = stem_end;
        _.buf[_.length] = '.';
        _.length += 1;
        @memcpy(_.buf[_.length .. _.length + ext.len], ext[0..ext.len]);
        _.length += ext.len;
        return true;
    }

    pub fn componentCount(): usize {
        let count: usize = 0;
        let cursor: usize = 0;
        while (true) {
            const step = camNextComponent(_.buf[0.._.length], _.length, cursor);
            if (!step.found) {
                break;
            }
            count += 1;
            cursor = step.next_pos;
        }
        return count;
    }

    pub fn hasPrefixPath(base: string): bool {
        let self_cur: usize = 0;
        let base_cur: usize = 0;
        while (true) {
            const bstep = camNextComponent(base, base.len, base_cur);
            if (!bstep.found) {
                return true;
            }
            const sstep = camNextComponent(_.buf[0.._.length], _.length, self_cur);
            if (!sstep.found) {
                return false;
            }
            if (bstep.is_root != sstep.is_root) {
                return false;
            }
            if (!bstep.is_root) {
                if (!camSliceEq(_.buf[0.._.length], sstep.start, sstep.end, base, bstep.start, bstep.end)) {
                    return false;
                }
            }
            self_cur = sstep.next_pos;
            base_cur = bstep.next_pos;
        }
        return true;
    }

    pub fn stripPrefix(base: string): Utf8PathBuf? {
        let self_cur: usize = 0;
        let base_cur: usize = 0;
        while (true) {
            const bstep = camNextComponent(base, base.len, base_cur);
            if (!bstep.found) {
                break;
            }
            const sstep = camNextComponent(_.buf[0.._.length], _.length, self_cur);
            if (!sstep.found) {
                return null;
            }
            if (bstep.is_root != sstep.is_root) {
                return null;
            }
            if (!bstep.is_root) {
                if (!camSliceEq(_.buf[0.._.length], sstep.start, sstep.end, base, bstep.start, bstep.end)) {
                    return null;
                }
            }
            self_cur = sstep.next_pos;
            base_cur = bstep.next_pos;
        }

        let out = Utf8PathBuf { buf: undefined, length: 0 };
        let first = true;
        while (true) {
            const sstep = camNextComponent(_.buf[0.._.length], _.length, self_cur);
            if (!sstep.found) {
                break;
            }
            if (sstep.is_root) {
                if (out.length < CAMINO_MAX_PATH) {
                    out.buf[out.length] = '/';
                    out.length += 1;
                }
                first = false;
            } else {
                if (!first && out.length > 0 && out.buf[out.length - 1] != '/') {
                    if (out.length < CAMINO_MAX_PATH) {
                        out.buf[out.length] = '/';
                        out.length += 1;
                    }
                }
                const seg_len = sstep.end - sstep.start;
                const avail = CAMINO_MAX_PATH - out.length;
                const take = if (seg_len < avail) seg_len else avail;
                if (take > 0) {
                    @memcpy(out.buf[out.length .. out.length + take], _.buf[sstep.start .. sstep.start + take]);
                    out.length += take;
                }
                first = false;
            }
            self_cur = sstep.next_pos;
        }
        return out;
    }
}
