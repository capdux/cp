// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T16:05:00Z (W3C Content-Security-Policy Level 3 Parser & Builder)
// Pure-CP zero-allocation W3C CSP Level 3 directive parser, source lookup, fallback resolution, and builder.

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

fn startsWithIgnoreCase(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    return eqIgnoreCase(s[0..prefix.len], prefix);
}

export struct CspDirective {
    name: []const u8,
    value: []const u8,

    /// Check if this directive's whitespace-separated source list contains `token` (case-insensitive).
    pub fn hasToken(token: []const u8): bool {
        let it = CspSourceIter.init(_.value);
        while (src := it.next()) {
            if (eqIgnoreCase(src, token)) return true;
        }
        return false;
    }

    pub fn allowsNone(): bool {
        return _.hasToken("'none'");
    }

    pub fn allowsSelf(): bool {
        return _.hasToken("'self'");
    }

    pub fn allowsUnsafeInline(): bool {
        return _.hasToken("'unsafe-inline'");
    }

    pub fn allowsUnsafeEval(): bool {
        return _.hasToken("'unsafe-eval'");
    }

    /// Check if this directive authorizes a specific nonce value (without `'nonce-...'` wrapper).
    pub fn allowsNonce(nonce: []const u8): bool {
        if (nonce.len == 0) return false;
        let it = CspSourceIter.init(_.value);
        while (src := it.next()) {
            // Format: 'nonce-<value>'
            if (src.len == nonce.len + 8 && src[0] == '\'' && src[src.len - 1] == '\'') {
                if (startsWithIgnoreCase(src[1..], "nonce-")) {
                    // Nonce comparison is case-sensitive per W3C CSP3
                    const inner = src[7..src.len - 1];
                    if (inner.len == nonce.len) {
                        let same = true;
                        let i: usize = 0;
                        while (i < nonce.len) {
                            if (inner[i] != nonce[i]) {
                                same = false;
                                break;
                            }
                            i += 1;
                        }
                        if (same) return true;
                    }
                }
            }
        }
        return false;
    }
}

export struct CspSourceIter {
    src: []const u8,
    pos: usize,

    pub fn init(value: []const u8): CspSourceIter {
        return CspSourceIter { src: value, pos: 0 };
    }

    pub fn next(*_): ([]const u8)? {
        while (_.pos < _.src.len && isWs(_.src[_.pos])) {
            _.pos += 1;
        }
        if (_.pos >= _.src.len) return null;
        const start = _.pos;
        while (_.pos < _.src.len && !isWs(_.src[_.pos])) {
            _.pos += 1;
        }
        return _.src[start.._.pos];
    }
}

export struct CspDirectiveIter {
    src: []const u8,
    pos: usize,

    pub fn init(policy: []const u8): CspDirectiveIter {
        return CspDirectiveIter { src: policy, pos: 0 };
    }

    pub fn next(*_): CspDirective? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != ';') {
                _.pos += 1;
            }
            const end = _.pos;
            if (_.pos < _.src.len && _.src[_.pos] == ';') {
                _.pos += 1;
            }
            const seg = trimWs(_.src[start..end]);
            if (seg.len == 0) continue;

            let sp: usize = 0;
            while (sp < seg.len && !isWs(seg[sp])) {
                sp += 1;
            }
            const name = seg[0..sp];
            let val: []const u8 = "";
            if (sp < seg.len) {
                val = trimWs(seg[sp..]);
            }
            return CspDirective { name: name, value: val };
        }
        return null;
    }
}

/// Find the first occurrence of `directive_name` in `policy` (per W3C CSP3 §2.2.1, duplicates are ignored).
export fn findDirective(policy: []const u8, directive_name: []const u8): CspDirective? {
    let it = CspDirectiveIter.init(policy);
    while (d := it.next()) {
        if (eqIgnoreCase(d.name, directive_name)) return d;
    }
    return null;
}

/// Find the effective directive for a fetch directive (falls back to `default-src` if absent).
export fn effectiveFetchDirective(policy: []const u8, directive_name: []const u8): CspDirective? {
    if (d := findDirective(policy, directive_name)) return d;
    return findDirective(policy, "default-src");
}

/// Append a directive (`name value`) to a CSP header buffer at `pos`, inserting `; ` if `pos > 0`.
export fn appendCspDirective(out: []u8, pos: *usize, name: []const u8, value: []const u8): bool {
    const clean_name = trimWs(name);
    if (clean_name.len == 0) return false;
    const clean_val = trimWs(value);

    if (pos.* > 0) {
        if (pos.* + 2 > out.len) return false;
        out[pos.*] = ';';
        out[pos.* + 1] = ' ';
        pos.* += 2;
    }
    if (pos.* + clean_name.len > out.len) return false;
    let i: usize = 0;
    while (i < clean_name.len) {
        out[pos.* + i] = toLowerAscii(clean_name[i]);
        i += 1;
    }
    pos.* += clean_name.len;

    if (clean_val.len > 0) {
        if (pos.* + 1 + clean_val.len > out.len) return false;
        out[pos.*] = ' ';
        pos.* += 1;
        let j: usize = 0;
        while (j < clean_val.len) {
            out[pos.* + j] = clean_val[j];
            j += 1;
        }
        pos.* += clean_val.len;
    }
    return true;
}
