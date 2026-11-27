// @cap/cookie-sign — Zero-heap RFC 6265 Cookie parser, Set-Cookie formatter & HMAC-SHA256 sign/unsign
// MIT License (clean-room CP implementation, zero external dependencies)

pub const SAME_SITE_OMIT: u8 = 0;
pub const SAME_SITE_STRICT: u8 = 1;
pub const SAME_SITE_LAX: u8 = 2;
pub const SAME_SITE_NONE: u8 = 3;

const SIG_B64_LEN: usize = 43;

pub struct CookieSlice {
    pub start: usize,
    pub len: usize,
    pub found: bool,
}

pub struct CookieOptions {
    pub max_age: i64,
    pub path: []const u8,
    pub domain: []const u8,
    pub http_only: bool,
    pub secure: bool,
    pub same_site: u8,
}

// ============================================================================
// Built-in FIPS 180-4 SHA-256 (wrapping u32 via u64 mask, Debug-safe)
// ============================================================================

fn add32(a: u32, b: u32) -> u32 {
    return (((a as u64) + (b as u64)) & 0xFFFFFFFF) as u32;
}

fn rotr32(x: u32, n: u32) -> u32 {
    return (x >> (n as u5)) | (x << ((32 - n) as u5));
}

fn sha256K(i: usize) -> u32 {
    const table: [64]u32 = [
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
    ];
    return table[i];
}

pub struct UnsignResult {
    pub ok: bool,
    pub val_len: usize,
}

pub struct Sha256State {
    pub h: [8]u32,
    pub block: [64]u8,
    pub block_len: usize,
    pub total_bytes: u64,

    static fn init() -> Sha256State {
        let s = Sha256State {
            h: [
                0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
            ],
            block: undefined,
            block_len: 0,
            total_bytes: 0,
        };
        return s;
    }

