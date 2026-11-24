// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-23T01:59:15Z (RFC 9110 Vary Header Parser & Cache-Key Normalizer)
// Pure-CP zero-allocation RFC 9110 §12.5.5 Vary header iterator, membership check, and canonical merger.

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

fn isValidTokenChar(b: u8): bool {
    if ((b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z') || (b >= '0' && b <= '9')) return true;
    return b == '!' || b == '#' || b == '$' || b == '%' || b == '&' || b == '\'' ||
        b == '*' || b == '+' || b == '-' || b == '.' || b == '^' || b == '_' ||
        b == '`' || b == '|' || b == '~';
}

fn isValidFieldName(name: []const u8): bool {
    if (name.len == 0) return false;
    let i: usize = 0;
    while (i < name.len) {
        if (!isValidTokenChar(name[i])) return false;
        i += 1;
    }
    return true;
}

export struct VaryIter {
    src: []const u8,
    pos: usize,

    pub static fn init(header: []const u8): VaryIter {
        return VaryIter { src: header, pos: 0 };
    }

    pub fn next(*_): ([]const u8)? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != ',') {
                _.pos += 1;
            }
            const end = _.pos;
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
            }
            const tok = trimWs(_.src[start..end]);
            if (tok.len > 0 && isValidFieldName(tok)) {
                return tok;
            }
        }
        return null;
    }
}

/// Returns true if the `Vary` header contains `*` (uncacheable / matches nothing per RFC 9110 §12.5.5).
export fn isVaryWildcard(header: []const u8): bool {
    let it = VaryIter.init(header);
    while (name := it.next()) {
        if (name.len == 1 && name[0] == '*') return true;
    }
    return false;
}

/// Check if `field_name` is listed in `Vary` header (case-insensitive).
/// Note: returns false for `*` unless `field_name` is literally `"*"`; use `isVaryWildcard` separately.
export fn varyContains(header: []const u8, field_name: []const u8): bool {
    if (field_name.len == 0) return false;
    let it = VaryIter.init(header);
    while (name := it.next()) {
        if (eqIgnoreCase(name, field_name)) return true;
    }
    return false;
}

/// Append a field name to an existing `Vary` header value with case-insensitive deduplication.
/// If either existing or `field_name` is `*`, outputs `"*"`.
export fn appendVary(existing: []const u8, field_name: []const u8, out: []u8): ([]const u8)? {
    const clean_add = trimWs(field_name);
    if (!isValidFieldName(clean_add)) return null;

    if (isVaryWildcard(existing) || (clean_add.len == 1 && clean_add[0] == '*')) {
        if (out.len < 1) return null;
        out[0] = '*';
        return out[0..1];
    }

    let pos: usize = 0;
    let first = true;
    let already = false;

    let it = VaryIter.init(existing);
    while (name := it.next()) {
        if (eqIgnoreCase(name, clean_add)) {
            already = true;
        }
        if (!first) {
            if (pos + 2 > out.len) return null;
            out[pos] = ',';
            out[pos + 1] = ' ';
            pos += 2;
        }
        first = false;
        if (pos + name.len > out.len) return null;
        let i: usize = 0;
        while (i < name.len) {
            out[pos + i] = name[i];
            i += 1;
        }
        pos += name.len;
    }

    if (!already) {
        if (!first) {
            if (pos + 2 > out.len) return null;
            out[pos] = ',';
            out[pos + 1] = ' ';
            pos += 2;
        }
        if (pos + clean_add.len > out.len) return null;
        let i: usize = 0;
        while (i < clean_add.len) {
            out[pos + i] = clean_add[i];
            i += 1;
        }
        pos += clean_add.len;
    }

    return out[0..pos];
}

/// Merge two `Vary` headers into a single deduplicated lowercase canonical header value.
/// If either contains `*`, returns `"*"`.
export fn mergeVary(a: []const u8, b: []const u8, out: []u8): ([]const u8)? {
    if (isVaryWildcard(a) || isVaryWildcard(b)) {
        if (out.len < 1) return null;
        out[0] = '*';
        return out[0..1];
    }

    let pos: usize = 0;
    let first = true;

    let it_a = VaryIter.init(a);
    while (name := it_a.next()) {
        if (varyContains(out[0..pos], name)) continue;
        if (!appendLower(out, &pos, &first, name)) return null;
    }

    let it_b = VaryIter.init(b);
    while (name := it_b.next()) {
        if (varyContains(out[0..pos], name)) continue;
        if (!appendLower(out, &pos, &first, name)) return null;
    }

    return out[0..pos];
}

fn appendLower(out: []u8, pos: *usize, first: *bool, name: []const u8): bool {
    if (!first.*) {
        if (pos.* + 2 > out.len) return false;
        out[pos.*] = ',';
        out[pos.* + 1] = ' ';
        pos.* += 2;
    }
    first.* = false;
    if (pos.* + name.len > out.len) return false;
    let i: usize = 0;
    while (i < name.len) {
        out[pos.* + i] = toLowerAscii(name[i]);
        i += 1;
    }
    pos.* += name.len;
    return true;
}
