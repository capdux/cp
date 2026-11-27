// @cap/content-type — Zero-heap RFC 7231 HTTP Content-Type header parser & formatter.
// Clean-room MIT CP implementation modeled on npm `content-type` v1.0.
// Reuses `eqAscii` from `@cap/unicase` for case-insensitive parameter name lookup.

import { eqAscii } from "../../unicase/src/index.cp";

// RFC 7230 optional whitespace: SP (0x20) / HTAB (0x09).
fn ctypIsOws(c: u8): bool {
    return c == 32 || c == 9;
}

// RFC 7230 tchar: "!" / "#" / "$" / "%" / "&" / "'" / "*" / "+" / "-" / "." /
//                 "^" / "_" / "`" / "|" / "~" / DIGIT / ALPHA
fn ctypIsTokenChar(c: u8): bool {
    if ((c >= 48 && c <= 57) || (c >= 65 && c <= 90) || (c >= 97 && c <= 122)) {
        return true;
    }
    return c == 33 || c == 35 || c == 36 || c == 37 || c == 38 || c == 39
        || c == 42 || c == 43 || c == 45 || c == 46 || c == 94 || c == 95
        || c == 96 || c == 124 || c == 126;
}

fn ctypIsValidToken(s: string): bool {
    if (s.len == 0) { return false; }
    let i: usize = 0;
    while (i < s.len) {
        if (!ctypIsTokenChar(s[i])) { return false; }
        i = i + 1;
    }
    return true;
}

// RFC 7230 quoted-pair / qdtext allowed byte check (HTAB, SP, VCHAR 0x21..0x7E, obs-text 0x80..0xFF).
fn ctypIsQuotedAllowed(c: u8): bool {
    return c == 9 || c == 32 || (c >= 33 && c <= 126) || c >= 128;
}

fn ctypWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn ctypWriteSlice(out: []u8, pos: usize, s: string): usize {
    let w = pos;
    let i: usize = 0;
    while (i < s.len && w < out.len) {
        out[w] = s[i];
        w = w + 1;
        i = i + 1;
    }
    return w;
}

/// Single `Content-Type` parameter key/value pair.
pub struct ContentTypeParam {
    pub name: string,
    pub value: string,

    pub static fn new(name: string, value: string): ContentTypeParam {
        return ContentTypeParam { name: name, value: value };
    }
}

/// Zero-heap parsed RFC 7231 `Content-Type` header.
pub struct ContentType {
    pub valid: bool,
    pub media_type: string,
    pub params: [8]ContentTypeParam,
    pub param_count: usize,

    pub static fn invalid(): ContentType {
        let empty = ContentTypeParam { name: "", value: "" };
        return ContentType {
            valid: false,
            media_type: "",
            params: [empty, empty, empty, empty, empty, empty, empty, empty],
            param_count: 0,
        };
    }

    /// Case-insensitive parameter lookup via `@cap/unicase::eqAscii`.
    pub fn getParam(name: string): string? {
        if (!_.valid) { return null; }
        let i: usize = 0;
        while (i < _.param_count) {
            if (eqAscii(_.params[i].name, name)) {
                return _.params[i].value;
            }
            i = i + 1;
        }
        return null;
    }

    /// Looks up a parameter case-insensitively and writes its RFC 7231 unescaped value into `out`.
    /// Returns bytes written (0 if parameter is absent or empty).
    pub fn getParamUnescaped(name: string, out: []u8): usize {
        if (!_.valid) { return 0; }
        let i: usize = 0;
        while (i < _.param_count) {
            if (eqAscii(_.params[i].name, name)) {
                return unescapeQuotedString(_.params[i].value, out);
            }
            i = i + 1;
        }
        return 0;
    }

    /// Convenience accessor for the `charset` parameter.
    pub fn charset(): string? {
        return _.getParam("charset");
    }

    /// Convenience accessor for the `boundary` parameter.
    pub fn boundary(): string? {
        return _.getParam("boundary");
    }
}

/// Validates an RFC 7231 media type (`type/subtype` where both parts are non-empty tokens).
pub fn isValidMediaType(s: string): bool {
    if (s.len < 3) { return false; }
    let slash_idx: usize = s.len;
    let i: usize = 0;
    while (i < s.len) {
        let c = s[i];
        if (c == 47) { // '/'
            if (slash_idx != s.len) { return false; } // duplicate '/'
            slash_idx = i;
        } else if (!ctypIsTokenChar(c)) {
            return false;
        }
        i = i + 1;
    }
    if (slash_idx == 0 || slash_idx >= s.len - 1) {
        return false;
    }
    return true;
}

/// Unescapes an RFC 7230 quoted-string body (`\"` -> `"`, `\\` -> `\`, `\x` -> `x`) into `out`.
/// Returns bytes written.
pub fn unescapeQuotedString(raw: string, out: []u8): usize {
    let i: usize = 0;
    let w: usize = 0;
    while (i < raw.len && w < out.len) {
        if (raw[i] == 92 && i + 1 < raw.len) {
            out[w] = raw[i + 1];
            w = w + 1;
            i = i + 2;
        } else {
            out[w] = raw[i];
            w = w + 1;
            i = i + 1;
        }
    }
    return w;
}