    fn compress(*_, blk: []const u8) {
        let w: [64]u32 = undefined;
        let i: usize = 0;
        while (i < 16) {
            const b0: u32 = (blk[i * 4] as u32) << 24;
            const b1: u32 = (blk[i * 4 + 1] as u32) << 16;
            const b2: u32 = (blk[i * 4 + 2] as u32) << 8;
            const b3: u32 = blk[i * 4 + 3] as u32;
            w[i] = b0 | b1 | b2 | b3;
            i += 1;
        }
        while (i < 64) {
            const s0: u32 = rotr32(w[i - 15], 7) ^ rotr32(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const s1: u32 = rotr32(w[i - 2], 17) ^ rotr32(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = add32(add32(w[i - 16], s0), add32(w[i - 7], s1));
            i += 1;
        }

        let a: u32 = _.h[0];
        let b: u32 = _.h[1];
        let c: u32 = _.h[2];
        let d: u32 = _.h[3];
        let e: u32 = _.h[4];
        let f: u32 = _.h[5];
        let g: u32 = _.h[6];
        let hv: u32 = _.h[7];

        i = 0;
        while (i < 64) {
            const sum1: u32 = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
            const ch: u32 = (e & f) ^ ((e ^ 0xFFFFFFFF) & g);
            const temp1: u32 = add32(add32(add32(hv, sum1), add32(ch, sha256K(i))), w[i]);
            const sum0: u32 = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
            const maj: u32 = (a & b) ^ (a & c) ^ (b & c);
            const temp2: u32 = add32(sum0, maj);

            hv = g;
            g = f;
            f = e;
            e = add32(d, temp1);
            d = c;
            c = b;
            b = a;
            a = add32(temp1, temp2);
            i += 1;
        }

        _.h[0] = add32(_.h[0], a);
        _.h[1] = add32(_.h[1], b);
        _.h[2] = add32(_.h[2], c);
        _.h[3] = add32(_.h[3], d);
        _.h[4] = add32(_.h[4], e);
        _.h[5] = add32(_.h[5], f);
        _.h[6] = add32(_.h[6], g);
        _.h[7] = add32(_.h[7], hv);
    }

    fn update(*_, data: []const u8) {
        _.total_bytes = _.total_bytes + (data.len as u64);
        let pos: usize = 0;
        while (pos < data.len) {
            _.block[_.block_len] = data[pos];
            _.block_len += 1;
            pos += 1;
            if (_.block_len == 64) {
                _.compress(_.block[0..64]);
                _.block_len = 0;
            }
        }
    }

    fn finish(*_, out: []u8) {
        const bit_len: u64 = _.total_bytes * 8;
        _.block[_.block_len] = 0x80;
        _.block_len += 1;

        if (_.block_len > 56) {
            while (_.block_len < 64) {
                _.block[_.block_len] = 0;
                _.block_len += 1;
            }
            _.compress(_.block[0..64]);
            _.block_len = 0;
        }
        while (_.block_len < 56) {
            _.block[_.block_len] = 0;
            _.block_len += 1;
        }

        let k: usize = 0;
        while (k < 8) {
            const shift: u6 = ((7 - k) * 8) as u6;
            _.block[56 + k] = ((bit_len >> shift) & 0xFF) as u8;
            k += 1;
        }
        _.compress(_.block[0..64]);

        let i: usize = 0;
        while (i < 8) {
            out[i * 4] = ((_.h[i] >> 24) & 0xFF) as u8;
            out[i * 4 + 1] = ((_.h[i] >> 16) & 0xFF) as u8;
            out[i * 4 + 2] = ((_.h[i] >> 8) & 0xFF) as u8;
            out[i * 4 + 3] = (_.h[i] & 0xFF) as u8;
            i += 1;
        }
    }
}

pub fn sha256Init() -> Sha256State {
    return Sha256State.init();
}

pub fn sha256Update(st: *Sha256State, data: []const u8) {
    st.update(data);
}

pub fn sha256Final(st: *Sha256State, out32: []u8) {
    st.finish(out32);
}

pub fn sha256(data: []const u8, out32: []u8) {
    let st: Sha256State = Sha256State.init();
    st.update(data);
    st.finish(out32);
}

pub fn ctEqual(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) {
        diff = diff | (a[i] ^ b[i]);
        i += 1;
    }
    return diff == 0;
}

pub fn hmacSha256(key: []const u8, msg: []const u8, out: []u8) {
    let k_pad: [64]u8 = undefined;
    let i: usize = 0;
    while (i < 64) {
        k_pad[i] = 0;
        i += 1;
    }

    if (key.len > 64) {
        let ks: Sha256State = Sha256State.init();
        ks.update(key);
        ks.finish(k_pad[0..32]);
    } else {
        i = 0;
        while (i < key.len) {
            k_pad[i] = key[i];
            i += 1;
        }
    }

    let ipad: [64]u8 = undefined;
    let opad: [64]u8 = undefined;
    i = 0;
    while (i < 64) {
        ipad[i] = k_pad[i] ^ 0x36;
        opad[i] = k_pad[i] ^ 0x5C;
        i += 1;
    }

    let inner_hash: [32]u8 = undefined;
    let inner_st: Sha256State = Sha256State.init();
    inner_st.update(ipad[0..64]);
    inner_st.update(msg);
    inner_st.finish(inner_hash[0..32]);

    let outer_st: Sha256State = Sha256State.init();
    outer_st.update(opad[0..64]);
    outer_st.update(inner_hash[0..32]);
    outer_st.finish(out);
}

fn b64UrlChar(v: u8) -> u8 {
    const alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    return alphabet[v as usize];
}

fn encodeBase64Url32(mac: []const u8, dst: []u8) {
    // 32 bytes = 10 full 3-byte groups (40 chars) + 2 remaining bytes (3 chars) = 43 chars
    let out_i: usize = 0;
    let i: usize = 0;
    while (i < 30) {
        const b0: u32 = mac[i] as u32;
        const b1: u32 = mac[i + 1] as u32;
        const b2: u32 = mac[i + 2] as u32;
        const triple: u32 = (b0 << 16) | (b1 << 8) | b2;
        dst[out_i] = b64UrlChar(((triple >> 18) & 0x3F) as u8);
        dst[out_i + 1] = b64UrlChar(((triple >> 12) & 0x3F) as u8);
        dst[out_i + 2] = b64UrlChar(((triple >> 6) & 0x3F) as u8);
        dst[out_i + 3] = b64UrlChar((triple & 0x3F) as u8);
        out_i += 4;
        i += 3;
    }
    const r0: u32 = mac[30] as u32;
    const r1: u32 = mac[31] as u32;
    const rem: u32 = (r0 << 16) | (r1 << 8);
    dst[40] = b64UrlChar(((rem >> 18) & 0x3F) as u8);
    dst[41] = b64UrlChar(((rem >> 12) & 0x3F) as u8);
    dst[42] = b64UrlChar(((rem >> 6) & 0x3F) as u8);
}

// ============================================================================
// Public Cookie API
// ============================================================================

fn isWs(c: u8) -> bool {
    return c == ' ' as u8 || c == '\t' as u8;
}

fn csBytesEq(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn getCookie(header: []const u8, name: []const u8) -> CookieSlice {
    const miss = CookieSlice { start: 0, len: 0, found: false };
    if (header.len == 0 || name.len == 0) {
        return miss;
    }

    let pos: usize = 0;
    while (pos < header.len) {
        let seg_end: usize = pos;
        while (seg_end < header.len && header[seg_end] != ';' as u8) {
            seg_end += 1;
        }

        // Find '=' inside [pos, seg_end)
        let eq_pos: usize = pos;
        while (eq_pos < seg_end && header[eq_pos] != '=' as u8) {
            eq_pos += 1;
        }

        if (eq_pos < seg_end) {
            // Trim key
            let k_start: usize = pos;
            let k_end: usize = eq_pos;
            while (k_start < k_end && isWs(header[k_start])) {
                k_start += 1;
            }
            while (k_end > k_start && isWs(header[k_end - 1])) {
                k_end -= 1;
            }

            if (csBytesEq(header[k_start..k_end], name)) {
                let v_start: usize = eq_pos + 1;
                let v_end: usize = seg_end;
                while (v_start < v_end && isWs(header[v_start])) {
                    v_start += 1;
                }
                while (v_end > v_start && isWs(header[v_end - 1])) {
                    v_end -= 1;
                }
                if (v_end - v_start >= 2 && header[v_start] == '"' as u8 && header[v_end - 1] == '"' as u8) {
                    v_start += 1;
                    v_end -= 1;
                }
                return CookieSlice {
                    start: v_start,
                    len: v_end - v_start,
                    found: true,
                };
            }
        }

        pos = seg_end;
        if (pos < header.len && header[pos] == ';' as u8) {
            pos += 1;
        }
    }
    return miss;
}

pub fn sign(val: []const u8, secret: []const u8, dst: []u8) -> usize {
    const total: usize = val.len + 1 + SIG_B64_LEN;
    if (total > dst.len) {
        return 0;
    }

    let i: usize = 0;
    while (i < val.len) {
        dst[i] = val[i];
        i += 1;
    }
    dst[val.len] = '.' as u8;

    let mac: [32]u8 = undefined;
    hmacSha256(secret, val, mac[0..32]);
    encodeBase64Url32(mac[0..32], dst[(val.len + 1)..total]);
    return total;
}

pub fn unsign(signed_val: []const u8, secret: []const u8) -> UnsignResult {
    const miss = UnsignResult { ok: false, val_len: 0 };
    if (signed_val.len < 1 + SIG_B64_LEN) {
        return miss;
    }

    // Locate the LAST '.' separator
    let found_dot: bool = false;
    let dot_pos: usize = 0;
    let i: usize = signed_val.len;
    while (i > 0) {
        i -= 1;
        if (signed_val[i] == '.' as u8) {
            dot_pos = i;
            found_dot = true;
            break;
        }
    }
    if (!found_dot) {
        return miss;
    }

    const sig_start: usize = dot_pos + 1;
    if (signed_val.len - sig_start != SIG_B64_LEN) {
        return miss;
    }

    const val_part: []const u8 = signed_val[0..dot_pos];
    let mac: [32]u8 = undefined;
    let expected: [43]u8 = undefined;
    hmacSha256(secret, val_part, mac[0..32]);
    encodeBase64Url32(mac[0..32], expected[0..43]);

    if (!ctEqual(signed_val[sig_start..signed_val.len], expected[0..SIG_B64_LEN])) {
        return miss;
    }

    return UnsignResult { ok: true, val_len: dot_pos };
}

fn appendBytes(dst: []u8, pos: usize, src: []const u8) -> i64 {
    if (src.len > dst.len - pos) {
        return -1;
    }
    let i: usize = 0;
    while (i < src.len) {
        dst[pos + i] = src[i];
        i += 1;
    }
    return (pos + src.len) as i64;
}

fn appendU64(dst: []u8, pos: usize, val: u64) -> i64 {
    let digits: [24]u8 = undefined;
    let count: usize = 0;
    if (val == 0) {
        digits[0] = '0' as u8;
        count = 1;
    } else {
        let cur: u64 = val;
        while (cur > 0) {
            digits[count] = ('0' as u8) + ((cur % 10) as u8);
            count += 1;
            cur = cur / 10;
        }
    }
    if (count > dst.len - pos) {
        return -1;
    }
    let p: usize = pos;
    let k: usize = count;
    while (k > 0) {
        k -= 1;
        dst[p] = digits[k];
        p += 1;
    }
    return p as i64;
}

pub fn formatSetCookie(
    name: []const u8,
    value: []const u8,
    opts: CookieOptions,
    dst: []u8,
) -> i64 {
    if (name.len == 0) {
        return -1;
    }
    let p: i64 = appendBytes(dst, 0, name);
    if (p < 0) {
        return -1;
    }
    p = appendBytes(dst, p as usize, "=");
    if (p < 0) {
        return -1;
    }
    p = appendBytes(dst, p as usize, value);
    if (p < 0) {
        return -1;
    }

    if (opts.max_age >= 0) {
        p = appendBytes(dst, p as usize, "; Max-Age=");
        if (p < 0) {
            return -1;
        }
        p = appendU64(dst, p as usize, opts.max_age as u64);
        if (p < 0) {
            return -1;
        }
    }
    if (opts.domain.len > 0) {
        p = appendBytes(dst, p as usize, "; Domain=");
        if (p < 0) {
            return -1;
        }
        p = appendBytes(dst, p as usize, opts.domain);
        if (p < 0) {
            return -1;
        }
    }
    if (opts.path.len > 0) {
        p = appendBytes(dst, p as usize, "; Path=");
        if (p < 0) {
            return -1;
        }
        p = appendBytes(dst, p as usize, opts.path);
        if (p < 0) {
            return -1;
        }
    }
    if (opts.http_only) {
        p = appendBytes(dst, p as usize, "; HttpOnly");
        if (p < 0) {
            return -1;
        }
    }
    if (opts.secure) {
        p = appendBytes(dst, p as usize, "; Secure");
        if (p < 0) {
            return -1;
        }
    }
    if (opts.same_site == SAME_SITE_STRICT) {
        p = appendBytes(dst, p as usize, "; SameSite=Strict");
        if (p < 0) {
            return -1;
        }
    } else if (opts.same_site == SAME_SITE_LAX) {
        p = appendBytes(dst, p as usize, "; SameSite=Lax");
        if (p < 0) {
            return -1;
        }
    } else if (opts.same_site == SAME_SITE_NONE) {
        p = appendBytes(dst, p as usize, "; SameSite=None");
        if (p < 0) {
            return -1;
        }
    }
    return p;
}
