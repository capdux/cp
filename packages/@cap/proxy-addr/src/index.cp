// @cap/proxy-addr — Pure CP zero-allocation reverse-proxy client IP resolver
// Parity with npm `proxy-addr` (Express req.ip / req.ips trusted proxy chain resolution).

pub const TRUST_NONE: u8 = 0;
pub const TRUST_LOOPBACK: u8 = 1;
pub const TRUST_LINKLOCAL: u8 = 2;
pub const TRUST_UNIQUELOCAL: u8 = 3;
pub const TRUST_ALL_PRIVATE: u8 = 4;

pub struct IpSpan {
    pub start: usize,
    pub len: usize,
}

struct PaIpv4 {
    ok: bool,
    o0: u8,
    o1: u8,
    o2: u8,
    o3: u8,
}

struct PaIpv6Info {
    ok: bool,
    is_loopback: bool,
    first_word: u16,
}

fn paIsSpace(c: u8): bool {
    return c == (' ' as u8) || c == ('\t' as u8) || c == ('\r' as u8) || c == ('\n' as u8);
}

fn paToLower(c: u8): u8 {
    if (c >= ('A' as u8) && c <= ('Z' as u8)) {
        return c + (32 as u8);
    }
    return c;
}

fn paHexVal(c: u8): i32 {
    if (c >= ('0' as u8) && c <= ('9' as u8)) {
        return (c - ('0' as u8)) as i32;
    }
    let lc = paToLower(c);
    if (lc >= ('a' as u8) && lc <= ('f' as u8)) {
        return ((lc - ('a' as u8)) as i32) + 10;
    }
    return -1;
}

fn paTrimBounds(s: string): IpSpan {
    let start: usize = 0;
    while (start < s.len && paIsSpace(s[start])) {
        start = start + 1;
    }
    let end: usize = s.len;
    while (end > start && paIsSpace(s[end - 1])) {
        end = end - 1;
    }
    return IpSpan { start: start, len: end - start };
}

fn paStripV4Mapped(s: string): string {
    // Strip case-insensitive "::ffff:" prefix if present
    if (s.len > 7 && s[0] == (':' as u8) && s[1] == (':' as u8) && s[6] == (':' as u8)) {
        if (paToLower(s[2]) == ('f' as u8)
            && paToLower(s[3]) == ('f' as u8)
            && paToLower(s[4]) == ('f' as u8)
            && paToLower(s[5]) == ('f' as u8))
        {
            return s[7..s.len];
        }
    }
    return s;
}

fn paParseIpv4(s: string): PaIpv4 {
    let fail = PaIpv4 { ok: false, o0: 0, o1: 0, o2: 0, o3: 0 };
    if (s.len < 7 || s.len > 15) {
        return fail;
    }
    let octs: [4]u8 = [0, 0, 0, 0];
    let oct_idx: usize = 0;
    let i: usize = 0;

    while (oct_idx < 4) {
        if (i >= s.len) {
            return fail;
        }
        let val: u32 = 0;
        let digits: usize = 0;
        while (i < s.len && s[i] >= ('0' as u8) && s[i] <= ('9' as u8)) {
            val = val * 10 + ((s[i] - ('0' as u8)) as u32);
            digits = digits + 1;
            if (digits > 3 || val > 255) {
                return fail;
            }
            i = i + 1;
        }
        if (digits == 0) {
            return fail;
        }
        octs[oct_idx] = val as u8;
        oct_idx = oct_idx + 1;
        if (oct_idx < 4) {
            if (i >= s.len || s[i] != ('.' as u8)) {
                return fail;
            }
            i = i + 1;
        }
    }

    if (i != s.len) {
        return fail;
    }
    return PaIpv4 { ok: true, o0: octs[0], o1: octs[1], o2: octs[2], o3: octs[3] };
}

