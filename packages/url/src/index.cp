// @cap/whatwg-url — Pure CP zero-allocation WHATWG URL Standard parser & resolver
// Clean-room MIT implementation (Rust `url` v2 / WHATWG URL Standard parity).
// Reuses @cap/percent-encoding and @cap/idna-uts46 with zero duplication.

import {
    userinfoSet,
    pathSet,
    querySet,
    fragmentSet,
    percentEncode,
    percentDecode
} from "../../percent-encoding/src/index.cp";
import { domainToAscii } from "../../idna/src/index.cp";
import { toLowerByte, isAlpha, isDigit, isHexDigit } from "cap:ascii";
import { bytesEq } from "cap:bin";

fn wuIsSchemeChar(ch: u8): bool {
    return isAlpha(ch) || isDigit(ch) || ch == 0x2B || ch == 0x2D || ch == 0x2E;
}

fn wuAppendBytes(out: []u8, pos: *usize, src: []const u8): bool {
    if (pos.* + src.len > out.len) {
        return false;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i = i + 1;
    }
    pos.* = pos.* + src.len;
    return true;
}

fn wuAppendByte(out: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out.len) {
        return false;
    }
    out[pos.*] = b;
    pos.* = pos.* + 1;
    return true;
}

fn wuAppendPort(out: []u8, pos: *usize, port: i32): bool {
    let tmp: [8]u8 = undefined;
    let n: usize = 0;
    let v: u32 = port as u32;
    if (v == 0) {
        return wuAppendByte(out, pos, 48);
    }
    while (v > 0) {
        tmp[n] = (48 + (v % 10)) as u8;
        v = v / 10;
        n = n + 1;
    }
    while (n > 0) {
        n = n - 1;
        if (!wuAppendByte(out, pos, tmp[n])) {
            return false;
        }
    }
    return true;
}

fn wuIsSpecialScheme(scheme: []const u8): bool {
    return bytesEq(scheme, "http")
        || bytesEq(scheme, "https")
        || bytesEq(scheme, "ws")
        || bytesEq(scheme, "wss")
        || bytesEq(scheme, "ftp")
        || bytesEq(scheme, "file");
}

fn wuIsOriginScheme(scheme: []const u8): bool {
    return bytesEq(scheme, "http")
        || bytesEq(scheme, "https")
        || bytesEq(scheme, "ws")
        || bytesEq(scheme, "wss")
        || bytesEq(scheme, "ftp");
}

fn wuIsForbiddenHostByte(b: u8): bool {
    if (b <= 0x20 || b == 0x7F) {
        return true;
    }
    return b == 0x23 || b == 0x2F || b == 0x3A || b == 0x3C
        || b == 0x3E || b == 0x3F || b == 0x40 || b == 0x5B
        || b == 0x5C || b == 0x5D || b == 0x5E || b == 0x7C;
}

fn wuIsSingleDotSegment(seg: []const u8): bool {
    if (seg.len == 1 && seg[0] == 0x2E) {
        return true;
    }
    if (seg.len == 3 && seg[0] == 0x25 && seg[1] == 0x32
        && (seg[2] == 0x65 || seg[2] == 0x45)) {
        return true;
    }
    return false;
}

fn wuIsDoubleDotSegment(seg: []const u8): bool {
    if (seg.len == 2 && seg[0] == 0x2E && seg[1] == 0x2E) {
        return true;
    }
    if (seg.len == 4) {
        if (seg[0] == 0x2E && seg[1] == 0x25 && seg[2] == 0x32
            && (seg[3] == 0x65 || seg[3] == 0x45)) {
            return true;
        }
        if (seg[0] == 0x25 && seg[1] == 0x32
            && (seg[2] == 0x65 || seg[2] == 0x45) && seg[3] == 0x2E) {
            return true;
        }
    }
    if (seg.len == 6 && seg[0] == 0x25 && seg[1] == 0x32
        && (seg[2] == 0x65 || seg[2] == 0x45) && seg[3] == 0x25
        && seg[4] == 0x32 && (seg[5] == 0x65 || seg[5] == 0x45)) {
        return true;
    }
    return false;
}

