// @cap/ipnet — Zero-heap pure-CP IPv4 & CIDR subnet calculation engine
// Clean-room MIT equivalent of crates.io `ipnet`.

fn ipnParseIpv4Range(s: string, start: usize, end: usize): u32? {
    if (start >= end) {
        return null;
    }
    let result: u32 = 0;
    let parts: usize = 0;
    let i: usize = start;

    while (i < end) {
        if (parts >= 4) {
            return null;
        }
        let seg_start: usize = i;
        let val: u32 = 0;
        let digits: usize = 0;

        while (i < end && s[i] != 46) { // '.'
            let ch: u8 = s[i];
            if (ch < 48 || ch > 57) {
                return null;
            }
            if (digits >= 3) {
                return null;
            }
            val = val * 10 + ((ch - 48) as u32);
            digits = digits + 1;
            i = i + 1;
        }

        if (digits == 0) {
            return null;
        }
        // Reject leading zeros like "01" or "00"
        if (digits > 1 && s[seg_start] == 48) {
            return null;
        }
        if (val > 255) {
            return null;
        }

        result = (result << 8) | val;
        parts = parts + 1;

        if (i < end) {
            // Must be '.' and cannot be trailing dot
            if (s[i] != 46 || i + 1 == end) {
                return null;
            }
            i = i + 1;
        }
    }

    if (parts != 4) {
        return null;
    }
    return result;
}

fn ipnParsePrefixRange(s: string, start: usize, end: usize): u8? {
    if (start >= end) {
        return null;
    }
    let digits: usize = end - start;
    if (digits > 2) {
        return null;
    }
    if (digits > 1 && s[start] == 48) {
        return null;
    }
    let val: u32 = 0;
    let i: usize = start;
    while (i < end) {
        let ch: u8 = s[i];
        if (ch < 48 || ch > 57) {
            return null;
        }
        val = val * 10 + ((ch - 48) as u32);
        i = i + 1;
    }
    if (val > 32) {
        return null;
    }
    return val as u8;
}

fn ipnWriteU8Dec(v: u8, out: []u8, pos: usize): usize {
    let p: usize = pos;
    if (v >= 100) {
        let d2: u8 = v / 100;
        let rem2: u8 = v % 100;
        let d1: u8 = rem2 / 10;
        let d0: u8 = rem2 % 10;
        if (p < out.len) { out[p] = 48 + d2; p = p + 1; }
        if (p < out.len) { out[p] = 48 + d1; p = p + 1; }
        if (p < out.len) { out[p] = 48 + d0; p = p + 1; }
        return p;
    }
    if (v >= 10) {
        let d1: u8 = v / 10;
        let d0: u8 = v % 10;
        if (p < out.len) { out[p] = 48 + d1; p = p + 1; }
        if (p < out.len) { out[p] = 48 + d0; p = p + 1; }
        return p;
    }
    if (p < out.len) { out[p] = 48 + v; p = p + 1; }
    return p;
}

fn ipnNetmask(prefix_len: u8): u32 {
    if (prefix_len == 0) {
        return 0;
    }
    if (prefix_len >= 32) {
        return 4294967295; // 0xFFFFFFFF
    }
    let m: u32 = 0;
    let i: u8 = 0;
    while (i < prefix_len) {
        m = (m >> 1) | 2147483648; // 0x80000000
        i = i + 1;
    }
    return m;
}

pub fn parseIpv4(s: string): u32? {
    return ipnParseIpv4Range(s, 0, s.len);
}

pub fn formatIpv4(ip: u32, out: []u8): usize {
    let o0: u8 = ((ip >> 24) & 255) as u8;
    let o1: u8 = ((ip >> 16) & 255) as u8;
    let o2: u8 = ((ip >> 8) & 255) as u8;
    let o3: u8 = (ip & 255) as u8;

    let p: usize = 0;
    p = ipnWriteU8Dec(o0, out, p);
    if (p < out.len) { out[p] = 46; p = p + 1; }
    p = ipnWriteU8Dec(o1, out, p);
    if (p < out.len) { out[p] = 46; p = p + 1; }
    p = ipnWriteU8Dec(o2, out, p);
    if (p < out.len) { out[p] = 46; p = p + 1; }
    p = ipnWriteU8Dec(o3, out, p);
    return p;
}

pub struct Ipv4Net {
    pub addr: u32,
    pub prefix_len: u8,

    pub static fn new(addr: u32, prefix_len: u8): Ipv4Net? {
        if (prefix_len > 32) {
            return null;
        }
        return Ipv4Net { addr: addr, prefix_len: prefix_len };
    }

    pub static fn parse(cidr: string): Ipv4Net? {
        let slash: usize = cidr.len;
        let i: usize = 0;
        while (i < cidr.len) {
            if (cidr[i] == 47) { // '/'
                if (slash != cidr.len) {
                    return null;
                }
                slash = i;
            }
            i = i + 1;
        }
        if (slash == cidr.len || slash == 0 || slash + 1 >= cidr.len) {
            return null;
        }
        let ip_opt = ipnParseIpv4Range(cidr, 0, slash);
        if (ip_opt == null) {
            return null;
        }
        let prefix_opt = ipnParsePrefixRange(cidr, slash + 1, cidr.len);
        if (prefix_opt == null) {
            return null;
        }
        let ip: u32 = ip_opt ?? 0;
        let pfx: u8 = prefix_opt ?? 0;
        return Ipv4Net { addr: ip, prefix_len: pfx };
    }

    pub fn netmask(): u32 {
        return ipnNetmask(_.prefix_len);
    }

    pub fn hostmask(): u32 {
        return ipnNetmask(_.prefix_len) ^ 4294967295;
    }

    pub fn network(): u32 {
        return _.addr & ipnNetmask(_.prefix_len);
    }

    pub fn broadcast(): u32 {
        return _.network() | _.hostmask();
    }

    pub fn trunc(): Ipv4Net {
        return Ipv4Net { addr: _.network(), prefix_len: _.prefix_len };
    }

    pub fn containsIp(ip: u32): bool {
        let mask: u32 = ipnNetmask(_.prefix_len);
        return (ip & mask) == (_.addr & mask);
    }

    pub fn containsNet(other: Ipv4Net): bool {
        if (_.prefix_len > other.prefix_len) {
            return false;
        }
        return _.containsIp(other.network());
    }

    pub fn supernet(): Ipv4Net? {
        if (_.prefix_len == 0) {
            return null;
        }
        let new_prefix: u8 = _.prefix_len - 1;
        let new_mask: u32 = ipnNetmask(new_prefix);
        return Ipv4Net { addr: _.addr & new_mask, prefix_len: new_prefix };
    }

    pub fn hostCount(): u64 {
        if (_.prefix_len == 32) {
            return 1;
        }
        if (_.prefix_len == 31) {
            return 2;
        }
        let host_bits: u8 = 32 - _.prefix_len;
        let total: u64 = 1;
        let i: u8 = 0;
        while (i < host_bits) {
            total = total * 2;
            i = i + 1;
        }
        return total - 2;
    }

    pub fn formatCidr(out: []u8): usize {
        let p: usize = formatIpv4(_.addr, out);
        if (p < out.len) {
            out[p] = 47; // '/'
            p = p + 1;
        }
        p = ipnWriteU8Dec(_.prefix_len, out, p);
        return p;
    }
}
