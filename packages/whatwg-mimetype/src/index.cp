// @cap/whatwg-mimetype — Zero-allocation WHATWG MIME Sniffing Standard MIMEType parser in pure CP
// Clean-room MIT implementation of npm `whatwg-mimetype` (#200, 215M/wk).

import { isAlnum, eqIgnoreCase, toLower } from "cap:ascii";
import { writeStr } from "cap:fmt";

pub const WMIME_MAX_PARAMS: usize = 8;

pub struct MimeParam {
    pub name: string,
    pub value: string,
}

fn wmimeIsHttpWs(b: u8): bool {
    return b == 0x09 || b == 0x0A || b == 0x0D || b == 0x20;
}

fn wmimeIsTokenChar(b: u8): bool {
    if (isAlnum(b)) {
        return true;
    }
    return b == 33 || b == 35 || b == 36 || b == 37 || b == 38 || b == 39
        || b == 42 || b == 43 || b == 45 || b == 46 || b == 94 || b == 95
        || b == 96 || b == 124 || b == 126;
}

fn wmimeEndsWithIgnoreCase(s: string, suffix: string): bool {
    if (suffix.len > s.len) {
        return false;
    }
    return eqIgnoreCase(s[(s.len - suffix.len)..s.len], suffix);
}

fn wmimePushLower(out: []u8, pos: usize, s: string): usize {
    if (pos >= out.len) {
        return pos;
    }
    return pos + toLower(s, out[pos..out.len]);
}

/// Parsed WHATWG MIME Type with zero-heap inline parameter table.
pub struct MimeType {
    pub valid: bool,
    pub main_type: string,
    pub subtype: string,
    params: [8]MimeParam,
    param_count: usize,

    pub fn paramCount(): usize {
        return _.param_count;
    }

    pub fn getParam(name: string): string? {
        let i: usize = 0;
        while (i < _.param_count) {
            if (eqIgnoreCase(_.params[i].name, name)) {
                return _.params[i].value;
            }
            i += 1;
        }
        return null;
    }

    /// Writes `<type>/<subtype>` (lowercased) into `out`.
    pub fn essence(out: []u8): usize {
        if (!_.valid) {
            return 0;
        }
        let p = wmimePushLower(out, 0, _.main_type);
        p = writeStr("/", out, p);
        return wmimePushLower(out, p, _.subtype);
    }

    /// WHATWG MIME Sniffing §4.6 HTML MIME type (`text/html`).
    pub fn isHTML(): bool {
        return _.valid && eqIgnoreCase(_.main_type, "text") && eqIgnoreCase(_.subtype, "html");
    }

    /// WHATWG MIME Sniffing §4.6 XML MIME type (`text/xml`, `application/xml`, or `*/*+xml`).
    pub fn isXML(): bool {
        if (!_.valid) { return false; }
        if (wmimeEndsWithIgnoreCase(_.subtype, "+xml")) { return true; }
        if (eqIgnoreCase(_.subtype, "xml")) {
            return eqIgnoreCase(_.main_type, "text") || eqIgnoreCase(_.main_type, "application");
        }
        return false;
    }

    /// WHATWG MIME Sniffing §4.6 JavaScript MIME type.
    pub fn isJavaScript(): bool {
        if (!_.valid) { return false; }
        if (eqIgnoreCase(_.main_type, "text")) {
            return eqIgnoreCase(_.subtype, "javascript")
                || eqIgnoreCase(_.subtype, "ecmascript")
                || eqIgnoreCase(_.subtype, "javascript1.0")
                || eqIgnoreCase(_.subtype, "javascript1.5")
                || eqIgnoreCase(_.subtype, "jscript")
                || eqIgnoreCase(_.subtype, "livescript")
                || eqIgnoreCase(_.subtype, "x-ecmascript")
                || eqIgnoreCase(_.subtype, "x-javascript");
        }
        if (eqIgnoreCase(_.main_type, "application")) {
            return eqIgnoreCase(_.subtype, "ecmascript")
                || eqIgnoreCase(_.subtype, "javascript")
                || eqIgnoreCase(_.subtype, "x-ecmascript")
                || eqIgnoreCase(_.subtype, "x-javascript");
        }
        return false;
    }

    /// WHATWG MIME Sniffing §4.6 JSON MIME type (`application/json`, `text/json`, or `*/*+json`).
    pub fn isJSON(): bool {
        if (!_.valid) { return false; }
        if (wmimeEndsWithIgnoreCase(_.subtype, "+json")) { return true; }
        if (eqIgnoreCase(_.subtype, "json")) {
            return eqIgnoreCase(_.main_type, "application") || eqIgnoreCase(_.main_type, "text");
        }
        return false;
    }