fn wuTrimBounds(input: string, out_start: *usize, out_end: *usize) {
    let s: usize = 0;
    let e: usize = input.len;
    while (s < e && input[s] <= 0x20) {
        s = s + 1;
    }
    while (e > s && input[e - 1] <= 0x20) {
        e = e - 1;
    }
    out_start.* = s;
    out_end.* = e;
}

/// Checks if `s` starts with a valid URL scheme followed by ':'.
/// Returns index of ':' if valid, or -1 otherwise.
fn wuFindSchemeColon(s: []const u8): i64 {
    if (s.len < 2 || !isAlpha(s[0])) {
        return -1;
    }
    let i: usize = 1;
    while (i < s.len) {
        let ch = s[i];
        if (ch == 0x3A) { // ':'
            return i as i64;
        }
        if (!wuIsSchemeChar(ch)) {
            return -1;
        }
        i = i + 1;
    }
    return -1;
}

/// Normalizes a hierarchical path (resolving '.' and '..' segments) and
/// percent-encodes segments with WHATWG `pathSet()`.
fn wuNormalizePath(raw: []const u8, is_special: bool, out: []u8): usize {
    let pset = pathSet();
    let seg_starts: [64]usize = undefined;
    let seg_lens: [64]usize = undefined;
    let seg_count: usize = 0;
    let enc_pool: [512]u8 = undefined;
    let pool_pos: usize = 0;

    let i: usize = 0;
    let trailing_slash = false;

    while (i < raw.len) {
        let ch = raw[i];
        if (ch == 0x2F || (is_special && ch == 0x5C)) {
            trailing_slash = true;
            i = i + 1;
            continue;
        }
        let seg_start = i;
        while (i < raw.len && raw[i] != 0x2F && !(is_special && raw[i] == 0x5C)) {
            i = i + 1;
        }
        let seg = raw[seg_start..i];
        if (wuIsSingleDotSegment(seg)) {
            if (i == raw.len) {
                trailing_slash = true;
            }
            continue;
        }
        if (wuIsDoubleDotSegment(seg)) {
            if (seg_count > 0) {
                seg_count = seg_count - 1;
            }
            if (i == raw.len) {
                trailing_slash = true;
            }
            continue;
        }
        trailing_slash = false;
        if (seg_count < 64 && pool_pos < 512) {
            let written = percentEncode(seg, pset, enc_pool[pool_pos..512]);
            seg_starts[seg_count] = pool_pos;
            seg_lens[seg_count] = written;
            pool_pos = pool_pos + written;
            seg_count = seg_count + 1;
        }
    }

    let pos: usize = 0;
    if (seg_count == 0) {
        if (pos < out.len) {
            out[pos] = 0x2F; // '/'
            pos = pos + 1;
        }
        return pos;
    }

    let k: usize = 0;
    while (k < seg_count) {
        if (!wuAppendByte(out, &pos, 0x2F)) {
            break;
        }
        let st = seg_starts[k];
        let ln = seg_lens[k];
        if (!wuAppendBytes(out, &pos, enc_pool[st..(st + ln)])) {
            break;
        }
        k = k + 1;
    }
    if (trailing_slash) {
        * = wuAppendByte(out, &pos, 0x2F);
    }
    return pos;
}

fn wuParsePortStr(port_str: []const u8): i32 {
    if (port_str.len == 0) {
        return -1;
    }
    let val: u32 = 0;
    let i: usize = 0;
    while (i < port_str.len) {
        let ch = port_str[i];
        if (!isDigit(ch)) {
            return -2;
        }
        val = val * 10 + ((ch - 48) as u32);
        if (val > 65535) {
            return -2;
        }
        i = i + 1;
    }
    return val as i32;
}

