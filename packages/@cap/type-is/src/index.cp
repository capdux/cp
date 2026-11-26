// @cap/type-is — Zero-allocation HTTP Content-Type MIME matcher for CP
// Clean-room MIT implementation inspired by npm `type-is` (227M/wk).
// Delegates RFC 6838 media-type parsing & validation to `@cap/media-typer`.

import { MediaType, parse, testMediaType } from "../../media-typer/src/index.cp";

fn tiIsSpace(c: u8) -> bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn tiToLowerAscii(c: u8) -> u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + (32 as u8);
    }
    return c;
}

fn tiTrimSlice(s: []const u8) -> []const u8 {
    let start: usize = 0;
    while (start < s.len && tiIsSpace(s[start])) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && tiIsSpace(s[end - (1 as usize)])) {
        end -= 1;
    }
    return s[start..end];
}

fn tiEqIgnoreCase(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (tiToLowerAscii(a[i]) != tiToLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn tiIsStar(s: []const u8) -> bool {
    return s.len == (1 as usize) && s[0] == '*';
}

fn tiFindSingleSlash(s: []const u8) -> i64 {
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

fn tiMatchStructuredSuffix(ct: []const u8, parsed: MediaType, plusSuffix: []const u8) -> bool {
    if (plusSuffix.len < (2 as usize) || plusSuffix[0] != '+') {
        return false;
    }
    const bare = plusSuffix[1..plusSuffix.len];
    if (parsed.has_suffix) {
        const ctSuffix = ct[parsed.suffix_start..(parsed.suffix_start + parsed.suffix_len)];
        return tiEqIgnoreCase(ctSuffix, bare);
    }
    // Also match bare subtype equal to suffix name (e.g. "application/json" matches "+json")
    const ctSub = ct[parsed.subtype_start..(parsed.subtype_start + parsed.subtype_len)];
    return tiEqIgnoreCase(ctSub, bare);
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
    return tiTrimSlice(content_type[0..semi]);
}

pub fn normalizeType(short_or_mime: []const u8) -> []const u8 {
    const s = tiTrimSlice(short_or_mime);
    if (tiEqIgnoreCase(s, "json")) { return "application/json"; }
    if (tiEqIgnoreCase(s, "urlencoded")) { return "application/x-www-form-urlencoded"; }
    if (tiEqIgnoreCase(s, "multipart")) { return "multipart/*"; }
    if (tiEqIgnoreCase(s, "html")) { return "text/html"; }
    if (tiEqIgnoreCase(s, "text")) { return "text/plain"; }
    if (tiEqIgnoreCase(s, "xml")) { return "application/xml"; }
    if (tiEqIgnoreCase(s, "js")) { return "text/javascript"; }
    if (tiEqIgnoreCase(s, "css")) { return "text/css"; }
    if (tiEqIgnoreCase(s, "bin")) { return "application/octet-stream"; }
    return s;
}

pub fn mimeMatch(content_type: []const u8, pattern: []const u8) -> bool {
    const ct = stripParams(content_type);
    const parsed = parse(ct);
    if (!parsed.valid) {
        return false;
    }
    const ctType = ct[parsed.type_start..(parsed.type_start + parsed.type_len)];
    const ctFullSub = ct[parsed.subtype_start..ct.len];

    const patTrim = tiTrimSlice(pattern);
    if (patTrim.len == (0 as usize)) {
        return false;
    }
    if (tiIsStar(patTrim)) {
        return true;
    }
    // RFC 6839 structured suffix pattern like "+json", "+xml"
    if (patTrim[0] == '+') {
        return tiMatchStructuredSuffix(ct, parsed, patTrim);
    }

    const norm = normalizeType(patTrim);
    if (testMediaType(norm)) {
        const pParsed = parse(norm);
        const pType = norm[pParsed.type_start..(pParsed.type_start + pParsed.type_len)];
        const pFullSub = norm[pParsed.subtype_start..norm.len];
        return tiEqIgnoreCase(pType, ctType) && tiEqIgnoreCase(pFullSub, ctFullSub);
    }

    // Wildcard or type/+suffix pattern (e.g. "image/*", "*/json", "*/*", "application/+json")
    const pSlash = tiFindSingleSlash(norm);
    if (pSlash <= (0 as i64) || ((pSlash as usize) + (1 as usize)) >= norm.len) {
        return false;
    }
    const pType = tiTrimSlice(norm[0..(pSlash as usize)]);
    const pSub = tiTrimSlice(norm[((pSlash as usize) + (1 as usize))..norm.len]);
    if (pType.len == (0 as usize) || pSub.len == (0 as usize)) {
        return false;
    }

    const typeOk = tiIsStar(pType) || tiEqIgnoreCase(pType, ctType);
    if (!typeOk) {
        return false;
    }
    if (tiIsStar(pSub) || tiEqIgnoreCase(pSub, ctFullSub)) {
        return true;
    }
    if (pSub[0] == '+') {
        return tiMatchStructuredSuffix(ct, parsed, pSub);
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
