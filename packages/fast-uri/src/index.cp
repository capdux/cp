// @cap/fast-uri — Zero-allocation RFC 3986 URI parser, serializer, resolver & comparator
// Clean-room implementation modeled on npm `fast-uri` (Fastify / AJV).

pub struct UriComponents {
    valid: bool,
    scheme: string,
    userinfo: string,
    host: string,
    port: i32,
    path: string,
    query: string,
    has_query: bool,
    fragment: string,
    has_fragment: bool,

    static fn invalid(): UriComponents {
        return UriComponents{
            valid: false,
            scheme: "",
            userinfo: "",
            host: "",
            port: -1,
            path: "",
            query: "",
            has_query: false,
            fragment: "",
            has_fragment: false,
        };
    }

    static fn empty(): UriComponents {
        return UriComponents{
            valid: true,
            scheme: "",
            userinfo: "",
            host: "",
            port: -1,
            path: "",
            query: "",
            has_query: false,
            fragment: "",
            has_fragment: false,
        };
    }
}

fn furiIsAlpha(b: u8): bool {
    return (b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z');
}

fn furiIsDigit(b: u8): bool {
    return b >= '0' && b <= '9';
}

fn furiIsHex(b: u8): bool {
    return furiIsDigit(b) || (b >= 'a' && b <= 'f') || (b >= 'A' && b <= 'F');
}

fn furiIsSchemeChar(b: u8): bool {
    return furiIsAlpha(b) || furiIsDigit(b) || b == '+' || b == '-' || b == '.';
}

fn furiToLower(b: u8): u8 {
    if (b >= 'A' && b <= 'Z') return b + 32;
    return b;
}

fn furiToUpper(b: u8): u8 {
    if (b >= 'a' && b <= 'z') return b - 32;
    return b;
}

fn furiPush(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn furiPushSlice(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        p = furiPush(out, p, s[i]);
    }
    return p;
}

fn furiPushLower(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        p = furiPush(out, p, furiToLower(s[i]));
    }
    return p;
}

fn furiPushPercentUpper(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == '%' && i + 2 < s.len && furiIsHex(s[i + 1]) && furiIsHex(s[i + 2])) {
            p = furiPush(out, p, '%');
            p = furiPush(out, p, furiToUpper(s[i + 1]));
            p = furiPush(out, p, furiToUpper(s[i + 2]));
            i += 2;
        } else {
            p = furiPush(out, p, s[i]);
        }
    }
    return p;
}

fn furiPushPort(out: []u8, pos: usize, port: i32): usize {
    if (port < 0) return pos;
    let digits: [10]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    let count: usize = 0;
    let v = u32(port);
    if (v == 0) {
        digits[0] = '0';
        count = 1;
    } else {
        while (v > 0 && count < 10) {
            digits[count] = u8('0') + u8(v % 10);
            count += 1;
            v = v / 10;
        }
    }
    let p = pos;
    let k = count;
    while (k > 0) {
        k -= 1;
        p = furiPush(out, p, digits[k]);
    }
    return p;
}

fn furiStrEqLower(a: string, b_lower: string): bool {
    if (a.len != b_lower.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (furiToLower(a[i]) != b_lower[i]) return false;
    }
    return true;
}

fn furiDefaultPort(scheme: string): i32 {
    if (furiStrEqLower(scheme, "http") || furiStrEqLower(scheme, "ws")) return 80;
    if (furiStrEqLower(scheme, "https") || furiStrEqLower(scheme, "wss")) return 443;
    if (furiStrEqLower(scheme, "ftp")) return 21;
    return -1;
}

fn furiHasColon(s: string): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == ':') return true;
    }
    return false;
}

fn furiValidateIpv6Inner(s: string): bool {
    if (s.len == 0) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const b = s[i];
        const ok = furiIsHex(b) || b == ':' || b == '.' || b == 'v' || b == 'V' || b == '_' || b == '-' || b == '~';
        if (!ok) return false;
    }
    return true;
}

fn furiParsePortStr(s: string): i32 {
    if (s.len == 0) return -1;
    if (s.len > 5) return -2;
    let val: i32 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const b = s[i];
        if (!furiIsDigit(b)) return -2;
        val = val * 10 + i32(b - '0');
    }
    if (val > 65535) return -2;
    return val;
}

