// @cap/mailto-uri — Zero-allocation RFC 6068 mailto: URI Parser & Builder
// Clean-room MIT implementation in pure CP.

import { eqIgnoreCase, isAlnum, hexCharUpper, findByte } from "cap:ascii";
import { writeStr } from "cap:fmt";

pub struct MailtoUri {
    pub to: string,
    pub cc: string,
    pub bcc: string,
    pub subject: string,
    pub body: string,
    pub valid: bool,
}

fn invalidMailto() -> MailtoUri {
    return MailtoUri {
        to: "",
        cc: "",
        bcc: "",
        subject: "",
        body: "",
        valid: false,
    };
}

fn mailtoIsSpace(b: u8) -> bool {
    return b == 32 || b == 9;
}

fn trimSlice(s: string) -> string {
    let a: usize = 0;
    let b: usize = s.len;
    while (a < b && mailtoIsSpace(s[a])) {
        a += 1;
    }
    while (b > a && mailtoIsSpace(s[b - 1])) {
        b -= 1;
    }
    return s[a..b];
}

/// Parses an RFC 6068 `mailto:` URI (zero-copy).
/// Matches `mailto:` case-insensitively and extracts `to`, `cc`, `bcc`, `subject`, and `body`.
pub fn parseMailto(uri: string) -> MailtoUri {
    const s = trimSlice(uri);
    if (s.len < 7 || !eqIgnoreCase(s[0..7], "mailto:")) {
        return invalidMailto();
    }

    const rest = s[7..s.len];
    const q_idx: usize = findByte(rest, 63, 0) ?? rest.len;

    let res = invalidMailto();
    res.to = trimSlice(rest[0..q_idx]);

    let has_query_field = false;
    if (q_idx < rest.len) {
        const query = rest[q_idx + 1..rest.len];
        let pos: usize = 0;
        while (pos < query.len) {
            const start = pos;
            while (pos < query.len && query[pos] != 38) {
                pos += 1; // '&'
            }
            const pair = query[start..pos];
            if (pos < query.len && query[pos] == 38) {
                pos += 1;
            }
            let eq: usize = 0;
            while (eq < pair.len && pair[eq] != 61) {
                eq += 1; // '='
            }
            if (eq < pair.len && eq > 0) {
                const k = trimSlice(pair[0..eq]);
                const v = pair[eq + 1..pair.len];
                if (eqIgnoreCase(k, "subject")) {
                    res.subject = v;
                    has_query_field = true;
                } else if (eqIgnoreCase(k, "cc")) {
                    res.cc = v;
                    has_query_field = true;
                } else if (eqIgnoreCase(k, "bcc")) {
                    res.bcc = v;
                    has_query_field = true;
                } else if (eqIgnoreCase(k, "body")) {
                    res.body = v;
                    has_query_field = true;
                } else if (eqIgnoreCase(k, "to")) {
                    if (res.to.len == 0) {
                        res.to = v;
                    }
                    has_query_field = true;
                }
            }
        }
    }

    res.valid = res.to.len > 0 || has_query_field;
    return res;
}

/// Counts non-empty comma-separated recipients in `to_list`.
pub fn countRecipients(to_list: string) -> usize {
    let pos: usize = 0;
    const n = to_list.len;
    let count: usize = 0;
    while (pos < n) {
        const start = pos;
        while (pos < n && to_list[pos] != 44) {
            pos += 1; // ','
        }
        const item = trimSlice(to_list[start..pos]);
        if (item.len > 0) {
            count += 1;
        }
        if (pos < n && to_list[pos] == 44) {
            pos += 1;
        }
    }
    return count;
}

/// Lightweight email syntax check: single `@`, non-empty local part, domain with `.` (not leading/trailing), no whitespace.
pub fn isBasicEmail(addr: string) -> bool {
    if (addr.len < 3) {
        return false;
    }
    let at_pos: usize = addr.len;
    let i: usize = 0;
    while (i < addr.len) {
        const ch = addr[i];
        if (ch <= 32 || ch >= 127) {
            return false;
        }
        if (ch == 64) {
            if (at_pos != addr.len) {
                return false; // multiple '@'
            }
            at_pos = i;
        }
        i += 1;
    }
    if (at_pos == 0 || at_pos + 1 >= addr.len) {
        return false;
    }
    const dom_start = at_pos + 1;
    const dom_len = addr.len - dom_start;
    if (dom_len < 3 || addr[dom_start] == 46 || addr[addr.len - 1] == 46) {
        return false;
    }
    let has_dot = false;
    let j = dom_start;
    while (j < addr.len) {
        if (addr[j] == 46) {
            has_dot = true;
            if (j + 1 < addr.len && addr[j + 1] == 46) {
                return false; // consecutive '..'
            }
        }
        j += 1;
    }
    return has_dot;
}

fn isUnreserved(b: u8) -> bool {
    if (isAlnum(b)) { return true; }
    return b == 45 || b == 46 || b == 95 || b == 126; // - . _ ~
}

fn pushSlice(dst: []u8, pos: usize, src: string) -> i64 {
    if (pos + src.len > dst.len) {
        return -1;
    }
    return writeStr(src, dst, pos) as i64;
}

fn pushPctEncoded(dst: []u8, pos: usize, src: string, allow_addr_chars: bool) -> i64 {
    let w = pos;
    let i: usize = 0;
    while (i < src.len) {
        const b = src[i];
        const keep = isUnreserved(b) || (allow_addr_chars && (b == 64 || b == 44));
        if (keep) {
            if (w >= dst.len) { return -1; }
            dst[w] = b;
            w += 1;
        } else {
            if (w + 3 > dst.len) { return -1; }
            dst[w] = 37; // '%'
            dst[w + 1] = hexCharUpper((b >> 4) & 0x0F);
            dst[w + 2] = hexCharUpper(b & 0x0F);
            w += 3;
        }
        i += 1;
    }
    return w as i64;
}

/// Formats an RFC 6068 `mailto:` URI into `out`, percent-encoding `subject`, `body`, and `cc`.
/// Returns bytes written, or `0` if all fields are empty or `out` is too small.
pub fn formatMailto(to: string, subject: string, body: string, cc: string, out: []u8) -> usize {
    if (to.len == 0 && subject.len == 0 && body.len == 0 && cc.len == 0) {
        return 0;
    }
    let p = pushSlice(out, 0, "mailto:");
    if (p < 0) { return 0; }
    if (to.len > 0) {
        p = pushSlice(out, p as usize, to);
        if (p < 0) { return 0; }
    }

    let first_q = true;
    if (subject.len > 0) {
        p = pushSlice(out, p as usize, "?subject=");
        if (p < 0) { return 0; }
        p = pushPctEncoded(out, p as usize, subject, false);
        if (p < 0) { return 0; }
        first_q = false;
    }
    if (body.len > 0) {
        if (first_q) {
            p = pushSlice(out, p as usize, "?body=");
        } else {
            p = pushSlice(out, p as usize, "&body=");
        }
        if (p < 0) { return 0; }
        p = pushPctEncoded(out, p as usize, body, false);
        if (p < 0) { return 0; }
        first_q = false;
    }
    if (cc.len > 0) {
        if (first_q) {
            p = pushSlice(out, p as usize, "?cc=");
        } else {
            p = pushSlice(out, p as usize, "&cc=");
        }
        if (p < 0) { return 0; }
        p = pushPctEncoded(out, p as usize, cc, true);
        if (p < 0) { return 0; }
    }

    return p as usize;
}
