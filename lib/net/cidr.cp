// Source: cp/lib/net/cidr.cp (RFC 4632 / RFC 4291 / RFC 1918)
// lib/net/cidr.cp
// Pure CP IPv4 / IPv6 CIDR parsing, matching, classification & SSRF allowlist

import { Ipv4Addr, Ipv6Addr } from "./address";

pub enum IpKind: u8 {
    Unspecified = 0,
    Loopback = 1,
    Private = 2,
    LinkLocal = 3,
    Cgnat = 4,
    Multicast = 5,
    Broadcast = 6,
    Documentation = 7,
    GlobalUnicast = 8,
    Reserved = 9,
}

pub enum IpAddr {
    V4: Ipv4Addr,
    V6: Ipv6Addr,

    static fn parse(text: []const u8): IpAddr? {
        if (v4 := Ipv4Addr.parse(text)) {
            return IpAddr.V4(v4);
        }
        if (v6 := IpAddrCodec.parseV6(text)) {
            return IpAddr.V6(v6);
        }
        return null;
    }

    fn isV4(): bool {
        switch (_) {
            .V4 => return true,
            .V6 => return false,
        }
    }

    fn isV6(): bool {
        return !_.isV4();
    }
}

pub static struct IpAddrCodec {
    fn writeDecU8(out: []u8, pos: usize, val: u8): usize {
        if (val >= 100) {
            if (pos + 3 > out.len) { return 0; }
            out[pos] = (48 as u8) + (val / 100);
            out[pos + 1] = (48 as u8) + ((val / 10) % 10);
            out[pos + 2] = (48 as u8) + (val % 10);
            return 3;
        }
        if (val >= 10) {
            if (pos + 2 > out.len) { return 0; }
            out[pos] = (48 as u8) + (val / 10);
            out[pos + 1] = (48 as u8) + (val % 10);
            return 2;
        }
        if (pos + 1 > out.len) { return 0; }
        out[pos] = (48 as u8) + val;
        return 1;
    }

    fn writeHexU16(out: []u8, pos: usize, val: u16): usize {
        if (val == 0) {
            if (pos + 1 > out.len) { return 0; }
            out[pos] = '0';
            return 1;
        }
        let tmp: [4]u8 = [0; 4];
        let len: usize = 0;
        let v: u16 = val;
        while (v > 0) {
            const nib = (v & 0xF) as u8;
            tmp[len] = if (nib < 10) ((48 as u8) + nib) else ((87 as u8) + nib);
            len += 1;
            v >>= 4;
        }
        if (pos + len > out.len) { return 0; }
        let i: usize = 0;
        while (i < len) {
            out[pos + i] = tmp[len - 1 - i];
            i += 1;
        }
        return len;
    }

    fn hexVal(ch: u8): u16? {
        if (ch >= '0' && ch <= '9') { return (ch - '0') as u16; }
        if (ch >= 'a' && ch <= 'f') { return (ch - 'a' + 10) as u16; }
        if (ch >= 'A' && ch <= 'F') { return (ch - 'A' + 10) as u16; }
        return null;
    }

    fn parseDecU8Strict(text: []const u8): u8? {
        if (text.len == 0 || text.len > 3) { return null; }
        if (text.len > 1 && text[0] == '0') { return null; }
        let acc: u16 = 0;
        let i: usize = 0;
        while (i < text.len) {
            const ch = text[i];
            if (ch < '0' || ch > '9') { return null; }
            acc = acc * 10 + ((ch - '0') as u16);
            i += 1;
        }
        if (acc > 255) { return null; }
        return acc as u8;
    }

    fn parseV4(text: []const u8): Ipv4Addr? {
        return Ipv4Addr.parse(text);
    }

    fn formatV4(ip: Ipv4Addr, out: []u8): usize {
        let pos: usize = 0;
        let i: usize = 0;
        while (i < 4) {
            if (i > 0) {
                if (pos + 1 > out.len) { return 0; }
                out[pos] = '.';
                pos += 1;
            }
            const n = IpAddrCodec.writeDecU8(out, pos, ip.octets[i]);
            if (n == 0) { return 0; }
            pos += n;
            i += 1;
        }
        return pos;
    }

