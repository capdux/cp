// packages/@cap/media-type/src/index.cp
// Zero-allocation RFC 9110 / RFC 6838 HTTP Media Type (MIME) parser & matcher.
// MIT clean-room pure CP implementation.

/// Zero-copy parsed HTTP Media Type components.
/// Example: `application/vnd.api+json; charset=utf-8`
/// -> `top="application"`, `subtype="vnd.api"`, `suffix="json"`, `params="charset=utf-8"`.
pub struct MediaType {
    pub top: string,
    pub subtype: string,
    pub suffix: string,
    pub params: string,
}

fn mtToLower(c: u8): u8 {
    if (c >= ('A' as u8) && c <= ('Z' as u8)) {
        return c + (32 as u8);
    }
    return c;
}

fn mtIsOws(c: u8): bool {
    return c == (' ' as u8) || c == ('\t' as u8);
}

fn mtTrimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && mtIsOws(s[start])) {
        start += 1;
    }
    while (end > start && mtIsOws(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn mtEqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (mtToLower(a[i]) != mtToLower(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn mtIsTchar(c: u8): bool {
    if ((c >= ('a' as u8) && c <= ('z' as u8)) || (c >= ('A' as u8) && c <= ('Z' as u8)) || (c >= ('0' as u8) && c <= ('9' as u8))) {
        return true;
    }
    if (c == ('!' as u8) || c == ('#' as u8) || c == ('$' as u8) || c == ('%' as u8) || c == ('&' as u8)) {
        return true;
    }
    if (c == ('\'' as u8) || c == ('*' as u8) || c == ('+' as u8) || c == ('-' as u8) || c == ('.' as u8)) {
        return true;
    }
    if (c == ('^' as u8) || c == ('_' as u8) || c == ('`' as u8) || c == ('|' as u8) || c == ('~' as u8)) {
        return true;
    }
    return false;
}

/// Validate an RFC 9110 / RFC 6838 HTTP token (non-empty sequence of tchar).
pub fn isValidToken(s: string): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        if (!mtIsTchar(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Strictly parse an HTTP Media Type (`type/subtype[+suffix][; params]`) per RFC 9110 / RFC 6838.
/// Writes zero-copy slices into `out.*` and returns `true` on success, `false` if malformed.
pub fn parseMediaType(input: string, out: *MediaType): bool {
    const trimmed = mtTrimOws(input);
    if (trimmed.len == 0) {
        return false;
    }

    // Split essence and optional parameter section at first ';'.
    let semi: usize = 0;
    while (semi < trimmed.len && trimmed[semi] != (';' as u8)) {
        semi += 1;
    }

    const essence = mtTrimOws(trimmed[0..semi]);
    let paramsSlice: []const u8 = "";
    if (semi < trimmed.len) {
        paramsSlice = mtTrimOws(trimmed[(semi + 1)..trimmed.len]);
    }

    // Split top / fullSub at '/'.
    let slash: usize = 0;
    while (slash < essence.len && essence[slash] != ('/' as u8)) {
        slash += 1;
    }
    if (slash == 0 || slash + 1 >= essence.len) {
        return false;
    }

    const top = essence[0..slash];
    const fullSub = essence[(slash + 1)..essence.len];

    if (!isValidToken(top) || !isValidToken(fullSub)) {
        return false;
    }

    // Extract optional RFC 6838 structured syntax suffix after the last '+'.
    let hasPlus = false;
    let plusIdx: usize = 0;
    let i: usize = 0;
    while (i < fullSub.len) {
        if (fullSub[i] == ('+' as u8)) {
            hasPlus = true;
            plusIdx = i;
        }
        i += 1;
    }

    let sub = fullSub;
    let suffix: []const u8 = "";
    if (hasPlus) {
        if (plusIdx == 0 || plusIdx + 1 >= fullSub.len) {
            return false;
        }
        sub = fullSub[0..plusIdx];
        suffix = fullSub[(plusIdx + 1)..fullSub.len];
    }

    out.* = MediaType{
        top: top,
        subtype: sub,
        suffix: suffix,
        params: paramsSlice,
    };
    return true;
}

/// Extract a parameter value from `mt.params` by case-insensitive `name`.
/// Automatically strips surrounding double quotes and unescapes `\"` / `\\` pairs.
/// Returns bytes written into `out`, or `0` if not found.
pub fn getParam(mt: MediaType, name: string, out: []u8): usize {
    if (mt.params.len == 0 || name.len == 0) {
        return 0;
    }
    const p = mt.params;
    let i: usize = 0;

    while (i < p.len) {
        // Skip leading whitespace and semicolons.
        while (i < p.len && (mtIsOws(p[i]) || p[i] == (';' as u8))) {
            i += 1;
        }
        if (i >= p.len) {
            break;
        }

        // Read parameter key up to '=' or ';'.
        const keyStart = i;
        while (i < p.len && p[i] != ('=' as u8) && p[i] != (';' as u8)) {
            i += 1;
        }
        const key = mtTrimOws(p[keyStart..i]);

        if (i >= p.len || p[i] == (';' as u8)) {
            if (i < p.len) {
                i += 1;
            }
            continue;
        }

        // Skip '=' and leading whitespace before value.
        i += 1;
        while (i < p.len && mtIsOws(p[i])) {
            i += 1;
        }

        const matched = mtEqIgnoreCase(key, name);

        if (i < p.len && p[i] == ('"' as u8)) {
            // Quoted-string parameter value.
            i += 1;
            let written: usize = 0;
            while (i < p.len) {
                const c = p[i];
                if (c == ('\\' as u8) && i + 1 < p.len) {
                    if (matched && written < out.len) {
                        out[written] = p[i + 1];
                        written += 1;
                    }
                    i += 2;
                } else if (c == ('"' as u8)) {
                    i += 1;
                    break;
                } else {
                    if (matched && written < out.len) {
                        out[written] = c;
                        written += 1;
                    }
                    i += 1;
                }
            }
            if (matched) {
                return written;
            }
            // Advance to next ';'.
            while (i < p.len && p[i] != (';' as u8)) {
                i += 1;
            }
        } else {
            // Unquoted token parameter value.
            const valStart = i;
            while (i < p.len && p[i] != (';' as u8)) {
                i += 1;
            }
            const val = mtTrimOws(p[valStart..i]);
            if (matched) {
                let copyLen = val.len;
                if (copyLen > out.len) {
                    copyLen = out.len;
                }
                let ci: usize = 0;
                while (ci < copyLen) {
                    out[ci] = val[ci];
                    ci += 1;
                }
                return copyLen;
            }
        }
    }

    return 0;
}

/// Format `mt` as normalized lowercase `type/subtype[+suffix]` into `out`.
/// Returns bytes written into `out`.
pub fn formatMediaType(mt: MediaType, out: []u8): usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < mt.top.len && pos < out.len) {
        out[pos] = mtToLower(mt.top[i]);
        pos += 1;
        i += 1;
    }
    if (pos < out.len) {
        out[pos] = '/' as u8;
        pos += 1;
    }
    let j: usize = 0;
    while (j < mt.subtype.len && pos < out.len) {
        out[pos] = mtToLower(mt.subtype[j]);
        pos += 1;
        j += 1;
    }
    if (mt.suffix.len > 0) {
        if (pos < out.len) {
            out[pos] = '+' as u8;
            pos += 1;
        }
        let k: usize = 0;
        while (k < mt.suffix.len && pos < out.len) {
            out[pos] = mtToLower(mt.suffix[k]);
            pos += 1;
            k += 1;
        }
    }
    return pos;
}

/// Returns true if `mt` represents JSON (`application/json`, `text/json`, or `+json` suffix).
pub fn isJsonLike(mt: MediaType): bool {
    if (mtEqIgnoreCase(mt.suffix, "json")) {
        return true;
    }
    if (mt.suffix.len == 0 && mtEqIgnoreCase(mt.subtype, "json")) {
        if (mtEqIgnoreCase(mt.top, "application") || mtEqIgnoreCase(mt.top, "text")) {
            return true;
        }
    }
    return false;
}

/// Returns true if `mt` represents textual content (`text/*`, `application/xml`, `+xml`, `application/javascript`).
pub fn isTextLike(mt: MediaType): bool {
    if (mtEqIgnoreCase(mt.top, "text")) {
        return true;
    }
    if (mtEqIgnoreCase(mt.suffix, "xml")) {
        return true;
    }
    if (mt.suffix.len == 0 && mtEqIgnoreCase(mt.top, "application")) {
        if (mtEqIgnoreCase(mt.subtype, "xml") || mtEqIgnoreCase(mt.subtype, "javascript")) {
            return true;
        }
    }
    return false;
}

export {
    MediaType,
    isValidToken,
    parseMediaType,
    getParam,
    formatMediaType,
    isJsonLike,
    isTextLike,
};
