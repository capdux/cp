// Source: Absorbed from /data/dev/bun/src/http/HeaderValueIterator.cp (RFC 6266 / RFC 5987 Content-Disposition)
// Pure-CP zero-allocation HTTP `Content-Disposition` parser, RFC 5987 `filename*`
// UTF-8 percent-decoder, path-traversal filename sanitizer, and header formatter.

pub enum DispositionType: u8 {
    unknown = 0,
    inline_disp = 1,
    attachment = 2,
    form_data = 3,

    pub static fn parse(s: []const u8): DispositionType {
        const t = trimOws(s);
        if (eqIgnoreCase(t, "inline")) return .inline_disp;
        if (eqIgnoreCase(t, "attachment")) return .attachment;
        if (eqIgnoreCase(t, "form-data")) return .form_data;
        return .unknown;
    }

    pub fn asStr(): []const u8 {
        return switch (_) {
            .unknown => "",
            .inline_disp => "inline",
            .attachment => "attachment",
            .form_data => "form-data",
        };
    }
}

pub struct ContentDisposition {
    kind: DispositionType,
    /// `name` parameter (used in `multipart/form-data`).
    name: []const u8,
    /// ASCII `filename` parameter (unquoted).
    filename: []const u8,
    /// Raw RFC 5987 `filename*` value (e.g. `UTF-8''report%202026.pdf`).
    filename_star: []const u8,

    /// Parse an HTTP `Content-Disposition` header per RFC 6266 / RFC 7578.
    pub static fn parse(header: []const u8): ContentDisposition? {
        const s = trimOws(header);
        if (s.len == 0) return null;

        let pos: usize = 0;
        const first = nextSemi(s, &pos) ?? return null;
        const kind = DispositionType.parse(first);
        if (kind == .unknown) return null;

        let res = ContentDisposition{
            kind: kind,
            name: "",
            filename: "",
            filename_star: "",
        };

        while (seg := nextSemi(s, &pos)) {
            const p = trimOws(seg);
            if (p.len == 0) continue;
            if (eq := findChar(p, '=')) {
                const k = trimOws(p[0..eq]);
                let v = trimOws(p[eq + 1 ..]);
                if (v.len >= 2 && v[0] == '"' && v[v.len - 1] == '"') {
                    v = v[1 .. v.len - 1];
                }
                if (eqIgnoreCase(k, "name")) {
                    res.name = v;
                } else if (eqIgnoreCase(k, "filename")) {
                    res.filename = v;
                } else if (eqIgnoreCase(k, "filename*")) {
                    res.filename_star = v;
                }
            }
        }
        return res;
    }

    /// Extract the preferred decoded filename into `out`:
    /// RFC 6266 §4.3 prioritizes `filename*` (RFC 5987 `charset'lang'percent-encoded`) over `filename`.
    pub fn decodedFilename(out: []u8): ([]const u8)? {
        if (_.filename_star.len > 0) {
            if (decoded := decodeRfc5987(_.filename_star, out)) return decoded;
        }
        if (_.filename.len == 0) return null;
        if (_.filename.len > out.len) return null;
        let i: usize = 0;
        while (i < _.filename.len) : (i += 1) {
            out[i] = _.filename[i];
        }
        return out[0.._.filename.len];
    }
}

/// Decode an RFC 5987 extended parameter value (`UTF-8'en'hello%20world.txt`) into `out`.
pub fn decodeRfc5987(ext_val: []const u8, out: []u8): ([]const u8)? {
    const q1 = findChar(ext_val, '\'') ?? return null;
    const rest1 = ext_val[q1 + 1 ..];
    const q2 = findChar(rest1, '\'') ?? return null;
    const encoded = rest1[q2 + 1 ..];

    let p: usize = 0;
    let i: usize = 0;
    while (i < encoded.len) {
        if (encoded[i] == '%' && i + 2 < encoded.len) {
            const hi = hexVal(encoded[i + 1]);
            const lo = hexVal(encoded[i + 2]);
            if (hi != null && lo != null) {
                if (p >= out.len) return null;
                out[p] = (hi.? << 4) | lo.?;
                p += 1;
                i += 3;
                continue;
            }
        }
        if (p >= out.len) return null;
        out[p] = encoded[i];
        p += 1;
        i += 1;
    }
    return out[0..p];
}