    fn parseV6(text: []const u8): Ipv6Addr? {
        if (text.len < 2 || text.len > 45) { return null; }

        let left: [8]u16 = [0; 8];
        let right: [8]u16 = [0; 8];
        let left_count: usize = 0;
        let right_count: usize = 0;
        let has_double_colon = false;

        let i: usize = 0;
        if (text[0] == ':') {
            if (text.len < 2 || text[1] != ':') { return null; }
            has_double_colon = true;
            i = 2;
        }

        while (i < text.len) {
            // Scan token up to next ':'
            let seg_start = i;
            let has_dot = false;
            while (i < text.len && text[i] != ':') {
                if (text[i] == '.') {
                    has_dot = true;
                }
                i += 1;
            }

            if (has_dot) {
                // Embedded IPv4 must be the trailing part
                if (i != text.len) { return null; }
                const v4_slice = text[seg_start..text.len];
                const v4 = Ipv4Addr.parse(v4_slice) ?? return null;
                const hi: u16 = ((v4.octets[0] as u16) << 8) | (v4.octets[1] as u16);
                const lo: u16 = ((v4.octets[2] as u16) << 8) | (v4.octets[3] as u16);
                if (has_double_colon) {
                    if (right_count + 2 > 8) { return null; }
                    right[right_count] = hi;
                    right[right_count + 1] = lo;
                    right_count += 2;
                } else {
                    if (left_count + 2 > 8) { return null; }
                    left[left_count] = hi;
                    left[left_count + 1] = lo;
                    left_count += 2;
                }
                break;
            }

            const seg_len = i - seg_start;
            if (seg_len == 0 || seg_len > 4) { return null; }

            let val: u16 = 0;
            let k: usize = seg_start;
            while (k < i) {
                const hv = IpAddrCodec.hexVal(text[k]) ?? return null;
                val = (val << 4) | hv;
                k += 1;
            }

            if (has_double_colon) {
                if (right_count >= 8) { return null; }
                right[right_count] = val;
                right_count += 1;
            } else {
                if (left_count >= 8) { return null; }
                left[left_count] = val;
                left_count += 1;
            }

            if (i < text.len) {
                // Consume ':'
                i += 1;
                if (i == text.len) {
                    // Trailing single ':' is invalid
                    return null;
                }
                if (text[i] == ':') {
                    if (has_double_colon) {
                        // Multiple '::' is invalid
                        return null;
                    }
                    has_double_colon = true;
                    i += 1;
                }
            }
        }

        let segs: [8]u16 = [0; 8];
        if (has_double_colon) {
            if (left_count + right_count >= 8) { return null; }
            let idx: usize = 0;
            while (idx < left_count) {
                segs[idx] = left[idx];
                idx += 1;
            }
            const right_start = 8 - right_count;
            idx = 0;
            while (idx < right_count) {
                segs[right_start + idx] = right[idx];
                idx += 1;
            }
        } else {
            if (left_count != 8) { return null; }
            let idx: usize = 0;
            while (idx < 8) {
                segs[idx] = left[idx];
                idx += 1;
            }
        }

        return Ipv6Addr { segments: segs };
    }

    fn formatV6(ip: Ipv6Addr, out: []u8): usize {
        // Find longest run of >= 2 zero segments (RFC 5952)
        let best_start: usize = 8;
        let best_len: usize = 0;
        let cur_start: usize = 0;
        let cur_len: usize = 0;

        let i: usize = 0;
        while (i < 8) {
            if (ip.segments[i] == 0) {
                if (cur_len == 0) {
                    cur_start = i;
                }
                cur_len += 1;
                if (cur_len > best_len) {
                    best_start = cur_start;
                    best_len = cur_len;
                }
            } else {
                cur_len = 0;
            }
            i += 1;
        }
        if (best_len < 2) {
            best_start = 8;
            best_len = 0;
        }

        let pos: usize = 0;
        i = 0;
        while (i < 8) {
            if (best_len >= 2 && i == best_start) {
                if (pos + 2 > out.len) { return 0; }
                out[pos] = ':';
                out[pos + 1] = ':';
                pos += 2;
                i += best_len;
                continue;
            }
            if (i > 0 && !(best_len >= 2 && i == best_start + best_len)) {
                if (pos + 1 > out.len) { return 0; }
                out[pos] = ':';
                pos += 1;
            }
            const n = IpAddrCodec.writeHexU16(out, pos, ip.segments[i]);
            if (n == 0) { return 0; }
            pos += n;
            i += 1;
        }
        return pos;
    }

