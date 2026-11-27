// @cap/digest-auth — Zero-allocation RFC 7616 / RFC 2617 HTTP Digest Authentication Parser & Formatter
// Clean-room MIT implementation in pure CP.

pub struct DigestChallenge {
    pub realm: string,
    pub nonce: string,
    pub opaque_val: string,
    pub qop: string,
    pub algorithm: string,
    pub stale: bool,
    pub valid: bool,
}

pub struct DigestResponse {
    pub username: string,
    pub realm: string,
    pub nonce: string,
    pub uri: string,
    pub qop: string,
    pub nc: string,
    pub cnonce: string,
    pub response: string,
    pub opaque_val: string,
    pub valid: bool,
}

fn isSpace(b: u8) -> bool {
    return b == 32 || b == 9;
}

fn toLowerAscii(b: u8) -> u8 {
    if (b >= 65 && b <= 90) {
        return b + 32;
    }
    return b;
}

fn trimSlice(s: string) -> string {
    let a: usize = 0;
    let b: usize = s.len;
    while (a < b && isSpace(s[a])) {
        a += 1;
    }
    while (b > a && isSpace(s[b - 1])) {
        b -= 1;
    }
    return s[a..b];
}

fn eqIgnoreCase(a: string, lower_b: string) -> bool {
    if (a.len != lower_b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != lower_b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn stripDigestPrefix(header: string) -> string {
    const s = trimSlice(header);
    if (s.len < 7) {
        return "";
    }
    if (toLowerAscii(s[0]) != 100
        || toLowerAscii(s[1]) != 105
        || toLowerAscii(s[2]) != 103
        || toLowerAscii(s[3]) != 101
        || toLowerAscii(s[4]) != 115
        || toLowerAscii(s[5]) != 116
        || !isSpace(s[6]))
    {
        return "";
    }
    return trimSlice(s[7..s.len]);
}

fn unquoteVal(v: string) -> string {
    if (v.len >= 2 && v[0] == 34 && v[v.len - 1] == 34) {
        return v[1..v.len - 1];
    }
    return v;
}

/// Parses a `WWW-Authenticate: Digest ...` challenge header (zero-copy).
/// Requires at least `realm` and `nonce` to be non-empty.
pub fn parseChallenge(header: string) -> DigestChallenge {
    let ch = DigestChallenge {
        realm: "",
        nonce: "",
        opaque_val: "",
        qop: "",
        algorithm: "",
        stale: false,
        valid: false,
    };

    const body = stripDigestPrefix(header);
    if (body.len == 0) {
        return ch;
    }

    let pos: usize = 0;
    const n = body.len;
    while (pos < n) {
        const start = pos;
        let in_quote = false;
        while (pos < n) {
            const c = body[pos];
            if (c == 92 && in_quote && pos + 1 < n) {
                pos += 2;
                continue;
            }
            if (c == 34) {
                in_quote = !in_quote;
            } else if (c == 44 && !in_quote) {
                break;
            }
            pos += 1;
        }
        const pair = trimSlice(body[start..pos]);
        if (pos < n && body[pos] == 44) {
            pos += 1;
        }
        if (pair.len == 0) {
            continue;
        }

        let eq: usize = 0;
        while (eq < pair.len && pair[eq] != 61) {
            eq += 1;
        }
        if (eq >= pair.len || eq == 0) {
            continue;
        }
        const k = trimSlice(pair[0..eq]);
        const v = unquoteVal(trimSlice(pair[eq + 1..pair.len]));

        if (eqIgnoreCase(k, "realm")) {
            ch.realm = v;
        } else if (eqIgnoreCase(k, "nonce")) {
            ch.nonce = v;
        } else if (eqIgnoreCase(k, "opaque")) {
            ch.opaque_val = v;
        } else if (eqIgnoreCase(k, "qop")) {
            ch.qop = v;
        } else if (eqIgnoreCase(k, "algorithm")) {
            ch.algorithm = v;
        } else if (eqIgnoreCase(k, "stale")) {
            ch.stale = eqIgnoreCase(v, "true");
        }
    }

    ch.valid = ch.realm.len > 0 && ch.nonce.len > 0;
    return ch;
}

/// Parses an `Authorization: Digest ...` request header (zero-copy).
/// Requires `username`, `realm`, `nonce`, `uri`, and `response` to be non-empty.
pub fn parseAuthorization(header: string) -> DigestResponse {
    let resp = DigestResponse {
        username: "",
        realm: "",
        nonce: "",
        uri: "",
        qop: "",
        nc: "",
        cnonce: "",
        response: "",
        opaque_val: "",
        valid: false,
    };

    const body = stripDigestPrefix(header);
    if (body.len == 0) {
        return resp;
    }

    let pos: usize = 0;
    const n = body.len;
    while (pos < n) {
        const start = pos;
        let in_quote = false;
        while (pos < n) {
            const c = body[pos];
            if (c == 92 && in_quote && pos + 1 < n) {
                pos += 2;
                continue;
            }
            if (c == 34) {
                in_quote = !in_quote;
            } else if (c == 44 && !in_quote) {
                break;
            }
            pos += 1;
        }
        const pair = trimSlice(body[start..pos]);
        if (pos < n && body[pos] == 44) {
            pos += 1;
        }
        if (pair.len == 0) {
            continue;
        }

        let eq: usize = 0;
        while (eq < pair.len && pair[eq] != 61) {
            eq += 1;
        }
        if (eq >= pair.len || eq == 0) {
            continue;
        }
        const k = trimSlice(pair[0..eq]);
        const v = unquoteVal(trimSlice(pair[eq + 1..pair.len]));

        if (eqIgnoreCase(k, "username")) {
            resp.username = v;
        } else if (eqIgnoreCase(k, "realm")) {
            resp.realm = v;
        } else if (eqIgnoreCase(k, "nonce")) {
            resp.nonce = v;
        } else if (eqIgnoreCase(k, "uri")) {
            resp.uri = v;
        } else if (eqIgnoreCase(k, "qop")) {
            resp.qop = v;
        } else if (eqIgnoreCase(k, "nc")) {
            resp.nc = v;
        } else if (eqIgnoreCase(k, "cnonce")) {
            resp.cnonce = v;
        } else if (eqIgnoreCase(k, "response")) {
            resp.response = v;
        } else if (eqIgnoreCase(k, "opaque")) {
            resp.opaque_val = v;
        }
    }

    resp.valid = resp.username.len > 0
        && resp.realm.len > 0
        && resp.nonce.len > 0
        && resp.uri.len > 0
        && resp.response.len > 0;
    return resp;
}

/// Checks if a comma-separated `qop_list` (e.g. `"auth, auth-int"`) contains `target` (case-insensitive).
pub fn supportsQop(qop_list: string, target: string) -> bool {
    if (qop_list.len == 0 || target.len == 0) {
        return false;
    }
    let pos: usize = 0;
    const n = qop_list.len;
    while (pos < n) {
        const start = pos;
        while (pos < n && qop_list[pos] != 44) {
            pos += 1;
        }
        const item = trimSlice(qop_list[start..pos]);
        if (item.len == target.len) {
            let ok = true;
            let i: usize = 0;
            while (i < item.len) {
                if (toLowerAscii(item[i]) != toLowerAscii(target[i])) {
                    ok = false;
                    break;
                }
                i += 1;
            }
            if (ok) {
                return true;
            }
        }
        if (pos < n && qop_list[pos] == 44) {
            pos += 1;
        }
    }
    return false;
}

fn pushSlice(dst: []u8, pos: usize, src: string) -> i64 {
    if (pos + src.len > dst.len) {
        return -1;
    }
    let i: usize = 0;
    while (i < src.len) {
        dst[pos + i] = src[i];
        i += 1;
    }
    return (pos + src.len) as i64;
}

fn pushQuotedField(dst: []u8, pos: usize, first: bool, key: string, val: string) -> i64 {
    let p = pos as i64;
    if (first) {
        p = pushSlice(dst, p as usize, " ");
    } else {
        p = pushSlice(dst, p as usize, ", ");
    }
    if (p < 0) { return -1; }
    p = pushSlice(dst, p as usize, key);
    if (p < 0) { return -1; }
    p = pushSlice(dst, p as usize, "=\"");
    if (p < 0) { return -1; }
    let i: usize = 0;
    while (i < val.len) {
        const ch = val[i];
        if (ch == 34 || ch == 92) {
            if ((p as usize) + 2 > dst.len) { return -1; }
            dst[p as usize] = 92;
            dst[(p as usize) + 1] = ch;
            p += 2;
        } else {
            if ((p as usize) >= dst.len) { return -1; }
            dst[p as usize] = ch;
            p += 1;
        }
        i += 1;
    }
    return pushSlice(dst, p as usize, "\"");
}

fn pushTokenField(dst: []u8, pos: usize, first: bool, key: string, val: string) -> i64 {
    let p = pos as i64;
    if (first) {
        p = pushSlice(dst, p as usize, " ");
    } else {
        p = pushSlice(dst, p as usize, ", ");
    }
    if (p < 0) { return -1; }
    p = pushSlice(dst, p as usize, key);
    if (p < 0) { return -1; }
    p = pushSlice(dst, p as usize, "=");
    if (p < 0) { return -1; }
    return pushSlice(dst, p as usize, val);
}

/// Formats a `DigestResponse` into an `Authorization: Digest ...` header value in `out`.
/// Returns bytes written, or `0` if required fields are missing or `out` is too small.
pub fn formatAuthorization(resp: DigestResponse, out: []u8) -> usize {
    if (resp.username.len == 0 || resp.realm.len == 0 || resp.nonce.len == 0 || resp.uri.len == 0 || resp.response.len == 0) {
        return 0;
    }
    let p = pushSlice(out, 0, "Digest");
    if (p < 0) { return 0; }

    p = pushQuotedField(out, p as usize, true, "username", resp.username);
    if (p < 0) { return 0; }
    p = pushQuotedField(out, p as usize, false, "realm", resp.realm);
    if (p < 0) { return 0; }
    p = pushQuotedField(out, p as usize, false, "nonce", resp.nonce);
    if (p < 0) { return 0; }
    p = pushQuotedField(out, p as usize, false, "uri", resp.uri);
    if (p < 0) { return 0; }
    p = pushQuotedField(out, p as usize, false, "response", resp.response);
    if (p < 0) { return 0; }

    if (resp.qop.len > 0) {
        p = pushTokenField(out, p as usize, false, "qop", resp.qop);
        if (p < 0) { return 0; }
    }
    if (resp.nc.len > 0) {
        p = pushTokenField(out, p as usize, false, "nc", resp.nc);
        if (p < 0) { return 0; }
    }
    if (resp.cnonce.len > 0) {
        p = pushQuotedField(out, p as usize, false, "cnonce", resp.cnonce);
        if (p < 0) { return 0; }
    }
    if (resp.opaque_val.len > 0) {
        p = pushQuotedField(out, p as usize, false, "opaque", resp.opaque_val);
        if (p < 0) { return 0; }
    }

    return p as usize;
}

/// Formats a `DigestChallenge` into a `WWW-Authenticate: Digest ...` header value in `out`.
/// Returns bytes written, or `0` if `realm`/`nonce` are empty or `out` is too small.
pub fn formatChallenge(ch: DigestChallenge, out: []u8) -> usize {
    if (ch.realm.len == 0 || ch.nonce.len == 0) {
        return 0;
    }
    let p = pushSlice(out, 0, "Digest");
    if (p < 0) { return 0; }

    p = pushQuotedField(out, p as usize, true, "realm", ch.realm);
    if (p < 0) { return 0; }
    p = pushQuotedField(out, p as usize, false, "nonce", ch.nonce);
    if (p < 0) { return 0; }

    if (ch.opaque_val.len > 0) {
        p = pushQuotedField(out, p as usize, false, "opaque", ch.opaque_val);
        if (p < 0) { return 0; }
    }
    if (ch.qop.len > 0) {
        p = pushQuotedField(out, p as usize, false, "qop", ch.qop);
        if (p < 0) { return 0; }
    }
    if (ch.algorithm.len > 0) {
        p = pushTokenField(out, p as usize, false, "algorithm", ch.algorithm);
        if (p < 0) { return 0; }
    }
    if (ch.stale) {
        p = pushTokenField(out, p as usize, false, "stale", "true");
        if (p < 0) { return 0; }
    }

    return p as usize;
}