fn wuParseIpv6Host(inner: []const u8, out: []u8): i64 {
    if (inner.len < 2 || inner.len + 2 > out.len) {
        return -1;
    }
    let has_colon = false;
    let pos: usize = 0;
    out[pos] = 0x5B; // '['
    pos = pos + 1;

    let i: usize = 0;
    while (i < inner.len) {
        let ch = inner[i];
        if (ch == 0x3A) { // ':'
            has_colon = true;
            out[pos] = ch;
        } else if (isHexDigit(ch)) {
            out[pos] = toLowerByte(ch);
        } else if (ch == 0x2E) { // '.' (IPv4-mapped IPv6)
            out[pos] = ch;
        } else {
            return -1;
        }
        pos = pos + 1;
        i = i + 1;
    }
    if (!has_colon) {
        return -1;
    }
    out[pos] = 0x5D; // ']'
    pos = pos + 1;
    return pos as i64;
}

/// Returns the WHATWG default port for `scheme`, or -1 if none.
pub fn defaultPortForScheme(scheme: string): i32 {
    if (bytesEq(scheme, "http") || bytesEq(scheme, "ws")) {
        return 80;
    }
    if (bytesEq(scheme, "https") || bytesEq(scheme, "wss")) {
        return 443;
    }
    if (bytesEq(scheme, "ftp")) {
        return 21;
    }
    return -1;
}

/// Zero-allocation parsed and normalized WHATWG URL.
pub struct WhatwgUrl {
    pub scheme: [32]u8,
    pub scheme_len: usize,
    pub username: [128]u8,
    pub username_len: usize,
    pub password: [128]u8,
    pub password_len: usize,
    pub host: [256]u8,
    pub host_len: usize,
    pub port: i32,
    pub path: [512]u8,
    pub path_len: usize,
    pub query: [512]u8,
    pub query_len: usize,
    pub fragment: [256]u8,
    pub fragment_len: usize,
    pub has_authority: bool,
    pub has_query: bool,
    pub has_fragment: bool,
    pub ok: bool,

    pub static fn invalid(): WhatwgUrl {
        return WhatwgUrl {
            scheme: undefined,
            scheme_len: 0,
            username: undefined,
            username_len: 0,
            password: undefined,
            password_len: 0,
            host: undefined,
            host_len: 0,
            port: -1,
            path: undefined,
            path_len: 0,
            query: undefined,
            query_len: 0,
            fragment: undefined,
            fragment_len: 0,
            has_authority: false,
            has_query: false,
            has_fragment: false,
            ok: false
        };
    }

    pub fn schemeStr(): []const u8 {
        return _.scheme[0.._.scheme_len];
    }

    pub fn usernameStr(): []const u8 {
        return _.username[0.._.username_len];
    }

    pub fn passwordStr(): []const u8 {
        return _.password[0.._.password_len];
    }

    pub fn hostStr(): []const u8 {
        return _.host[0.._.host_len];
    }

    pub fn pathStr(): []const u8 {
        return _.path[0.._.path_len];
    }

    pub fn queryStr(): []const u8 {
        return _.query[0.._.query_len];
    }

    pub fn fragmentStr(): []const u8 {
        return _.fragment[0.._.fragment_len];
    }

    pub fn effectivePort(): i32 {
        if (_.port >= 0) {
            return _.port;
        }
        return defaultPortForScheme(_.scheme[0.._.scheme_len]);
    }