    fn v6ToU128(ip: Ipv6Addr): u128 {
        let acc: u128 = 0;
        let i: usize = 0;
        while (i < 8) {
            acc = (acc << 16) | (ip.segments[i] as u128);
            i += 1;
        }
        return acc;
    }

    fn v6FromU128(val: u128): Ipv6Addr {
        let segs: [8]u16 = [0; 8];
        let cur = val;
        let i: usize = 8;
        while (i > 0) {
            i -= 1;
            segs[i] = (cur & 0xFFFF) as u16;
            cur >>= 16;
        }
        return Ipv6Addr { segments: segs };
    }

    fn v6ToBytes(ip: Ipv6Addr): [16]u8 {
        let out: [16]u8 = [0; 16];
        let i: usize = 0;
        while (i < 8) {
            out[i * 2] = ((ip.segments[i] >> 8) & 0xFF) as u8;
            out[i * 2 + 1] = (ip.segments[i] & 0xFF) as u8;
            i += 1;
        }
        return out;
    }

    fn v6FromBytes(bytes: [16]u8): Ipv6Addr {
        let segs: [8]u16 = [0; 8];
        let i: usize = 0;
        while (i < 8) {
            segs[i] = ((bytes[i * 2] as u16) << 8) | (bytes[i * 2 + 1] as u16);
            i += 1;
        }
        return Ipv6Addr { segments: segs };
    }

    fn v4Equal(a: Ipv4Addr, b: Ipv4Addr): bool {
        return a.toU32() == b.toU32();
    }

    fn v6Equal(a: Ipv6Addr, b: Ipv6Addr): bool {
        return IpAddrCodec.v6ToU128(a) == IpAddrCodec.v6ToU128(b);
    }
}

pub struct Ipv4Cidr {
    addr: Ipv4Addr = Ipv4Addr {};
    prefix_len: u8 = 0;

    static fn maskForPrefix(prefix_len: u8): u32 {
        if (prefix_len == 0) { return 0; }
        if (prefix_len >= 32) { return 0xFFFFFFFF; }
        let mask: u32 = 0xFFFFFFFF;
        let rem = 32 - prefix_len;
        while (rem > 0) {
            mask <<= 1;
            rem -= 1;
        }
        return mask;
    }

    static fn init(addr: Ipv4Addr, prefix_len: u8): Ipv4Cidr? {
        if (prefix_len > 32) { return null; }
        const mask = Ipv4Cidr.maskForPrefix(prefix_len);
        const net = Ipv4Addr.fromU32(addr.toU32() & mask);
        return Ipv4Cidr { addr: net, prefix_len: prefix_len };
    }

    static fn parse(text: []const u8): Ipv4Cidr? {
        let slash_idx: usize? = null;
        let i: usize = 0;
        while (i < text.len) {
            if (text[i] == '/') {
                if (slash_idx != null) { return null; }
                slash_idx = i;
            }
            i += 1;
        }
        const s_idx = slash_idx ?? return null;
        if (s_idx == 0 || s_idx + 1 >= text.len) { return null; }
        const ip = Ipv4Addr.parse(text[0..s_idx]) ?? return null;
        const plen = IpAddrCodec.parseDecU8Strict(text[s_idx + 1..text.len]) ?? return null;
        return Ipv4Cidr.init(ip, plen);
    }

    fn prefixLen(): u8 {
        return _.prefix_len;
    }

    fn networkAddress(): Ipv4Addr {
        const mask = Ipv4Cidr.maskForPrefix(_.prefix_len);
        return Ipv4Addr.fromU32(_.addr.toU32() & mask);
    }

