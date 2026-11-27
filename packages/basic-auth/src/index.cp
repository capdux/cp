// @cap/basic-auth — Zero-heap RFC 7617 HTTP Basic Authentication codec & verifier
// MIT License (clean-room CP implementation)

import { eqIgnoreCase } from "cap:ascii";
import { ctEqual, findByteInBytes } from "cap:bin";
import { writeStr } from "cap:fmt";

pub struct BasicCredentials {
    pub user_len: usize,
    pub pass_len: usize,
    pub valid: bool,
}

fn isWs(c: u8) -> bool {
    return c == ' ' as u8 || c == '\t' as u8;
}

fn b64EncChar(v: u8) -> u8 {
    const alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    return alphabet[v as usize];
}

fn b64DecVal(c: u8) -> i32 {
    if (c >= 'A' as u8 && c <= 'Z' as u8) {
        return (c - ('A' as u8)) as i32;
    }
    if (c >= 'a' as u8 && c <= 'z' as u8) {
        return (c - ('a' as u8) + 26) as i32;
    }
    if (c >= '0' as u8 && c <= '9' as u8) {
        return (c - ('0' as u8) + 52) as i32;
    }
    if (c == '+' as u8) {
        return 62;
    }
    if (c == '/' as u8) {
        return 63;
    }
    return -1;
}

fn b64Encode(src: []const u8, dst: []u8) -> i64 {
    const out_len: usize = ((src.len + 2) / 3) * 4;
    if (out_len > dst.len) {
        return -1;
    }
    let si: usize = 0;
    let di: usize = 0;
    while (si + 3 <= src.len) {
        const v: u32 = ((src[si] as u32) << 16)
            | ((src[si + 1] as u32) << 8)
            | (src[si + 2] as u32);
        dst[di] = b64EncChar(((v >> 18) & 0x3F) as u8);
        dst[di + 1] = b64EncChar(((v >> 12) & 0x3F) as u8);
        dst[di + 2] = b64EncChar(((v >> 6) & 0x3F) as u8);
        dst[di + 3] = b64EncChar((v & 0x3F) as u8);
        si += 3;
        di += 4;
    }
    const rem: usize = src.len - si;
    if (rem == 1) {
        const v: u32 = (src[si] as u32) << 16;
        dst[di] = b64EncChar(((v >> 18) & 0x3F) as u8);
        dst[di + 1] = b64EncChar(((v >> 12) & 0x3F) as u8);
        dst[di + 2] = '=' as u8;
        dst[di + 3] = '=' as u8;
        di += 4;
    } else if (rem == 2) {
        const v: u32 = ((src[si] as u32) << 16) | ((src[si + 1] as u32) << 8);
        dst[di] = b64EncChar(((v >> 18) & 0x3F) as u8);
        dst[di + 1] = b64EncChar(((v >> 12) & 0x3F) as u8);
        dst[di + 2] = b64EncChar(((v >> 6) & 0x3F) as u8);
        dst[di + 3] = '=' as u8;
        di += 4;
    }
    return di as i64;
}

fn b64Decode(src: []const u8, dst: []u8) -> i64 {
    if (src.len == 0 || (src.len % 4) != 0) {
        return -1;
    }
    let si: usize = 0;
    let di: usize = 0;
    while (si < src.len) {
        const c0: u8 = src[si];
        const c1: u8 = src[si + 1];
        const c2: u8 = src[si + 2];
        const c3: u8 = src[si + 3];
        const is_last: bool = (si + 4) == src.len;

        const v0: i32 = b64DecVal(c0);
        const v1: i32 = b64DecVal(c1);
        if (v0 < 0 || v1 < 0) {
            return -1;
        }

        if (c2 == '=' as u8) {
            if (!is_last || c3 != '=' as u8) {
                return -1;
            }
            if (di >= dst.len) {
                return -1;
            }
            const t: u32 = ((v0 as u32) << 18) | ((v1 as u32) << 12);
            dst[di] = ((t >> 16) & 0xFF) as u8;
            di += 1;
            break;
        }

        const v2: i32 = b64DecVal(c2);
        if (v2 < 0) {
            return -1;
        }

        if (c3 == '=' as u8) {
            if (!is_last) {
                return -1;
            }
            if (di + 2 > dst.len) {
                return -1;
            }
            const t: u32 = ((v0 as u32) << 18) | ((v1 as u32) << 12) | ((v2 as u32) << 6);
            dst[di] = ((t >> 16) & 0xFF) as u8;
            dst[di + 1] = ((t >> 8) & 0xFF) as u8;
            di += 2;
            break;
        }

        const v3: i32 = b64DecVal(c3);
        if (v3 < 0) {
            return -1;
        }
        if (di + 3 > dst.len) {
            return -1;
        }
        const t: u32 = ((v0 as u32) << 18) | ((v1 as u32) << 12) | ((v2 as u32) << 6) | (v3 as u32);
        dst[di] = ((t >> 16) & 0xFF) as u8;
        dst[di + 1] = ((t >> 8) & 0xFF) as u8;
        dst[di + 2] = (t & 0xFF) as u8;
        di += 3;
        si += 4;
    }
    return di as i64;
}