/// Parses an HTTP `Content-Type` header strictly according to RFC 7231 Section 3.1.1.1.
/// Rejects invalid tokens, malformed semicolons, missing values, and unclosed/control quotes.
pub fn parseContentType(header: string): ContentType {
    let empty = ContentTypeParam { name: "", value: "" };
    let res = ContentType {
        valid: false,
        media_type: "",
        params: [empty, empty, empty, empty, empty, empty, empty, empty],
        param_count: 0,
    };

    let i: usize = 0;
    while (i < header.len && ctypIsOws(header[i])) {
        i = i + 1;
    }
    if (i >= header.len) { return res; }

    // Parse media-type = type "/" subtype
    let mt_start = i;
    let type_start = i;
    while (i < header.len && ctypIsTokenChar(header[i])) {
        i = i + 1;
    }
    if (i == type_start || i >= header.len || header[i] != 47) {
        return res;
    }
    i = i + 1; // skip '/'
    let sub_start = i;
    while (i < header.len && ctypIsTokenChar(header[i])) {
        i = i + 1;
    }
    if (i == sub_start) {
        return res;
    }
    res.media_type = header[mt_start..i];

    while (i < header.len && ctypIsOws(header[i])) {
        i = i + 1;
    }

    // Parse *( OWS ";" OWS parameter )
    while (i < header.len) {
        if (header[i] != 59) { // ';'
            return ContentType.invalid();
        }
        i = i + 1;
        while (i < header.len && ctypIsOws(header[i])) {
            i = i + 1;
        }
        // Reject trailing or empty semicolon (e.g. "text/html;" or "text/html; ; a=b")
        if (i >= header.len) {
            return ContentType.invalid();
        }

        // Parameter name (token)
        let k_start = i;
        while (i < header.len && ctypIsTokenChar(header[i])) {
            i = i + 1;
        }
        if (i == k_start) {
            return ContentType.invalid();
        }
        let param_name = header[k_start..i];

        // Must immediately be followed by '='
        if (i >= header.len || header[i] != 61) {
            return ContentType.invalid();
        }
        i = i + 1;
        if (i >= header.len) {
            return ContentType.invalid();
        }

        // Parameter value: quoted-string or token
        let v_start: usize = i;
        let v_end: usize = i;
        if (header[i] == 34) { // '"'
            i = i + 1;
            v_start = i;
            let closed = false;
            while (i < header.len) {
                let c = header[i];
                if (c == 34) {
                    v_end = i;
                    i = i + 1;
                    closed = true;
                    break;
                }
                if (c == 92) { // '\' quoted-pair
                    if (i + 1 >= header.len || !ctypIsQuotedAllowed(header[i + 1])) {
                        return ContentType.invalid();
                    }
                    i = i + 2;
                    continue;
                }
                if (!ctypIsQuotedAllowed(c)) {
                    return ContentType.invalid();
                }
                i = i + 1;
            }
            if (!closed) {
                return ContentType.invalid();
            }
        } else {
            v_start = i;
            while (i < header.len && ctypIsTokenChar(header[i])) {
                i = i + 1;
            }
            v_end = i;
            if (v_end == v_start) {
                return ContentType.invalid();
            }
        }

        let param_val = header[v_start..v_end];

        // Overwrite existing parameter if duplicate key (case-insensitive), else append up to 8
        let updated = false;
        let idx: usize = 0;
        while (idx < res.param_count) {
            if (eqAscii(res.params[idx].name, param_name)) {
                res.params[idx] = ContentTypeParam { name: param_name, value: param_val };
                updated = true;
                break;
            }
            idx = idx + 1;
        }
        if (!updated && res.param_count < 8) {
            res.params[res.param_count] = ContentTypeParam { name: param_name, value: param_val };
            res.param_count = res.param_count + 1;
        }

        while (i < header.len && ctypIsOws(header[i])) {
            i = i + 1;
        }
    }

    res.valid = true;
    return res;
}

/// Formats a `Content-Type` header per RFC 7231 into `out`, returning bytes written (0 on invalid input).
/// Parameter values that are not valid tokens are automatically double-quoted with `\"` and `\\` escaping.
pub fn formatContentType(media_type: string, params: []const ContentTypeParam, out: []u8): usize {
    if (!isValidMediaType(media_type)) { return 0; }

    // Validate all parameter names and values up front
    let k: usize = 0;
    while (k < params.len) {
        if (!ctypIsValidToken(params[k].name)) { return 0; }
        let v = params[k].value;
        let j: usize = 0;
        while (j < v.len) {
            if (!ctypIsQuotedAllowed(v[j])) { return 0; }
            j = j + 1;
        }
        k = k + 1;
    }

    let w = ctypWriteSlice(out, 0, media_type);
    k = 0;
    while (k < params.len) {
        w = ctypWriteSlice(out, w, "; ");
        w = ctypWriteSlice(out, w, params[k].name);
        w = ctypWriteByte(out, w, 61); // '='

        let val = params[k].value;
        if (val.len > 0 && ctypIsValidToken(val)) {
            w = ctypWriteSlice(out, w, val);
        } else {
            w = ctypWriteByte(out, w, 34); // '"'
            let j: usize = 0;
            while (j < val.len) {
                let b = val[j];
                if (b == 34 || b == 92) {
                    w = ctypWriteByte(out, w, 92); // '\'
                }
                w = ctypWriteByte(out, w, b);
                j = j + 1;
            }
            w = ctypWriteByte(out, w, 34); // '"'
        }
        k = k + 1;
    }
    return w;
}
