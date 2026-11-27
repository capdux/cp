// @cap/vary — Zero-heap RFC 9110 HTTP Vary response header manipulation
// MIT License (clean-room CP implementation)
// Accelerated with comptime 256-entry tchar LUT and 16-lane @Vector(16, u8) SIMD scanning.

import { eqIgnoreCase } from "cap:ascii";
import { writeStr } from "cap:fmt";

type VaryVec16 = @Vector(16, u8);

fn varyBuildTcharLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        const b: u8 = c as u8;
        if ((b >= ('a' as u8) && b <= ('z' as u8))
            || (b >= ('A' as u8) && b <= ('Z' as u8))
            || (b >= ('0' as u8) && b <= ('9' as u8))
            || b == '!' as u8 || b == '#' as u8 || b == '$' as u8 || b == '%' as u8
            || b == '&' as u8 || b == '\'' as u8 || b == '*' as u8 || b == '+' as u8
            || b == '-' as u8 || b == '.' as u8 || b == '^' as u8 || b == '_' as u8
            || b == '`' as u8 || b == '|' as u8 || b == '~' as u8)
        {
            lut[c] = 1;
        }
        c += 1;
    }
    return lut;
}

const VARY_TCHAR_LUT: [256]u8 = varyBuildTcharLut();

fn varyFindComma(src: []const u8, start: usize) -> usize {
    let i: usize = start;
    const len = src.len;
    const v_comma: VaryVec16 = @splat(',' as u8);
    while (i + 16 <= len) {
        const v: VaryVec16 = src[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_comma)) {
            let k: usize = 0;
            while (k < 16) {
                if (src[i + k] == ',' as u8) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        if (src[i] == ',' as u8) {
            return i;
        }
        i += 1;
    }
    return len;
}

fn isWs(c: u8) -> bool {
    return c == ' ' as u8 || c == '\t' as u8;
}

fn isTchar(c: u8) -> bool {
    return VARY_TCHAR_LUT[c as usize] != 0;
}

pub fn isValidFieldName(field: []const u8) -> bool {
    if (field.len == 0) {
        return false;
    }
    let i: usize = 0;
    const len = field.len;
    const v_a: VaryVec16 = @splat('a' as u8);
    const v_z: VaryVec16 = @splat('z' as u8);
    const v_A: VaryVec16 = @splat('A' as u8);
    const v_Z: VaryVec16 = @splat('Z' as u8);
    const v_0: VaryVec16 = @splat('0' as u8);
    const v_9: VaryVec16 = @splat('9' as u8);
    const v_dash: VaryVec16 = @splat('-' as u8);
    const v_us: VaryVec16 = @splat('_' as u8);
    const ones: VaryVec16 = @splat(1 as u8);
    const zeros: VaryVec16 = @splat(0 as u8);
    while (i + 16 <= len) {
        const v: VaryVec16 = field[i .. i + 16][0..16].*;
        const m_lo: VaryVec16 = @select(u8, v >= v_a, @select(u8, v <= v_z, ones, zeros), zeros);
        const m_hi: VaryVec16 = @select(u8, v >= v_A, @select(u8, v <= v_Z, ones, zeros), zeros);
        const m_dg: VaryVec16 = @select(u8, v >= v_0, @select(u8, v <= v_9, ones, zeros), zeros);
        const m_sp: VaryVec16 = @select(u8, v == v_dash, ones, @select(u8, v == v_us, ones, zeros));
        const ok: VaryVec16 = m_lo | m_hi | m_dg | m_sp;
        if (@reduce(.And, ok != zeros)) {
            i += 16;
            continue;
        }
        let k: usize = 0;
        while (k < 16) {
            if (VARY_TCHAR_LUT[field[i + k] as usize] == 0) {
                return false;
            }
            k += 1;
        }
        i += 16;
    }
    while (i < len) {
        if (VARY_TCHAR_LUT[field[i] as usize] == 0) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isWildcard(tok: []const u8) -> bool {
    return tok.len == 1 && tok[0] == '*' as u8;
}

pub fn hasField(header: []const u8, field: []const u8) -> bool {
    let f_start: usize = 0;
    let f_end: usize = field.len;
    while (f_start < f_end && isWs(field[f_start])) {
        f_start += 1;
    }
    while (f_end > f_start && isWs(field[f_end - 1])) {
        f_end -= 1;
    }
    if (f_start >= f_end) {
        return false;
    }
    const target: []const u8 = field[f_start..f_end];

    let pos: usize = 0;
    while (pos < header.len) {
        const seg_end: usize = varyFindComma(header, pos);
        let s: usize = pos;
        let e: usize = seg_end;
        while (s < e && isWs(header[s])) {
            s += 1;
        }
        while (e > s && isWs(header[e - 1])) {
            e -= 1;
        }
        if (s < e) {
            const tok: []const u8 = header[s..e];
            if (isWildcard(tok) || eqIgnoreCase(tok, target)) {
                return true;
            }
        }
        pos = seg_end;
        if (pos < header.len && header[pos] == ',' as u8) {
            pos += 1;
        }
    }
    return false;
}

fn headerHasWildcard(src: []const u8) -> bool {
    let pos: usize = 0;
    while (pos < src.len) {
        const seg_end: usize = varyFindComma(src, pos);
        let s: usize = pos;
        let e: usize = seg_end;
        while (s < e && isWs(src[s])) {
            s += 1;
        }
        while (e > s && isWs(src[e - 1])) {
            e -= 1;
        }
        if (s < e && isWildcard(src[s..e])) {
            return true;
        }
        pos = seg_end;
        if (pos < src.len && src[pos] == ',' as u8) {
            pos += 1;
        }
    }
    return false;
}

// Validate every comma-separated token in `field_list` (must have >= 1 valid token)
fn validateFieldList(field_list: []const u8) -> bool {
    let count: usize = 0;
    let pos: usize = 0;
    while (pos < field_list.len) {
        const seg_end: usize = varyFindComma(field_list, pos);
        let s: usize = pos;
        let e: usize = seg_end;
        while (s < e && isWs(field_list[s])) {
            s += 1;
        }
        while (e > s && isWs(field_list[e - 1])) {
            e -= 1;
        }
        if (s < e) {
            if (!isValidFieldName(field_list[s..e])) {
                return false;
            }
            count += 1;
        }
        pos = seg_end;
        if (pos < field_list.len && field_list[pos] == ',' as u8) {
            pos += 1;
        }
    }
    return count > 0;
}

fn dstContainsToken(dst: []const u8, written: usize, tok: []const u8) -> bool {
    let pos: usize = 0;
    while (pos < written) {
        const seg_end: usize = varyFindComma(dst[0..written], pos);
        let s: usize = pos;
        let e: usize = seg_end;
        while (s < e && isWs(dst[s])) {
            s += 1;
        }
        while (e > s && isWs(dst[e - 1])) {
            e -= 1;
        }
        if (s < e && eqIgnoreCase(dst[s..e], tok)) {
            return true;
        }
        pos = seg_end;
        if (pos < written && dst[pos] == ',' as u8) {
            pos += 1;
        }
    }
    return false;
}

fn appendTokens(src: []const u8, dst: []u8, start_pos: usize) -> i64 {
    let cur: usize = start_pos;
    let pos: usize = 0;
    while (pos < src.len) {
        const seg_end: usize = varyFindComma(src, pos);
        let s: usize = pos;
        let e: usize = seg_end;
        while (s < e && isWs(src[s])) {
            s += 1;
        }
        while (e > s && isWs(src[e - 1])) {
            e -= 1;
        }
        if (s < e) {
            const tok: []const u8 = src[s..e];
            if (!dstContainsToken(dst, cur, tok)) {
                if (cur > 0) {
                    if (2 > dst.len - cur) {
                        return -1;
                    }
                    dst[cur] = ',' as u8;
                    dst[cur + 1] = ' ' as u8;
                    cur += 2;
                }
                if (tok.len > dst.len - cur) {
                    return -1;
                }
                cur = writeStr(tok, dst, cur);
            }
        }
        pos = seg_end;
        if (pos < src.len && src[pos] == ',' as u8) {
            pos += 1;
        }
    }
    return cur as i64;
}

pub fn append(header: []const u8, field: []const u8, dst: []u8) -> i64 {
    if (!validateFieldList(field)) {
        return -1;
    }
    if (headerHasWildcard(header) || headerHasWildcard(field)) {
        if (dst.len < 1) {
            return -1;
        }
        dst[0] = '*' as u8;
        return 1;
    }
    const after_hdr: i64 = appendTokens(header, dst, 0);
    if (after_hdr < 0) {
        return -1;
    }
    return appendTokens(field, dst, after_hdr as usize);
}

pub fn appendMany(header: []const u8, fields: []const []const u8, dst: []u8) -> i64 {
    let any_wildcard: bool = headerHasWildcard(header);
    let i: usize = 0;
    while (i < fields.len) {
        if (!validateFieldList(fields[i])) {
            return -1;
        }
        if (headerHasWildcard(fields[i])) {
            any_wildcard = true;
        }
        i += 1;
    }

    if (any_wildcard) {
        if (dst.len < 1) {
            return -1;
        }
        dst[0] = '*' as u8;
        return 1;
    }

    let cur: i64 = appendTokens(header, dst, 0);
    if (cur < 0) {
        return -1;
    }
    i = 0;
    while (i < fields.len) {
        cur = appendTokens(fields[i], dst, cur as usize);
        if (cur < 0) {
            return -1;
        }
        i += 1;
    }
    return cur;
}
