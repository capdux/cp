// @cap/content-type — Zero-allocation RFC 9110 / RFC 6838 HTTP Content-Type parser & formatter for CP
// Clean-room MIT implementation inspired by npm `content-type` and `media-typer`.
//
// Features:
// - Strict RFC 9110 token validation for type, subtype, and parameter names/values.
// - RFC 6838 structured syntax `+suffix` extraction (e.g. `application/problem+json` -> `json`).
// - Quoted-string parameter parsing supporting embedded semicolons and `\"` escapes.
// - Case-insensitive parameter lookups (`charset()`, `boundary()`, `getParam()`) and type predicates
//   (`isJsonContentType`, `isTextContentType`, `isMultipartContentType`).
// - Zero-allocation canonical formatting (`formatContentType`) into caller-provided `out: []u8`.

const MAX_PARAMS: usize = 16;

pub struct MediaParam {
    pub key: string,
    pub value: string,
}

pub struct ContentType {
    pub ok: bool,
    pub type_str: string,
    pub subtype: string,
    pub suffix: string,
    pub essence: string,
    pub params: [MAX_PARAMS]MediaParam,
    pub param_count: usize,

    pub static fn invalid() -> ContentType {
        let p: [MAX_PARAMS]MediaParam = undefined;
        return ContentType{
            ok: false,
            type_str: "",
            subtype: "",
            suffix: "",
            essence: "",
            params: p,
            param_count: 0,
        };
    }

    pub fn getParam(key: string) -> string? {
        let i: usize = 0;
        while (i < _.param_count) {
            if (eqIgnoreCase(_.params[i].key, key)) {
                return _.params[i].value;
            }
            i += 1;
        }
        return null;
    }

    pub fn charset() -> string? {
        return _.getParam("charset");
    }

    pub fn boundary() -> string? {
        return _.getParam("boundary");
    }
}

fn isSpace(c: u8) -> bool {
    return c == ' ' || c == '\t';
}

fn toLowerAscii(c: u8) -> u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn eqIgnoreCase(a: string, b: string) -> bool {
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

/// RFC 9110 Section 5.6.2 `tchar` check.
fn isTchar(c: u8) -> bool {
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
        return true;
    }
    return c == '!' || c == '#' || c == '$' || c == '%' || c == '&' || c == '\'' ||
        c == '*' || c == '+' || c == '-' || c == '.' || c == '^' || c == '_' ||
        c == '`' || c == '|' || c == '~';
}

