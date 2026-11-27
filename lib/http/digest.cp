// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-23T01:59:15Z (RFC 9530 Content-Digest / Repr-Digest / Want-Digest)
// Pure-CP zero-allocation RFC 9530 HTTP Digest Fields Structured-Field dictionary parser and formatter.
// Accelerated with 16-lane @Vector(16, u8) SIMD scanning/comparison, comptime 256-entry LUTs, and @memcpy.

type DigestVec16 = @Vector(16, u8);

fn makeDigestSfKeyLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        const c = @as(u8, @intCast(i));
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            c == '_' || c == '-' || c == '.' || c == '*')
        {
            lut[i] = 1;
        }
    }
    return lut;
}

fn makeDigestB64Lut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        const c = @as(u8, @intCast(i));
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
            c == '+' || c == '/' || c == '=')
        {
            lut[i] = 1;
        }
    }
    return lut;
}

const DIGEST_SF_KEY_LUT: [u8; 256] = makeDigestSfKeyLut();
const DIGEST_B64_LUT: [u8; 256] = makeDigestB64Lut();

fn isWs(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) start += 1;
    let end: usize = s.len;
    while (end > start && isWs(s[end - 1])) end -= 1;
    return s[start..end];
}

fn toLowerAscii(b: u8): u8 {
    if (b >= 'A' && b <= 'Z') return b + 32;
    return b;
}

fn simdFindByte(s: []const u8, start: usize, needle: u8): usize? {
    let i: usize = start;
    const n = s.len;
    const v_needle: DigestVec16 = @splat(needle);
    while (i + 16 <= n) {
        const chunk: DigestVec16 = s[i..][0..16].*;
        const mask: u16 = @bitCast(chunk == v_needle);
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < n) : (i += 1) {
        if (s[i] == needle) return i;
    }
    return null;
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    const n = a.len;
    const v_A: DigestVec16 = @splat('A');
    const v_Z: DigestVec16 = @splat('Z');
    const v_case: DigestVec16 = @splat(32);
    while (i + 16 <= n) : (i += 16) {
        const va: DigestVec16 = a[i..][0..16].*;
        const vb: DigestVec16 = b[i..][0..16].*;
        const ma = (va >= v_A) & (va <= v_Z);
        const mb = (vb >= v_A) & (vb <= v_Z);
        const la = @select(u8, ma, va +% v_case, va);
        const lb = @select(u8, mb, vb +% v_case, vb);
        const neq: u16 = @bitCast(la != lb);
        if (neq != 0) return false;
    }
    while (i < n) : (i += 1) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
    }
    return true;
}

fn isValidSfKey(key: []const u8): bool {
    if (key.len == 0) return false;
    const c0 = key[0];
    if (!((c0 >= 'a' && c0 <= 'z') || (c0 >= 'A' && c0 <= 'Z') || c0 == '*')) return false;
    let i: usize = 1;
    const n = key.len;
    const v_0: DigestVec16 = @splat('0');
    const v_10: DigestVec16 = @splat(10);
    const v_a: DigestVec16 = @splat('a');
    const v_26: DigestVec16 = @splat(26);
    const v_case: DigestVec16 = @splat(0x20);
    const v_dash: DigestVec16 = @splat('-');
    const v_under: DigestVec16 = @splat('_');
    while (i + 16 <= n) {
        const chunk: DigestVec16 = key[i..][0..16].*;
        const m_dig: u16 = @bitCast((chunk -% v_0) < v_10);
        const m_alp: u16 = @bitCast(((chunk | v_case) -% v_a) < v_26);
        const m_dash: u16 = @bitCast(chunk == v_dash);
        const m_under: u16 = @bitCast(chunk == v_under);
        const bad: u16 = ~(m_dig | m_alp | m_dash | m_under);
        if (bad != 0) {
            const idx = i + (@ctz(bad) as usize);
            if (DIGEST_SF_KEY_LUT[key[idx] as usize] == 0) return false;
            i = idx + 1;
        } else {
            i += 16;
        }
    }
    while (i < n) : (i += 1) {
        if (DIGEST_SF_KEY_LUT[key[i] as usize] == 0) return false;
    }
    return true;
}

fn isValidBase64Char(b: u8): bool {
    return DIGEST_B64_LUT[b as usize] != 0;
}

fn isValidBase64Slice(s: []const u8): bool {
    let i: usize = 0;
    const n = s.len;
    const v_0: DigestVec16 = @splat('0');
    const v_10: DigestVec16 = @splat(10);
    const v_a: DigestVec16 = @splat('a');
    const v_26: DigestVec16 = @splat(26);
    const v_case: DigestVec16 = @splat(0x20);
    const v_plus: DigestVec16 = @splat('+');
    const v_slash: DigestVec16 = @splat('/');
    const v_eq: DigestVec16 = @splat('=');
    while (i + 16 <= n) : (i += 16) {
        const chunk: DigestVec16 = s[i..][0..16].*;
        const m_dig: u16 = @bitCast((chunk -% v_0) < v_10);
        const m_alp: u16 = @bitCast(((chunk | v_case) -% v_a) < v_26);
        const m_plus: u16 = @bitCast(chunk == v_plus);
        const m_slash: u16 = @bitCast(chunk == v_slash);
        const m_eq: u16 = @bitCast(chunk == v_eq);
        if ((~(m_dig | m_alp | m_plus | m_slash | m_eq)) != 0) return false;
    }
    while (i < n) : (i += 1) {
        if (DIGEST_B64_LUT[s[i] as usize] == 0) return false;
    }
    return true;
}

