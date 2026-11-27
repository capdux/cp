// @cap/set-cookie-parser — RFC 6265 Set-Cookie HTTP header parser in pure CP
// Clean-room MIT implementation of npm `set-cookie-parser` (#537, 107M/wk).
// Accelerated with 16-lane @Vector(16, u8) SIMD delimiter scanning for `;`, `=`, and `,`.

import { eqIgnoreCase, trim } from "cap:ascii";
import { parseI64 } from "cap:fmt";

type ScpVec16 = @Vector(16, u8);

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

fn scpFindByte(s: string, target: u8, start: usize): usize {
    let i = start;
    const len = s.len;
    const vt: ScpVec16 = @splat(target);
    while (i + 16 <= len) {
        const v: ScpVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == vt)) {
            let k: usize = 0;
            while (k < 16) {
                if (s[i + k] == target) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len && s[i] != target) {
        i += 1;
    }
    return i;
}

fn scpIsNewCookieAfterComma(t: string, comma_pos: usize): bool {
    let j = comma_pos + 1;
    const len = t.len;
    const v_eq: ScpVec16 = @splat(61 as u8);
    const v_semi: ScpVec16 = @splat(59 as u8);
    const v_comma: ScpVec16 = @splat(44 as u8);
    while (j + 16 <= len) {
        const v: ScpVec16 = t[j .. j + 16][0..16].*;
        if (@reduce(.Or, (v == v_eq) | (v == v_semi) | (v == v_comma))) {
            let k: usize = 0;
            while (k < 16) {
                const c = t[j + k];
                if (c == 61) {
                    return true;
                }
                if (c == 59 || c == 44) {
                    return false;
                }
                k += 1;
            }
        }
        j += 16;
    }
    while (j < len) {
        const c = t[j];
        if (c == 61) {
            return true;
        }
        if (c == 59 || c == 44) {
            return false;
        }
        j += 1;
    }
    return false;
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
    const first_semi = scpFindByte(trimmed, 59, 0);
    const first_part = trimmed[0..first_semi];
    let pos: usize = if (first_semi < trimmed.len) { first_semi + 1 } else { trimmed.len };

    const eq = scpFindByte(first_part, 61, 0);
    if (eq < first_part.len) {
        out.name = trim(first_part[0..eq]);
        out.value = scpStripQuotes(trim(first_part[(eq + 1)..first_part.len]));
    } else {
        out.name = trim(first_part);
    }

    // Parse remaining ';'-delimited attributes
    while (pos < trimmed.len) {
        const next_semi = scpFindByte(trimmed, 59, pos);
        const seg = trim(trimmed[pos..next_semi]);
        pos = if (next_semi < trimmed.len) { next_semi + 1 } else { trimmed.len };

        if (seg.len == 0) {
            continue;
        }

        const aeq = scpFindByte(seg, 61, 0);
        if (aeq < seg.len) {
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
        const comma = scpFindByte(t, 44, i);
        if (comma >= t.len) {
            break;
        }
        if (scpIsNewCookieAfterComma(t, comma)) {
            count += 1;
        }
        i = comma + 1;
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
        const comma = scpFindByte(t, 44, i);
        if (comma >= t.len) {
            break;
        }
        if (scpIsNewCookieAfterComma(t, comma)) {
            if (cur_idx == target_idx) {
                return trim(t[seg_start..comma]);
            }
            cur_idx += 1;
            seg_start = comma + 1;
        }
        i = comma + 1;
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
