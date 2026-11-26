// @cap/forwarded-http — Zero-heap RFC 7239 `Forwarded` & `X-Forwarded-For` proxy chain parser in pure CP
// Clean-room MIT implementation inspired by npm `forwarded` + `proxy-addr`.
// Reuses:
//   - `eqAscii` from `@cap/unicase` for case-insensitive RFC 7239 parameter key matching.

import { eqAscii } from "../../unicase/src/index.cp";

pub struct ForwardedEntry {
    pub for_val: string,
    pub by_val: string,
    pub host: string,
    pub proto: string,
}

// ============================================================================
// Private helpers (all prefixed with `fwd` to avoid module flattening clashes)
// ============================================================================

fn fwdIsSpace(c: u8): bool {
    return c == 32 || c == 9 || c == 10 || c == 13;
}

fn fwdIsDigit(c: u8): bool {
    return c >= 48 && c <= 57;
}

fn fwdTrim(s: string): string {
    let a: usize = 0;
    let b: usize = s.len;
    while (a < b && fwdIsSpace(s[a])) {
        a = a + 1;
    }
    while (b > a && fwdIsSpace(s[b - 1])) {
        b = b - 1;
    }
    return s[a..b];
}

fn fwdUnquote(s: string): string {
    const t: string = fwdTrim(s);
    if (t.len >= 2 && t[0] == 34 && t[t.len - 1] == 34) {
        return t[1..(t.len - 1)];
    }
    return t;
}

fn fwdStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn fwdIsValidIpv4Loopback(s: string): bool {
    // Must start with "127." and have 4 valid decimal octets 0..255
    if (s.len < 7 || s[0] != 49 || s[1] != 50 || s[2] != 55 || s[3] != 46) {
        return false;
    }
    let i: usize = 4;
    let octet_count: usize = 1;
    while (i < s.len) {
        let val: u64 = 0;
        let digits: usize = 0;
        while (i < s.len && fwdIsDigit(s[i])) {
            val = val * 10 + ((s[i] - 48) as u64);
            digits = digits + 1;
            if (digits > 3 || val > 255) {
                return false;
            }
            i = i + 1;
        }
        if (digits == 0) {
            return false;
        }
        octet_count = octet_count + 1;
        if (i < s.len) {
            if (s[i] != 46 || octet_count >= 4) {
                return false;
            }
            i = i + 1;
            if (i == s.len) {
                return false;
            }
        }
    }
    return octet_count == 4;
}

fn fwdParseElement(seg: string, entry_out: *ForwardedEntry): bool {
    let e = ForwardedEntry {
        for_val: "",
        by_val: "",
        host: "",
        proto: "",
    };
    let has_any: bool = false;
    let i: usize = 0;
    while (i < seg.len) {
        // Find next ';' outside double quotes
        let start: usize = i;
        let in_quotes: bool = false;
        while (i < seg.len) {
            const c: u8 = seg[i];
            if (in_quotes) {
                if (c == 92 && i + 1 < seg.len) {
                    i = i + 2;
                    continue;
                }
                if (c == 34) {
                    in_quotes = false;
                }
            } else {
                if (c == 34) {
                    in_quotes = true;
                } else if (c == 59) {
                    break;
                }
            }
            i = i + 1;
        }
        const pair: string = fwdTrim(seg[start..i]);
        if (i < seg.len && seg[i] == 59) {
            i = i + 1;
        }
        if (pair.len == 0) {
            continue;
        }
        // Split on first '='
        let eq: i64 = -1;
        let k: usize = 0;
        while (k < pair.len) {
            if (pair[k] == 61) {
                eq = k as i64;
                break;
            }
            k = k + 1;
        }
        if (eq <= 0) {
            continue;
        }
        const eq_idx: usize = eq as usize;
        const key: string = fwdTrim(pair[0..eq_idx]);
        const val: string = fwdUnquote(pair[(eq_idx + 1)..pair.len]);

        if (eqAscii(key, "for")) {
            e.for_val = val;
            has_any = true;
        } else if (eqAscii(key, "by")) {
            e.by_val = val;
            has_any = true;
        } else if (eqAscii(key, "host")) {
            e.host = val;
            has_any = true;
        } else if (eqAscii(key, "proto")) {
            e.proto = val;
            has_any = true;
        }
    }
    entry_out.* = e;
    return has_any;
}

// ============================================================================
// Public API
// ============================================================================