    /// Parses and normalizes an absolute URL according to the WHATWG URL Standard.
    pub static fn parse(input: string): WhatwgUrl {
        let url = WhatwgUrl.invalid();
        let trim_start: usize = 0;
        let trim_end: usize = 0;
        wuTrimBounds(input, &trim_start, &trim_end);
        if (trim_start >= trim_end) {
            return url;
        }
        let s = input[trim_start..trim_end];

        // 1. Scheme
        let colon_i64 = wuFindSchemeColon(s);
        if (colon_i64 <= 0) {
            return url;
        }
        let colon_idx = colon_i64 as usize;
        if (colon_idx > 32) {
            return url;
        }
        let si: usize = 0;
        while (si < colon_idx) {
            url.scheme[si] = toLowerByte(s[si]);
            si = si + 1;
        }
        url.scheme_len = colon_idx;

        let scheme_slice = url.scheme[0..url.scheme_len];
        let is_special = wuIsSpecialScheme(scheme_slice);
        let is_file = bytesEq(scheme_slice, "file");
        let rest = s[(colon_idx + 1)..s.len];

        // 2. Authority
        if (rest.len >= 2 && rest[0] == 0x2F && rest[1] == 0x2F) {
            url.has_authority = true;
            let after_slashes = rest[2..rest.len];
            let auth_end: usize = 0;
            while (auth_end < after_slashes.len) {
                let ch = after_slashes[auth_end];
                if (ch == 0x2F || ch == 0x3F || ch == 0x23 || (is_special && ch == 0x5C)) {
                    break;
                }
                auth_end = auth_end + 1;
            }
            let authority = after_slashes[0..auth_end];
            rest = after_slashes[auth_end..after_slashes.len];

            // 2a. Userinfo (split on last '@')
            let at_pos: i64 = -1;
            let ai: usize = 0;
            while (ai < authority.len) {
                if (authority[ai] == 0x40) {
                    at_pos = ai as i64;
                }
                ai = ai + 1;
            }

            let host_port = authority;
            if (at_pos >= 0) {
                let ui = authority[0..(at_pos as usize)];
                host_port = authority[((at_pos as usize) + 1)..authority.len];
                let u_colon: i64 = -1;
                let ui_i: usize = 0;
                while (ui_i < ui.len) {
                    if (ui[ui_i] == 0x3A) {
                        u_colon = ui_i as i64;
                        break;
                    }
                    ui_i = ui_i + 1;
                }
                let uset = userinfoSet();
                if (u_colon >= 0) {
                    url.username_len = percentEncode(ui[0..(u_colon as usize)], uset, url.username[0..128]);
                    url.password_len = percentEncode(ui[((u_colon as usize) + 1)..ui.len], uset, url.password[0..128]);
                } else {
                    url.username_len = percentEncode(ui, uset, url.username[0..128]);
                }
            }

            // 2b. Host & Port
            if (host_port.len == 0) {
                if (!is_file) {
                    return WhatwgUrl.invalid();
                }
            } else if (host_port[0] == 0x5B) {
                // IPv6 host literal [...]
                let close_idx: i64 = -1;
                let bi: usize = 1;
                while (bi < host_port.len) {
                    if (host_port[bi] == 0x5D) {
                        close_idx = bi as i64;
                        break;
                    }
                    bi = bi + 1;
                }
                if (close_idx < 0) {
                    return WhatwgUrl.invalid();
                }
                let cidx = close_idx as usize;
                let hlen = wuParseIpv6Host(host_port[1..cidx], url.host[0..256]);
                if (hlen < 0) {
                    return WhatwgUrl.invalid();
                }
                url.host_len = hlen as usize;

                if (cidx + 1 < host_port.len) {
                    if (host_port[cidx + 1] != 0x3A) {
                        return WhatwgUrl.invalid();
                    }
                    let p = wuParsePortStr(host_port[(cidx + 2)..host_port.len]);
                    if (p == -2) {
                        return WhatwgUrl.invalid();
                    }
                    url.port = p;
                }
            } else {
                // Domain or IPv4 host
                let p_colon: i64 = -1;
                let hi: usize = 0;
                while (hi < host_port.len) {
                    if (host_port[hi] == 0x3A) {
                        p_colon = hi as i64;
                        break;
                    }
                    hi = hi + 1;
                }
                let raw_host = host_port;
                if (p_colon >= 0) {
                    raw_host = host_port[0..(p_colon as usize)];
                    let p = wuParsePortStr(host_port[((p_colon as usize) + 1)..host_port.len]);
                    if (p == -2) {
                        return WhatwgUrl.invalid();
                    }
                    url.port = p;
                }
                if (raw_host.len == 0) {
                    if (!is_file) {
                        return WhatwgUrl.invalid();
                    }
                } else {
                    // Reject forbidden raw host bytes
                    let fi: usize = 0;
                    while (fi < raw_host.len) {
                        if (wuIsForbiddenHostByte(raw_host[fi])) {
                            return WhatwgUrl.invalid();
                        }
                        fi = fi + 1;
                    }
                    // Percent-decode host via @cap/percent-encoding, then IDNA UTS #46 via @cap/idna-uts46
                    let dec_host: [256]u8 = undefined;
                    let dec_len = percentDecode(raw_host, dec_host[0..256]);
                    let di: usize = 0;
                    while (di < dec_len) {
                        if (wuIsForbiddenHostByte(dec_host[di])) {
                            return WhatwgUrl.invalid();
                        }
                        di = di + 1;
                    }
                    let ascii_len = domainToAscii(dec_host[0..dec_len], url.host[0..256]);
                    if (ascii_len <= 0) {
                        return WhatwgUrl.invalid();
                    }
                    url.host_len = ascii_len as usize;
                }
            }

            // Canonical default port omission
            if (url.port >= 0 && url.port == defaultPortForScheme(scheme_slice)) {
                url.port = -1;
            }
        } else if (is_special && !is_file) {
            return WhatwgUrl.invalid();
        }

        // 3. Fragment (#...)
        let hash_pos: i64 = -1;
        let ri: usize = 0;
        while (ri < rest.len) {
            if (rest[ri] == 0x23) {
                hash_pos = ri as i64;
                break;
            }
            ri = ri + 1;
        }
        let pre_frag = rest;
        if (hash_pos >= 0) {
            let hp = hash_pos as usize;
            pre_frag = rest[0..hp];
            url.has_fragment = true;
            url.fragment_len = percentEncode(rest[(hp + 1)..rest.len], fragmentSet(), url.fragment[0..256]);
        }

        // 4. Query (?...)
        let q_pos: i64 = -1;
        let qi: usize = 0;
        while (qi < pre_frag.len) {
            if (pre_frag[qi] == 0x3F) {
                q_pos = qi as i64;
                break;
            }
            qi = qi + 1;
        }
        let raw_path = pre_frag;
        if (q_pos >= 0) {
            let qp = q_pos as usize;
            raw_path = pre_frag[0..qp];
            url.has_query = true;
            url.query_len = percentEncode(pre_frag[(qp + 1)..pre_frag.len], querySet(), url.query[0..512]);
        }

        // 5. Path normalization
        if (url.has_authority) {
            if (raw_path.len == 0) {
                if (is_special) {
                    url.path[0] = 0x2F;
                    url.path_len = 1;
                }
            } else {
                url.path_len = wuNormalizePath(raw_path, is_special, url.path[0..512]);
            }
        } else {
            url.path_len = percentEncode(raw_path, pathSet(), url.path[0..512]);
        }

        url.ok = true;
        return url;
    }

