// @cap/uuencode — Zero-heap POSIX/Unix uuencode & uudecode codec
// MIT License (clean-room CP implementation)
// Accelerated with 16-lane @Vector(16, u8) SIMD encode/decode/newline scanning, comptime 256-entry decode LUT, @memcpy, and undefined stack buffers.

import { strEq, startsWith } from "cap:ascii";

pub const MAX_LINE_BYTES: usize = 45;

type UuVec16 = @Vector(16, u8);

fn uuBuildDecLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = undefined;
    let i: usize = 0;
    while (i < 256) {
        lut[i] = 0xFF;
        i += 1;
    }
    lut['`' as usize] = 0;
    lut[' ' as usize] = 0;
    let c: usize = 33;
    while (c <= 95) {
        lut[c] = (c - 32) as u8;
        c += 1;
    }
    return lut;
}

const UU_DEC_LUT: [256]u8 = uuBuildDecLut();

fn uuEncChar(v: u8) -> u8 {
    const m: u8 = v & 0x3F;
    if (m == 0) {
        return '`' as u8;
    }
    return m + 32;
}

fn uuFindNewline(s: string, start: usize) -> usize {
    let i = start;
    const n = s.len;
    const v_nl: UuVec16 = @splat('\n' as u8);
    while (i + 16 <= n) {
        const v: UuVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_nl)) {
            let k: usize = 0;
            while (k < 16) {
                if (s[i + k] == '\n' as u8) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < n) {
        if (s[i] == '\n' as u8) {
            return i;
        }
        i += 1;
    }
    return n;
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

    // 16-lane SIMD fast path: 12 input bytes -> 16 UU ASCII characters per iteration
    const v_zero: UuVec16 = @splat(0 as u8);
    const v_tick: UuVec16 = @splat('`' as u8);
    const v_32: UuVec16 = @splat(32 as u8);

    while (si + 12 <= data.len) {
        let s16: [16]u8 = undefined;
        const a0 = data[si]; const a1 = data[si + 1]; const a2 = data[si + 2];
        s16[0] = a0 >> 2;
        s16[1] = ((a0 & 0x03) << 4) | (a1 >> 4);
        s16[2] = ((a1 & 0x0F) << 2) | (a2 >> 6);
        s16[3] = a2 & 0x3F;

        const b0 = data[si + 3]; const b1 = data[si + 4]; const b2 = data[si + 5];
        s16[4] = b0 >> 2;
        s16[5] = ((b0 & 0x03) << 4) | (b1 >> 4);
        s16[6] = ((b1 & 0x0F) << 2) | (b2 >> 6);
        s16[7] = b2 & 0x3F;

        const c0 = data[si + 6]; const c1 = data[si + 7]; const c2 = data[si + 8];
        s16[8] = c0 >> 2;
        s16[9] = ((c0 & 0x03) << 4) | (c1 >> 4);
        s16[10] = ((c1 & 0x0F) << 2) | (c2 >> 6);
        s16[11] = c2 & 0x3F;

        const d0 = data[si + 9]; const d1 = data[si + 10]; const d2 = data[si + 11];
        s16[12] = d0 >> 2;
        s16[13] = ((d0 & 0x03) << 4) | (d1 >> 4);
        s16[14] = ((d1 & 0x0F) << 2) | (d2 >> 6);
        s16[15] = d2 & 0x3F;

        const vs: UuVec16 = s16;
        const v_out: UuVec16 = @select(u8, vs == v_zero, v_tick, vs + v_32);
        const out_arr: [16]u8 = v_out;
        @memcpy(out[di .. di + 16], out_arr[0..16]);
        si += 12;
        di += 16;
    }

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

    const byte_len_u: u8 = UU_DEC_LUT[line[0] as usize];
    if (byte_len_u > 45) {
        return -1;
    }
    const byte_len: usize = byte_len_u as usize;
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

    // 16-lane SIMD validation + 16 chars -> 12 bytes unrolled decode loop
    const v_inv: UuVec16 = @splat(64 as u8);
    while (g + 4 <= groups && di + 12 <= byte_len) {
        let v16: [16]u8 = undefined;
        v16[0] = UU_DEC_LUT[line[si] as usize];
        v16[1] = UU_DEC_LUT[line[si + 1] as usize];
        v16[2] = UU_DEC_LUT[line[si + 2] as usize];
        v16[3] = UU_DEC_LUT[line[si + 3] as usize];
        v16[4] = UU_DEC_LUT[line[si + 4] as usize];
        v16[5] = UU_DEC_LUT[line[si + 5] as usize];
        v16[6] = UU_DEC_LUT[line[si + 6] as usize];
        v16[7] = UU_DEC_LUT[line[si + 7] as usize];
        v16[8] = UU_DEC_LUT[line[si + 8] as usize];
        v16[9] = UU_DEC_LUT[line[si + 9] as usize];
        v16[10] = UU_DEC_LUT[line[si + 10] as usize];
        v16[11] = UU_DEC_LUT[line[si + 11] as usize];
        v16[12] = UU_DEC_LUT[line[si + 12] as usize];
        v16[13] = UU_DEC_LUT[line[si + 13] as usize];
        v16[14] = UU_DEC_LUT[line[si + 14] as usize];
        v16[15] = UU_DEC_LUT[line[si + 15] as usize];

        const vv: UuVec16 = v16;
        if (@reduce(.Or, vv >= v_inv)) {
            return -1;
        }

        out[di] = (v16[0] << 2) | (v16[1] >> 4);
        out[di + 1] = ((v16[1] & 0x0F) << 4) | (v16[2] >> 2);
        out[di + 2] = ((v16[2] & 0x03) << 6) | v16[3];

        out[di + 3] = (v16[4] << 2) | (v16[5] >> 4);
        out[di + 4] = ((v16[5] & 0x0F) << 4) | (v16[6] >> 2);
        out[di + 5] = ((v16[6] & 0x03) << 6) | v16[7];

        out[di + 6] = (v16[8] << 2) | (v16[9] >> 4);
        out[di + 7] = ((v16[9] & 0x0F) << 4) | (v16[10] >> 2);
        out[di + 8] = ((v16[10] & 0x03) << 6) | v16[11];

        out[di + 9] = (v16[12] << 2) | (v16[13] >> 4);
        out[di + 10] = ((v16[13] & 0x0F) << 4) | (v16[14] >> 2);
        out[di + 11] = ((v16[14] & 0x03) << 6) | v16[15];

        si += 16;
        di += 12;
        g += 4;
    }

    while (g < groups) {
        const v0: u8 = UU_DEC_LUT[line[si] as usize];
        const v1: u8 = UU_DEC_LUT[line[si + 1] as usize];
        const v2: u8 = UU_DEC_LUT[line[si + 2] as usize];
        const v3: u8 = UU_DEC_LUT[line[si + 3] as usize];
        if ((v0 | v1 | v2 | v3) >= 64) {
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
        const line_end: usize = uuFindNewline(input, pos);
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
