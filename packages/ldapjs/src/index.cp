// @cap/ldap-dn — Zero-heap RFC 4514 LDAP Distinguished Name parser & escaper
// MIT License (clean-room CP implementation)

import { isAlpha, isAlnum, isDigit, isHexDigit, eqIgnoreCase, hexVal } from "cap:ascii";

pub struct RdnAttr {
    pub attr: string,
    pub raw_value: string,
}

fn isWs(c: u8) -> bool {
    return c == ' ' as u8 || c == '\t' as u8;
}

fn isValidAttrType(attr: string) -> bool {
    if (attr.len == 0) {
        return false;
    }
    const c0: u8 = attr[0];
    // Descr: [A-Za-z][A-Za-z0-9-]*
    if (isAlpha(c0)) {
        let i: usize = 1;
        while (i < attr.len) {
            const c: u8 = attr[i];
            if (!isAlnum(c) && c != '-' as u8) {
                return false;
            }
            i += 1;
        }
        return true;
    }
    // Numeric OID: [0-9]+(\.[0-9]+)*
    if (isDigit(c0)) {
        if (attr[attr.len - 1] == '.' as u8) {
            return false;
        }
        let i: usize = 1;
        while (i < attr.len) {
            const c: u8 = attr[i];
            if (!(isDigit(c) || c == '.' as u8)) {
                return false;
            }
            if (c == '.' as u8 && attr[i - 1] == '.' as u8) {
                return false;
            }
            i += 1;
        }
        return true;
    }
    return false;
}

fn isEscapedAt(s: string, start: usize, idx: usize) -> bool {
    let bs: usize = 0;
    let k: usize = idx;
    while (k > start) {
        k -= 1;
        if (s[k] == '\\' as u8) {
            bs += 1;
        } else {
            break;
        }
    }
    return (bs % 2) == 1;
}

fn hasValidEscapes(raw: string) -> bool {
    let i: usize = 0;
    while (i < raw.len) {
        if (raw[i] == '\\' as u8) {
            if (i + 1 >= raw.len) {
                return false;
            }
            if (i + 2 < raw.len && isHexDigit(raw[i + 1]) && isHexDigit(raw[i + 2])) {
                i += 3;
            } else {
                i += 2;
            }
        } else {
            i += 1;
        }
    }
    return true;
}

pub fn parseDn(dn: string, out: []RdnAttr) -> usize {
    if (dn.len == 0) {
        return 0;
    }

    let pos: usize = 0;
    let stored: usize = 0;
    let total: usize = 0;

    while (pos <= dn.len) {
        // Scan until unescaped ',' or end of string
        let seg_end: usize = pos;
        while (seg_end < dn.len) {
            if (dn[seg_end] == '\\' as u8) {
                if (seg_end + 1 >= dn.len) {
                    return 0;
                }
                seg_end += 2;
                continue;
            }
            if (dn[seg_end] == ',' as u8) {
                break;
            }
            seg_end += 1;
        }

        // Trim leading whitespace
        let s: usize = pos;
        let e: usize = seg_end;
        while (s < e && isWs(dn[s])) {
            s += 1;
        }
        // Trim trailing unescaped whitespace
        while (e > s && isWs(dn[e - 1]) && !isEscapedAt(dn, s, e - 1)) {
            e -= 1;
        }
        if (s >= e) {
            return 0;
        }

        // Find first unescaped '='
        let eq: usize = s;
        while (eq < e && dn[eq] != '=' as u8) {
            if (dn[eq] == '\\' as u8) {
                return 0;
            }
            eq += 1;
        }
        if (eq >= e) {
            return 0;
        }

        let k_end: usize = eq;
        while (k_end > s && isWs(dn[k_end - 1])) {
            k_end -= 1;
        }
        const attr: string = dn[s..k_end];
        if (!isValidAttrType(attr)) {
            return 0;
        }

        let v_s: usize = eq + 1;
        while (v_s < e && isWs(dn[v_s])) {
            v_s += 1;
        }
        const raw_val: string = dn[v_s..e];
        if (!hasValidEscapes(raw_val)) {
            return 0;
        }

        if (stored < out.len) {
            out[stored] = RdnAttr { attr: attr, raw_value: raw_val };
            stored += 1;
        }
        total += 1;

        if (seg_end >= dn.len) {
            break;
        }
        // Must have another segment after ',' (no trailing comma)
        pos = seg_end + 1;
        if (pos >= dn.len) {
            return 0;
        }
    }

    if (total == 0) {
        return 0;
    }
    return stored;
}

pub fn escapeValue(value: string, out: []u8) -> usize {
    if (value.len == 0) {
        return 0;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < value.len) {
        const c: u8 = value[i];
        if (c == 0) {
            if (pos + 3 > out.len) {
                return 0;
            }
            out[pos] = '\\' as u8;
            out[pos + 1] = '0' as u8;
            out[pos + 2] = '0' as u8;
            pos += 3;
            i += 1;
            continue;
        }

        let needs_esc: bool = c == ',' as u8 || c == '+' as u8 || c == '"' as u8
            || c == '\\' as u8 || c == '<' as u8 || c == '>' as u8 || c == ';' as u8;
        if (i == 0 && (c == ' ' as u8 || c == '#' as u8)) {
            needs_esc = true;
        }
        if (i == value.len - 1 && c == ' ' as u8) {
            needs_esc = true;
        }

        if (needs_esc) {
            if (pos + 2 > out.len) {
                return 0;
            }
            out[pos] = '\\' as u8;
            out[pos + 1] = c;
            pos += 2;
        } else {
            if (pos >= out.len) {
                return 0;
            }
            out[pos] = c;
            pos += 1;
        }
        i += 1;
    }
    return pos;
}

pub fn unescapeValue(raw: string, out: []u8) -> usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < raw.len) {
        const c: u8 = raw[i];
        if (c == '\\' as u8) {
            if (i + 1 >= raw.len) {
                return 0;
            }
            if (i + 2 < raw.len) {
                const h0 = hexVal(raw[i + 1]);
                const h1 = hexVal(raw[i + 2]);
                if (h0 != null && h1 != null) {
                    if (pos >= out.len) {
                        return 0;
                    }
                    out[pos] = (((h0 ?? (0 as u8)) << 4) | (h1 ?? (0 as u8)));
                    pos += 1;
                    i += 3;
                    continue;
                }
            }
            if (pos >= out.len) {
                return 0;
            }
            out[pos] = raw[i + 1];
            pos += 1;
            i += 2;
        } else {
            if (pos >= out.len) {
                return 0;
            }
            out[pos] = c;
            pos += 1;
            i += 1;
        }
    }
    return pos;
}

pub fn findAttr(dn: string, target_attr: string, out: []u8, out_len: *usize) -> bool {
    out_len.* = 0;
    if (target_attr.len == 0) {
        return false;
    }
    let attrs: [32]RdnAttr = undefined;
    const n: usize = parseDn(dn, attrs[0..32]);
    if (n == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < n) {
        if (eqIgnoreCase(attrs[i].attr, target_attr)) {
            if (attrs[i].raw_value.len == 0) {
                out_len.* = 0;
                return true;
            }
            const u_len: usize = unescapeValue(attrs[i].raw_value, out);
            if (u_len == 0) {
                return false;
            }
            out_len.* = u_len;
            return true;
        }
        i += 1;
    }
    return false;
}

pub fn isValidDn(dn: string) -> bool {
    let attrs: [32]RdnAttr = undefined;
    return parseDn(dn, attrs[0..32]) > 0;
}
