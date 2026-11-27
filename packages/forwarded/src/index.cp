// @cap/forwarded — Zero-allocation RFC 7239 Forwarded & X-Forwarded-For parser for CP
// Clean-room MIT implementation inspired by npm `forwarded` and `proxy-addr`.
//
// Features:
// - Full RFC 7239 `Forwarded` header parser (quoted strings, IPv6 brackets, port stripping).
// - Classic `X-Forwarded-For` proxy chain parser with `remote_addr` appending.
// - Anti-spoofing client IP extraction (`extractClientIp`, `ipAtTrustDepth`) walking right-to-left
//   past `trusted_hops` trusted proxies.
// - Private/internal IPv4 & IPv6 detection (`10/8`, `172.16/12`, `192.168/16`, `127/8`, `::1`, `fc00::/7`, `fe80::/10`).
// - Zero heap allocation (all returned strings slice into caller inputs).

const MAX_HOPS: usize = 32;

pub struct ForwardedHop {
    pub for_addr: string,
    pub by_addr: string,
    pub host: string,
    pub proto: string,

    pub static fn empty() -> ForwardedHop {
        return ForwardedHop{
            for_addr: "",
            by_addr: "",
            host: "",
            proto: "",
        };
    }
}

pub struct ForwardedChain {
    pub hops: [MAX_HOPS]ForwardedHop,
    pub count: usize,

    pub static fn empty() -> ForwardedChain {
        let arr: [MAX_HOPS]ForwardedHop = undefined;
        return ForwardedChain{ hops: arr, count: 0 };
    }

    pub fn hopAt(i: usize) -> ForwardedHop {
        if (i >= _.count) {
            return ForwardedHop.empty();
        }
        return _.hops[i];
    }

    /// Returns the leftmost (original client) `for_addr`, or `""` if empty.
    pub fn clientIp() -> string {
        if (_.count == 0) {
            return "";
        }
        return _.hops[0].for_addr;
    }

    /// Returns the leftmost `proto`, or `""` if absent.
    pub fn clientProto() -> string {
        if (_.count == 0) {
            return "";
        }
        return _.hops[0].proto;
    }

    /// Returns the leftmost `host`, or `""` if absent.
    pub fn clientHost() -> string {
        if (_.count == 0) {
            return "";
        }
        return _.hops[0].host;
    }

    /// Anti-spoofing client IP extraction: walks right-to-left skipping `trusted_proxies`
    /// trusted proxy hops. If `trusted_proxies == 0`, returns the rightmost hop (direct peer).
    /// If `trusted_proxies >= count`, clamps to index 0.
    pub fn ipAtTrustDepth(trusted_proxies: usize) -> string {
        if (_.count == 0) {
            return "";
        }
        if (trusted_proxies >= _.count) {
            return _.hops[0].for_addr;
        }
        return _.hops[_.count - 1 - trusted_proxies].for_addr;
    }
}

fn isSpace(c: u8) -> bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isDigit(c: u8) -> bool {
    return c >= '0' && c <= '9';
}

