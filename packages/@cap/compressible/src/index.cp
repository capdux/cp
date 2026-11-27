// @cap/compressible — Zero-allocation HTTP Content-Type compressibility checker

fn cmprIsSpace(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn cmprToLower(b: u8): u8 {
    if (b >= 'A' && b <= 'Z') {
        return b + 32;
    }
    return b;
}

fn cmprEqlIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (cmprToLower(a[i]) != cmprToLower(b[i])) {
            return false;
        }
    }
    return true;
}

fn cmprHasPrefixIgnoreCase(s: string, prefix: string): bool {
    if (s.len < prefix.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (cmprToLower(s[i]) != cmprToLower(prefix[i])) {
            return false;
        }
    }
    return true;
}

fn cmprHasSuffixIgnoreCase(s: string, suffix: string): bool {
    if (s.len < suffix.len) {
        return false;
    }
    const offset: usize = s.len - suffix.len;
    let i: usize = 0;
    while (i < suffix.len) : (i += 1) {
        if (cmprToLower(s[offset + i]) != cmprToLower(suffix[i])) {
            return false;
        }
    }
    return true;
}

fn cmprFindSlash(s: string): usize {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == '/') {
            return i;
        }
    }
    return s.len;
}

pub fn stripContentTypeParams(ct: string): string {
    let end: usize = ct.len;
    let i: usize = 0;
    while (i < ct.len) : (i += 1) {
        if (ct[i] == ';') {
            end = i;
            break;
        }
    }
    let start: usize = 0;
    while (start < end && cmprIsSpace(ct[start])) : (start += 1) {}
    while (end > start && cmprIsSpace(ct[end - 1])) : (end -= 1) {}
    return ct[start..end];
}

pub fn isCompressible(content_type: string): bool {
    const mime = stripContentTypeParams(content_type);
    if (mime.len == 0) {
        return false;
    }
    const slash = cmprFindSlash(mime);
    if (slash == 0 || slash + 1 >= mime.len) {
        return false;
    }
    const subtype = mime[(slash + 1)..mime.len];

    if (cmprHasPrefixIgnoreCase(mime, "text/")) {
        return true;
    }

    if (cmprHasSuffixIgnoreCase(subtype, "+json") ||
        cmprHasSuffixIgnoreCase(subtype, "+xml") ||
        cmprHasSuffixIgnoreCase(subtype, "+text")) {
        return true;
    }

    if (cmprHasPrefixIgnoreCase(mime, "video/") || cmprHasPrefixIgnoreCase(mime, "audio/")) {
        return false;
    }

    if (cmprHasPrefixIgnoreCase(mime, "application/")) {
        if (cmprEqlIgnoreCase(subtype, "json") ||
            cmprEqlIgnoreCase(subtype, "javascript") ||
            cmprEqlIgnoreCase(subtype, "x-javascript") ||
            cmprEqlIgnoreCase(subtype, "ecmascript") ||
            cmprEqlIgnoreCase(subtype, "xml") ||
            cmprEqlIgnoreCase(subtype, "wasm") ||
            cmprEqlIgnoreCase(subtype, "graphql") ||
            cmprEqlIgnoreCase(subtype, "x-www-form-urlencoded") ||
            cmprEqlIgnoreCase(subtype, "rtf") ||
            cmprEqlIgnoreCase(subtype, "tar") ||
            cmprEqlIgnoreCase(subtype, "x-tar") ||
            cmprEqlIgnoreCase(subtype, "postscript") ||
            cmprEqlIgnoreCase(subtype, "dart") ||
            cmprEqlIgnoreCase(subtype, "toml") ||
            cmprEqlIgnoreCase(subtype, "yaml") ||
            cmprEqlIgnoreCase(subtype, "x-yaml") ||
            cmprEqlIgnoreCase(subtype, "sql") ||
            cmprEqlIgnoreCase(subtype, "x-ndjson") ||
            cmprEqlIgnoreCase(subtype, "x-sh") ||
            cmprEqlIgnoreCase(subtype, "x-httpd-php") ||
            cmprEqlIgnoreCase(subtype, "vnd.ms-fontobject") ||
            cmprEqlIgnoreCase(subtype, "x-font-ttf") ||
            cmprEqlIgnoreCase(subtype, "x-font-opentype")) {
            return true;
        }
        return false;
    }

    if (cmprHasPrefixIgnoreCase(mime, "font/")) {
        if (cmprEqlIgnoreCase(subtype, "ttf") ||
            cmprEqlIgnoreCase(subtype, "otf") ||
            cmprEqlIgnoreCase(subtype, "eot") ||
            cmprEqlIgnoreCase(subtype, "sfnt")) {
            return true;
        }
        return false;
    }

    if (cmprHasPrefixIgnoreCase(mime, "image/")) {
        if (cmprEqlIgnoreCase(subtype, "x-icon") ||
            cmprEqlIgnoreCase(subtype, "vnd.microsoft.icon") ||
            cmprEqlIgnoreCase(subtype, "bmp") ||
            cmprEqlIgnoreCase(subtype, "x-ms-bmp")) {
            return true;
        }
        return false;
    }

    if (cmprEqlIgnoreCase(mime, "message/rfc822")) {
        return true;
    }

    return false;
}
