// @cap/type-is — Zero-allocation HTTP Content-Type MIME matcher for CP
// Clean-room MIT implementation inspired by npm `type-is` (227M/wk).

fn isSpace(c: u8) -> bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn toLowerAscii(c: u8) -> u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + (32 as u8);
    }
    return c;
}

fn trimSlice(s: []const u8) -> []const u8 {
    let start: usize = 0;
    while (start < s.len && isSpace(s[start])) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && isSpace(s[end - (1 as usize)])) {
        end -= 1;
    }
    return s[start..end];
}

fn eqIgnoreCase(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn endsWithIgnoreCase(s: []const u8, suffix: []const u8) -> bool {
    if (suffix.len > s.len) {
        return false;
    }
    const off = s.len - suffix.len;
    let i: usize = 0;
    while (i < suffix.len) {
        if (toLowerAscii(s[off + i]) != toLowerAscii(suffix[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isStar(s: []const u8) -> bool {
    return s.len == (1 as usize) && s[0] == '*';
}

fn findSingleSlash(s: []const u8) -> i64 {
    let idx: i64 = -1;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '/') {
            if (idx >= (0 as i64)) {
                return -1 as i64;
            }
            idx = i as i64;
        }
        i += 1;
    }
    return idx;
}

pub fn stripParams(content_type: []const u8) -> []const u8 {
    let semi: usize = content_type.len;
    let i: usize = 0;
    while (i < content_type.len) {
        if (content_type[i] == ';') {
            semi = i;
            break;
        }
        i += 1;
    }
    return trimSlice(content_type[0..semi]);
}

pub fn normalizeType(short_or_mime: []const u8) -> []const u8 {
    const s = trimSlice(short_or_mime);
    if (eqIgnoreCase(s, "json")) { return "application/json"; }
    if (eqIgnoreCase(s, "urlencoded")) { return "application/x-www-form-urlencoded"; }
    if (eqIgnoreCase(s, "multipart")) { return "multipart/*"; }
    if (eqIgnoreCase(s, "html")) { return "text/html"; }
    if (eqIgnoreCase(s, "text")) { return "text/plain"; }
    if (eqIgnoreCase(s, "xml")) { return "application/xml"; }
    if (eqIgnoreCase(s, "js")) { return "text/javascript"; }
    if (eqIgnoreCase(s, "css")) { return "text/css"; }
    if (eqIgnoreCase(s, "bin")) { return "application/octet-stream"; }
    return s;
}

fn matchStructuredSuffix(ctSub: []const u8, plusSuffix: []const u8) -> bool {
    if (plusSuffix.len < (2 as usize) || plusSuffix[0] != '+') {
        return false;
    }
    // Match "vnd.api+json" or "problem+json"
    if (endsWithIgnoreCase(ctSub, plusSuffix)) {
        return true;
    }
    // Also match bare subtype equal to suffix name (e.g. "application/json" matches "+json")
    const bare = plusSuffix[1..plusSuffix.len];
    return eqIgnoreCase(ctSub, bare);
}

pub fn mimeMatch(content_type: []const u8, pattern: []const u8) -> bool {
    const ct = stripParams(content_type);
    const ctSlash = findSingleSlash(ct);
    if (ctSlash <= (0 as i64) || ((ctSlash as usize) + (1 as usize)) >= ct.len) {
        return false;
    }
    const ctType = trimSlice(ct[0..(ctSlash as usize)]);
    const ctSub = trimSlice(ct[((ctSlash as usize) + (1 as usize))..ct.len]);
    if (ctType.len == (0 as usize) || ctSub.len == (0 as usize)) {
        return false;
    }

    const patTrim = trimSlice(pattern);
    if (patTrim.len == (0 as usize)) {
        return false;
    }
    if (isStar(patTrim)) {
        return true;
    }
    // RFC 6839 structured suffix pattern like "+json", "+xml"
    if (patTrim[0] == '+') {
        return matchStructuredSuffix(ctSub, patTrim);
    }

    const norm = normalizeType(patTrim);
    const pSlash = findSingleSlash(norm);
    if (pSlash <= (0 as i64) || ((pSlash as usize) + (1 as usize)) >= norm.len) {
        return false;
    }
    const pType = trimSlice(norm[0..(pSlash as usize)]);
    const pSub = trimSlice(norm[((pSlash as usize) + (1 as usize))..norm.len]);
    if (pType.len == (0 as usize) || pSub.len == (0 as usize)) {
        return false;
    }

    const typeOk = isStar(pType) || eqIgnoreCase(pType, ctType);
    if (!typeOk) {
        return false;
    }
    if (isStar(pSub) || eqIgnoreCase(pSub, ctSub)) {
        return true;
    }
    if (pSub[0] == '+') {
        return matchStructuredSuffix(ctSub, pSub);
    }
    return false;
}

pub fn matchAny(content_type: []const u8, patterns: []const []const u8) -> i64 {
    let i: usize = 0;
    while (i < patterns.len) {
        if (mimeMatch(content_type, patterns[i])) {
            return i as i64;
        }
        i += 1;
    }
    return -1 as i64;
}

export {
    stripParams,
    normalizeType,
    mimeMatch,
    matchAny,
};