    /// Serializes per WHATWG §4.5 (`<type>/<subtype>;<k>=<v>`), quoting values that need quotes.
    pub fn serialize(out: []u8): usize {
        if (!_.valid) {
            return 0;
        }
        let p = _.essence(out);
        let i: usize = 0;
        while (i < _.param_count) {
            p = writeStr(";", out, p);
            p = wmimePushLower(out, p, _.params[i].name);
            p = writeStr("=", out, p);
            const val = _.params[i].value;
            let needs_quote = val.len == 0;
            let k: usize = 0;
            while (k < val.len) {
                if (!wmimeIsTokenChar(val[k])) {
                    needs_quote = true;
                    break;
                }
                k += 1;
            }
            if (needs_quote) {
                p = writeStr("\"", out, p);
                let q: usize = 0;
                while (q < val.len && p < out.len) {
                    if (val[q] == 34 || val[q] == 92) {
                        p = writeStr("\\", out, p);
                    }
                    if (p < out.len) {
                        out[p] = val[q];
                        p += 1;
                    }
                    q += 1;
                }
                p = writeStr("\"", out, p);
            } else {
                p = writeStr(val, out, p);
            }
            i += 1;
        }
        return p;
    }
}

/// Parses a MIME type string per WHATWG MIME Sniffing Standard §4.4.
pub fn parseMimeType(input: string): MimeType {
    const empty_params: [8]MimeParam = undefined;
    let invalid = MimeType { valid: false, main_type: "", subtype: "", params: empty_params, param_count: 0 };

    let a: usize = 0;
    let b: usize = input.len;
    while (a < b && wmimeIsHttpWs(input[a])) { a += 1; }
    while (b > a && wmimeIsHttpWs(input[b - 1])) { b -= 1; }
    if (a >= b) { return invalid; }

    // 1. Parse main type up to '/'
    let pos = a;
    while (pos < b && input[pos] != 47) {
        if (!wmimeIsTokenChar(input[pos])) { return invalid; }
        pos += 1;
    }
    if (pos == a || pos >= b) { return invalid; }
    const main_type = input[a..pos];
    pos += 1; // skip '/'

    // 2. Parse subtype up to ';'
    const sub_start = pos;
    while (pos < b && input[pos] != 59) {
        pos += 1;
    }
    let sub_end = pos;
    while (sub_end > sub_start && wmimeIsHttpWs(input[sub_end - 1])) { sub_end -= 1; }
    if (sub_end == sub_start) { return invalid; }
    let sidx = sub_start;
    while (sidx < sub_end) {
        if (!wmimeIsTokenChar(input[sidx])) { return invalid; }
        sidx += 1;
    }
    const subtype = input[sub_start..sub_end];

    let res = MimeType {
        valid: true,
        main_type: main_type,
        subtype: subtype,
        params: empty_params,
        param_count: 0,
    };

    // 3. Parse parameters
    while (pos < b) {
        pos += 1; // skip ';'
        while (pos < b && wmimeIsHttpWs(input[pos])) { pos += 1; }
        const name_start = pos;
        while (pos < b && input[pos] != 59 && input[pos] != 61) {
            pos += 1;
        }
        const name_end = pos;
        if (pos >= b || input[pos] == 59) {
            continue;
        }
        pos += 1; // skip '='
        if (pos >= b) {
            break;
        }
        let val_start = pos;
        let val_end = pos;
        if (input[pos] == 34) { // quoted-string
            pos += 1;
            val_start = pos;
            while (pos < b) {
                if (input[pos] == 92 && pos + 1 < b) {
                    pos += 2;
                    continue;
                }
                if (input[pos] == 34) {
                    val_end = pos;
                    pos += 1;
                    break;
                }
                pos += 1;
            }
            if (val_end < val_start) { val_end = pos; }
            while (pos < b && input[pos] != 59) { pos += 1; }
        } else {
            val_start = pos;
            while (pos < b && input[pos] != 59) { pos += 1; }
            val_end = pos;
            while (val_end > val_start && wmimeIsHttpWs(input[val_end - 1])) { val_end -= 1; }
        }

        // Validate parameter name (non-empty token chars) & ignore duplicate parameter names (WHATWG §4.4)
        if (name_end > name_start && val_end >= val_start) {
            let name_ok = true;
            let nk = name_start;
            while (nk < name_end) {
                if (!wmimeIsTokenChar(input[nk])) {
                    name_ok = false;
                    break;
                }
                nk += 1;
            }
            if (name_ok && res.param_count < WMIME_MAX_PARAMS) {
                const pname = input[name_start..name_end];
                if (res.getParam(pname) == null) {
                    res.params[res.param_count] = MimeParam { name: pname, value: input[val_start..val_end] };
                    res.param_count += 1;
                }
            }
        }
    }
    return res;
}

export {
    WMIME_MAX_PARAMS,
    MimeParam,
    MimeType,
    parseMimeType,
};
