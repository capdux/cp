// humanize-url — Zero-allocation & zero-copy URL humanizer for CP
// Clean-room MIT implementation with parity to npm `humanize-url` v3.

import { isAlpha, isAlnum, toLowerByte, trim, eqIgnoreCase } from "cap:ascii";
import { writeStr } from "cap:fmt";

fn hurlStartsWithLower(s: string, lower_prefix: string): bool {
    if (s.len < lower_prefix.len) {
        return false;
    }
    return eqIgnoreCase(s[0..lower_prefix.len], lower_prefix);
}

fn hurlHasDot(s: string): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '.') {
            return true;
        }
        if (c == '/' || c == '?' || c == '#' || c == ':') {
            return false;
        }
    }
    return false;
}

/// Returns true if `url` (after trimming whitespace) starts with protocol-relative `//`.
pub fn isProtocolRelative(url: string): bool {
    const s = trim(url);
    return s.len >= 2 && s[0] == '/' && s[1] == '/';
}

/// Extracts the scheme prefix before `://` (e.g. `"https"`, `"http"`, `"ftp"`), or `""` if absent.
pub fn extractScheme(url: string): string {
    const s = trim(url);
    if (s.len < 4 || !isAlpha(s[0])) {
        return "";
    }
    let i: usize = 1;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == ':') {
            if (i + 2 < s.len && s[i + 1] == '/' && s[i + 2] == '/') {
                return s[0..i];
            }
            return "";
        }
        if (!(isAlnum(c) || c == '+' || c == '-' || c == '.')) {
            return "";
        }
    }
    return "";
}

/// Strips leading `https://`, `http://` (case-insensitive), or protocol-relative `//`.
pub fn stripProtocol(url: string): string {
    const s = trim(url);
    if (hurlStartsWithLower(s, "https://")) {
        return s[8..s.len];
    }
    if (hurlStartsWithLower(s, "http://")) {
        return s[7..s.len];
    }
    if (s.len >= 2 && s[0] == '/' && s[1] == '/') {
        return s[2..s.len];
    }
    return s;
}

/// Strips case-insensitive leading `www.` only when the remaining host contains a dot
/// (so bare domains like `www.com` or `www.com:8080` stay intact).
pub fn stripWww(host_and_rest: string): string {
    if (hurlStartsWithLower(host_and_rest, "www.")) {
        const after_www = host_and_rest[4..host_and_rest.len];
        if (hurlHasDot(after_www)) {
            return after_www;
        }
    }
    return host_and_rest;
}

/// Strips a single trailing `/` if present.
pub fn stripTrailingSlash(url: string): string {
    if (url.len > 0 && url[url.len - 1] == '/') {
        return url[0..url.len - 1];
    }
    return url;
}

/// Zero-copy humanize: trims whitespace, strips `http://`, `https://`, or `//`,
/// strips case-insensitive leading `www.` (preserving bare domains like `www.com`),
/// and strips a trailing `/` while preserving paths, query strings, ports, and fragments.
pub fn humanizeUrlSlice(url: string): string {
    let s = trim(url);
    if (s.len == 0) {
        return "";
    }

    s = stripProtocol(s);
    if (s.len == 0) {
        return "";
    }

    s = stripWww(s);
    s = stripTrailingSlash(s);
    return s;
}

/// Returns the exact byte length of the humanized URL.
pub fn humanizedLen(url: string): usize {
    return humanizeUrlSlice(url).len;
}

/// Humanize `url` and write the result into `out`, returning the number of bytes written.
pub fn humanizeUrl(url: string, out: []u8): usize {
    const res = humanizeUrlSlice(url);
    if (res.len == 0 || out.len == 0) {
        return 0;
    }
    return writeStr(res, out, 0);
}
