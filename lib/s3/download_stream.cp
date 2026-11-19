// Source: /data/dev/bun/src/s3_signing/
const std = @import("std");

pub struct S3ContentRange {
    pub start: usize = 0,
    pub end: usize = 0,
    pub total: usize = 0,
}

pub struct S3DownloadStreamOptions {
    pub chunk_size: usize = 64 * 1024,
}

fn streamAppendSlice(dst: []u8, offset: usize, src: []const u8): usize {
    let i: usize = 0;
    while (i < src.len and offset + i < dst.len) {
        dst[offset + i] = src[i];
        i += 1;
    }
    return offset + i;
}

fn streamAppendUsize(dst: []u8, offset: usize, val: usize): usize {
    if (val == 0) {
        if (offset < dst.len) {
            dst[offset] = 48 as u8; // '0'
            return offset + 1;
        }
        return offset;
    }
    let temp: [20]u8 = undefined;
    let count: usize = 0;
    let v = val;
    while (v > 0) {
        temp[count] = ((48 as u8) + ((v % 10) as u8)) as u8;
        v /= 10;
        count += 1;
    }
    let cur = offset;
    while (count > 0 and cur < dst.len) {
        count -= 1;
        dst[cur] = temp[count];
        cur += 1;
    }
    return cur;
}

/// Formats standard HTTP Range request header value:
/// - with size: `bytes={offset}-{offset + size - 1}`
/// - without size: `bytes={offset}-`
pub fn formatRangeHeader(offset: usize, size: usize?, out: []u8): usize {
    let cur: usize = 0;
    cur = streamAppendSlice(out, cur, "bytes=");
    cur = streamAppendUsize(out, cur, offset);
    cur = streamAppendSlice(out, cur, "-");

    if (sz := size) {
        if (sz > 0) {
            let end = (offset + sz) - 1;
            cur = streamAppendUsize(out, cur, end);
        } else {
            cur = streamAppendUsize(out, cur, offset);
        }
    }
    return cur;
}

/// Formats Range header into an allocated string.
pub fn formatRangeString(offset: usize, size: usize?, allocator: std.mem.Allocator): !string {
    let buf: [64]u8 = undefined;
    let len = formatRangeHeader(offset, size, buf[0..64]);
    let result = allocator.alloc(u8, len)?;
    std.mem.copyForwards(u8, result, buf[0..len]);
    return result;
}

fn streamParseUsize(s: string): usize? {
    if (s.len == 0) return null;
    let i: usize = 0;
    while (i < s.len and (s[i] == (32 as u8) or s[i] == (9 as u8))) {
        i += 1;
    }
    if (i >= s.len) return null;
    let val: usize = 0;
    let found = false;
    while (i < s.len and s[i] >= (48 as u8) and s[i] <= (57 as u8)) {
        val = val * 10 + ((s[i] - (48 as u8)) as usize);
        found = true;
        i += 1;
    }
    if (!found) return null;
    return val;
}

/// Parses standard HTTP Content-Range header: `bytes <start>-<end>/<total>`
pub fn parseContentRange(header: string): S3ContentRange? {
    // Trim leading whitespace
    let i: usize = 0;
    while (i < header.len and (header[i] == (32 as u8) or header[i] == (9 as u8))) {
        i += 1;
    }
    let rest = header[i..header.len];
    if (rest.len < 6) return null;
    // Check "bytes " prefix
    if (rest[0] != (98 as u8) or rest[1] != (121 as u8) or rest[2] != (116 as u8) or
        rest[3] != (101 as u8) or rest[4] != (115 as u8) or rest[5] != (32 as u8)) {
        return null;
    }

    let range_str = rest[6..rest.len];
    let dash_pos: usize? = null;
    let slash_pos: usize? = null;

    let j: usize = 0;
    while (j < range_str.len) {
        if (range_str[j] == (45 as u8) and dash_pos == null) {
            dash_pos = j;
        } else if (range_str[j] == (47 as u8)) {
            slash_pos = j;
            break;
        }
        j += 1;
    }

    let d: usize = 0;
    if (pos := dash_pos) {
        d = pos;
    } else {
        return null;
    }

    let s: usize = 0;
    if (pos := slash_pos) {
        s = pos;
    } else {
        return null;
    }

    if (d >= s) return null;

    let start_part = range_str[0..d];
    let end_part = range_str[d + 1 .. s];
    let total_part = range_str[s + 1 .. range_str.len];

    let start_val: usize = 0;
    if (v := streamParseUsize(start_part)) {
        start_val = v;
    } else {
        return null;
    }

    let end_val: usize = 0;
    if (v := streamParseUsize(end_part)) {
        end_val = v;
    } else {
        return null;
    }

    let total_val: usize = 0;
    if (v := streamParseUsize(total_part)) {
        total_val = v;
    }

    return S3ContentRange{
        start: start_val,
        end: end_val,
        total: total_val,
    };
}

/// Calculates chunk count for chunked downloads
pub fn calculateChunkCount(total_size: usize, chunk_size: usize): usize {
    if (chunk_size == 0) return 0;
    return (total_size + chunk_size - 1) / chunk_size;
}
