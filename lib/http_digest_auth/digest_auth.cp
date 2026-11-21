// Source: Absorbed from /data/dev/bun/src/http/http.cp (RFC 7616 HTTP Digest Auth Header Parser)

fn isOws(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    let end = s.len;
    while (start < end && isOws(s[start])) { start += 1; }
    while (end > start && isOws(s[end - 1])) { end -= 1; }
    return s[start..end];
}

fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        let ca = a[i];
        let cb = b[i];
        if (ca >= 'A' && ca <= 'Z') { ca += 32; }
        if (cb >= 'A' && cb <= 'Z') { cb += 32; }
        if (ca != cb) { return false; }
        i += 1;
    }
    return true;
}

fn stripDigestScheme(header: []const u8): ([]const u8)? {
    const s = trimOws(header);
    if (s.len < 7) { return null; }
    if (!eqlIgnoreCaseAscii(s[0..6], "digest") || !isOws(s[6])) {
        return null;
    }
    return trimOws(s[6..s.len]);
}

struct AuthParam {
    key: []const u8,
    val: []const u8,
}

struct AuthParamIter {
    src: []const u8,
    pos: usize = 0,
    had_error: bool = false,

    fn next(*_): AuthParam? {
        while (_.pos < _.src.len) {
            while (_.pos < _.src.len && (isOws(_.src[_.pos]) || _.src[_.pos] == ',')) {
                _.pos += 1;
            }
            if (_.pos >= _.src.len) { return null; }

            const k_start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != '=' && _.src[_.pos] != ',') {
                _.pos += 1;
            }
            if (_.pos >= _.src.len || _.src[_.pos] != '=') {
                _.had_error = true;
                _.pos = _.src.len;
                return null;
            }
            const key = trimOws(_.src[k_start.._.pos]);
            _.pos += 1; // 跳过 '='
            while (_.pos < _.src.len && isOws(_.src[_.pos])) { _.pos += 1; }

            if (key.len == 0 || _.pos >= _.src.len) {
                _.had_error = true;
                _.pos = _.src.len;
                return null;
            }

            if (_.src[_.pos] == '"') {
                _.pos += 1;
                const v_start = _.pos;
                let closed = false;
                while (_.pos < _.src.len) {
                    if (_.src[_.pos] == '\\' && _.pos + 1 < _.src.len) {
                        _.pos += 2;
                        continue;
                    }
                    if (_.src[_.pos] == '"') {
                        closed = true;
                        break;
                    }
                    _.pos += 1;
                }
                if (!closed) {
                    _.had_error = true;
                    _.pos = _.src.len;
                    return null;
                }
                const val = _.src[v_start.._.pos];
                _.pos += 1; // 跳过闭合 '"'
                return AuthParam{ key: key, val: val };
            }

            const v_start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != ',') {
                _.pos += 1;
            }
            const val = trimOws(_.src[v_start.._.pos]);
            return AuthParam{ key: key, val: val };
        }
        return null;
    }
}

/// RFC 7616 `WWW-Authenticate: Digest ...` 服务端质询头解析结果（零堆分配）
pub struct DigestChallenge {
    realm: []const u8 = "",
    nonce: []const u8 = "",
    opaque_val: ([]const u8)? = null,
    algorithm: ([]const u8)? = null,
    qop: ([]const u8)? = null,
    stale: bool = false,
    userhash: bool = false,

    static fn parse(header: []const u8): DigestChallenge? {
        const body = stripDigestScheme(header) ?? return null;
        let it = AuthParamIter{ src: body };
        let res = DigestChallenge{};
        let has_realm = false;
        let has_nonce = false;

        while (p := it.next()) {
            if (eqlIgnoreCaseAscii(p.key, "realm")) {
                res.realm = p.val;
                has_realm = true;
            } else if (eqlIgnoreCaseAscii(p.key, "nonce")) {
                res.nonce = p.val;
                has_nonce = true;
            } else if (eqlIgnoreCaseAscii(p.key, "opaque")) {
                res.opaque_val = p.val;
            } else if (eqlIgnoreCaseAscii(p.key, "algorithm")) {
                res.algorithm = p.val;
            } else if (eqlIgnoreCaseAscii(p.key, "qop")) {
                res.qop = p.val;
            } else if (eqlIgnoreCaseAscii(p.key, "stale")) {
                res.stale = eqlIgnoreCaseAscii(p.val, "true");
            } else if (eqlIgnoreCaseAscii(p.key, "userhash")) {
                res.userhash = eqlIgnoreCaseAscii(p.val, "true");
            }
        }

        if (it.had_error || !has_realm || !has_nonce || res.realm.len == 0 || res.nonce.len == 0) {
            return null;
        }
        return res;
    }
}