fn isValidToken(s: string) -> bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        if (!isTchar(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn trimSpaces(s: string) -> string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isSpace(s[start])) {
        start += 1;
    }
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

/// Parses an RFC 9110 HTTP `Content-Type` header into a zero-allocation `ContentType`.
pub fn parseContentType(header: string) -> ContentType {
    const trimmed = trimSpaces(header);
    if (trimmed.len == 0) {
        return ContentType.invalid();
    }

    // Locate first ';' separating media-type essence from parameters
    let semi_idx: usize = 0;
    while (semi_idx < trimmed.len && trimmed[semi_idx] != ';') {
        semi_idx += 1;
    }

    const essence = trimSpaces(trimmed[0..semi_idx]);
    if (essence.len == 0) {
        return ContentType.invalid();
    }

    // Split type / subtype (must have exactly one '/')
    let slash_idx: usize = 0;
    let slash_count: usize = 0;
    let i: usize = 0;
    while (i < essence.len) {
        if (essence[i] == '/') {
            slash_idx = i;
            slash_count += 1;
        }
        i += 1;
    }
    if (slash_count != 1) {
        return ContentType.invalid();
    }

    const type_str = essence[0..slash_idx];
    const subtype = essence[(slash_idx + 1)..essence.len];
    if (!isValidToken(type_str) || !isValidToken(subtype)) {
        return ContentType.invalid();
    }

    // Extract RFC 6838 structured syntax +suffix (after last '+')
    let suffix = "";
    let plus_found = false;
    let last_plus: usize = 0;
    let si: usize = 0;
    while (si < subtype.len) {
        if (subtype[si] == '+') {
            plus_found = true;
            last_plus = si;
        }
        si += 1;
    }
    if (plus_found && last_plus + 1 < subtype.len) {
        suffix = subtype[(last_plus + 1)..subtype.len];
    }

    let ct = ContentType.invalid();
    ct.ok = true;
    ct.type_str = type_str;
    ct.subtype = subtype;
    ct.suffix = suffix;
    ct.essence = essence;

    // Parse optional `; key=value` parameters
    let pos = semi_idx;
    while (pos < trimmed.len) {
        // Expect ';'
        if (trimmed[pos] != ';') {
            return ContentType.invalid();
        }
        pos += 1;
        while (pos < trimmed.len && isSpace(trimmed[pos])) {
            pos += 1;
        }
        if (pos >= trimmed.len) {
            break;
        }

        // Parse parameter name up to '='
        const key_start = pos;
        while (pos < trimmed.len && trimmed[pos] != '=' && trimmed[pos] != ';') {
            pos += 1;
        }
        if (pos >= trimmed.len || trimmed[pos] != '=') {
            return ContentType.invalid();
        }
        const key = trimSpaces(trimmed[key_start..pos]);
        if (!isValidToken(key)) {
            return ContentType.invalid();
        }
        pos += 1; // skip '='
        while (pos < trimmed.len && isSpace(trimmed[pos])) {
            pos += 1;
        }
        if (pos >= trimmed.len) {
            return ContentType.invalid();
        }

        // Parse parameter value: quoted-string or token
        let val = "";
        if (trimmed[pos] == '"') {
            pos += 1; // skip opening '"'
            const val_start = pos;
            let closed = false;
            while (pos < trimmed.len) {
                if (trimmed[pos] == '\\' && pos + 1 < trimmed.len) {
                    pos += 2;
                    continue;
                }
                if (trimmed[pos] == '"') {
                    closed = true;
                    break;
                }
                pos += 1;
            }
            if (!closed) {
                return ContentType.invalid();
            }
            val = trimmed[val_start..pos];
            pos += 1; // skip closing '"'
            while (pos < trimmed.len && isSpace(trimmed[pos])) {
                pos += 1;
            }
            if (pos < trimmed.len && trimmed[pos] != ';') {
                return ContentType.invalid();
            }
        } else {
            const val_start = pos;
            while (pos < trimmed.len && trimmed[pos] != ';') {
                pos += 1;
            }
            val = trimSpaces(trimmed[val_start..pos]);
            if (!isValidToken(val)) {
                return ContentType.invalid();
            }
        }

        if (ct.param_count < MAX_PARAMS) {
            ct.params[ct.param_count] = MediaParam{ key: key, value: val };
            ct.param_count += 1;
        }
    }

    return ct;
}

/// Returns `true` for `application/json`, `text/json`, or any `*/*+json` media type.
pub fn isJsonContentType(header: string) -> bool {
    const ct = parseContentType(header);
    if (!ct.ok) {
        return false;
    }
    if (eqIgnoreCase(ct.suffix, "json")) {
        return true;
    }
    if (eqIgnoreCase(ct.subtype, "json") &&
        (eqIgnoreCase(ct.type_str, "application") || eqIgnoreCase(ct.type_str, "text"))) {
        return true;
    }
    return false;
}

/// Returns `true` for any valid `text/*` media type.
pub fn isTextContentType(header: string) -> bool {
    const ct = parseContentType(header);
    if (!ct.ok) {
        return false;
    }
    return eqIgnoreCase(ct.type_str, "text");
}

/// Returns `true` for any valid `multipart/*` media type.
pub fn isMultipartContentType(header: string) -> bool {
    const ct = parseContentType(header);
    if (!ct.ok) {
        return false;
    }
    return eqIgnoreCase(ct.type_str, "multipart");
}

fn pushByte(out: []u8, pos: usize, b: u8) -> usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn pushLower(out: []u8, pos: usize, s: string) -> usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = toLowerAscii(s[i]);
        p += 1;
        i += 1;
    }
    return p;
}

fn pushRaw(out: []u8, pos: usize, s: string) -> usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

/// Formats a `ContentType` into canonical RFC 9110 form in caller-provided `out` (zero heap allocation).
/// Lowercases type/subtype and parameter names, and automatically quotes/escapes non-token parameter values.
pub fn formatContentType(ct: ContentType, out: []u8) -> string {
    if (!ct.ok || ct.type_str.len == 0 || ct.subtype.len == 0) {
        return out[0..0];
    }

    let pos: usize = 0;
    pos = pushLower(out, pos, ct.type_str);
    pos = pushByte(out, pos, '/');
    pos = pushLower(out, pos, ct.subtype);

    let i: usize = 0;
    while (i < ct.param_count) {
        const param = ct.params[i];
        pos = pushRaw(out, pos, "; ");
        pos = pushLower(out, pos, param.key);
        pos = pushByte(out, pos, '=');

        if (isValidToken(param.value)) {
            pos = pushRaw(out, pos, param.value);
        } else {
            pos = pushByte(out, pos, '"');
            let k: usize = 0;
            while (k < param.value.len) {
                const c = param.value[k];
                if (c == '\\' && k + 1 < param.value.len && (param.value[k + 1] == '"' || param.value[k + 1] == '\\')) {
                    pos = pushByte(out, pos, '\\');
                    pos = pushByte(out, pos, param.value[k + 1]);
                    k += 2;
                    continue;
                }
                if (c == '"' || c == '\\') {
                    pos = pushByte(out, pos, '\\');
                }
                pos = pushByte(out, pos, c);
                k += 1;
            }
            pos = pushByte(out, pos, '"');
        }
        i += 1;
    }

    return out[0..pos];
}
