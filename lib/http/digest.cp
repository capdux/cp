// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T05:31:41Z (RFC 9530 Content-Digest / Repr-Digest / Want-Digest)
// Pure-CP zero-allocation RFC 9530 HTTP Digest Fields Structured-Field dictionary parser and formatter.

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

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
        i += 1;
    }
    return true;
}

fn isValidSfKey(key: []const u8): bool {
    if (key.len == 0) return false;
    const c0 = key[0];
    if (!((c0 >= 'a' && c0 <= 'z') || (c0 >= 'A' && c0 <= 'Z') || c0 == '*')) return false;
    let i: usize = 1;
    while (i < key.len) {
        const c = key[i];
        const ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            c == '_' || c == '-' || c == '.' || c == '*';
        if (!ok) return false;
        i += 1;
    }
    return true;
}

fn isValidBase64Char(b: u8): bool {
    return (b >= 'A' && b <= 'Z') || (b >= 'a' && b <= 'z') || (b >= '0' && b <= '9') ||
        b == '+' || b == '/' || b == '=';
}

export struct DigestEntry {
    algorithm: []const u8,
    base64_value: []const u8,
}

export struct DigestIter {
    src: []const u8,
    pos: usize,

    pub fn init(header: []const u8): DigestIter {
        return DigestIter { src: header, pos: 0 };
    }

    /// Yield next valid RFC 9530 `alg=:base64:` entry from `Content-Digest` or `Repr-Digest`.
    pub fn next(*_): DigestEntry? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != ',') {
                _.pos += 1;
            }
            const end = _.pos;
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
            }
            const item = trimWs(_.src[start..end]);
            if (item.len == 0) continue;

            // Find '=' separating algorithm key and `:base64:` byte sequence
            let eq_pos: usize? = null;
            let k: usize = 0;
            while (k < item.len) {
                if (item[k] == '=') {
                    eq_pos = k;
                    break;
                }
                k += 1;
            }
            if (eq_pos == null) continue;
            const ep = eq_pos.?;
            const alg = trimWs(item[0..ep]);
            if (!isValidSfKey(alg)) continue;

            let rhs = trimWs(item[ep + 1..]);
            // Strip optional SF parameters starting at ';'
            let si: usize = 0;
            while (si < rhs.len) {
                if (rhs[si] == ';') {
                    rhs = trimWs(rhs[0..si]);
                    break;
                }
                si += 1;
            }
            // RFC 8941 Byte Sequence: `:<base64>:`
            if (rhs.len < 2 || rhs[0] != ':' || rhs[rhs.len - 1] != ':') continue;
            const b64 = rhs[1..rhs.len - 1];
            let valid = true;
            let bi: usize = 0;
            while (bi < b64.len) {
                if (!isValidBase64Char(b64[bi])) {
                    valid = false;
                    break;
                }
                bi += 1;
            }
            if (!valid) continue;
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
    const total = algorithm.len + 3 + base64_value.len;
    if (total > out.len) return null;
    let pos: usize = 0;
    let i: usize = 0;
    while (i < algorithm.len) {
        out[pos] = toLowerAscii(algorithm[i]);
        pos += 1;
        i += 1;
    }
    out[pos] = '=';
    out[pos + 1] = ':';
    pos += 2;
    let j: usize = 0;
    while (j < base64_value.len) {
        if (!isValidBase64Char(base64_value[j])) return null;
        out[pos] = base64_value[j];
        pos += 1;
        j += 1;
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

    pub fn init(header: []const u8): WantDigestIter {
        return WantDigestIter { src: header, pos: 0 };
    }

    pub fn next(*_): WantDigestPref? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != ',') {
                _.pos += 1;
            }
            const end = _.pos;
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
            }
            const item = trimWs(_.src[start..end]);
            if (item.len == 0) continue;

            let eq_pos: usize? = null;
            let k: usize = 0;
            while (k < item.len) {
                if (item[k] == '=') {
                    eq_pos = k;
                    break;
                }
                k += 1;
            }
            if (eq_pos == null) continue;
            const ep = eq_pos.?;
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