fn paParseIpv6Info(s: string): PaIpv6Info {
    let fail = PaIpv6Info { ok: false, is_loopback: false, first_word: 0 };
    if (s.len < 2) {
        return fail;
    }
    // Strip optional zone id (%eth0)
    let end: usize = s.len;
    let zi: usize = 0;
    while (zi < s.len) {
        if (s[zi] == ('%' as u8)) {
            end = zi;
            break;
        }
        zi = zi + 1;
    }
    if (end < 2) {
        return fail;
    }

    let has_colon = false;
    let double_colons: usize = 0;
    let first_word: u16 = 0;
    let got_first = false;
    let all_prev_zero = true;
    let last_word: u16 = 0;
    let groups: usize = 0;
    let i: usize = 0;

    while (i < end) {
        if (s[i] == (':' as u8)) {
            has_colon = true;
            if (i + 1 < end && s[i + 1] == (':' as u8)) {
                double_colons = double_colons + 1;
                if (double_colons > 1) {
                    return fail;
                }
                if (!got_first) {
                    first_word = 0;
                    got_first = true;
                }
                i = i + 2;
                continue;
            }
            if (i == 0 || i + 1 == end) {
                return fail;
            }
            i = i + 1;
            continue;
        }

        let word: u32 = 0;
        let hex_len: usize = 0;
        while (i < end && s[i] != (':' as u8)) {
            let hv = paHexVal(s[i]);
            if (hv < 0) {
                return fail;
            }
            word = (word << 4) | (hv as u32);
            hex_len = hex_len + 1;
            if (hex_len > 4) {
                return fail;
            }
            i = i + 1;
        }
        if (hex_len == 0) {
            return fail;
        }
        if (!got_first) {
            first_word = word as u16;
            got_first = true;
        }
        if (groups > 0 && last_word != 0) {
            all_prev_zero = false;
        }
        last_word = word as u16;
        groups = groups + 1;
        if (groups > 8) {
            return fail;
        }
    }

    if (!has_colon) {
        return fail;
    }
    if (double_colons == 0 && groups != 8) {
        return fail;
    }

    // Check loopback: "::1" or "0:0:0:0:0:0:0:1" (ends with hex word 1, all preceding words 0, no trailing "::")
    let ends_with_colon = s[end - 1] == (':' as u8);
    let is_lb = !ends_with_colon && all_prev_zero && last_word == 1 && groups >= 1;
    return PaIpv6Info {
        ok: true,
        is_loopback: is_lb,
        first_word: first_word,
    };
}

pub fn isLoopback(ip: string): bool {
    let b = paTrimBounds(ip);
    if (b.len == 0) {
        return false;
    }
    let trimmed = ip[b.start..(b.start + b.len)];
    let mapped = paStripV4Mapped(trimmed);
    let v4 = paParseIpv4(mapped);
    if (v4.ok) {
        return v4.o0 == 127;
    }
    let v6 = paParseIpv6Info(trimmed);
    if (v6.ok) {
        return v6.is_loopback;
    }
    return false;
}

pub fn isLinkLocal(ip: string): bool {
    let b = paTrimBounds(ip);
    if (b.len == 0) {
        return false;
    }
    let trimmed = ip[b.start..(b.start + b.len)];
    let mapped = paStripV4Mapped(trimmed);
    let v4 = paParseIpv4(mapped);
    if (v4.ok) {
        return v4.o0 == 169 && v4.o1 == 254;
    }
    let v6 = paParseIpv6Info(trimmed);
    if (v6.ok) {
        return (v6.first_word & 0xFFC0) == 0xFE80;
    }
    return false;
}

