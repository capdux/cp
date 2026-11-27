import { trim, strEq } from "cap:ascii";
import { getCookieHeaderValue } from "../../tough-cookie/src/index.cp";
import { sign, unsign } from "../../cookie-signature/src/index.cp";

pub const MAX_PARSED_COOKIES: usize = 32;

pub struct CookieEntry {
    pub name: string,
    pub value: string,
}

fn cprStartsWith2(s: string, c0: u8, c1: u8): bool {
    return s.len >= 2 && s[0] == c0 && s[1] == c1;
}

pub fn isSignedCookie(val: string): bool {
    return cprStartsWith2(val, 115, 58); // "s:"
}

pub fn isJsonCookie(val: string): bool {
    return cprStartsWith2(val, 106, 58); // "j:"
}

pub fn stripJsonCookiePrefix(val: string): string? {
    if (!isJsonCookie(val)) {
        return null;
    }
    return val[2..val.len];
}

pub fn signCookieValue(val: string, secret: string, out: []u8): usize {
    if (out.len < 2) {
        return 0;
    }
    out[0] = 115; // 's'
    out[1] = 58;  // ':'
    let sig_len = sign(val, secret, out[2..out.len]);
    if (sig_len == 0) {
        return 0;
    }
    return 2 + sig_len;
}

pub fn signedCookie(val: string, secret: string): string? {
    if (!isSignedCookie(val)) {
        return val;
    }
    let payload = val[2..val.len];
    let res = unsign(payload, secret);
    if (!res.ok) {
        return null;
    }
    return payload[0..res.val_len];
}

pub fn signedCookieWithSecrets(val: string, secrets: [4]string, secret_count: usize): string? {
    if (!isSignedCookie(val)) {
        return val;
    }
    let payload = val[2..val.len];
    let limit = if (secret_count > 4) { 4 } else { secret_count };
    let i: usize = 0;
    while (i < limit) {
        let res = unsign(payload, secrets[i]);
        if (res.ok) {
            return payload[0..res.val_len];
        }
        i = i + 1;
    }
    return null;
}

pub fn lookupRawCookie(header: string, name: string): string? {
    return getCookieHeaderValue(header, name);
}

pub struct ParsedCookies {
    pub cookies: [32]CookieEntry,
    pub cookie_count: usize,
    pub signed_cookies: [32]CookieEntry,
    pub signed_count: usize,

    pub static fn empty(): ParsedCookies {
        let blank = CookieEntry { name: "", value: "" };
        let c_arr: [32]CookieEntry = [blank; 32];
        let s_arr: [32]CookieEntry = [blank; 32];
        return ParsedCookies {
            cookies: c_arr,
            cookie_count: 0,
            signed_cookies: s_arr,
            signed_count: 0,
        };
    }

    pub fn get(name: string): string? {
        let i: usize = 0;
        while (i < _.cookie_count) {
            if (strEq(_.cookies[i].name, name)) {
                return _.cookies[i].value;
            }
            i = i + 1;
        }
        return null;
    }

    pub fn getSigned(name: string): string? {
        let i: usize = 0;
        while (i < _.signed_count) {
            if (strEq(_.signed_cookies[i].name, name)) {
                return _.signed_cookies[i].value;
            }
            i = i + 1;
        }
        return null;
    }

    pub fn totalCount(): usize {
        return _.cookie_count + _.signed_count;
    }
}

fn cprParseInto(header: string, secrets: [4]string, secret_count: usize): ParsedCookies {
    let parsed = ParsedCookies.empty();
    let i: usize = 0;
    while (i < header.len) {
        let start = i;
        while (i < header.len && header[i] != 59) { // ';'
            i = i + 1;
        }
        let seg = trim(header[start..i]);
        if (i < header.len && header[i] == 59) {
            i = i + 1;
        }
        if (seg.len == 0) {
            continue;
        }
        let eq_pos: usize = 0;
        let found_eq = false;
        while (eq_pos < seg.len) {
            if (seg[eq_pos] == 61) { // '='
                found_eq = true;
                break;
            }
            eq_pos = eq_pos + 1;
        }
        if (!found_eq || eq_pos == 0) {
            continue;
        }
        let k = trim(seg[0..eq_pos]);
        let v = trim(seg[(eq_pos + 1)..seg.len]);
        if (v.len >= 2 && v[0] == 34 && v[v.len - 1] == 34) { // strip optional quotes
            v = v[1..(v.len - 1)];
        }
        if (secret_count > 0 && isSignedCookie(v)) {
            let verified = signedCookieWithSecrets(v, secrets, secret_count);
            if (verified != null) {
                if (parsed.signed_count < MAX_PARSED_COOKIES) {
                    parsed.signed_cookies[parsed.signed_count] = CookieEntry {
                        name: k,
                        value: verified ?? "",
                    };
                    parsed.signed_count = parsed.signed_count + 1;
                }
            }
        } else {
            if (parsed.cookie_count < MAX_PARSED_COOKIES) {
                parsed.cookies[parsed.cookie_count] = CookieEntry {
                    name: k,
                    value: v,
                };
                parsed.cookie_count = parsed.cookie_count + 1;
            }
        }
    }
    return parsed;
}

pub fn parseCookieHeader(header: string, secret: string): ParsedCookies {
    let secrets: [4]string = [secret, "", "", ""];
    let cnt: usize = if (secret.len > 0) { 1 } else { 0 };
    return cprParseInto(header, secrets, cnt);
}

pub fn parseCookieHeaderWithSecrets(header: string, secrets: [4]string, secret_count: usize): ParsedCookies {
    return cprParseInto(header, secrets, secret_count);
}
