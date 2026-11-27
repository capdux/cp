// @cap/uuencode — Zero-heap POSIX/Unix uuencode & uudecode codec
// MIT License (clean-room CP implementation)

import { strEq, startsWith, findByte } from "cap:ascii";

pub const MAX_LINE_BYTES: usize = 45;

fn uuEncChar(v: u8) -> u8 {
    const m: u8 = v & 0x3F;
    if (m == 0) {
        return '`' as u8;
    }
    return m + 32;
}

fn uuDecVal(c: u8) -> i32 {
    if (c == '`' as u8 || c == ' ' as u8) {
        return 0;
    }
    if (c >= 33 && c <= 95) {
        return (c - 32) as i32;
    }
    return -1;
}

pub fn encodeLine(data: []const u8, out: []u8) -> usize {
    if (data.len == 0 || data.len > MAX_LINE_BYTES) {
        return 0;
    }
    const groups: usize = (data.len + 2) / 3;
    const out_len: usize = groups * 4 + 1;
    if (out_len > out.len) {
        return 0;
    }

    out[0] = uuEncChar(data.len as u8);
    let si: usize = 0;
    let di: usize = 1;
    while (si < data.len) {
        const b0: u32 = data[si] as u32;
        let b1: u32 = 0;
        let b2: u32 = 0;
        if (si + 1 < data.len) {
            b1 = data[si + 1] as u32;
        }
        if (si + 2 < data.len) {
            b2 = data[si + 2] as u32;
        }
        const triple: u32 = (b0 << 16) | (b1 << 8) | b2;
        out[di] = uuEncChar(((triple >> 18) & 0x3F) as u8);
        out[di + 1] = uuEncChar(((triple >> 12) & 0x3F) as u8);
        out[di + 2] = uuEncChar(((triple >> 6) & 0x3F) as u8);
        out[di + 3] = uuEncChar((triple & 0x3F) as u8);
        si += 3;
        di += 4;
    }
    return di;
}

pub fn decodeLine(line: string, out: []u8) -> i64 {
    let eff_len: usize = line.len;
    while (eff_len > 0 && (line[eff_len - 1] == '\r' as u8 || line[eff_len - 1] == '\n' as u8)) {
        eff_len -= 1;
    }
    if (eff_len == 0) {
        return -1;
    }

    const byte_len_i: i32 = uuDecVal(line[0]);
    if (byte_len_i < 0 || byte_len_i > 45) {
        return -1;
    }
    const byte_len: usize = byte_len_i as usize;
    if (byte_len == 0) {
        return 0;
    }

    const groups: usize = (byte_len + 2) / 3;
    const need_chars: usize = groups * 4 + 1;
    if (eff_len < need_chars || byte_len > out.len) {
        return -1;
    }

    let si: usize = 1;
    let di: usize = 0;
    let g: usize = 0;
    while (g < groups) {
        const v0: i32 = uuDecVal(line[si]);
        const v1: i32 = uuDecVal(line[si + 1]);
        const v2: i32 = uuDecVal(line[si + 2]);
        const v3: i32 = uuDecVal(line[si + 3]);
        if (v0 < 0 || v1 < 0 || v2 < 0 || v3 < 0) {
            return -1;
        }
        const triple: u32 = ((v0 as u32) << 18)
            | ((v1 as u32) << 12)
            | ((v2 as u32) << 6)
            | (v3 as u32);
        if (di < byte_len) {
            out[di] = ((triple >> 16) & 0xFF) as u8;
            di += 1;
        }
        if (di < byte_len) {
            out[di] = ((triple >> 8) & 0xFF) as u8;
            di += 1;
        }
        if (di < byte_len) {
            out[di] = (triple & 0xFF) as u8;
            di += 1;
        }
        si += 4;
        g += 1;
    }
    return byte_len as i64;
}

pub fn encode(data: []const u8, out: []u8) -> usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < data.len) {
        let chunk_len: usize = data.len - i;
        if (chunk_len > MAX_LINE_BYTES) {
            chunk_len = MAX_LINE_BYTES;
        }
        const w: usize = encodeLine(data[i..(i + chunk_len)], out[pos..out.len]);
        if (w == 0) {
            return 0;
        }
        pos += w;
        if (pos >= out.len) {
            return 0;
        }
        out[pos] = '\n' as u8;
        pos += 1;
        i += chunk_len;
    }

    if (pos + 2 > out.len) {
        return 0;
    }
    out[pos] = '`' as u8;
    out[pos + 1] = '\n' as u8;
    return pos + 2;
}

pub fn decode(input: string, out: []u8) -> i64 {
    let pos: usize = 0;
    let out_pos: usize = 0;

    while (pos < input.len) {
        const line_end: usize = findByte(input, '\n' as u8, pos) ?? input.len;
        let raw_end: usize = line_end;
        if (raw_end > pos && input[raw_end - 1] == '\r' as u8) {
            raw_end -= 1;
        }

        const line: string = input[pos..raw_end];
        pos = line_end;
        if (pos < input.len && input[pos] == '\n' as u8) {
            pos += 1;
        }

        if (line.len == 0) {
            continue;
        }
        if (startsWith(line, "begin ")) {
            continue;
        }
        if (strEq(line, "end")) {
            break;
        }

        const n: i64 = decodeLine(line, out[out_pos..out.len]);
        if (n < 0) {
            return -1;
        }
        if (n == 0) {
            break;
        }
        out_pos += n as usize;
    }

    return out_pos as i64;
}