/// Parse an RFC 3986 URI or relative reference without heap allocation.
pub fn parseUri(input: string): UriComponents {
    // Reject ASCII control characters & whitespace
    let idx: usize = 0;
    while (idx < input.len) : (idx += 1) {
        const b = input[idx];
        if (b <= 0x20 || b == 0x7F) {
            return UriComponents.invalid();
        }
    }

    let res = UriComponents.empty();
    let end = input.len;

    // 1. Extract fragment (#...)
    let hash_pos: usize = end;
    let i: usize = 0;
    while (i < end) : (i += 1) {
        if (input[i] == '#') {
            hash_pos = i;
            break;
        }
    }
    if (hash_pos < end) {
        res.has_fragment = true;
        res.fragment = input[hash_pos + 1..end];
        end = hash_pos;
    }

    // 2. Extract query (?...)
    let q_pos: usize = end;
    let qi: usize = 0;
    while (qi < end) : (qi += 1) {
        if (input[qi] == '?') {
            q_pos = qi;
            break;
        }
    }
    if (q_pos < end) {
        res.has_query = true;
        res.query = input[q_pos + 1..end];
        end = q_pos;
    }

    // 3. Extract scheme (ALPHA *( ALPHA / DIGIT / "+" / "-" / "." ) ":")
    let pos: usize = 0;
    if (pos < end && furiIsAlpha(input[pos])) {
        let s_end: usize = pos + 1;
        while (s_end < end && furiIsSchemeChar(input[s_end])) : (s_end += 1) {}
        if (s_end < end && input[s_end] == ':') {
            res.scheme = input[pos..s_end];
            pos = s_end + 1;
        }
    } else if (pos < end && input[pos] != '/') {
        // Check for malformed scheme like "1http://..."
        let probe: usize = pos;
        while (probe < end && input[probe] != '/' && input[probe] != ':') : (probe += 1) {}
        if (probe < end && input[probe] == ':') {
            return UriComponents.invalid();
        }
    }

    // 4. Extract authority (//...)
    if (pos + 1 < end && input[pos] == '/' && input[pos + 1] == '/') {
        pos += 2;
        let auth_end = pos;
        while (auth_end < end && input[auth_end] != '/') : (auth_end += 1) {}

        const auth = input[pos..auth_end];
        // Find last '@' for userinfo
        let at_idx: usize = auth.len;
        let k: usize = 0;
        while (k < auth.len) : (k += 1) {
            if (auth[k] == '@') {
                at_idx = k;
            }
        }
        let host_start: usize = 0;
        if (at_idx < auth.len) {
            res.userinfo = auth[0..at_idx];
            host_start = at_idx + 1;
        }

        const host_port = auth[host_start..auth.len];
        if (host_port.len > 0 && host_port[0] == '[') {
            // IPv6 / IPvFuture literal
            let close_br: usize = host_port.len;
            let j: usize = 1;
            while (j < host_port.len) : (j += 1) {
                if (host_port[j] == ']') {
                    close_br = j;
                    break;
                }
            }
            if (close_br >= host_port.len) {
                return UriComponents.invalid();
            }
            if (!furiValidateIpv6Inner(host_port[1..close_br])) {
                return UriComponents.invalid();
            }
            res.host = host_port[0..close_br + 1];
            if (close_br + 1 < host_port.len) {
                if (host_port[close_br + 1] != ':') {
                    return UriComponents.invalid();
                }
                const port_str = host_port[close_br + 2..host_port.len];
                const parsed_port = furiParsePortStr(port_str);
                if (parsed_port == -2) {
                    return UriComponents.invalid();
                }
                res.port = parsed_port;
            }
        } else {
            // Regular host[:port]
            let colon_idx: usize = host_port.len;
            let colon_count: usize = 0;
            let j: usize = 0;
            while (j < host_port.len) : (j += 1) {
                if (host_port[j] == ':') {
                    colon_idx = j;
                    colon_count += 1;
                } else if (host_port[j] == '[' || host_port[j] == ']') {
                    return UriComponents.invalid();
                }
            }
            if (colon_count > 1) {
                return UriComponents.invalid();
            }
            if (colon_count == 1) {
                res.host = host_port[0..colon_idx];
                const port_str = host_port[colon_idx + 1..host_port.len];
                const parsed_port = furiParsePortStr(port_str);
                if (parsed_port == -2) {
                    return UriComponents.invalid();
                }
                res.port = parsed_port;
            } else {
                res.host = host_port;
            }
        }
        pos = auth_end;
    }

    // 5. Remaining slice is path
    res.path = input[pos..end];
    return res;
}

