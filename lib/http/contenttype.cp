// Source: Absorbed from /data/dev/bun/src/http/MimeType.zig | Synced: 2026-11-21T16:45:50Z (RFC 9110 §8.3.1 Content-Type Header Parser)
// Pure-CP zero-allocation RFC 9110 Content-Type parser, parameter extractor (charset/boundary), and media-type classifier.

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

fn endsWithIgnoreCase(s: []const u8, suffix: []const u8): bool {
    if (s.len < suffix.len) return false;
    return eqIgnoreCase(s[s.len - suffix.len..], suffix);
}

fn isTokenChar(b: u8): bool {
    if ((b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z') || (b >= '0' && b <= '9')) return true;
    return b == '!' || b == '#' || b == '$' || b == '%' || b == '&' || b == '\'' ||
        b == '*' || b == '+' || b == '-' || b == '.' || b == '^' || b == '_' ||
        b == '`' || b == '|' || b == '~';
}

fn isValidToken(s: []const u8): bool {
    if (s.len == 0) return false;
    let i: usize = 0;
    while (i < s.len) {
        if (!isTokenChar(s[i])) return false;
        i += 1;
    }
    return true;
}

export struct ContentTypeParam {
    name: []const u8,
    value: []const u8, // unquoted if wrapped in "..."
}

export struct ContentTypeParamIter {
    src: []const u8,
    pos: usize,

    pub fn init(params_raw: []const u8): ContentTypeParamIter {
        return ContentTypeParamIter { src: params_raw, pos: 0 };
    }

    pub fn next(*_): ContentTypeParam? {
        while (_.pos < _.src.len) {
            while (_.pos < _.src.len && (_.src[_.pos] == ';' || isWs(_.src[_.pos]))) {
                _.pos += 1;
            }
            if (_.pos >= _.src.len) return null;

            const start = _.pos;
            let in_quote = false;
            let escaped = false;
            while (_.pos < _.src.len) {
                const c = _.src[_.pos];
                if (in_quote) {
                    if (escaped) {
                        escaped = false;
                    } else if (c == '\\') {
                        escaped = true;
                    } else if (c == '"') {
                        in_quote = false;
                    }
                } else {
                    if (c == '"') {
                        in_quote = true;
                    } else if (c == ';') {
                        break;
                    }
                }
                _.pos += 1;
            }
            const seg = trimWs(_.src[start.._.pos]);
            if (seg.len == 0) continue;

            let eq_pos: usize? = null;
            let k: usize = 0;
            while (k < seg.len) {
                if (seg[k] == '=') {
                    eq_pos = k;
                    break;
                }
                k += 1;
            }
            if (eq_pos == null) continue;
            const ep = eq_pos.?;
            const name = trimWs(seg[0..ep]);
            if (!isValidToken(name)) continue;
            let val = trimWs(seg[ep + 1..]);
            if (val.len >= 2 && val[0] == '"' && val[val.len - 1] == '"') {
                val = val[1..val.len - 1];
            }
            return ContentTypeParam { name: name, value: val };
        }
        return null;
    }
}

export struct ContentType {
    type_name: []const u8,
    subtype: []const u8,
    essence: []const u8, // "type/subtype" without parameters
    params_raw: []const u8,

    /// Parse an RFC 9110 `Content-Type` header (e.g. `"application/json; charset=utf-8"`).
    pub fn parse(header: []const u8): ContentType? {
        const s = trimWs(header);
        if (s.len == 0) return null;

        let semi_pos = s.len;
        let i: usize = 0;
        while (i < s.len) {
            if (s[i] == ';') {
                semi_pos = i;
                break;
            }
            i += 1;
        }
        const essence = trimWs(s[0..semi_pos]);
        let params: []const u8 = "";
        if (semi_pos < s.len) {
            params = s[semi_pos + 1..];
        }

        let slash_pos: usize? = null;
        let k: usize = 0;
        while (k < essence.len) {
            if (essence[k] == '/') {
                if (slash_pos != null) return null;
                slash_pos = k;
            }
            k += 1;
        }
        const sp = slash_pos ?? return null;
        const tn = trimWs(essence[0..sp]);
        const st = trimWs(essence[sp + 1..]);
        if (!isValidToken(tn) || !isValidToken(st)) return null;

        return ContentType {
            type_name: tn,
            subtype: st,
            essence: essence,
            params_raw: params,
        };
    }

    /// Lookup a parameter by name (case-insensitive; first occurrence wins per WHATWG MIME Sniffing).
    pub fn param(name: []const u8): ([]const u8)? {
        let it = ContentTypeParamIter.init(_.params_raw);
        while (p := it.next()) {
            if (eqIgnoreCase(p.name, name)) return p.value;
        }
        return null;
    }

    pub fn charset(): ([]const u8)? {
        return _.param("charset");
    }

    pub fn boundary(): ([]const u8)? {
        return _.param("boundary");
    }

    pub fn isJson(): bool {
        if (eqIgnoreCase(_.type_name, "application") && eqIgnoreCase(_.subtype, "json")) return true;
        if (eqIgnoreCase(_.type_name, "text") && eqIgnoreCase(_.subtype, "json")) return true;
        return endsWithIgnoreCase(_.subtype, "+json");
    }

    pub fn isXml(): bool {
        if ((eqIgnoreCase(_.type_name, "application") || eqIgnoreCase(_.type_name, "text")) &&
            eqIgnoreCase(_.subtype, "xml"))
        {
            return true;
        }
        return endsWithIgnoreCase(_.subtype, "+xml");
    }

    pub fn isText(): bool {
        return eqIgnoreCase(_.type_name, "text");
    }

    pub fn isMultipart(): bool {
        return eqIgnoreCase(_.type_name, "multipart");
    }

    pub fn isFormUrlEncoded(): bool {
        return eqIgnoreCase(_.type_name, "application") &&
            eqIgnoreCase(_.subtype, "x-www-form-urlencoded");
    }

    pub fn isEventStream(): bool {
        return eqIgnoreCase(_.type_name, "text") && eqIgnoreCase(_.subtype, "event-stream");
    }
}