/// Parse `remote_addr` and `X-Forwarded-For` header into `out_addrs` in
/// reverse proxy order: `[remote_addr, closest_proxy, ..., original_client_ip]`.
/// Matches npm `forwarded` behavior. Returns the number of addresses written.
pub fn parseXForwardedFor(remote_addr: string, xff_header: string, out_addrs: []string): usize {
    if (out_addrs.len == 0) {
        return 0;
    }
    let count: usize = 0;
    const rem: string = fwdTrim(remote_addr);
    if (rem.len > 0) {
        out_addrs[count] = rem;
        count = count + 1;
    }
    if (xff_header.len == 0 || count >= out_addrs.len) {
        return count;
    }

    // Scan xff_header right-to-left on ','
    let end: usize = xff_header.len;
    while (end > 0 && count < out_addrs.len) {
        let start: usize = end;
        while (start > 0 && xff_header[start - 1] != 44) {
            start = start - 1;
        }
        const addr: string = fwdTrim(xff_header[start..end]);
        if (addr.len > 0) {
            out_addrs[count] = addr;
            count = count + 1;
        }
        if (start == 0) {
            break;
        }
        end = start - 1;
    }
    return count;
}

/// Extract the real client IP given `remote_addr`, `X-Forwarded-For`, and the
/// number of trusted reverse-proxy hops `trust_hops`:
/// - `trust_hops == 0`: always returns `remote_addr` (ignores XFF).
/// - `trust_hops == k`: skips `k` trusted proxies from right to left and returns
///   the client IP (clamped to the leftmost XFF entry if fewer than `k` exist).
pub fn extractClientIp(remote_addr: string, xff_header: string, trust_hops: usize): string {
    const rem: string = fwdTrim(remote_addr);
    if (trust_hops == 0 || xff_header.len == 0) {
        return rem;
    }
    let last_valid: string = rem;
    let hops_seen: usize = 0;
    let end: usize = xff_header.len;
    while (end > 0) {
        let start: usize = end;
        while (start > 0 && xff_header[start - 1] != 44) {
            start = start - 1;
        }
        const addr: string = fwdTrim(xff_header[start..end]);
        if (addr.len > 0) {
            last_valid = addr;
            hops_seen = hops_seen + 1;
            if (hops_seen == trust_hops) {
                return addr;
            }
        }
        if (start == 0) {
            break;
        }
        end = start - 1;
    }
    return last_valid;
}

/// Parse an RFC 7239 `Forwarded` header (comma-separated forwarded-elements with
/// semicolon-separated `for`, `by`, `host`, `proto` parameters, supporting quoted
/// strings and bracketed IPv6 addresses) into `out_entries`.
/// Returns the number of entries parsed.
pub fn parseForwardedHeader(header: string, out_entries: []ForwardedEntry): usize {
    if (header.len == 0 || out_entries.len == 0) {
        return 0;
    }
    let count: usize = 0;
    let i: usize = 0;
    while (i < header.len && count < out_entries.len) {
        const start: usize = i;
        let in_quotes: bool = false;
        while (i < header.len) {
            const c: u8 = header[i];
            if (in_quotes) {
                if (c == 92 && i + 1 < header.len) {
                    i = i + 2;
                    continue;
                }
                if (c == 34) {
                    in_quotes = false;
                }
            } else {
                if (c == 34) {
                    in_quotes = true;
                } else if (c == 44) {
                    break;
                }
            }
            i = i + 1;
        }
        let entry = ForwardedEntry {
            for_val: "",
            by_val: "",
            host: "",
            proto: "",
        };
        if (fwdParseElement(header[start..i], &entry)) {
            out_entries[count] = entry;
            count = count + 1;
        }
        if (i < header.len && header[i] == 44) {
            i = i + 1;
        }
    }
    return count;
}

/// Return true if `ip` is a loopback address:
/// `127.0.0.1`, any `127.x.y.z`, `::1`, `0:0:0:0:0:0:0:1`, or IPv4-mapped `::ffff:127.x.y.z`.
/// Also handles bracketed IPv6 (`[::1]`, `[::1]:8080`) and IPv4 with port (`127.0.0.1:8080`).
pub fn isLoopbackIp(ip: string): bool {
    let s: string = fwdTrim(ip);
    if (s.len == 0) {
        return false;
    }
    // Strip `[ipv6]:port` or `[ipv6]`
    if (s[0] == 91) {
        let close: usize = 1;
        while (close < s.len && s[close] != 93) {
            close = close + 1;
        }
        if (close >= s.len) {
            return false;
        }
        s = s[1..close];
    } else {
        // If exactly one ':', strip `:port` for IPv4:port
        let colon_count: usize = 0;
        let colon_pos: usize = 0;
        let k: usize = 0;
        while (k < s.len) {
            if (s[k] == 58) {
                colon_count = colon_count + 1;
                colon_pos = k;
            }
            k = k + 1;
        }
        if (colon_count == 1) {
            s = s[0..colon_pos];
        }
    }

    if (fwdStrEq(s, "::1") || fwdStrEq(s, "0:0:0:0:0:0:0:1")) {
        return true;
    }
    // IPv4-mapped IPv6: `::ffff:127.x.y.z` (case-insensitive `::ffff:`)
    if (s.len > 7 && eqAscii(s[0..7], "::ffff:")) {
        return fwdIsValidIpv4Loopback(s[7..s.len]);
    }
    return fwdIsValidIpv4Loopback(s);
}