/// Sanitize an untrusted download filename to prevent path traversal (`/` and `\` stripped
/// to basename, control chars replaced with `_`, `.` and `..` rejected).
pub fn sanitizeFilename(raw: []const u8, out: []u8): ([]const u8)? {
    // Take only basename after last '/' or '\\'
    let start: usize = 0;
    let i: usize = 0;
    while (i < raw.len) : (i += 1) {
        if (raw[i] == '/' || raw[i] == '\\') start = i + 1;
    }
    const base = trimOws(raw[start..]);
    if (base.len == 0 || strEq(base, ".") || strEq(base, "..")) return null;
    if (base.len > out.len) return null;

    let p: usize = 0;
    let k: usize = 0;
    while (k < base.len) : (k += 1) {
        const c = base[k];
        if (c < 0x20 || c == 0x7F || c == '"' || c == ':') {
            out[p] = '_';
        } else {
            out[p] = c;
        }
        p += 1;
    }
    return out[0..p];
}

/// Format an `attachment; filename="..."` header (with RFC 5987 `filename*` if non-ASCII) into `out`.
pub fn formatAttachment(filename: []const u8, out: []u8): ([]const u8)? {
    if (filename.len == 0) return null;
    let p: usize = 0;
    if (!appendStr(out, &p, "attachment; filename=\"")) return null;
    let has_non_ascii = false;
    let i: usize = 0;
    while (i < filename.len) : (i += 1) {
        const c = filename[i];
        if (c >= 0x80) {
            has_non_ascii = true;
            if (!appendByte(out, &p, '_')) return null;
        } else if (c == '"' || c == '\\') {
            if (!appendByte(out, &p, '_')) return null;
        } else {
            if (!appendByte(out, &p, c)) return null;
        }
    }
    if (!appendByte(out, &p, '"')) return null;

    if (has_non_ascii) {
        if (!appendStr(out, &p, "; filename*=UTF-8''")) return null;
        i = 0;
        while (i < filename.len) : (i += 1) {
            const c = filename[i];
            if (isRfc5987AttrChar(c)) {
                if (!appendByte(out, &p, c)) return null;
            } else {
                if (!appendByte(out, &p, '%')) return null;
                if (!appendByte(out, &p, hexUpper((c >> 4) & 0xF))) return null;
                if (!appendByte(out, &p, hexUpper(c & 0xF))) return null;
            }
        }
    }
    return out[0..p];
}

// ---------- Helpers ----------

fn isRfc5987AttrChar(c: u8): bool {
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) return true;
    return c == '!' || c == '#' || c == '$' || c == '&' || c == '+' || c == '-' || c == '.' ||
        c == '^' || c == '_' || c == '`' || c == '|' || c == '~';
}

fn nextSemi(s: []const u8, pos: *usize): ([]const u8)? {
    if (pos.* >= s.len) return null;
    const start = pos.*;
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
            } else if (c == ';') {
                const seg = s[start..pos.*];
                pos.* += 1;
                return seg;
            }
        }
    }
    return s[start..pos.*];
}

fn findChar(s: []const u8, ch: u8): usize? {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == ch) return i;
    }
    return null;
}

fn hexVal(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

fn hexUpper(n: u8): u8 {
    return if (n < 10) '0' + n else 'A' + (n - 10);
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t')) : (start += 1) {}
    let end = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) : (end -= 1) {}
    return s[start..end];
}

fn toLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLower(a[i]) != toLower(b[i])) return false;
    }
    return true;
}

fn appendByte(out: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out.len) return false;
    out[pos.*] = b;
    pos.* += 1;
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