pub fn decodeHeader(header: string, out_user: []u8, out_pass: []u8) -> BasicCredentials {
    const fail = BasicCredentials { user_len: 0, pass_len: 0, valid: false };

    let s: usize = 0;
    let e: usize = header.len;
    while (s < e && isWs(header[s])) {
        s += 1;
    }
    while (e > s && isWs(header[e - 1])) {
        e -= 1;
    }
    // Need at least "Basic " (6 chars) + 4 base64 chars
    if (e - s < 7) {
        return fail;
    }

    // Match "basic" case-insensitively followed by whitespace
    if (!eqIgnoreCase(header[s..(s + 5)], "basic")) {
        return fail;
    }
    if (!isWs(header[s + 5])) {
        return fail;
    }

    let tok_s: usize = s + 6;
    while (tok_s < e && isWs(header[tok_s])) {
        tok_s += 1;
    }
    if (tok_s >= e) {
        return fail;
    }

    let raw: [512]u8 = undefined;
    const raw_len_i: i64 = b64Decode(header[tok_s..e], raw[0..512]);
    if (raw_len_i < 0) {
        return fail;
    }
    const raw_len: usize = raw_len_i as usize;

    // Split on FIRST ':' (password may contain ':')
    const colon_opt = findByteInBytes(raw[0..raw_len], ':' as u8, 0);
    if (colon_opt == null) {
        return fail;
    }
    const colon: usize = colon_opt.?;

    const u_len: usize = colon;
    const p_len: usize = raw_len - colon - 1;
    if (u_len > out_user.len || p_len > out_pass.len) {
        return fail;
    }

    * = writeStr(raw[0..u_len], out_user, 0);
    * = writeStr(raw[(colon + 1)..raw_len], out_pass, 0);

    return BasicCredentials {
        user_len: u_len,
        pass_len: p_len,
        valid: true,
    };
}

pub fn encodeHeader(user: string, pass: string, out: []u8) -> usize {
    // RFC 7617: userid MUST NOT contain ':'
    if (findByteInBytes(user, ':' as u8, 0) != null) {
        return 0;
    }

    const raw_len: usize = user.len + 1 + pass.len;
    if (raw_len > 512) {
        return 0;
    }
    let raw: [512]u8 = undefined;
    let rp: usize = writeStr(user, raw[0..512], 0);
    raw[rp] = ':' as u8;
    * = writeStr(pass, raw[0..512], rp + 1);

    const prefix = "Basic ";
    const b64_len: usize = ((raw_len + 2) / 3) * 4;
    if (prefix.len + b64_len > out.len) {
        return 0;
    }

    * = writeStr(prefix, out, 0);
    const enc: i64 = b64Encode(raw[0..raw_len], out[prefix.len..out.len]);
    if (enc < 0) {
        return 0;
    }
    return prefix.len + (enc as usize);
}

pub fn constantTimeEq(a: []const u8, b: []const u8) -> bool {
    return ctEqual(a, b);
}

pub fn verifyHeader(header: string, expected_user: string, expected_pass: string) -> bool {
    let u_buf: [256]u8 = undefined;
    let p_buf: [256]u8 = undefined;
    const creds: BasicCredentials = decodeHeader(header, u_buf[0..256], p_buf[0..256]);
    if (!creds.valid) {
        return false;
    }
    const u_ok: bool = constantTimeEq(u_buf[0..creds.user_len], expected_user);
    const p_ok: bool = constantTimeEq(p_buf[0..creds.pass_len], expected_pass);
    return u_ok && p_ok;
}

fn appendSlice(out: []u8, pos: usize, s: string) -> i64 {
    if (s.len > out.len - pos) {
        return -1;
    }
    return writeStr(s, out, pos) as i64;
}

pub fn formatChallenge(realm: string, utf8: bool, out: []u8) -> usize {
    let p: i64 = appendSlice(out, 0, "Basic realm=\"");
    if (p < 0) {
        return 0;
    }
    let i: usize = 0;
    while (i < realm.len) {
        const c: u8 = realm[i];
        if (c == '"' as u8 || c == '\\' as u8) {
            const cur: usize = p as usize;
            if (2 > out.len - cur) {
                return 0;
            }
            out[cur] = '\\' as u8;
            out[cur + 1] = c;
            p = (cur + 2) as i64;
        } else {
            const cur: usize = p as usize;
            if (cur >= out.len) {
                return 0;
            }
            out[cur] = c;
            p = (cur + 1) as i64;
        }
        i += 1;
    }

    p = appendSlice(out, p as usize, "\"");
    if (p < 0) {
        return 0;
    }
    if (utf8) {
        p = appendSlice(out, p as usize, ", charset=\"UTF-8\"");
        if (p < 0) {
            return 0;
        }
    }
    return p as usize;
}
