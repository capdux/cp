// Source: Absorbed from /data/dev/bun/src/http/HeaderValueIterator.cp (RFC 8288 Web Linking)
// Pure-CP zero-allocation RFC 8288 HTTP `Link` header parser & serializer
// for pagination (`next`/`prev`/`last`), Early Hints / Preload (`rel=preload; as=style`), and canonical URLs.

pub struct WebLink {
    /// Target URI inside `<...>`.
    uri: []const u8,
    /// `rel` relation type (e.g. `"next"`, `"preload"`, `"canonical"`).
    rel: []const u8,
    /// Optional `as` destination (e.g. `"style"`, `"script"`, `"font"`).
    as_dest: []const u8,
    /// Optional `type` MIME type.
    media_type: []const u8,
    /// Optional `title`.
    title: []const u8,
    /// True if `crossorigin` attribute is present.
    crossorigin: bool,
    /// True if `nopush` attribute is present.
    nopush: bool,
}

/// Zero-allocation iterator over an RFC 8288 HTTP `Link` header.
pub struct LinkHeaderIterator {
    src: []const u8,
    pos: usize,

    pub static fn init(header: []const u8): LinkHeaderIterator {
        return LinkHeaderIterator{ src: header, pos: 0 };
    }

    /// Parse the next `<uri>; param=val; ...` entry from the `Link` header.
    pub fn next(*_): WebLink? {
        while (_.pos < _.src.len) {
            skipOwsAndCommas(_.src, &_.pos);
            if (_.pos >= _.src.len) return null;
            if (_.src[_.pos] != '<') {
                // Skip malformed segment up to next top-level comma
                skipToNextComma(_.src, &_.pos);
                continue;
            }
            _.pos += 1; // skip '<'
            const uri_start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != '>') : (_.pos += 1) {}
            if (_.pos >= _.src.len) return null;
            const uri = trimOws(_.src[uri_start.._.pos]);
            _.pos += 1; // skip '>'

            let link = WebLink{
                uri: uri,
                rel: "",
                as_dest: "",
                media_type: "",
                title: "",
                crossorigin: false,
                nopush: false,
            };

            // Parse `; key=val` parameters until top-level ',' or EOF
            while (_.pos < _.src.len) {
                skipOws(_.src, &_.pos);
                if (_.pos >= _.src.len) break;
                if (_.src[_.pos] == ',') {
                    _.pos += 1;
                    break;
                }
                if (_.src[_.pos] != ';') {
                    skipToNextComma(_.src, &_.pos);
                    break;
                }
                _.pos += 1; // skip ';'
                skipOws(_.src, &_.pos);

                const k_start = _.pos;
                while (_.pos < _.src.len && _.src[_.pos] != '=' && _.src[_.pos] != ';' && _.src[_.pos] != ',') : (_.pos += 1) {}
                const key = trimOws(_.src[k_start.._.pos]);

                let val: []const u8 = "";
                if (_.pos < _.src.len && _.src[_.pos] == '=') {
                    _.pos += 1;
                    skipOws(_.src, &_.pos);
                    if (_.pos < _.src.len && _.src[_.pos] == '"') {
                        _.pos += 1;
                        const v_start = _.pos;
                        while (_.pos < _.src.len && _.src[_.pos] != '"') : (_.pos += 1) {
                            if (_.src[_.pos] == '\\' && _.pos + 1 < _.src.len) _.pos += 1;
                        }
                        val = _.src[v_start.._.pos];
                        if (_.pos < _.src.len && _.src[_.pos] == '"') _.pos += 1;
                    } else {
                        const v_start = _.pos;
                        while (_.pos < _.src.len && _.src[_.pos] != ';' && _.src[_.pos] != ',') : (_.pos += 1) {}
                        val = trimOws(_.src[v_start.._.pos]);
                    }
                }

                if (eqIgnoreCase(key, "rel")) {
                    link.rel = val;
                } else if (eqIgnoreCase(key, "as")) {
                    link.as_dest = val;
                } else if (eqIgnoreCase(key, "type")) {
                    link.media_type = val;
                } else if (eqIgnoreCase(key, "title")) {
                    link.title = val;
                } else if (eqIgnoreCase(key, "crossorigin")) {
                    link.crossorigin = true;
                } else if (eqIgnoreCase(key, "nopush")) {
                    link.nopush = true;
                }
            }

            if (link.uri.len > 0) return link;
        }
        return null;
    }
}

/// Find the first link matching relation `rel` (case-insensitive, e.g. `"next"`).
pub fn findLinkByRel(header: []const u8, rel: []const u8): WebLink? {
    let it = LinkHeaderIterator.init(header);
    while (link := it.next()) {
        if (eqIgnoreCase(link.rel, rel)) return link;
    }
    return null;
}

/// Format a `WebLink` into RFC 8288 header syntax (`<uri>; rel="next"`) into `out`.
pub fn formatWebLink(link: WebLink, out: []u8): ([]const u8)? {
    if (link.uri.len == 0) return null;
    let p: usize = 0;
    if (!appendStr(out, &p, "<")) return null;
    if (!appendStr(out, &p, link.uri)) return null;
    if (!appendStr(out, &p, ">")) return null;

    if (link.rel.len > 0) {
        if (!appendStr(out, &p, "; rel=\"")) return null;
        if (!appendStr(out, &p, link.rel)) return null;
        if (!appendStr(out, &p, "\"")) return null;
    }
    if (link.as_dest.len > 0) {
        if (!appendStr(out, &p, "; as=")) return null;
        if (!appendStr(out, &p, link.as_dest)) return null;
    }
    if (link.media_type.len > 0) {
        if (!appendStr(out, &p, "; type=\"")) return null;
        if (!appendStr(out, &p, link.media_type)) return null;
        if (!appendStr(out, &p, "\"")) return null;
    }
    if (link.title.len > 0) {
        if (!appendStr(out, &p, "; title=\"")) return null;
        if (!appendStr(out, &p, link.title)) return null;
        if (!appendStr(out, &p, "\"")) return null;
    }
    if (link.crossorigin) {
        if (!appendStr(out, &p, "; crossorigin")) return null;
    }
    if (link.nopush) {
        if (!appendStr(out, &p, "; nopush")) return null;
    }
    return out[0..p];
}

// ---------- Helpers ----------

fn isOws(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn skipOws(s: []const u8, pos: *usize) {
    while (pos.* < s.len && isOws(s[pos.*])) : (pos.* += 1) {}
}

fn skipOwsAndCommas(s: []const u8, pos: *usize) {
    while (pos.* < s.len && (isOws(s[pos.*]) || s[pos.*] == ',')) : (pos.* += 1) {}
}

fn skipToNextComma(s: []const u8, pos: *usize) {
    let in_quote = false;
    while (pos.* < s.len) : (pos.* += 1) {
        const c = s[pos.*];
        if (in_quote) {
            if (c == '\\' && pos.* + 1 < s.len) {
                pos.* += 1;
            } else if (c == '"') {
                in_quote = false;
            }
        } else {
            if (c == '"') {
                in_quote = true;
            } else if (c == ',') {
                pos.* += 1;
                return;
            }
        }
    }
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isOws(s[start])) : (start += 1) {}
    let end = s.len;
    while (end > start && isOws(s[end - 1])) : (end -= 1) {}
    return s[start..end];
}

fn toLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLower(a[i]) != toLower(b[i])) return false;
    }
    return true;
}

fn appendStr(out: []u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out[pos.*] = s[i];
        pos.* += 1;
    }
    return true;
}