    fn broadcastAddress(): Ipv4Addr {
        const mask = Ipv4Cidr.maskForPrefix(_.prefix_len);
        return Ipv4Addr.fromU32((_.addr.toU32() & mask) | (~mask));
    }

    fn hostMask(): u32 {
        return ~Ipv4Cidr.maskForPrefix(_.prefix_len);
    }

    fn contains(ip: Ipv4Addr): bool {
        const mask = Ipv4Cidr.maskForPrefix(_.prefix_len);
        return (ip.toU32() & mask) == (_.addr.toU32() & mask);
    }

    fn overlaps(other: Ipv4Cidr): bool {
        return _.contains(other.networkAddress()) || other.contains(_.networkAddress());
    }

    fn format(out: []u8): usize {
        let pos = IpAddrCodec.formatV4(_.networkAddress(), out);
        if (pos == 0) { return 0; }
        if (pos + 1 > out.len) { return 0; }
        out[pos] = '/';
        pos += 1;
        const pn = IpAddrCodec.writeDecU8(out, pos, _.prefix_len);
        if (pn == 0) { return 0; }
        return pos + pn;
    }
}

pub struct Ipv6Cidr {
    addr: Ipv6Addr = Ipv6Addr {};
    prefix_len: u8 = 0;

    static fn maskForPrefix(prefix_len: u8): u128 {
        if (prefix_len == 0) { return 0; }
        let mask: u128 = ~((0 as u128));
        if (prefix_len >= 128) { return mask; }
        let rem = 128 - prefix_len;
        while (rem > 0) {
            mask <<= 1;
            rem -= 1;
        }
        return mask;
    }

    static fn init(addr: Ipv6Addr, prefix_len: u8): Ipv6Cidr? {
        if (prefix_len > 128) { return null; }
        const mask = Ipv6Cidr.maskForPrefix(prefix_len);
        const net = IpAddrCodec.v6FromU128(IpAddrCodec.v6ToU128(addr) & mask);
        return Ipv6Cidr { addr: net, prefix_len: prefix_len };
    }

    static fn parse(text: []const u8): Ipv6Cidr? {
        let slash_idx: usize? = null;
        let i: usize = 0;
        while (i < text.len) {
            if (text[i] == '/') {
                if (slash_idx != null) { return null; }
                slash_idx = i;
            }
            i += 1;
        }
        const s_idx = slash_idx ?? return null;
        if (s_idx < 2 || s_idx + 1 >= text.len) { return null; }
        const ip = IpAddrCodec.parseV6(text[0..s_idx]) ?? return null;
        const plen = IpAddrCodec.parseDecU8Strict(text[s_idx + 1..text.len]) ?? return null;
        return Ipv6Cidr.init(ip, plen);
    }

    fn prefixLen(): u8 {
        return _.prefix_len;
    }

    fn networkAddress(): Ipv6Addr {
        const mask = Ipv6Cidr.maskForPrefix(_.prefix_len);
        return IpAddrCodec.v6FromU128(IpAddrCodec.v6ToU128(_.addr) & mask);
    }

    fn contains(ip: Ipv6Addr): bool {
        const mask = Ipv6Cidr.maskForPrefix(_.prefix_len);
        return (IpAddrCodec.v6ToU128(ip) & mask) == (IpAddrCodec.v6ToU128(_.addr) & mask);
    }

    fn overlaps(other: Ipv6Cidr): bool {
        return _.contains(other.networkAddress()) || other.contains(_.networkAddress());
    }

    fn format(out: []u8): usize {
        let pos = IpAddrCodec.formatV6(_.networkAddress(), out);
        if (pos == 0) { return 0; }
        if (pos + 1 > out.len) { return 0; }
        out[pos] = '/';
        pos += 1;
        const pn = IpAddrCodec.writeDecU8(out, pos, _.prefix_len);
        if (pn == 0) { return 0; }
        return pos + pn;
    }
}

pub static struct IpClass {
    fn isUnspecifiedV4(ip: Ipv4Addr): bool {
        return ip.isUnspecified();
    }

    fn isUnspecifiedV6(ip: Ipv6Addr): bool {
        return ip.isUnspecified();
    }