    /// Resolves a relative or absolute URL reference `rel` against `base` per WHATWG URL Standard.
    pub static fn join(base: WhatwgUrl, rel: string): WhatwgUrl {
        if (!base.ok) {
            return WhatwgUrl.invalid();
        }
        let trim_start: usize = 0;
        let trim_end: usize = 0;
        wuTrimBounds(rel, &trim_start, &trim_end);
        let r = rel[trim_start..trim_end];

        // Empty relative reference: copy base without fragment
        if (r.len == 0) {
            let res = wuCloneUrl(base);
            res.has_fragment = false;
            res.fragment_len = 0;
            return res;
        }

        // Absolute URL with scheme
        if (wuFindSchemeColon(r) > 0) {
            return WhatwgUrl.parse(r);
        }

        // Protocol-relative URL (//host/path...)
        if (r.len >= 2 && r[0] == 0x2F && r[1] == 0x2F) {
            let buf: [1024]u8 = undefined;
            let pos: usize = 0;
            if (!wuAppendBytes(buf[0..1024], &pos, base.scheme[0..base.scheme_len])) {
                return WhatwgUrl.invalid();
            }
            if (!wuAppendByte(buf[0..1024], &pos, 0x3A)) {
                return WhatwgUrl.invalid();
            }
            if (!wuAppendBytes(buf[0..1024], &pos, r)) {
                return WhatwgUrl.invalid();
            }
            return WhatwgUrl.parse(buf[0..pos]);
        }

        // Fragment-only (#frag)
        if (r[0] == 0x23) {
            let res = wuCloneUrl(base);
            res.has_fragment = true;
            res.fragment_len = percentEncode(r[1..r.len], fragmentSet(), res.fragment[0..256]);
            return res;
        }

        // Query-only (?query[#frag])
        if (r[0] == 0x3F) {
            let res = wuCloneUrl(base);
            res.has_fragment = false;
            res.fragment_len = 0;
            let q_body = r[1..r.len];
            let hash_pos: i64 = -1;
            let i: usize = 0;
            while (i < q_body.len) {
                if (q_body[i] == 0x23) {
                    hash_pos = i as i64;
                    break;
                }
                i = i + 1;
            }
            res.has_query = true;
            if (hash_pos >= 0) {
                let hp = hash_pos as usize;
                res.query_len = percentEncode(q_body[0..hp], querySet(), res.query[0..512]);
                res.has_fragment = true;
                res.fragment_len = percentEncode(q_body[(hp + 1)..q_body.len], fragmentSet(), res.fragment[0..256]);
            } else {
                res.query_len = percentEncode(q_body, querySet(), res.query[0..512]);
            }
            return res;
        }

        // Root-relative (/...) or relative path (../..., foo/bar)
        let res = wuCloneUrl(base);
        res.has_query = false;
        res.query_len = 0;
        res.has_fragment = false;
        res.fragment_len = 0;

        let hash_pos: i64 = -1;
        let hi: usize = 0;
        while (hi < r.len) {
            if (r[hi] == 0x23) {
                hash_pos = hi as i64;
                break;
            }
            hi = hi + 1;
        }
        let pre_frag = r;
        if (hash_pos >= 0) {
            let hp = hash_pos as usize;
            pre_frag = r[0..hp];
            res.has_fragment = true;
            res.fragment_len = percentEncode(r[(hp + 1)..r.len], fragmentSet(), res.fragment[0..256]);
        }

        let q_pos: i64 = -1;
        let qi: usize = 0;
        while (qi < pre_frag.len) {
            if (pre_frag[qi] == 0x3F) {
                q_pos = qi as i64;
                break;
            }
            qi = qi + 1;
        }
        let rel_path = pre_frag;
        if (q_pos >= 0) {
            let qp = q_pos as usize;
            rel_path = pre_frag[0..qp];
            res.has_query = true;
            res.query_len = percentEncode(pre_frag[(qp + 1)..pre_frag.len], querySet(), res.query[0..512]);
        }

        let is_special = wuIsSpecialScheme(base.scheme[0..base.scheme_len]);
        if (rel_path.len > 0 && (rel_path[0] == 0x2F || (is_special && rel_path[0] == 0x5C))) {
            res.path_len = wuNormalizePath(rel_path, is_special, res.path[0..512]);
            return res;
        }

        // Merge relative path with base directory
        let merged: [1024]u8 = undefined;
        let mpos: usize = 0;
        let last_slash: i64 = -1;
        let bi: usize = 0;
        while (bi < base.path_len) {
            if (base.path[bi] == 0x2F) {
                last_slash = bi as i64;
            }
            bi = bi + 1;
        }
        if (last_slash >= 0) {
            * = wuAppendBytes(merged[0..1024], &mpos, base.path[0..((last_slash as usize) + 1)]);
        } else {
            * = wuAppendByte(merged[0..1024], &mpos, 0x2F);
        }
        * = wuAppendBytes(merged[0..1024], &mpos, rel_path);
        res.path_len = wuNormalizePath(merged[0..mpos], is_special, res.path[0..512]);
        return res;
    }