/// Serialize `UriComponents` into `out` per RFC 3986 §5.3 (lowercasing scheme and host).
pub fn serializeUri(c: UriComponents, out: []u8): usize {
    if (!c.valid) return 0;
    let pos: usize = 0;
    if (c.scheme.len > 0) {
        pos = furiPushLower(out, pos, c.scheme);
        pos = furiPush(out, pos, ':');
    }
    const has_auth = c.userinfo.len > 0 || c.host.len > 0 || c.port >= 0;
    if (has_auth) {
        pos = furiPush(out, pos, '/');
        pos = furiPush(out, pos, '/');
        if (c.userinfo.len > 0) {
            pos = furiPushSlice(out, pos, c.userinfo);
            pos = furiPush(out, pos, '@');
        }
        if (c.host.len > 0) {
            const need_brackets = furiHasColon(c.host) && c.host[0] != '[';
            if (need_brackets) pos = furiPush(out, pos, '[');
            pos = furiPushLower(out, pos, c.host);
            if (need_brackets) pos = furiPush(out, pos, ']');
        }
        if (c.port >= 0) {
            pos = furiPush(out, pos, ':');
            pos = furiPushPort(out, pos, c.port);
        }
    }
    pos = furiPushSlice(out, pos, c.path);
    if (c.has_query || c.query.len > 0) {
        pos = furiPush(out, pos, '?');
        pos = furiPushSlice(out, pos, c.query);
    }
    if (c.has_fragment || c.fragment.len > 0) {
        pos = furiPush(out, pos, '#');
        pos = furiPushSlice(out, pos, c.fragment);
    }
    return pos;
}

fn furiHasPrefix(s: string, p: string): bool {
    if (s.len < p.len) return false;
    let i: usize = 0;
    while (i < p.len) : (i += 1) {
        if (s[i] != p[i]) return false;
    }
    return true;
}

fn furiPopSegment(out: []u8, out_len: usize): usize {
    if (out_len == 0) return 0;
    let n = out_len;
    while (n > 0) {
        n -= 1;
        if (out[n] == '/') {
            return n;
        }
    }
    return 0;
}

/// RFC 3986 §5.2.4 remove_dot_segments into `out`. Returns output length.
fn furiRemoveDotSegments(input: string, out: []u8): usize {
    let work: [4096]u8 = undefined;
    let w_start: usize = 0;
    let w_end: usize = input.len;
    if (w_end > 4096) w_end = 4096;
    let ci: usize = 0;
    while (ci < w_end) : (ci += 1) {
        work[ci] = input[ci];
    }

    let out_len: usize = 0;
    while (w_start < w_end) {
        const cur: string = work[w_start..w_end];
        // Rule A: prefix "../" or "./" -> remove prefix
        if (furiHasPrefix(cur, "../")) {
            w_start += 3;
            continue;
        }
        if (furiHasPrefix(cur, "./")) {
            w_start += 2;
            continue;
        }
        // Rule B: prefix "/./" or equal "/." -> replace with "/"
        if (furiHasPrefix(cur, "/./")) {
            w_start += 2;
            work[w_start] = '/';
            continue;
        }
        if (cur.len == 2 && cur[0] == '/' && cur[1] == '.') {
            w_start += 1;
            work[w_start] = '/';
            continue;
        }
        // Rule C: prefix "/../" or equal "/.." -> replace with "/" and pop last segment
        if (furiHasPrefix(cur, "/../")) {
            w_start += 3;
            work[w_start] = '/';
            out_len = furiPopSegment(out, out_len);
            continue;
        }
        if (cur.len == 3 && cur[0] == '/' && cur[1] == '.' && cur[2] == '.') {
            w_start += 2;
            work[w_start] = '/';
            out_len = furiPopSegment(out, out_len);
            continue;
        }
        // Rule D: input is "." or ".." -> remove
        if ((cur.len == 1 && cur[0] == '.') || (cur.len == 2 && cur[0] == '.' && cur[1] == '.')) {
            w_start = w_end;
            continue;
        }
        // Rule E: move first path segment (including initial '/' if any) to output
        let seg_end: usize = 0;
        if (cur[0] == '/') {
            seg_end = 1;
        }
        while (seg_end < cur.len && cur[seg_end] != '/') : (seg_end += 1) {}
        let k: usize = 0;
        while (k < seg_end) : (k += 1) {
            out_len = furiPush(out, out_len, cur[k]);
        }
        w_start += seg_end;
    }
    return out_len;
}