    fn isLoopbackV4(ip: Ipv4Addr): bool {
        return ip.isLoopback();
    }

    fn isLoopbackV6(ip: Ipv6Addr): bool {
        return ip.isLoopback();
    }

    // RFC 1918: 10.0.0.0/8, 172.16.0.0/12, 192.168.0.0/16
    fn isPrivateV4(ip: Ipv4Addr): bool {
        return ip.isPrivate();
    }

    // RFC 4193 Unique Local IPv6 Unicast: fc00::/7
    fn isPrivateV6(ip: Ipv6Addr): bool {
        return (ip.segments[0] & 0xFE00) == 0xFC00;
    }

    // RFC 3927: 169.254.0.0/16
    fn isLinkLocalV4(ip: Ipv4Addr): bool {
        return ip.octets[0] == 169 && ip.octets[1] == 254;
    }

    // RFC 4291: fe80::/10
    fn isLinkLocalV6(ip: Ipv6Addr): bool {
        return (ip.segments[0] & 0xFFC0) == 0xFE80;
    }

    // RFC 6598 Shared Address Space (CGNAT): 100.64.0.0/10
    fn isCgnatV4(ip: Ipv4Addr): bool {
        return ip.octets[0] == 100 && (ip.octets[1] & 0xC0) == 64;
    }

    fn isMulticastV4(ip: Ipv4Addr): bool {
        return ip.isMulticast();
    }

    // RFC 4291: ff00::/8
    fn isMulticastV6(ip: Ipv6Addr): bool {
        return (ip.segments[0] & 0xFF00) == 0xFF00;
    }

    fn isBroadcastV4(ip: Ipv4Addr): bool {
        return ip.octets[0] == 255 && ip.octets[1] == 255 && ip.octets[2] == 255 && ip.octets[3] == 255;
    }

    fn isDocumentationV4(ip: Ipv4Addr): bool {
        // 192.0.2.0/24 (TEST-NET-1), 198.51.100.0/24 (TEST-NET-2), 203.0.113.0/24 (TEST-NET-3)
        if (ip.octets[0] == 192 && ip.octets[1] == 0 && ip.octets[2] == 2) { return true; }
        if (ip.octets[0] == 198 && ip.octets[1] == 51 && ip.octets[2] == 100) { return true; }
        if (ip.octets[0] == 203 && ip.octets[1] == 0 && ip.octets[2] == 113) { return true; }
        return false;
    }

    fn isDocumentationV6(ip: Ipv6Addr): bool {
        // 2001:db8::/32
        return ip.segments[0] == 0x2001 && ip.segments[1] == 0x0DB8;
    }

    fn isReservedV4(ip: Ipv4Addr): bool {
        // 0.0.0.0/8 ("this" network), 240.0.0.0/4 (reserved for future use, excluding broadcast)
        if (ip.octets[0] == 0) { return true; }
        if (ip.octets[0] >= 240 && !IpClass.isBroadcastV4(ip)) { return true; }
        return false;
    }

    fn isGlobalUnicastV4(ip: Ipv4Addr): bool {
        if (ip.isUnspecified() || ip.isLoopback() || ip.isPrivate()) { return false; }
        if (IpClass.isLinkLocalV4(ip) || IpClass.isCgnatV4(ip)) { return false; }
        if (ip.isMulticast() || IpClass.isBroadcastV4(ip) || IpClass.isReservedV4(ip)) { return false; }
        if (IpClass.isDocumentationV4(ip)) { return false; }
        return true;
    }

    fn isGlobalUnicastV6(ip: Ipv6Addr): bool {
        if (ip.isUnspecified() || ip.isLoopback()) { return false; }
        if (IpClass.isPrivateV6(ip) || IpClass.isLinkLocalV6(ip) || IpClass.isMulticastV6(ip)) { return false; }
        if (IpClass.isDocumentationV6(ip)) { return false; }
        if (ip.isIpv4Mapped()) {
            const v4 = Ipv4Addr.init(
                ((ip.segments[6] >> 8) & 0xFF) as u8,
                (ip.segments[6] & 0xFF) as u8,
                ((ip.segments[7] >> 8) & 0xFF) as u8,
                (ip.segments[7] & 0xFF) as u8
            );
            return IpClass.isGlobalUnicastV4(v4);
        }
        // Global unicast 2000::/3 (first 3 bits == 001)
        return (ip.segments[0] & 0xE000) == 0x2000;
    }