    /// Serializes the normalized URL into `out`. Returns bytes written.
    pub fn toStr(out: []u8): usize {
        if (!_.ok) {
            return 0;
        }
        let pos: usize = 0;
        if (!wuAppendBytes(out, &pos, _.scheme[0.._.scheme_len])) {
            return pos;
        }
        if (!wuAppendByte(out, &pos, 0x3A)) {
            return pos;
        }
        if (_.has_authority) {
            if (!wuAppendBytes(out, &pos, "//")) {
                return pos;
            }
            if (_.username_len > 0 || _.password_len > 0) {
                if (!wuAppendBytes(out, &pos, _.username[0.._.username_len])) {
                    return pos;
                }
                if (_.password_len > 0) {
                    if (!wuAppendByte(out, &pos, 0x3A)) {
                        return pos;
                    }
                    if (!wuAppendBytes(out, &pos, _.password[0.._.password_len])) {
                        return pos;
                    }
                }
                if (!wuAppendByte(out, &pos, 0x40)) {
                    return pos;
                }
            }
            if (!wuAppendBytes(out, &pos, _.host[0.._.host_len])) {
                return pos;
            }
            if (_.port >= 0) {
                if (!wuAppendByte(out, &pos, 0x3A)) {
                    return pos;
                }
                if (!wuAppendPort(out, &pos, _.port)) {
                    return pos;
                }
            }
        }
        if (!wuAppendBytes(out, &pos, _.path[0.._.path_len])) {
            return pos;
        }
        if (_.has_query) {
            if (!wuAppendByte(out, &pos, 0x3F)) {
                return pos;
            }
            if (!wuAppendBytes(out, &pos, _.query[0.._.query_len])) {
                return pos;
            }
        }
        if (_.has_fragment) {
            if (!wuAppendByte(out, &pos, 0x23)) {
                return pos;
            }
            if (!wuAppendBytes(out, &pos, _.fragment[0.._.fragment_len])) {
                return pos;
            }
        }
        return pos;
    }

