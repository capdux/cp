// @cap/clf-log — Zero-allocation Apache/Nginx CLF & Combined Access Log Parser and Formatter
// Clean-room MIT implementation in pure CP.
// Accelerated with 16-lane @Vector(16, u8) SIMD token/quote/bracket scanning, comptime 256-entry LUT, and @memcpy.

import { writeU64 } from "cap:fmt";

type ClfVec16 = @Vector(16, u8);

fn clfBuildDigitLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let tbl: [256]u8 = undefined;
    let i: usize = 0;
    while (i < 256) {
        tbl[i] = if (i >= 48 && i <= 57) { (i - 48) as u8 } else { 0xFF as u8 };
        i += 1;
    }
    return tbl;
}

const CLF_DIGIT_LUT: [256]u8 = clfBuildDigitLut();

fn clfSkipNonSpaceSimd(s: []const u8, start: usize, end: usize) -> usize {
    let i = start;
    const v_sp: ClfVec16 = @splat(32 as u8);
    const v_tb: ClfVec16 = @splat(9 as u8);
    while (i + 16 <= end) {
        const v: ClfVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_sp) || @reduce(.Or, v == v_tb)) {
            break;
        }
        i += 16;
    }
    while (i < end && s[i] != 32 && s[i] != 9) {
        i += 1;
    }
    return i;
}

fn clfFindByteSimd(s: []const u8, start: usize, end: usize, target: u8) -> usize {
    let i = start;
    const vt: ClfVec16 = @splat(target);
    while (i + 16 <= end) {
        const v: ClfVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == vt)) {
            break;
        }
        i += 16;
    }
    while (i < end && s[i] != target) {
        i += 1;
    }
    return i;
}

fn clfScanQuotedEndSimd(s: []const u8, start: usize, end: usize) -> usize {
    let i = start;
    const v_qt: ClfVec16 = @splat(34 as u8);
    const v_bs: ClfVec16 = @splat(92 as u8);
    while (i < end) {
        while (i + 16 <= end) {
            const v: ClfVec16 = s[i .. i + 16][0..16].*;
            if (@reduce(.Or, v == v_qt) || @reduce(.Or, v == v_bs)) {
                break;
            }
            i += 16;
        }
        while (i < end) {
            const c = s[i];
            if (c == 34) {
                return i;
            }
            if (c == 92 && i + 1 < end) {
                i += 2;
                break;
            }
            i += 1;
        }
    }
    return end;
}

pub struct ClfRecord {
    pub host_start: usize,
    pub host_len: usize,
    pub user_start: usize,
    pub user_len: usize,
    pub time_start: usize,
    pub time_len: usize,
    pub method_start: usize,
    pub method_len: usize,
    pub path_start: usize,
    pub path_len: usize,
    pub proto_start: usize,
    pub proto_len: usize,
    pub status: u16,
    pub bytes: i64,
    pub referer_start: usize,
    pub referer_len: usize,
    pub ua_start: usize,
    pub ua_len: usize,
    pub is_combined: bool,
    pub valid: bool,
}

fn invalidClf() -> ClfRecord {
    return ClfRecord {
        host_start: 0,
        host_len: 0,
        user_start: 0,
        user_len: 0,
        time_start: 0,
        time_len: 0,
        method_start: 0,
        method_len: 0,
        path_start: 0,
        path_len: 0,
        proto_start: 0,
        proto_len: 0,
        status: 0,
        bytes: -1,
        referer_start: 0,
        referer_len: 0,
        ua_start: 0,
        ua_len: 0,
        is_combined: false,
        valid: false,
    };
}

fn isClfSpace(b: u8) -> bool {
    return b == 32 || b == 9;
}

fn isLineEnd(b: u8) -> bool {
    return b == 10 || b == 13;
}

