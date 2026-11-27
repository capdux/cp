// @cap/set-cookie-parser — RFC 6265 Set-Cookie HTTP header parser in pure CP
// Clean-room MIT implementation of npm `set-cookie-parser` (#537, 107M/wk).

import { eqIgnoreCase, trim, findByte } from "cap:ascii";
import { parseI64 } from "cap:fmt";

pub struct ParsedCookie {
    pub name: string,
    pub value: string,
    pub path: string,
    pub domain: string,
    pub expires: string,
    pub same_site: string,
    pub max_age: i64,
    pub has_max_age: bool,
    pub secure: bool,
    pub http_only: bool,
    pub partitioned: bool,
}

fn scpStripQuotes(s: string): string {
    if (s.len >= 2 && s[0] == 34 && s[s.len - 1] == 34) {
        return s[1..(s.len - 1)];
    }
    return s;
}

/// Parses a single `Set-Cookie` header value into a `ParsedCookie`.
pub fn parseSetCookie(header: string): ParsedCookie {
    let out = ParsedCookie {
        name: "",
        value: "",
        path: "",
        domain: "",
        expires: "",
        same_site: "",
        max_age: 0,
        has_max_age: false,
        secure: false,
        http_only: false,
        partitioned: false,
    };

    const trimmed = trim(header);
    if (trimmed.len == 0) {
        return out;
    }

    // Split name=value pair before first ';'
    let pos: usize = 0;
    const first_semi = findByte(trimmed, 59, 0);
    let first_part = trimmed;
    if (first_semi != null) {
        const idx: usize = first_semi ?? (0 as usize);
        first_part = trimmed[0..idx];
        pos = idx + 1;
    } else {
        pos = trimmed.len;
    }

    const eq_opt = findByte(first_part, 61, 0);
    if (eq_opt != null) {
        const eq: usize = eq_opt ?? (0 as usize);
        out.name = trim(first_part[0..eq]);
        out.value = scpStripQuotes(trim(first_part[(eq + 1)..first_part.len]));
    } else {
        out.name = trim(first_part);
    }

    // Parse remaining ';'-delimited attributes
    while (pos < trimmed.len) {
        const next_semi = findByte(trimmed, 59, pos);
        let seg = "";
        if (next_semi != null) {
            const idx: usize = next_semi ?? trimmed.len;
            seg = trim(trimmed[pos..idx]);
            pos = idx + 1;
        } else {
            seg = trim(trimmed[pos..trimmed.len]);
            pos = trimmed.len;
        }

        if (seg.len == 0) {
            continue;
        }

        const attr_eq = findByte(seg, 61, 0);
        if (attr_eq != null) {
            const aeq: usize = attr_eq ?? (0 as usize);
            const key = trim(seg[0..aeq]);
            const val = trim(seg[(aeq + 1)..seg.len]);
            if (eqIgnoreCase(key, "path")) {
                out.path = val;
            } else if (eqIgnoreCase(key, "domain")) {
                out.domain = val;
            } else if (eqIgnoreCase(key, "expires")) {
                out.expires = val;
            } else if (eqIgnoreCase(key, "samesite")) {
                out.same_site = val;
            } else if (eqIgnoreCase(key, "max-age")) {
                const parsed_age = parseI64(val);
                if (parsed_age != null) {
                    out.max_age = parsed_age ?? 0;
                    out.has_max_age = true;
                }
            }
        } else {
            if (eqIgnoreCase(seg, "secure")) {
                out.secure = true;
            } else if (eqIgnoreCase(seg, "httponly")) {
                out.http_only = true;
            } else if (eqIgnoreCase(seg, "partitioned")) {
                out.partitioned = true;
            }
        }
    }

    return out;
}

/// Counts individual Set-Cookie headers inside a comma-joined `Set-Cookie` string,
/// properly ignoring commas inside `Expires=Wed, 21 Oct 2015 07:28:00 GMT` dates.
pub fn splitCookiesCount(combined: string): usize {
    const t = trim(combined);
    if (t.len == 0) {
        return 0;
    }
    let count: usize = 1;
    let i: usize = 0;
    while (i < t.len) {
        if (t[i] == 44) { // ','
            // Look ahead: if we find '=' before ';' or ',', this comma starts a new cookie
            let j = i + 1;
            let is_new_cookie = false;
            while (j < t.len) {
                const c = t[j];
                if (c == 61) { // '='
                    is_new_cookie = true;
                    break;
                }
                if (c == 59 || c == 44) { // ';' or ','
                    break;
                }
                j += 1;
            }
            if (is_new_cookie) {
                count += 1;
            }
        }
        i += 1;
    }
    return count;
}

/// Extracts the `target_idx`-th (0-based) cookie header from a comma-joined `Set-Cookie` string.
pub fn splitCookiesGet(combined: string, target_idx: usize): string? {
    const t = trim(combined);
    if (t.len == 0) {
        return null;
    }
    let cur_idx: usize = 0;
    let seg_start: usize = 0;
    let i: usize = 0;
    while (i < t.len) {
        if (t[i] == 44) {
            let j = i + 1;
            let is_new_cookie = false;
            while (j < t.len) {
                const c = t[j];
                if (c == 61) {
                    is_new_cookie = true;
                    break;
                }
                if (c == 59 || c == 44) {
                    break;
                }
                j += 1;
            }
            if (is_new_cookie) {
                if (cur_idx == target_idx) {
                    return trim(t[seg_start..i]);
                }
                cur_idx += 1;
                seg_start = i + 1;
            }
        }
        i += 1;
    }
    if (cur_idx == target_idx) {
        return trim(t[seg_start..t.len]);
    }
    return null;
}

export {
    ParsedCookie,
    parseSetCookie,
    splitCookiesCount,
    splitCookiesGet,
};
