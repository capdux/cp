// @cap/strip-json-comments — Zero-allocation JSONC comment & trailing-comma stripper for CP
// Clean-room MIT implementation inspired by npm `strip-json-comments` (150M+/wk).
// Accelerated with 16-lane @Vector(16, u8) SIMD scanning and @memcpy bulk segment copying.

import { isSpace } from "cap:ascii";
import { findByteInBytes } from "cap:bin";

type SjcVec16 = @Vector(16, u8);

pub struct StripOptions {
    pub preserve_whitespace: bool,
    pub strip_trailing_commas: bool,
}

fn sjcFindMainSpecial(src: []const u8, start: usize, check_comma: bool) -> usize {
    let i: usize = start;
    const len = src.len;
    const v_qt: SjcVec16 = @splat('"' as u8);
    const v_sl: SjcVec16 = @splat('/' as u8);
    const v_cm: SjcVec16 = @splat(',' as u8);
    while (i + 16 <= len) {
        const v: SjcVec16 = src[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_qt) ||
            @reduce(.Or, v == v_sl) ||
            (check_comma && @reduce(.Or, v == v_cm)))
        {
            let k: usize = 0;
            while (k < 16) {
                const c = src[i + k];
                if (c == '"' || c == '/' || (check_comma && c == ',')) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        const c = src[i];
        if (c == '"' || c == '/' || (check_comma && c == ',')) {
            return i;
        }
        i += 1;
    }
    return len;
}

fn sjcFindStrSpecial(src: []const u8, start: usize) -> usize {
    let i: usize = start;
    const len = src.len;
    const v_qt: SjcVec16 = @splat('"' as u8);
    const v_bs: SjcVec16 = @splat('\\' as u8);
    while (i + 16 <= len) {
        const v: SjcVec16 = src[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_qt) || @reduce(.Or, v == v_bs)) {
            let k: usize = 0;
            while (k < 16) {
                const c = src[i + k];
                if (c == '"' || c == '\\') {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        const c = src[i];
        if (c == '"' || c == '\\') {
            return i;
        }
        i += 1;
    }
    return len;
}

fn sjcFindNewline(src: []const u8, start: usize) -> usize {
    let i: usize = start;
    const len = src.len;
    const v_lf: SjcVec16 = @splat('\n' as u8);
    const v_cr: SjcVec16 = @splat('\r' as u8);
    while (i + 16 <= len) {
        const v: SjcVec16 = src[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_lf) || @reduce(.Or, v == v_cr)) {
            let k: usize = 0;
            while (k < 16) {
                const c = src[i + k];
                if (c == '\n' || c == '\r') {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        const c = src[i];
        if (c == '\n' || c == '\r') {
            return i;
        }
        i += 1;
    }
    return len;
}

fn sjcFindStar(src: []const u8, start: usize) -> usize {
    let i: usize = start;
    const len = src.len;
    const v_st: SjcVec16 = @splat('*' as u8);
    while (i + 16 <= len) {
        const v: SjcVec16 = src[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_st)) {
            let k: usize = 0;
            while (k < 16) {
                if (src[i + k] == '*') {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        if (src[i] == '*') {
            return i;
        }
        i += 1;
    }
    return len;
}

// Look ahead past whitespace and comments after a comma to check if next token is '}' or ']'.
fn isTrailingComma(src: []const u8, commaIdx: usize) -> bool {
    let i = commaIdx + (1 as usize);
    while (i < src.len) {
        const c = src[i];
        if (isSpace(c)) {
            i += 1;
            continue;
        }
        if (c == '/' && i + (1 as usize) < src.len && src[i + (1 as usize)] == '/') {
            i = findByteInBytes(src, '\n', i + 2) ?? src.len;
            continue;
        }
        if (c == '/' && i + (1 as usize) < src.len && src[i + (1 as usize)] == '*') {
            i += 2;
            let closed = false;
            while (i + (1 as usize) < src.len) {
                const next_star = sjcFindStar(src, i);
                if (next_star + (1 as usize) >= src.len) {
                    break;
                }
                if (src[next_star + (1 as usize)] == '/') {
                    i = next_star + 2;
                    closed = true;
                    break;
                }
                i = next_star + 1;
            }
            if (!closed) {
                return false;
            }
            continue;
        }
        return c == '}' || c == ']';
    }
    return false;
}

pub fn stripComments(src: []const u8, dst: []u8, opts: StripOptions) -> i64 {
    let i: usize = 0;
    let outPos: usize = 0;

    while (i < src.len) {
        const next_sp = sjcFindMainSpecial(src, i, opts.strip_trailing_commas);
        if (next_sp > i) {
            const seg_len = next_sp - i;
            if (outPos + seg_len > dst.len) { return -1 as i64; }
            @memcpy(dst[outPos .. outPos + seg_len], src[i..next_sp]);
            outPos += seg_len;
            i = next_sp;
            if (i >= src.len) { break; }
        }

        const c = src[i];

        // 1. Double-quoted string literal: preserve verbatim including \" and // inside
        if (c == '"') {
            if (outPos >= dst.len) { return -1 as i64; }
            dst[outPos] = '"';
            outPos += 1;
            i += 1;

            while (i < src.len) {
                const next_str = sjcFindStrSpecial(src, i);
                if (next_str > i) {
                    const seg_len = next_str - i;
                    if (outPos + seg_len > dst.len) { return -1 as i64; }
                    @memcpy(dst[outPos .. outPos + seg_len], src[i..next_str]);
                    outPos += seg_len;
                    i = next_str;
                    if (i >= src.len) { break; }
                }
                const sc = src[i];
                if (sc == '\\' && i + (1 as usize) < src.len) {
                    if (outPos + (2 as usize) > dst.len) { return -1 as i64; }
                    dst[outPos] = sc;
                    dst[outPos + (1 as usize)] = src[i + (1 as usize)];
                    outPos += 2;
                    i += 2;
                    continue;
                }
                if (outPos >= dst.len) { return -1 as i64; }
                dst[outPos] = sc;
                outPos += 1;
                i += 1;
                if (sc == '"') {
                    break;
                }
            }
            continue;
        }

        // 2. Single-line comment // ...
        if (c == '/' && i + (1 as usize) < src.len && src[i + (1 as usize)] == '/') {
            const line_end = sjcFindNewline(src, i);
            if (opts.preserve_whitespace) {
                const n = line_end - i;
                if (outPos + n > dst.len) { return -1 as i64; }
                let k: usize = 0;
                while (k < n) {
                    dst[outPos + k] = ' ';
                    k += 1;
                }
                outPos += n;
            }
            i = line_end;
            continue;
        }

        // 3. Block comment /* ... */
        if (c == '/' && i + (1 as usize) < src.len && src[i + (1 as usize)] == '*') {
            // Emit spaces for "/*" if preserve_whitespace
            if (opts.preserve_whitespace) {
                if (outPos + (2 as usize) > dst.len) { return -1 as i64; }
                dst[outPos] = ' ';
                dst[outPos + (1 as usize)] = ' ';
                outPos += 2;
            }
            i += 2;

            if (!opts.preserve_whitespace) {
                while (i < src.len) {
                    const next_star = sjcFindStar(src, i);
                    if (next_star >= src.len) {
                        i = src.len;
                        break;
                    }
                    if (next_star + (1 as usize) < src.len && src[next_star + (1 as usize)] == '/') {
                        i = next_star + 2;
                        break;
                    }
                    i = next_star + 1;
                }
                continue;
            }

            while (i < src.len) {
                if (src[i] == '*' && i + (1 as usize) < src.len && src[i + (1 as usize)] == '/') {
                    if (outPos + (2 as usize) > dst.len) { return -1 as i64; }
                    dst[outPos] = ' ';
                    dst[outPos + (1 as usize)] = ' ';
                    outPos += 2;
                    i += 2;
                    break;
                }
                const bc = src[i];
                if (outPos >= dst.len) { return -1 as i64; }
                if (bc == '\n' || bc == '\r') {
                    dst[outPos] = bc;
                } else {
                    dst[outPos] = ' ';
                }
                outPos += 1;
                i += 1;
            }
            continue;
        }

        // 4. Optional trailing comma removal before '}' or ']'
        if (c == ',' && opts.strip_trailing_commas && isTrailingComma(src, i)) {
            if (opts.preserve_whitespace) {
                if (outPos >= dst.len) { return -1 as i64; }
                dst[outPos] = ' ';
                outPos += 1;
            }
            i += 1;
            continue;
        }

        // 5. Regular byte ('/' not starting comment, or non-trailing ',')
        if (outPos >= dst.len) { return -1 as i64; }
        dst[outPos] = c;
        outPos += 1;
        i += 1;
    }

    return outPos as i64;
}

pub fn stripJsonc(src: []const u8, dst: []u8) -> i64 {
    const opts = StripOptions{
        preserve_whitespace: true,
        strip_trailing_commas: true,
    };
    return stripComments(src, dst, opts);
}

pub fn stripCompact(src: []const u8, dst: []u8) -> i64 {
    const opts = StripOptions{
        preserve_whitespace: false,
        strip_trailing_commas: true,
    };
    return stripComments(src, dst, opts);
}

export {
    StripOptions,
    stripComments,
    stripJsonc,
    stripCompact,
};