/// Parses an Apache/Nginx Common Log Format (CLF) or Combined Log Format line.
/// Extracts zero-copy byte offsets into `line`; `bytes == -1` when response size is `-`.
pub fn parseClf(line: []const u8) -> ClfRecord {
    let n = line.len;
    while (n > 0 && (isLineEnd(line[n - 1]) || isClfSpace(line[n - 1]))) {
        n -= 1;
    }
    let pos: usize = 0;
    while (pos < n && isClfSpace(line[pos])) {
        pos += 1;
    }
    if (pos >= n) {
        return invalidClf();
    }

    let rec = invalidClf();

    // 1. host
    const host_start = pos;
    pos = clfSkipNonSpaceSimd(line, pos, n);
    if (pos == host_start) {
        return invalidClf();
    }
    rec.host_start = host_start;
    rec.host_len = pos - host_start;

    // 2. ident (skip)
    while (pos < n && isClfSpace(line[pos])) {
        pos += 1;
    }
    const ident_start = pos;
    pos = clfSkipNonSpaceSimd(line, pos, n);
    if (pos == ident_start) {
        return invalidClf();
    }

    // 3. user
    while (pos < n && isClfSpace(line[pos])) {
        pos += 1;
    }
    const user_start = pos;
    pos = clfSkipNonSpaceSimd(line, pos, n);
    if (pos == user_start) {
        return invalidClf();
    }
    rec.user_start = user_start;
    rec.user_len = pos - user_start;

    // 4. [time]
    while (pos < n && isClfSpace(line[pos])) {
        pos += 1;
    }
    if (pos >= n || line[pos] != 91) {
        // '['
        return invalidClf();
    }
    pos += 1;
    const time_start = pos;
    pos = clfFindByteSimd(line, pos, n, 93 as u8);
    if (pos >= n || line[pos] != 93) {
        // ']'
        return invalidClf();
    }
    rec.time_start = time_start;
    rec.time_len = pos - time_start;
    pos += 1;

    // 5. "METHOD PATH PROTO"
    while (pos < n && isClfSpace(line[pos])) {
        pos += 1;
    }
    if (pos >= n || line[pos] != 34) {
        // '"'
        return invalidClf();
    }
    pos += 1;
    const req_start = pos;
    pos = clfScanQuotedEndSimd(line, pos, n);
    if (pos >= n || line[pos] != 34) {
        return invalidClf();
    }
    const req_end = pos;
    pos += 1;

    // Split request line into METHOD, PATH, PROTO
    let rp = req_start;
    while (rp < req_end && isClfSpace(line[rp])) {
        rp += 1;
    }
    const m_start = rp;
    rp = clfSkipNonSpaceSimd(line, rp, req_end);
    if (rp == m_start) {
        return invalidClf();
    }
    rec.method_start = m_start;
    rec.method_len = rp - m_start;

    while (rp < req_end && isClfSpace(line[rp])) {
        rp += 1;
    }
    const p_start = rp;
    rp = clfSkipNonSpaceSimd(line, rp, req_end);
    if (rp == p_start) {
        return invalidClf();
    }
    rec.path_start = p_start;
    rec.path_len = rp - p_start;

    while (rp < req_end && isClfSpace(line[rp])) {
        rp += 1;
    }
    const pr_start = rp;
    rp = clfSkipNonSpaceSimd(line, rp, req_end);
    if (rp == pr_start) {
        return invalidClf();
    }
    rec.proto_start = pr_start;
    rec.proto_len = rp - pr_start;

    // 6. status (3-digit HTTP status code 100..599 or valid u16 digits)
    while (pos < n && isClfSpace(line[pos])) {
        pos += 1;
    }
    const st_start = pos;
    let st_val: u32 = 0;
    while (pos < n && !isClfSpace(line[pos])) {
        const d = CLF_DIGIT_LUT[line[pos] as usize];
        if (d == (0xFF as u8)) {
            return invalidClf();
        }
        st_val = st_val * 10 + (d as u32);
        if (st_val > 999) {
            return invalidClf();
        }
        pos += 1;
    }
    if (pos == st_start || st_val < 100 || st_val > 599) {
        return invalidClf();
    }
    rec.status = st_val as u16;

    // 7. bytes ('-' -> -1, or non-negative integer)
    while (pos < n && isClfSpace(line[pos])) {
        pos += 1;
    }
    if (pos >= n) {
        return invalidClf();
    }
    if (line[pos] == 45 && (pos + 1 == n || isClfSpace(line[pos + 1]))) {
        rec.bytes = -1;
        pos += 1;
    } else {
        const b_start = pos;
        let b_val: i64 = 0;
        while (pos < n && !isClfSpace(line[pos])) {
            const d = CLF_DIGIT_LUT[line[pos] as usize];
            if (d == (0xFF as u8)) {
                return invalidClf();
            }
            b_val = b_val * 10 + (d as i64);
            pos += 1;
        }
        if (pos == b_start) {
            return invalidClf();
        }
        rec.bytes = b_val;
    }

    // 8. Optional Combined fields: "referer" "user-agent"
    while (pos < n && isClfSpace(line[pos])) {
        pos += 1;
    }
    if (pos < n) {
        if (line[pos] != 34) {
            return invalidClf();
        }
        pos += 1;
        const ref_start = pos;
        pos = clfScanQuotedEndSimd(line, pos, n);
        if (pos >= n || line[pos] != 34) {
            return invalidClf();
        }
        rec.referer_start = ref_start;
        rec.referer_len = pos - ref_start;
        pos += 1;

        while (pos < n && isClfSpace(line[pos])) {
            pos += 1;
        }
        if (pos >= n || line[pos] != 34) {
            return invalidClf();
        }
        pos += 1;
        const ua_start = pos;
        pos = clfScanQuotedEndSimd(line, pos, n);
        if (pos >= n || line[pos] != 34) {
            return invalidClf();
        }
        rec.ua_start = ua_start;
        rec.ua_len = pos - ua_start;
        rec.is_combined = true;
    }

    rec.valid = true;
    return rec;
}