    /// Serializes the WHATWG tuple origin (`scheme://host[:port]`, or `"null"` for opaque origins).
    pub fn origin(out: []u8): usize {
        if (!_.ok) {
            return 0;
        }
        let pos: usize = 0;
        if (wuIsOriginScheme(_.scheme[0.._.scheme_len]) && _.host_len > 0) {
            if (!wuAppendBytes(out, &pos, _.scheme[0.._.scheme_len])) {
                return pos;
            }
            if (!wuAppendBytes(out, &pos, "://")) {
                return pos;
            }
            if (!wuAppendBytes(out, &pos, _.host[0.._.host_len])) {
                return pos;
            }
            if (_.port >= 0) {
                if (!wuAppendByte(out, &pos, 0x3A)) {
                    return pos;
                }
                if (!wuAppendPort(out, &pos, _.port)) {
                    return pos;
                }
            }
            return pos;
        }
        * = wuAppendBytes(out, &pos, "null");
        return pos;
    }
}

fn wuCloneUrl(src: WhatwgUrl): WhatwgUrl {
    let dst = WhatwgUrl.invalid();
    let i: usize = 0;
    while (i < src.scheme_len) {
        dst.scheme[i] = src.scheme[i];
        i = i + 1;
    }
    dst.scheme_len = src.scheme_len;
    i = 0;
    while (i < src.username_len) {
        dst.username[i] = src.username[i];
        i = i + 1;
    }
    dst.username_len = src.username_len;
    i = 0;
    while (i < src.password_len) {
        dst.password[i] = src.password[i];
        i = i + 1;
    }
    dst.password_len = src.password_len;
    i = 0;
    while (i < src.host_len) {
        dst.host[i] = src.host[i];
        i = i + 1;
    }
    dst.host_len = src.host_len;
    dst.port = src.port;
    i = 0;
    while (i < src.path_len) {
        dst.path[i] = src.path[i];
        i = i + 1;
    }
    dst.path_len = src.path_len;
    i = 0;
    while (i < src.query_len) {
        dst.query[i] = src.query[i];
        i = i + 1;
    }
    dst.query_len = src.query_len;
    i = 0;
    while (i < src.fragment_len) {
        dst.fragment[i] = src.fragment[i];
        i = i + 1;
    }
    dst.fragment_len = src.fragment_len;
    dst.has_authority = src.has_authority;
    dst.has_query = src.has_query;
    dst.has_fragment = src.has_fragment;
    dst.ok = src.ok;
    return dst;
}

export {
    WhatwgUrl,
    defaultPortForScheme
};
