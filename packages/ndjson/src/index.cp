// @cap/ndjson — Zero-heap NDJSON / JSON Lines streaming splitter & writer
// MIT License (clean-room CP implementation)
// Accelerated with 16-lane @Vector(16, u8) SIMD newline/blank/CRLF scanning and @memcpy.

type NdjVec16 = @Vector(16, u8);

pub const MAX_LINE_BYTES: usize = 1024;

pub struct NdjsonLine {
    pub start: usize,
    pub len: usize,
    pub found: bool,
}

fn ndjFindNewline(src: []const u8, start: usize) -> usize {
    let i = start;
    const len = src.len;
    const v_nl: NdjVec16 = @splat(10 as u8);
    while (i + 16 <= len) {
        const v: NdjVec16 = src[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_nl)) {
            let k: usize = 0;
            while (k < 16) {
                if (src[i + k] == 10) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len && src[i] != 10) {
        i += 1;
    }
    return i;
}

fn isBlankSlice(s: []const u8) -> bool {
    let i: usize = 0;
    const len = s.len;
    const v_sp: NdjVec16 = @splat(32 as u8);
    const v_tab: NdjVec16 = @splat(9 as u8);
    while (i + 16 <= len) {
        const v: NdjVec16 = s[i .. i + 16][0..16].*;
        if (!@reduce(.And, (v == v_sp) | (v == v_tab))) {
            return false;
        }
        i += 16;
    }
    while (i < len) {
        const c: u8 = s[i];
        if (c != 32 && c != 9) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn ndjHasCrOrLf(s: []const u8) -> bool {
    let i: usize = 0;
    const len = s.len;
    const v_nl: NdjVec16 = @splat(10 as u8);
    const v_cr: NdjVec16 = @splat(13 as u8);
    while (i + 16 <= len) {
        const v: NdjVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, (v == v_nl) | (v == v_cr))) {
            return true;
        }
        i += 16;
    }
    while (i < len) {
        const c = s[i];
        if (c == 10 || c == 13) {
            return true;
        }
        i += 1;
    }
    return false;
}

pub fn nextLine(src: []const u8, start_pos: usize, out_next_pos: *usize) -> NdjsonLine {
    let pos: usize = start_pos;
    while (pos < src.len) {
        const line_end = ndjFindNewline(src, pos);
        let raw_end: usize = line_end;
        if (raw_end > pos && src[raw_end - 1] == '\r' as u8) {
            raw_end -= 1;
        }
        let next_p: usize = line_end;
        if (next_p < src.len && src[next_p] == '\n' as u8) {
            next_p += 1;
        }
        if (raw_end > pos && !isBlankSlice(src[pos..raw_end])) {
            out_next_pos.* = next_p;
            return NdjsonLine {
                start: pos,
                len: raw_end - pos,
                found: true,
            };
        }
        pos = next_p;
    }
    out_next_pos.* = src.len;
    return NdjsonLine { start: 0, len: 0, found: false };
}

pub fn countLines(src: []const u8) -> usize {
    let count: usize = 0;
    let pos: usize = 0;
    while (pos < src.len) {
        let next_pos: usize = pos;
        const line: NdjsonLine = nextLine(src, pos, &next_pos);
        if (!line.found) {
            break;
        }
        count += 1;
        pos = next_pos;
    }
    return count;
}

pub struct NdjsonSplitter {
    pub buf: [1024]u8,
    pub pos: usize,
    pub ready_len: usize,
    pub overflowed: bool,

    pub static fn init() -> NdjsonSplitter {
        return NdjsonSplitter {
            buf: undefined,
            pos: 0,
            ready_len: 0,
            overflowed: false,
        };
    }

    pub fn reset(*_) {
        _.pos = 0;
        _.ready_len = 0;
        _.overflowed = false;
    }

    pub fn pushByte(*_, b: u8) -> i64 {
        if (b == '\n' as u8) {
            if (_.overflowed) {
                _.overflowed = false;
                _.pos = 0;
                _.ready_len = 0;
                return 0;
            }
            let end_len: usize = _.pos;
            if (end_len > 0 && _.buf[end_len - 1] == '\r' as u8) {
                end_len -= 1;
            }
            _.pos = 0;
            if (end_len == 0 || isBlankSlice(_.buf[0..end_len])) {
                _.ready_len = 0;
                return 0;
            }
            _.ready_len = end_len;
            return end_len as i64;
        }

        if (_.overflowed) {
            return -1;
        }
        if (_.pos >= MAX_LINE_BYTES) {
            _.overflowed = true;
            _.ready_len = 0;
            return -1;
        }
        _.buf[_.pos] = b;
        _.pos += 1;
        return 0;
    }

    pub fn currentLine() -> []const u8 {
        return _.buf[0.._.ready_len];
    }

    pub fn flush(*_) -> i64 {
        if (_.overflowed) {
            _.overflowed = false;
            _.pos = 0;
            _.ready_len = 0;
            return -1;
        }
        let end_len: usize = _.pos;
        if (end_len > 0 && _.buf[end_len - 1] == '\r' as u8) {
            end_len -= 1;
        }
        _.pos = 0;
        if (end_len == 0 || isBlankSlice(_.buf[0..end_len])) {
            _.ready_len = 0;
            return 0;
        }
        _.ready_len = end_len;
        return end_len as i64;
    }
}

pub fn writeLine(json_record: []const u8, dst: []u8) -> i64 {
    if (json_record.len == 0 || json_record.len + 1 > dst.len) {
        return -1;
    }
    if (ndjHasCrOrLf(json_record)) {
        return -1;
    }
    @memcpy(dst[0..json_record.len], json_record);
    dst[json_record.len] = '\n' as u8;
    return (json_record.len + 1) as i64;
}