fn toLowerAscii(c: u8) -> u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn trim(s: string) -> string {
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

fn unquote(raw: string) -> string {
    let s = trim(raw);
    if (s.len >= 2 && s[0] == '"' && s[s.len - 1] == '"') {
        s = trim(s[1..(s.len - 1)]);
    }
    return s;
}

fn eqIgnoreCase(a: string, lower_lit: string) -> bool {
    if (a.len != lower_lit.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != lower_lit[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Strips quotes, IPv6 brackets `[...]`, and optional `:port` from a node identifier.
fn stripAddr(raw: string) -> string {
    const s = unquote(raw);
    if (s.len == 0) {
        return "";
    }

    // Bracketed IPv6: "[2001:db8::1]:4711" or "[::1]"
    if (s[0] == '[') {
        let i: usize = 1;
        while (i < s.len) {
            if (s[i] == ']') {
                return s[1..i];
            }
            i += 1;
        }
        return s;
    }

    // Count colons to distinguish IPv4:port (1 colon) from bare IPv6 (>=2 colons)
    let colon_count: usize = 0;
    let last_colon: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ':') {
            colon_count += 1;
            last_colon = i;
        }
        i += 1;
    }

    if (colon_count == 1 && last_colon + 1 < s.len) {
        // Verify port suffix is numeric (or RFC 7239 obfuscated port starting with '_')
        let all_digits = true;
        let p = last_colon + 1;
        while (p < s.len) {
            if (!isDigit(s[p])) {
                all_digits = false;
                break;
            }
            p += 1;
        }
        if (all_digits || s[last_colon + 1] == '_') {
            return s[0..last_colon];
        }
    }

    return s;
}

fn parseForwardedElement(elem: string) -> ForwardedHop {
    let hop = ForwardedHop.empty();
    let i: usize = 0;
    let seg_start: usize = 0;
    let in_quotes = false;

    while (i <= elem.len) {
        const at_end = (i == elem.len);
        if (!at_end && elem[i] == '\\' && in_quotes && i + 1 < elem.len) {
            i += 2;
            continue;
        }
        if (!at_end && elem[i] == '"') {
            in_quotes = !in_quotes;
            i += 1;
            continue;
        }
        if (at_end || (!in_quotes && elem[i] == ';')) {
            const pair = trim(elem[seg_start..i]);
            // Find '='
            let eq_idx: usize = 0;
            let found_eq = false;
            while (eq_idx < pair.len) {
                if (pair[eq_idx] == '=') {
                    found_eq = true;
                    break;
                }
                eq_idx += 1;
            }
            if (found_eq) {
                const key = trim(pair[0..eq_idx]);
                const val = trim(pair[(eq_idx + 1)..pair.len]);
                if (eqIgnoreCase(key, "for")) {
                    hop.for_addr = stripAddr(val);
                } else if (eqIgnoreCase(key, "by")) {
                    hop.by_addr = stripAddr(val);
                } else if (eqIgnoreCase(key, "host")) {
                    hop.host = unquote(val);
                } else if (eqIgnoreCase(key, "proto")) {
                    hop.proto = unquote(val);
                }
            }
            seg_start = i + 1;
        }
        i += 1;
    }

    return hop;
}

/// Parses an RFC 7239 `Forwarded` header into a zero-allocation `ForwardedChain`.
pub fn parseForwarded(header: string) -> ForwardedChain {
    let chain = ForwardedChain.empty();
    let i: usize = 0;
    let elem_start: usize = 0;
    let in_quotes = false;

    while (i <= header.len) {
        const at_end = (i == header.len);
        if (!at_end && header[i] == '\\' && in_quotes && i + 1 < header.len) {
            i += 2;
            continue;
        }
        if (!at_end && header[i] == '"') {
            in_quotes = !in_quotes;
            i += 1;
            continue;
        }
        if (at_end || (!in_quotes && header[i] == ',')) {
            const elem = trim(header[elem_start..i]);
            if (elem.len > 0 && chain.count < MAX_HOPS) {
                const hop = parseForwardedElement(elem);
                if (hop.for_addr.len > 0 || hop.by_addr.len > 0 || hop.host.len > 0 || hop.proto.len > 0) {
                    chain.hops[chain.count] = hop;
                    chain.count += 1;
                }
            }
            elem_start = i + 1;
        }
        i += 1;
    }

    return chain;
}

/// Parses a classic `X-Forwarded-For` header and appends `remote_addr` as the final hop.
pub fn parseXForwardedFor(xff: string, remote_addr: string) -> ForwardedChain {
    let chain = ForwardedChain.empty();
    let i: usize = 0;
    let seg_start: usize = 0;

    while (i <= xff.len) {
        if (i == xff.len || xff[i] == ',') {
            const addr = stripAddr(xff[seg_start..i]);
            if (addr.len > 0 && chain.count < MAX_HOPS) {
                let hop = ForwardedHop.empty();
                hop.for_addr = addr;
                chain.hops[chain.count] = hop;
                chain.count += 1;
            }
            seg_start = i + 1;
        }
        i += 1;
    }

    const remote = stripAddr(remote_addr);
    if (remote.len > 0 && chain.count < MAX_HOPS) {
        let hop = ForwardedHop.empty();
        hop.for_addr = remote;
        chain.hops[chain.count] = hop;
        chain.count += 1;
    }

    return chain;
}

/// Extracts the real client IP from `X-Forwarded-For` + `remote_addr`, skipping `trusted_hops`
/// trusted proxies from the right to defeat spoofed client-injected XFF headers.
pub fn extractClientIp(xff: string, remote_addr: string, trusted_hops: usize) -> string {
    const chain = parseXForwardedFor(xff, remote_addr);
    return chain.ipAtTrustDepth(trusted_hops);
}

fn hexVal(c: u8) -> i8 {
    if (c >= '0' && c <= '9') {
        return (c - '0') as i8;
    }
    if (c >= 'a' && c <= 'f') {
        return (c - 'a' + 10) as i8;
    }
    if (c >= 'A' && c <= 'F') {
        return (c - 'A' + 10) as i8;
    }
    return -1;
}

fn isPrivateIpv4(s: string) -> bool {
    let octets: [4]u16 = [0, 0, 0, 0];
    let idx: usize = 0;
    let digits: usize = 0;
    let i: usize = 0;

    while (i < s.len) {
        const c = s[i];
        if (c == '.') {
            if (digits == 0 || idx >= 3) {
                return false;
            }
            idx += 1;
            digits = 0;
        } else if (isDigit(c)) {
            const next_val = octets[idx] * 10 + ((c - '0') as u16);
            if (next_val > 255) {
                return false;
            }
            octets[idx] = next_val;
            digits += 1;
        } else {
            return false;
        }
        i += 1;
    }

    if (idx != 3 || digits == 0) {
        return false;
    }

    const a = octets[0];
    const b = octets[1];

    // 10.0.0.0/8
    if (a == 10) {
        return true;
    }
    // 172.16.0.0/12
    if (a == 172 && b >= 16 && b <= 31) {
        return true;
    }
    // 192.168.0.0/16
    if (a == 192 && b == 168) {
        return true;
    }
    // 127.0.0.0/8 (loopback)
    if (a == 127) {
        return true;
    }
    // 169.254.0.0/16 (link-local)
    if (a == 169 && b == 254) {
        return true;
    }
    return false;
}

/// Returns `true` if `ip` is a private, loopback, or link-local IPv4/IPv6 address
/// (`10.0.0.0/8`, `172.16.0.0/12`, `192.168.0.0/16`, `127.0.0.0/8`, `169.254.0.0/16`,
/// `::1`, `fc00::/7`, `fe80::/10`, and IPv4-mapped `::ffff:` private addresses).
pub fn isPrivateIp(ip: string) -> bool {
    const s = stripAddr(ip);
    if (s.len == 0) {
        return false;
    }

    // Check if IPv6 (contains ':')
    let has_colon = false;
    let first_colon: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ':') {
            has_colon = true;
            first_colon = i;
            break;
        }
        i += 1;
    }

    if (!has_colon) {
        return isPrivateIpv4(s);
    }

    // Loopback ::1
    if (s.len == 3 && s[0] == ':' && s[1] == ':' && s[2] == '1') {
        return true;
    }

    // IPv4-mapped IPv6: ::ffff:a.b.c.d
    if (s.len > 7 && eqIgnoreCase(s[0..7], "::ffff:")) {
        return isPrivateIpv4(s[7..s.len]);
    }

    // Parse first hextet for fc00::/7 (ULA) and fe80::/10 (link-local)
    if (first_colon >= 1 && first_colon <= 4) {
        let h0: u16 = 0;
        let k: usize = 0;
        while (k < first_colon) {
            const v = hexVal(s[k]);
            if (v < 0) {
                return false;
            }
            h0 = (h0 * 16) + (v as u16);
            k += 1;
        }
        // fc00::/7 => 0xfc00 .. 0xfdff
        if ((h0 & 0xfe00) == 0xfc00) {
            return true;
        }
        // fe80::/10 => 0xfe80 .. 0xfebf
        if ((h0 & 0xffc0) == 0xfe80) {
            return true;
        }
    }

    return false;
}
