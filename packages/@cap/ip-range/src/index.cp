// @cap/ip-range — Zero-heap IPv4 address, CIDR subnet & RFC special-use classifier
// MIT License (clean-room CP implementation)

pub struct Ipv4Result {
    pub ip: u32,
    pub valid: bool,
}

pub struct Ipv4Cidr {
    pub network: u32,
    pub broadcast: u32,
    pub mask: u32,
    pub prefix_len: u8,
    pub valid: bool,
}

fn parseOctetStrict(s: string) -> i32 {
    if (s.len == 0 || s.len > 3) {
        return -1;
    }
    // Reject leading zeros (e.g. "01", "010") to prevent octal ambiguity
    if (s.len > 1 && s[0] == '0' as u8) {
        return -1;
    }
    let val: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c: u8 = s[i];
        if (c < '0' as u8 || c > '9' as u8) {
            return -1;
        }
        val = val * 10 + ((c - ('0' as u8)) as u32);
        if (val > 255) {
            return -1;
        }
        i += 1;
    }
    return val as i32;
}

pub fn parseIpv4(s: string) -> Ipv4Result {
    const fail = Ipv4Result { ip: 0, valid: false };
    if (s.len < 7 || s.len > 15) {
        return fail;
    }

    let octets: [4]u32 = [0, 0, 0, 0];
    let pos: usize = 0;
    let part: usize = 0;

    while (part < 4) {
        let dot: usize = pos;
        while (dot < s.len && s[dot] != '.' as u8) {
            dot += 1;
        }
        const oct: i32 = parseOctetStrict(s[pos..dot]);
        if (oct < 0) {
            return fail;
        }
        octets[part] = oct as u32;
        part += 1;
        if (part < 4) {
            if (dot >= s.len) {
                return fail;
            }
            pos = dot + 1;
        } else {
            if (dot != s.len) {
                return fail;
            }
        }
    }

    const ip: u32 = (octets[0] << 24) | (octets[1] << 16) | (octets[2] << 8) | octets[3];
    return Ipv4Result { ip: ip, valid: true };
}

fn appendU8Dec(out: []u8, pos: usize, v: u8) -> i64 {
    let digits: [3]u8 = undefined;
    let count: usize = 0;
    if (v == 0) {
        digits[0] = '0' as u8;
        count = 1;
    } else {
        let cur: u8 = v;
        while (cur > 0) {
            digits[count] = ('0' as u8) + (cur % 10);
            count += 1;
            cur = cur / 10;
        }
    }
    if (count > out.len - pos) {
        return -1;
    }
    let p: usize = pos;
    let k: usize = count;
    while (k > 0) {
        k -= 1;
        out[p] = digits[k];
        p += 1;
    }
    return p as i64;
}

pub fn formatIpv4(ip: u32, out: []u8) -> usize {
    const o0: u8 = ((ip >> 24) & 0xFF) as u8;
    const o1: u8 = ((ip >> 16) & 0xFF) as u8;
    const o2: u8 = ((ip >> 8) & 0xFF) as u8;
    const o3: u8 = (ip & 0xFF) as u8;

    let p: i64 = appendU8Dec(out, 0, o0);
    if (p < 0 || (p as usize) >= out.len) {
        return 0;
    }
    out[p as usize] = '.' as u8;
    p = appendU8Dec(out, (p as usize) + 1, o1);
    if (p < 0 || (p as usize) >= out.len) {
        return 0;
    }
    out[p as usize] = '.' as u8;
    p = appendU8Dec(out, (p as usize) + 1, o2);
    if (p < 0 || (p as usize) >= out.len) {
        return 0;
    }
    out[p as usize] = '.' as u8;
    p = appendU8Dec(out, (p as usize) + 1, o3);
    if (p < 0) {
        return 0;
    }
    return p as usize;
}

pub fn parseCidr(s: string) -> Ipv4Cidr {
    const fail = Ipv4Cidr {
        network: 0,
        broadcast: 0,
        mask: 0,
        prefix_len: 0,
        valid: false,
    };

    let slash: usize = 0;
    while (slash < s.len && s[slash] != '/' as u8) {
        slash += 1;
    }
    if (slash == 0 || slash + 1 >= s.len) {
        return fail;
    }

    const ip_res: Ipv4Result = parseIpv4(s[0..slash]);
    if (!ip_res.valid) {
        return fail;
    }

    const pfx_i: i32 = parseOctetStrict(s[(slash + 1)..s.len]);
    if (pfx_i < 0 || pfx_i > 32) {
        return fail;
    }

    const pfx: u8 = pfx_i as u8;
    let mask: u32 = 0;
    if (pfx == 0) {
        mask = 0;
    } else if (pfx == 32) {
        mask = 0xFFFFFFFF;
    } else {
        const shift: u5 = (32 - (pfx as u32)) as u5;
        mask = (0xFFFFFFFF as u32) << shift;
    }

    const network: u32 = ip_res.ip & mask;
    const broadcast: u32 = network | (mask ^ 0xFFFFFFFF);
    return Ipv4Cidr {
        network: network,
        broadcast: broadcast,
        mask: mask,
        prefix_len: pfx,
        valid: true,
    };
}

pub fn cidrContains(cidr: Ipv4Cidr, ip: u32) -> bool {
    if (!cidr.valid) {
        return false;
    }
    return ip >= cidr.network && ip <= cidr.broadcast;
}

pub fn cidrOverlaps(a: Ipv4Cidr, b: Ipv4Cidr) -> bool {
    if (!a.valid || !b.valid) {
        return false;
    }
    return a.network <= b.broadcast && b.network <= a.broadcast;
}

pub fn isPrivateIpv4(ip: u32) -> bool {
    const a: u32 = (ip >> 24) & 0xFF;
    const b: u32 = (ip >> 16) & 0xFF;
    if (a == 10) {
        return true;
    }
    if (a == 172 && b >= 16 && b <= 31) {
        return true;
    }
    if (a == 192 && b == 168) {
        return true;
    }
    return false;
}

pub fn isLoopbackIpv4(ip: u32) -> bool {
    return ((ip >> 24) & 0xFF) == 127;
}

pub fn isLinkLocalIpv4(ip: u32) -> bool {
    const a: u32 = (ip >> 24) & 0xFF;
    const b: u32 = (ip >> 16) & 0xFF;
    return a == 169 && b == 254;
}

pub fn isPublicIpv4(ip: u32) -> bool {
    if (isPrivateIpv4(ip) || isLoopbackIpv4(ip) || isLinkLocalIpv4(ip)) {
        return false;
    }
    const a: u32 = (ip >> 24) & 0xFF;
    const b: u32 = (ip >> 16) & 0xFF;
    // 0.0.0.0/8 ("this network")
    if (a == 0) {
        return false;
    }
    // 100.64.0.0/10 (RFC 6598 Shared / CGNAT)
    if (a == 100 && b >= 64 && b <= 127) {
        return false;
    }
    // 224.0.0.0/4 multicast, 240.0.0.0/4 reserved & 255.255.255.255 broadcast
    if (a >= 224) {
        return false;
    }
    return true;
}