fn pushSlice(dst: []u8, pos: usize, src: []const u8) -> i64 {
    if (src.len > dst.len - pos) {
        return -1;
    }
    if (src.len > 0) {
        @memcpy(dst[pos .. pos + src.len], src[0..src.len]);
    }
    return (pos + src.len) as i64;
}

fn pushU64(dst: []u8, pos: usize, val: u64) -> i64 {
    let tmp: [24]u8 = undefined;
    const n: usize = writeU64(val, tmp[0..24], 0);
    return pushSlice(dst, pos, tmp[0..n]);
}

/// Formats a Common Log Format (CLF) line into `dst`.
/// Emits `"-"` when `user` is empty or `bytes < 0`. Returns bytes written, or `-1` on overflow.
pub fn formatClf(
    host: []const u8,
    user: []const u8,
    time: []const u8,
    method: []const u8,
    path: []const u8,
    proto: []const u8,
    status: u16,
    bytes: i64,
    dst: []u8,
) -> i64 {
    let p = pushSlice(dst, 0, host);
    if (p < 0) { return -1; }
    p = pushSlice(dst, p as usize, " - ");
    if (p < 0) { return -1; }
    if (user.len == 0) {
        p = pushSlice(dst, p as usize, "-");
    } else {
        p = pushSlice(dst, p as usize, user);
    }
    if (p < 0) { return -1; }
    p = pushSlice(dst, p as usize, " [");
    if (p < 0) { return -1; }
    p = pushSlice(dst, p as usize, time);
    if (p < 0) { return -1; }
    p = pushSlice(dst, p as usize, "] \"");
    if (p < 0) { return -1; }
    p = pushSlice(dst, p as usize, method);
    if (p < 0) { return -1; }
    p = pushSlice(dst, p as usize, " ");
    if (p < 0) { return -1; }
    p = pushSlice(dst, p as usize, path);
    if (p < 0) { return -1; }
    p = pushSlice(dst, p as usize, " ");
    if (p < 0) { return -1; }
    p = pushSlice(dst, p as usize, proto);
    if (p < 0) { return -1; }
    p = pushSlice(dst, p as usize, "\" ");
    if (p < 0) { return -1; }
    p = pushU64(dst, p as usize, status as u64);
    if (p < 0) { return -1; }
    p = pushSlice(dst, p as usize, " ");
    if (p < 0) { return -1; }
    if (bytes < 0) {
        return pushSlice(dst, p as usize, "-");
    }
    return pushU64(dst, p as usize, bytes as u64);
}