/// Resolve relative URI reference `rel` against `base` per RFC 3986 §5.2.
pub fn resolveUri(base: string, rel: string, out: []u8): usize {
    const b = parseUri(base);
    const r = parseUri(rel);
    if (!b.valid || !r.valid) return 0;

    let target = UriComponents.empty();
    let norm_buf: [4096]u8 = undefined;
    let merged_buf: [4096]u8 = undefined;

    const r_has_auth = r.userinfo.len > 0 || r.host.len > 0 || r.port >= 0;
    const b_has_auth = b.userinfo.len > 0 || b.host.len > 0 || b.port >= 0;

    if (r.scheme.len > 0) {
        target.scheme = r.scheme;
        target.userinfo = r.userinfo;
        target.host = r.host;
        target.port = r.port;
        const nlen = furiRemoveDotSegments(r.path, norm_buf[0..]);
        target.path = norm_buf[0..nlen];
        target.query = r.query;
        target.has_query = r.has_query;
    } else {
        if (r_has_auth) {
            target.userinfo = r.userinfo;
            target.host = r.host;
            target.port = r.port;
            const nlen = furiRemoveDotSegments(r.path, norm_buf[0..]);
            target.path = norm_buf[0..nlen];
            target.query = r.query;
            target.has_query = r.has_query;
        } else {
            if (r.path.len == 0) {
                target.path = b.path;
                if (r.has_query) {
                    target.query = r.query;
                    target.has_query = true;
                } else {
                    target.query = b.query;
                    target.has_query = b.has_query;
                }
            } else {
                if (r.path[0] == '/') {
                    const nlen = furiRemoveDotSegments(r.path, norm_buf[0..]);
                    target.path = norm_buf[0..nlen];
                } else {
                    // RFC 3986 §5.2.3 merge
                    let mlen: usize = 0;
                    if (b_has_auth && b.path.len == 0) {
                        mlen = furiPush(merged_buf[0..], mlen, '/');
                        mlen = furiPushSlice(merged_buf[0..], mlen, r.path);
                    } else {
                        let last_slash: usize = 0;
                        let found_slash = false;
                        let si: usize = 0;
                        while (si < b.path.len) : (si += 1) {
                            if (b.path[si] == '/') {
                                last_slash = si;
                                found_slash = true;
                            }
                        }
                        if (found_slash) {
                            mlen = furiPushSlice(merged_buf[0..], mlen, b.path[0..last_slash + 1]);
                        }
                        mlen = furiPushSlice(merged_buf[0..], mlen, r.path);
                    }
                    const nlen = furiRemoveDotSegments(merged_buf[0..mlen], norm_buf[0..]);
                    target.path = norm_buf[0..nlen];
                }
                target.query = r.query;
                target.has_query = r.has_query;
            }
            target.userinfo = b.userinfo;
            target.host = b.host;
            target.port = b.port;
        }
        target.scheme = b.scheme;
    }
    target.fragment = r.fragment;
    target.has_fragment = r.has_fragment;
    return serializeUri(target, out);
}

fn furiNormalizeForCompare(input: string, out: []u8): usize {
    const c = parseUri(input);
    if (!c.valid) return 0;

    let norm_path_buf: [4096]u8 = undefined;
    const nlen = furiRemoveDotSegments(c.path, norm_path_buf[0..]);
    let norm_path: string = norm_path_buf[0..nlen];

    const has_auth = c.userinfo.len > 0 || c.host.len > 0 || c.port >= 0;
    if (has_auth && norm_path.len == 0) {
        norm_path = "/";
    }

    let eff_port = c.port;
    const def_port = furiDefaultPort(c.scheme);
    if (eff_port >= 0 && eff_port == def_port) {
        eff_port = -1;
    }

    let pos: usize = 0;
    if (c.scheme.len > 0) {
        pos = furiPushLower(out, pos, c.scheme);
        pos = furiPush(out, pos, ':');
    }
    if (has_auth) {
        pos = furiPush(out, pos, '/');
        pos = furiPush(out, pos, '/');
        if (c.userinfo.len > 0) {
            pos = furiPushPercentUpper(out, pos, c.userinfo);
            pos = furiPush(out, pos, '@');
        }
        if (c.host.len > 0) {
            const need_brackets = furiHasColon(c.host) && c.host[0] != '[';
            if (need_brackets) pos = furiPush(out, pos, '[');
            pos = furiPushLower(out, pos, c.host);
            if (need_brackets) pos = furiPush(out, pos, ']');
        }
        if (eff_port >= 0) {
            pos = furiPush(out, pos, ':');
            pos = furiPushPort(out, pos, eff_port);
        }
    }
    pos = furiPushPercentUpper(out, pos, norm_path);
    if (c.has_query || c.query.len > 0) {
        pos = furiPush(out, pos, '?');
        pos = furiPushPercentUpper(out, pos, c.query);
    }
    if (c.has_fragment || c.fragment.len > 0) {
        pos = furiPush(out, pos, '#');
        pos = furiPushPercentUpper(out, pos, c.fragment);
    }
    return pos;
}

/// Compare two URIs for RFC 3986 normalized equivalence.
pub fn equalUri(a: string, b: string): bool {
    const ca = parseUri(a);
    const cb = parseUri(b);
    if (!ca.valid || !cb.valid) return false;

    let buf_a: [4096]u8 = undefined;
    let buf_b: [4096]u8 = undefined;
    const len_a = furiNormalizeForCompare(a, buf_a[0..]);
    const len_b = furiNormalizeForCompare(b, buf_b[0..]);
    if (len_a != len_b) return false;

    let i: usize = 0;
    while (i < len_a) : (i += 1) {
        if (buf_a[i] != buf_b[i]) return false;
    }
    return true;
}