    fn classifyV4(ip: Ipv4Addr): IpKind {
        if (ip.isUnspecified()) { return IpKind.Unspecified; }
        if (ip.isLoopback()) { return IpKind.Loopback; }
        if (ip.isPrivate()) { return IpKind.Private; }
        if (IpClass.isLinkLocalV4(ip)) { return IpKind.LinkLocal; }
        if (IpClass.isCgnatV4(ip)) { return IpKind.Cgnat; }
        if (IpClass.isBroadcastV4(ip)) { return IpKind.Broadcast; }
        if (ip.isMulticast()) { return IpKind.Multicast; }
        if (IpClass.isDocumentationV4(ip)) { return IpKind.Documentation; }
        if (IpClass.isReservedV4(ip)) { return IpKind.Reserved; }
        return IpKind.GlobalUnicast;
    }

    fn classifyV6(ip: Ipv6Addr): IpKind {
        if (ip.isUnspecified()) { return IpKind.Unspecified; }
        if (ip.isLoopback()) { return IpKind.Loopback; }
        if (IpClass.isPrivateV6(ip)) { return IpKind.Private; }
        if (IpClass.isLinkLocalV6(ip)) { return IpKind.LinkLocal; }
        if (IpClass.isMulticastV6(ip)) { return IpKind.Multicast; }
        if (IpClass.isDocumentationV6(ip)) { return IpKind.Documentation; }
        if (ip.isIpv4Mapped()) {
            const v4 = Ipv4Addr.init(
                ((ip.segments[6] >> 8) & 0xFF) as u8,
                (ip.segments[6] & 0xFF) as u8,
                ((ip.segments[7] >> 8) & 0xFF) as u8,
                (ip.segments[7] & 0xFF) as u8
            );
            return IpClass.classifyV4(v4);
        }
        if ((ip.segments[0] & 0xE000) == 0x2000) {
            return IpKind.GlobalUnicast;
        }
        return IpKind.Reserved;
    }

    fn isLoopback(ip: IpAddr): bool {
        switch (ip) {
            .V4 v4 => return IpClass.isLoopbackV4(v4),
            .V6 v6 => return IpClass.isLoopbackV6(v6),
        }
    }

    fn isPrivate(ip: IpAddr): bool {
        switch (ip) {
            .V4 v4 => return IpClass.isPrivateV4(v4),
            .V6 v6 => return IpClass.isPrivateV6(v6),
        }
    }

    fn isLinkLocal(ip: IpAddr): bool {
        switch (ip) {
            .V4 v4 => return IpClass.isLinkLocalV4(v4),
            .V6 v6 => return IpClass.isLinkLocalV6(v6),
        }
    }

    fn isCgnat(ip: IpAddr): bool {
        switch (ip) {
            .V4 v4 => return IpClass.isCgnatV4(v4),
            .V6 => return false,
        }
    }

    fn isMulticast(ip: IpAddr): bool {
        switch (ip) {
            .V4 v4 => return IpClass.isMulticastV4(v4),
            .V6 v6 => return IpClass.isMulticastV6(v6),
        }
    }

    fn isGlobalUnicast(ip: IpAddr): bool {
        switch (ip) {
            .V4 v4 => return IpClass.isGlobalUnicastV4(v4),
            .V6 v6 => return IpClass.isGlobalUnicastV6(v6),
        }
    }

    fn classify(ip: IpAddr): IpKind {
        switch (ip) {
            .V4 v4 => return IpClass.classifyV4(v4),
            .V6 v6 => return IpClass.classifyV6(v6),
        }
    }
}

pub struct CidrAllowlist {
    v4_rules: []Ipv4Cidr = [],
    v4_len: usize = 0,
    v4_cap: usize = 0,
    v6_rules: []Ipv6Cidr = [],
    v6_len: usize = 0,
    v6_cap: usize = 0,