/// RFC 7616 `Authorization: Digest ...` 客户端凭证头解析结果（零堆分配）
pub struct DigestCredentials {
    username: []const u8 = "",
    realm: []const u8 = "",
    nonce: []const u8 = "",
    uri: []const u8 = "",
    response: []const u8 = "",
    algorithm: ([]const u8)? = null,
    cnonce: ([]const u8)? = null,
    nc: ([]const u8)? = null,
    qop: ([]const u8)? = null,
    opaque_val: ([]const u8)? = null,

    static fn parse(header: []const u8): DigestCredentials? {
        const body = stripDigestScheme(header) ?? return null;
        let it = AuthParamIter{ src: body };
        let res = DigestCredentials{};

        while (p := it.next()) {
            if (eqlIgnoreCaseAscii(p.key, "username")) {
                res.username = p.val;
            } else if (eqlIgnoreCaseAscii(p.key, "realm")) {
                res.realm = p.val;
            } else if (eqlIgnoreCaseAscii(p.key, "nonce")) {
                res.nonce = p.val;
            } else if (eqlIgnoreCaseAscii(p.key, "uri")) {
                res.uri = p.val;
            } else if (eqlIgnoreCaseAscii(p.key, "response")) {
                res.response = p.val;
            } else if (eqlIgnoreCaseAscii(p.key, "algorithm")) {
                res.algorithm = p.val;
            } else if (eqlIgnoreCaseAscii(p.key, "cnonce")) {
                res.cnonce = p.val;
            } else if (eqlIgnoreCaseAscii(p.key, "nc")) {
                res.nc = p.val;
            } else if (eqlIgnoreCaseAscii(p.key, "qop")) {
                res.qop = p.val;
            } else if (eqlIgnoreCaseAscii(p.key, "opaque")) {
                res.opaque_val = p.val;
            }
        }

        if (it.had_error ||
            res.username.len == 0 ||
            res.realm.len == 0 ||
            res.nonce.len == 0 ||
            res.uri.len == 0 ||
            res.response.len == 0)
        {
            return null;
        }
        return res;
    }
}

struct OutWriter {
    buf: []u8,
    pos: usize = 0,
    overflow: bool = false,

    fn writeByte(*_, c: u8) {
        if (_.pos < _.buf.len) {
            _.buf[_.pos] = c;
            _.pos += 1;
        } else {
            _.overflow = true;
        }
    }

    fn writeStr(*_, s: []const u8) {
        let i: usize = 0;
        while (i < s.len) {
            _.writeByte(s[i]);
            i += 1;
        }
    }

    fn writeQuoted(*_, s: []const u8) {
        _.writeByte('"');
        let i: usize = 0;
        while (i < s.len) {
            if (s[i] == '"' || s[i] == '\\') {
                _.writeByte('\\');
            }
            _.writeByte(s[i]);
            i += 1;
        }
        _.writeByte('"');
    }

    fn finish(): ([]const u8)? {
        if (_.overflow) { return null; }
        return _.buf[0.._.pos];
    }
}

/// 规范化构造 RFC 7616 `Authorization: Digest ...` 头部值写入 `out`
pub fn formatDigestHeader(creds: DigestCredentials, out: []u8): ([]const u8)? {
    if (creds.username.len == 0 ||
        creds.realm.len == 0 ||
        creds.nonce.len == 0 ||
        creds.uri.len == 0 ||
        creds.response.len == 0)
    {
        return null;
    }

    let w = OutWriter{ buf: out };
    w.writeStr("Digest username=");
    w.writeQuoted(creds.username);
    w.writeStr(", realm=");
    w.writeQuoted(creds.realm);
    w.writeStr(", nonce=");
    w.writeQuoted(creds.nonce);
    w.writeStr(", uri=");
    w.writeQuoted(creds.uri);
    w.writeStr(", response=");
    w.writeQuoted(creds.response);

    if (alg := creds.algorithm) {
        w.writeStr(", algorithm=");
        w.writeStr(alg);
    }
    if (cn := creds.cnonce) {
        w.writeStr(", cnonce=");
        w.writeQuoted(cn);
    }
    if (op := creds.opaque_val) {
        w.writeStr(", opaque=");
        w.writeQuoted(op);
    }
    if (q := creds.qop) {
        w.writeStr(", qop=");
        w.writeStr(q);
    }
    if (n := creds.nc) {
        w.writeStr(", nc=");
        w.writeStr(n);
    }

    return w.finish();
}