pub fn isUniqueLocal(ip: string): bool {
    let b = paTrimBounds(ip);
    if (b.len == 0) {
        return false;
    }
    let trimmed = ip[b.start..(b.start + b.len)];
    let mapped = paStripV4Mapped(trimmed);
    let v4 = paParseIpv4(mapped);
    if (v4.ok) {
        if (v4.o0 == 10) {
            return true;
        }
        if (v4.o0 == 172 && v4.o1 >= 16 && v4.o1 <= 31) {
            return true;
        }
        if (v4.o0 == 192 && v4.o1 == 168) {
            return true;
        }
        return false;
    }
    let v6 = paParseIpv6Info(trimmed);
    if (v6.ok) {
        return (v6.first_word & 0xFE00) == 0xFC00;
    }
    return false;
}

pub fn isTrustedBuiltin(ip: string, preset: u8): bool {
    if (preset == TRUST_NONE) {
        return false;
    }
    if (preset == TRUST_LOOPBACK) {
        return isLoopback(ip);
    }
    if (preset == TRUST_LINKLOCAL) {
        return isLinkLocal(ip);
    }
    if (preset == TRUST_UNIQUELOCAL) {
        return isUniqueLocal(ip);
    }
    if (preset == TRUST_ALL_PRIVATE) {
        return isLoopback(ip) || isLinkLocal(ip) || isUniqueLocal(ip);
    }
    return false;
}

// Parse comma-separated X-Forwarded-For addresses in nearest-to-farthest order (right-to-left)
pub fn parseXffAddresses(remote_addr: string, xff_header: string, out: []IpSpan): usize {
    let rb = paTrimBounds(remote_addr);
    if (rb.len == 0 || xff_header.len == 0 || out.len == 0) {
        return 0;
    }

    let count: usize = 0;
    let seg_end: usize = xff_header.len;
    let pos: usize = xff_header.len;

    while (pos > 0 && count < out.len) {
        pos = pos - 1;
        if (xff_header[pos] == (',' as u8)) {
            let raw = xff_header[(pos + 1)..seg_end];
            let tb = paTrimBounds(raw);
            if (tb.len > 0) {
                out[count] = IpSpan { start: pos + 1 + tb.start, len: tb.len };
                count = count + 1;
            }
            seg_end = pos;
        }
    }

    if (count < out.len) {
        let raw = xff_header[0..seg_end];
        let tb = paTrimBounds(raw);
        if (tb.len > 0) {
            out[count] = IpSpan { start: tb.start, len: tb.len };
            count = count + 1;
        }
    }

    return count;
}

fn paCopyToOut(src: string, out: []u8): usize {
    if (src.len == 0 || out.len < src.len) {
        return 0;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[i] = src[i];
        i = i + 1;
    }
    return src.len;
}

pub fn resolveClientIp(
    remote_addr: string,
    xff_header: string,
    trust_preset: u8,
    max_hops: usize,
    out: []u8
): usize {
    let rb = paTrimBounds(remote_addr);
    if (rb.len == 0) {
        return 0;
    }
    let candidate = remote_addr[rb.start..(rb.start + rb.len)];

    if (!isTrustedBuiltin(candidate, trust_preset) || xff_header.len == 0) {
        return paCopyToOut(candidate, out);
    }

    let spans: [64]IpSpan = undefined;
    let n = parseXffAddresses(candidate, xff_header, spans[0..64]);
    let hops: usize = 0;
    let idx: usize = 0;

    while (idx < n && isTrustedBuiltin(candidate, trust_preset)) {
        if (max_hops > 0 && hops >= max_hops) {
            break;
        }
        let sp = spans[idx];
        candidate = xff_header[sp.start..(sp.start + sp.len)];
        hops = hops + 1;
        idx = idx + 1;
    }

    return paCopyToOut(candidate, out);
}

export {
    TRUST_NONE,
    TRUST_LOOPBACK,
    TRUST_LINKLOCAL,
    TRUST_UNIQUELOCAL,
    TRUST_ALL_PRIVATE,
    IpSpan,
    isLoopback,
    isLinkLocal,
    isUniqueLocal,
    isTrustedBuiltin,
    parseXffAddresses,
    resolveClientIp
};