    static fn init(): CidrAllowlist {
        return CidrAllowlist {};
    }

    fn addV4(*_, cidr: Ipv4Cidr) {
        if (_.v4_len >= _.v4_cap) {
            const new_cap: usize = if (_.v4_cap == 0) 8 else _.v4_cap * 2;
            let next = std.heap.page_allocator.alloc(Ipv4Cidr, new_cap) catch unreachable;
            let i: usize = 0;
            while (i < _.v4_len) {
                next[i] = _.v4_rules[i];
                i += 1;
            }
            if (_.v4_cap > 0) {
                std.heap.page_allocator.free(_.v4_rules);
            }
            _.v4_rules = next;
            _.v4_cap = new_cap;
        }
        _.v4_rules[_.v4_len] = cidr;
        _.v4_len += 1;
    }

    fn addV6(*_, cidr: Ipv6Cidr) {
        if (_.v6_len >= _.v6_cap) {
            const new_cap: usize = if (_.v6_cap == 0) 8 else _.v6_cap * 2;
            let next = std.heap.page_allocator.alloc(Ipv6Cidr, new_cap) catch unreachable;
            let i: usize = 0;
            while (i < _.v6_len) {
                next[i] = _.v6_rules[i];
                i += 1;
            }
            if (_.v6_cap > 0) {
                std.heap.page_allocator.free(_.v6_rules);
            }
            _.v6_rules = next;
            _.v6_cap = new_cap;
        }
        _.v6_rules[_.v6_len] = cidr;
        _.v6_len += 1;
    }

    fn addRule(*_, text: []const u8): bool {
        if (c4 := Ipv4Cidr.parse(text)) {
            _.addV4(c4);
            return true;
        }
        if (c6 := Ipv6Cidr.parse(text)) {
            _.addV6(c6);
            return true;
        }
        if (ip4 := Ipv4Addr.parse(text)) {
            _.addV4(Ipv4Cidr { addr: ip4, prefix_len: 32 });
            return true;
        }
        if (ip6 := IpAddrCodec.parseV6(text)) {
            _.addV6(Ipv6Cidr { addr: ip6, prefix_len: 128 });
            return true;
        }
        return false;
    }

    fn matchesV4(ip: Ipv4Addr): bool {
        let i: usize = 0;
        while (i < _.v4_len) {
            if (_.v4_rules[i].contains(ip)) { return true; }
            i += 1;
        }
        return false;
    }

    fn matchesV6(ip: Ipv6Addr): bool {
        let i: usize = 0;
        while (i < _.v6_len) {
            if (_.v6_rules[i].contains(ip)) { return true; }
            i += 1;
        }
        return false;
    }

    fn matches(ip: IpAddr): bool {
        switch (ip) {
            .V4 v4 => return _.matchesV4(v4),
            .V6 v6 => return _.matchesV6(v6),
        }
    }

    fn matchesStr(text: []const u8): bool {
        if (ip := IpAddr.parse(text)) {
            return _.matches(ip);
        }
        return false;
    }

    /// SSRF guard: permits global unicast IPs or IPs explicitly matched by this allowlist.
    fn isAllowedOutbound(ip: IpAddr): bool {
        if (IpClass.isGlobalUnicast(ip)) { return true; }
        return _.matches(ip);
    }

    fn isAllowedOutboundStr(text: []const u8): bool {
        if (ip := IpAddr.parse(text)) {
            return _.isAllowedOutbound(ip);
        }
        return false;
    }

    fn count(): usize {
        return _.v4_len + _.v6_len;
    }

    fn clear(*_) {
        _.v4_len = 0;
        _.v6_len = 0;
    }

    fn deinit(*_) {
        if (_.v4_cap > 0) {
            std.heap.page_allocator.free(_.v4_rules);
            _.v4_rules = [];
            _.v4_cap = 0;
            _.v4_len = 0;
        }
        if (_.v6_cap > 0) {
            std.heap.page_allocator.free(_.v6_rules);
            _.v6_rules = [];
            _.v6_cap = 0;
            _.v6_len = 0;
        }
    }
}