export struct DigestEntry {
    algorithm: []const u8,
    base64_value: []const u8,
}

export struct DigestIter {
    src: []const u8,
    pos: usize,

    pub static fn init(header: []const u8): DigestIter {
        return DigestIter { src: header, pos: 0 };
    }

    /// Yield next valid RFC 9530 `alg=:base64:` entry from `Content-Digest` or `Repr-Digest`.
    pub fn next(*_): DigestEntry? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            const end = simdFindByte(_.src, _.pos, ',') ?? _.src.len;
            _.pos = if (end < _.src.len) (end + 1) else _.src.len;
            const item = trimWs(_.src[start..end]);
            if (item.len == 0) continue;

            // Find '=' separating algorithm key and `:base64:` byte sequence
            const ep = simdFindByte(item, 0, '=') ?? continue;
            const alg = trimWs(item[0..ep]);
            if (!isValidSfKey(alg)) continue;

            let rhs = trimWs(item[ep + 1..]);
            // Strip optional SF parameters starting at ';'
            if (si := simdFindByte(rhs, 0, ';')) {
                rhs = trimWs(rhs[0..si]);
            }
            // RFC 8941 Byte Sequence: `:<base64>:`
            if (rhs.len < 2 || rhs[0] != ':' || rhs[rhs.len - 1] != ':') continue;
            const b64 = rhs[1..rhs.len - 1];
            if (!isValidBase64Slice(b64)) continue;
            return DigestEntry { algorithm: alg, base64_value: b64 };
        }
        return null;
    }
}

/// Lookup the base64 digest value for a specific algorithm (e.g. `"sha-256"`, `"sha-512"`).
export fn findDigest(header: []const u8, algorithm: []const u8): ([]const u8)? {
    let it = DigestIter.init(header);
    while (entry := it.next()) {
        if (eqIgnoreCase(entry.algorithm, algorithm)) {
            return entry.base64_value;
        }
    }
    return null;
}

/// Format a single RFC 9530 `Content-Digest` / `Repr-Digest` member `alg=:base64:` into `out`.
export fn formatDigest(algorithm: []const u8, base64_value: []const u8, out: []u8): ([]const u8)? {
    if (!isValidSfKey(algorithm)) return null;
    if (!isValidBase64Slice(base64_value)) return null;
    const total = algorithm.len + 3 + base64_value.len;
    if (total > out.len) return null;
    let pos: usize = 0;
    let i: usize = 0;
    const v_A: DigestVec16 = @splat('A');
    const v_Z: DigestVec16 = @splat('Z');
    const v_case: DigestVec16 = @splat(32);
    while (i + 16 <= algorithm.len) : (i += 16) {
        const va: DigestVec16 = algorithm[i..][0..16].*;
        const ma = (va >= v_A) & (va <= v_Z);
        out[pos..][0..16].* = @select(u8, ma, va +% v_case, va);
        pos += 16;
    }
    while (i < algorithm.len) {
        out[pos] = toLowerAscii(algorithm[i]);
        pos += 1;
        i += 1;
    }
    out[pos] = '=';
    out[pos + 1] = ':';
    pos += 2;
    if (base64_value.len > 0) {
        @memcpy(out[pos .. pos + base64_value.len], base64_value);
        pos += base64_value.len;
    }
    out[pos] = ':';
    pos += 1;
    return out[0..pos];
}

export struct WantDigestPref {
    algorithm: []const u8,
    weight: u8, // RFC 9530 §4: integer 0..=10
}

export struct WantDigestIter {
    src: []const u8,
    pos: usize,

    pub static fn init(header: []const u8): WantDigestIter {
        return WantDigestIter { src: header, pos: 0 };
    }

    pub fn next(*_): WantDigestPref? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            const end = simdFindByte(_.src, _.pos, ',') ?? _.src.len;
            _.pos = if (end < _.src.len) (end + 1) else _.src.len;
            const item = trimWs(_.src[start..end]);
            if (item.len == 0) continue;

            const ep = simdFindByte(item, 0, '=') ?? continue;
            const alg = trimWs(item[0..ep]);
            if (!isValidSfKey(alg)) continue;
            const w_str = trimWs(item[ep + 1..]);
            if (w_str.len == 0 || w_str.len > 2) continue;
            let w: u8 = 0;
            if (w_str.len == 1) {
                if (w_str[0] < '0' || w_str[0] > '9') continue;
                w = w_str[0] - '0';
            } else {
                if (w_str[0] == '1' && w_str[1] == '0') {
                    w = 10;
                } else {
                    continue;
                }
            }
            return WantDigestPref { algorithm: alg, weight: w };
        }
        return null;
    }
}

/// Select the highest-weighted supported algorithm index from a `Want-Content-Digest` header.
/// Weight 0 explicitly rejects an algorithm. Returns null if none acceptable.
export fn negotiateDigestAlg(want_header: []const u8, supported: []const []const u8): usize? {
    let best_idx: usize? = null;
    let best_w: u8 = 0;

    let si: usize = 0;
    while (si < supported.len) {
        let it = WantDigestIter.init(want_header);
        let found = false;
        let w: u8 = 0;
        while (pref := it.next()) {
            if (eqIgnoreCase(pref.algorithm, supported[si])) {
                found = true;
                w = pref.weight;
            }
        }
        if (found && w > 0 && (best_idx == null || w > best_w)) {
            best_idx = si;
            best_w = w;
        }
        si += 1;
    }
    return best_idx;
}
