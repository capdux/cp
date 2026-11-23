// Reference-hardened against krisprice/ipnet (MIT/Apache-2.0)
// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-23T01:59:15Z (RFC 4291 / 5952 / 6890 IPv4 / IPv6 & CIDR Engine)
// Pure-CP zero-allocation IPv4 & IPv6 parser, RFC 5952 canonical formatter, RFC 6890 SSRF classification,
// 16-lane @Vector(16, u8) SIMD IPv6 subnet matcher, and full Ipv4Net / Ipv6Net CIDR algebra.

type Ipv6Vec16u8 = @Vector(16, u8);

fn ipaddrHexVal(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

fn ipaddrHexDigitLower(v: u8): u8 {
    if (v < 10) return @as(u8, '0') + v;
    return @as(u8, 'a') + (v - 10);
}

fn ipaddrWriteByte(out: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out.len) return false;
    out[pos.*] = b;
    pos.* += 1;
    return true;
}

fn ipaddrWriteDecU8(out: []u8, pos: *usize, v: u8): bool {
    if (v >= 100) {
        if (!ipaddrWriteByte(out, pos, @as(u8, '0') + (v / 100))) return false;
        if (!ipaddrWriteByte(out, pos, @as(u8, '0') + ((v / 10) % 10))) return false;
        return ipaddrWriteByte(out, pos, @as(u8, '0') + (v % 10));
    }
    if (v >= 10) {
        if (!ipaddrWriteByte(out, pos, @as(u8, '0') + (v / 10))) return false;
        return ipaddrWriteByte(out, pos, @as(u8, '0') + (v % 10));
    }
    return ipaddrWriteByte(out, pos, @as(u8, '0') + v);
}

fn ipaddrWriteHexU16(out: []u8, pos: *usize, v: u16): bool {
    if (v == 0) return ipaddrWriteByte(out, pos, '0');
    let started = false;
    const d0: u8 = @as(u8, (v >> 12) & 0xF);
    const d1: u8 = @as(u8, (v >> 8) & 0xF);
    const d2: u8 = @as(u8, (v >> 4) & 0xF);
    const d3: u8 = @as(u8, v & 0xF);
    if (d0 != 0) {
        if (!ipaddrWriteByte(out, pos, ipaddrHexDigitLower(d0))) return false;
        started = true;
    }
    if (started || d1 != 0) {
        if (!ipaddrWriteByte(out, pos, ipaddrHexDigitLower(d1))) return false;
        started = true;
    }
    if (started || d2 != 0) {
        if (!ipaddrWriteByte(out, pos, ipaddrHexDigitLower(d2))) return false;
    }
    return ipaddrWriteByte(out, pos, ipaddrHexDigitLower(d3));
}

/// Strictly parses a CIDR prefix length in `0..=max_len`, rejecting leading zeros and overflow.
pub fn parsePrefixLen(s: []const u8, max_len: u8): u8? {
    if (s.len == 0 || s.len > 3) return null;
    if (s.len > 1 && s[0] == '0') return null;
    let val: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        val = val * 10 + @as(u32, c - '0');
        if (val > @as(u32, max_len)) return null;
        i += 1;
    }
    return @as(u8, val);
}

/// Builds a 16-byte IPv6 subnet mask for `prefix_len` in `0..=128` without shift UB.
pub fn ipv6MaskBytes(prefix_len: u8): [16]u8 {
    let mask: [16]u8 = [0; 16];
    let rem: u8 = if (prefix_len > 128) 128 else prefix_len;
    let i: usize = 0;
    while (i < 16 && rem > 0) {
        if (rem >= 8) {
            mask[i] = 0xFF;
            rem -= 8;
        } else {
            const shift: u3 = @as(u3, 8 - rem);
            mask[i] = @as(u8, 0xFF) << shift;
            rem = 0;
        }
        i += 1;
    }
    return mask;
}

/// 16-lane `@Vector(16, u8)` SIMD equality check for two 16-byte IPv6 addresses.
pub fn simdIpv6Eql(a: [16]u8, b: [16]u8): bool {
    const va: Ipv6Vec16u8 = a;
    const vb: Ipv6Vec16u8 = b;
    return @reduce(.And, va == vb);
}

/// Scalar reference for `simdIpv6Eql`.
pub fn simdIpv6EqlScalar(a: [16]u8, b: [16]u8): bool {
    let i: usize = 0;
    while (i < 16) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

/// 16-lane `@Vector(16, u8)` SIMD IPv6 CIDR containment check: `(ip & mask) == (network & mask)`.
pub fn simdIpv6InSubnet(ip: [16]u8, network: [16]u8, prefix_len: u8): bool {
    if (prefix_len > 128) return false;
    const ip_v: Ipv6Vec16u8 = ip;
    const net_v: Ipv6Vec16u8 = network;
    const mask_v: Ipv6Vec16u8 = ipv6MaskBytes(prefix_len);
    return @reduce(.And, (ip_v & mask_v) == (net_v & mask_v));
}

/// Scalar reference for `simdIpv6InSubnet` (used for differential verification).
pub fn simdIpv6InSubnetScalar(ip: [16]u8, network: [16]u8, prefix_len: u8): bool {
    if (prefix_len > 128) return false;
    const mask = ipv6MaskBytes(prefix_len);
    let i: usize = 0;
    while (i < 16) {
        if ((ip[i] & mask[i]) != (network[i] & mask[i])) return false;
        i += 1;
    }
    return true;
}

fn ipaddrFindSlash(s: []const u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '/') return i;
        i += 1;
    }
    return null;
}

export struct Ipv4Addr {
    octets: [4]u8,

    pub static fn fromOctets(a: u8, b: u8, c: u8, d: u8): Ipv4Addr {
        return Ipv4Addr { octets: [a, b, c, d] };
    }

    pub fn toU32(): u32 {
        return (@as(u32, _.octets[0]) << 24) |
            (@as(u32, _.octets[1]) << 16) |
            (@as(u32, _.octets[2]) << 8) |
            @as(u32, _.octets[3]);
    }

    pub static fn fromU32(val: u32): Ipv4Addr {
        return Ipv4Addr {
            octets: [
                @as(u8, (val >> 24) & 0xFF),
                @as(u8, (val >> 16) & 0xFF),
                @as(u8, (val >> 8) & 0xFF),
                @as(u8, val & 0xFF),
            ],
        };
    }

    pub fn eql(other: Ipv4Addr): bool {
        return _.toU32() == other.toU32();
    }

    /// Strict dotted-quad IPv4 parser (rejects leading zeros like "0177.0.0.1" and >255 octets).
    pub static fn parse(s: []const u8): Ipv4Addr? {
        if (s.len < 7 || s.len > 15) return null;
        let out: [4]u8 = [0; 4];
        let part: usize = 0;
        let i: usize = 0;
        while (part < 4) {
            if (i >= s.len) return null;
            const start = i;
            let val: u32 = 0;
            while (i < s.len && s[i] != '.') {
                const c = s[i];
                if (c < '0' || c > '9') return null;
                val = val * 10 + @as(u32, c - '0');
                if (val > 255) return null;
                i += 1;
            }
            const digits = i - start;
            if (digits == 0 || digits > 3) return null;
            if (digits > 1 && s[start] == '0') return null;
            out[part] = @as(u8, val);
            if (part < 3) {
                if (i >= s.len || s[i] != '.') return null;
                i += 1;
            }
            part += 1;
        }
        if (i != s.len) return null;
        return Ipv4Addr { octets: out };
    }

    pub fn isUnspecified(): bool {
        return _.octets[0] == 0 && _.octets[1] == 0 && _.octets[2] == 0 && _.octets[3] == 0;
    }

    /// RFC 1122 "this host on this network" (`0.0.0.0/8`).
    pub fn isThisNetwork(): bool {
        return _.octets[0] == 0;
    }

    pub fn isLoopback(): bool {
        return _.octets[0] == 127;
    }

    /// RFC 1918 private networks (`10.0.0.0/8`, `172.16.0.0/12`, `192.168.0.0/16`).
    pub fn isPrivate(): bool {
        if (_.octets[0] == 10) return true;
        if (_.octets[0] == 172 && _.octets[1] >= 16 && _.octets[1] <= 31) return true;
        if (_.octets[0] == 192 && _.octets[1] == 168) return true;
        return false;
    }

    /// RFC 3927 link-local (`169.254.0.0/16`, includes cloud metadata `169.254.169.254`).
    pub fn isLinkLocal(): bool {
        return _.octets[0] == 169 && _.octets[1] == 254;
    }

    /// RFC 6598 Shared Address Space / CGNAT (`100.64.0.0/10`).
    pub fn isCgnat(): bool {
        return _.octets[0] == 100 && _.octets[1] >= 64 && _.octets[1] <= 127;
    }

    /// RFC 5737 TEST-NET documentation blocks (`192.0.2.0/24`, `198.51.100.0/24`, `203.0.113.0/24`).
    pub fn isDocumentation(): bool {
        if (_.octets[0] == 192 && _.octets[1] == 0 && _.octets[2] == 2) return true;
        if (_.octets[0] == 198 && _.octets[1] == 51 && _.octets[2] == 100) return true;
        if (_.octets[0] == 203 && _.octets[1] == 0 && _.octets[2] == 113) return true;
        return false;
    }

    /// RFC 2544 benchmarking (`198.18.0.0/15`).
    pub fn isBenchmarking(): bool {
        return _.octets[0] == 198 && (_.octets[1] == 18 || _.octets[1] == 19);
    }

    /// RFC 6890 IETF protocol assignments (`192.0.0.0/24`).
    pub fn isIetfProtocol(): bool {
        return _.octets[0] == 192 && _.octets[1] == 0 && _.octets[2] == 0;
    }

    /// RFC 1112 reserved Class E (`240.0.0.0/4`, includes broadcast).
    pub fn isReserved(): bool {
        return _.octets[0] >= 240;
    }

    pub fn isMulticast(): bool {
        return _.octets[0] >= 224 && _.octets[0] <= 239;
    }

    pub fn isBroadcast(): bool {
        return _.octets[0] == 255 && _.octets[1] == 255 && _.octets[2] == 255 && _.octets[3] == 255;
    }

    /// RFC 6890 global unicast predicate (safe for outbound SSRF filtering).
    pub fn isGlobalUnicast(): bool {
        if (_.isThisNetwork()) return false;
        if (_.isLoopback()) return false;
        if (_.isPrivate()) return false;
        if (_.isLinkLocal()) return false;
        if (_.isCgnat()) return false;
        if (_.isDocumentation()) return false;
        if (_.isBenchmarking()) return false;
        if (_.isIetfProtocol()) return false;
        if (_.isMulticast()) return false;
        if (_.isReserved()) return false;
        return true;
    }

    pub fn inSubnet(network: Ipv4Addr, prefix_len: u8): bool {
        if (prefix_len > 32) return false;
        if (prefix_len == 0) return true;
        const shift: u5 = @as(u5, 32 - prefix_len);
        return (_.toU32() >> shift) == (network.toU32() >> shift);
    }

    pub fn format(out: []u8): ([]const u8)? {
        let pos: usize = 0;
        if (!ipaddrWriteDecU8(out, &pos, _.octets[0])) return null;
        if (!ipaddrWriteByte(out, &pos, '.')) return null;
        if (!ipaddrWriteDecU8(out, &pos, _.octets[1])) return null;
        if (!ipaddrWriteByte(out, &pos, '.')) return null;
        if (!ipaddrWriteDecU8(out, &pos, _.octets[2])) return null;
        if (!ipaddrWriteByte(out, &pos, '.')) return null;
        if (!ipaddrWriteDecU8(out, &pos, _.octets[3])) return null;
        return out[0..pos];
    }

    pub fn toIpv6Mapped(): Ipv6Addr {
        let segs: [8]u16 = [0; 8];
        segs[5] = 0xFFFF;
        segs[6] = (@as(u16, _.octets[0]) << 8) | @as(u16, _.octets[1]);
        segs[7] = (@as(u16, _.octets[2]) << 8) | @as(u16, _.octets[3]);
        return Ipv6Addr { segments: segs };
    }
}

export struct Ipv6Addr {
    segments: [8]u16,

    pub fn toBytes(): [16]u8 {
        let out: [16]u8 = [
            @as(u8, (_.segments[0] >> 8) & 0xFF), @as(u8, _.segments[0] & 0xFF),
            @as(u8, (_.segments[1] >> 8) & 0xFF), @as(u8, _.segments[1] & 0xFF),
            @as(u8, (_.segments[2] >> 8) & 0xFF), @as(u8, _.segments[2] & 0xFF),
            @as(u8, (_.segments[3] >> 8) & 0xFF), @as(u8, _.segments[3] & 0xFF),
            @as(u8, (_.segments[4] >> 8) & 0xFF), @as(u8, _.segments[4] & 0xFF),
            @as(u8, (_.segments[5] >> 8) & 0xFF), @as(u8, _.segments[5] & 0xFF),
            @as(u8, (_.segments[6] >> 8) & 0xFF), @as(u8, _.segments[6] & 0xFF),
            @as(u8, (_.segments[7] >> 8) & 0xFF), @as(u8, _.segments[7] & 0xFF),
        ];
        return out;
    }

    pub static fn fromBytes(b: [16]u8): Ipv6Addr {
        return Ipv6Addr {
            segments: [
                (@as(u16, b[0]) << 8) | @as(u16, b[1]),
                (@as(u16, b[2]) << 8) | @as(u16, b[3]),
                (@as(u16, b[4]) << 8) | @as(u16, b[5]),
                (@as(u16, b[6]) << 8) | @as(u16, b[7]),
                (@as(u16, b[8]) << 8) | @as(u16, b[9]),
                (@as(u16, b[10]) << 8) | @as(u16, b[11]),
                (@as(u16, b[12]) << 8) | @as(u16, b[13]),
                (@as(u16, b[14]) << 8) | @as(u16, b[15]),
            ],
        };
    }

    pub fn eql(other: Ipv6Addr): bool {
        return simdIpv6Eql(_.toBytes(), other.toBytes());
    }

    /// Parse RFC 4291 IPv6 literal including `::` compression and embedded IPv4 suffix (e.g. `::ffff:192.0.2.128`).
    /// Rejects Zone IDs (`%...`), multiple `::`, leading zeros in embedded IPv4, and malformed hex.
    pub static fn parse(raw: []const u8): Ipv6Addr? {
        if (raw.len < 2 || raw.len > 45) return null;
        let head: [8]u16 = [0; 8];
        let head_len: usize = 0;
        let tail: [8]u16 = [0; 8];
        let tail_len: usize = 0;
        let has_compress = false;

        let i: usize = 0;
        if (raw[0] == ':') {
            if (raw[1] != ':') return null;
            has_compress = true;
            i = 2;
            if (i == raw.len) {
                return Ipv6Addr { segments: [0; 8] };
            }
        }

        while (i < raw.len) {
            let has_dot = false;
            let k = i;
            while (k < raw.len) {
                if (raw[k] == '.') {
                    has_dot = true;
                    break;
                }
                if (raw[k] == ':') break;
                k += 1;
            }
            if (has_dot) {
                const v4 = Ipv4Addr.parse(raw[i..]) ?? return null;
                const hi: u16 = (@as(u16, v4.octets[0]) << 8) | @as(u16, v4.octets[1]);
                const lo: u16 = (@as(u16, v4.octets[2]) << 8) | @as(u16, v4.octets[3]);
                if (has_compress) {
                    if (tail_len + 2 > 8) return null;
                    tail[tail_len] = hi;
                    tail[tail_len + 1] = lo;
                    tail_len += 2;
                } else {
                    if (head_len + 2 > 8) return null;
                    head[head_len] = hi;
                    head[head_len + 1] = lo;
                    head_len += 2;
                }
                i = raw.len;
                break;
            }

            const start = i;
            let val: u16 = 0;
            while (i < raw.len && raw[i] != ':') {
                const hv = ipaddrHexVal(raw[i]) ?? return null;
                if (i - start >= 4) return null;
                val = (val << 4) | @as(u16, hv);
                i += 1;
            }
            if (i == start) return null;

            if (has_compress) {
                if (tail_len >= 8) return null;
                tail[tail_len] = val;
                tail_len += 1;
            } else {
                if (head_len >= 8) return null;
                head[head_len] = val;
                head_len += 1;
            }

            if (i < raw.len) {
                if (raw[i] != ':') return null;
                i += 1;
                if (i >= raw.len) return null;
                if (raw[i] == ':') {
                    if (has_compress) return null;
                    has_compress = true;
                    i += 1;
                    if (i == raw.len) break;
                }
            }
        }

        let segs: [8]u16 = [0; 8];
        if (!has_compress) {
            if (head_len != 8) return null;
            let idx: usize = 0;
            while (idx < 8) {
                segs[idx] = head[idx];
                idx += 1;
            }
            return Ipv6Addr { segments: segs };
        }

        if (head_len + tail_len >= 8) return null;
        let idx: usize = 0;
        while (idx < head_len) {
            segs[idx] = head[idx];
            idx += 1;
        }
        const tail_start: usize = @as(usize, 8) - tail_len;
        let ti: usize = 0;
        while (ti < tail_len) {
            segs[tail_start + ti] = tail[ti];
            ti += 1;
        }
        return Ipv6Addr { segments: segs };
    }

    pub fn isUnspecified(): bool {
        let i: usize = 0;
        while (i < 8) {
            if (_.segments[i] != 0) return false;
            i += 1;
        }
        return true;
    }

    pub fn isLoopback(): bool {
        if (v4 := _.toIpv4Mapped()) {
            return v4.isLoopback();
        }
        let i: usize = 0;
        while (i < 7) {
            if (_.segments[i] != 0) return false;
            i += 1;
        }
        return _.segments[7] == 1;
    }

    pub fn isMulticast(): bool {
        if (v4 := _.toIpv4Mapped()) {
            return v4.isMulticast();
        }
        return (_.segments[0] & 0xFF00) == 0xFF00;
    }

    pub fn isLinkLocal(): bool {
        if (v4 := _.toIpv4Mapped()) {
            return v4.isLinkLocal();
        }
        return (_.segments[0] & 0xFFC0) == 0xFE80;
    }

    pub fn isUniqueLocal(): bool {
        return (_.segments[0] & 0xFE00) == 0xFC00;
    }

    /// RFC 4193 ULA (`fc00::/7`) or IPv4-mapped RFC 1918 private address.
    pub fn isPrivate(): bool {
        if (v4 := _.toIpv4Mapped()) {
            return v4.isPrivate();
        }
        return _.isUniqueLocal();
    }

    /// IPv4-mapped RFC 6598 CGNAT (`::ffff:100.64.0.0/106`).
    pub fn isCgnat(): bool {
        if (v4 := _.toIpv4Mapped()) {
            return v4.isCgnat();
        }
        return false;
    }

    /// RFC 3849 IPv6 documentation (`2001:db8::/32`) or IPv4-mapped RFC 5737 documentation.
    pub fn isDocumentation(): bool {
        if (v4 := _.toIpv4Mapped()) {
            return v4.isDocumentation();
        }
        return _.segments[0] == 0x2001 && _.segments[1] == 0x0DB8;
    }

    /// RFC 6666 Discard-Only (`100::/64`).
    pub fn isDiscardOnly(): bool {
        return _.segments[0] == 0x0100 && _.segments[1] == 0 && _.segments[2] == 0 && _.segments[3] == 0;
    }

    /// RFC 5180 Benchmarking (`2001:2::/48`).
    pub fn isBenchmarking(): bool {
        return _.segments[0] == 0x2001 && _.segments[1] == 0x0002 && _.segments[2] == 0;
    }

    /// Deprecated IPv4-compatible IPv6 (`::/96` excluding `::` and `::1`).
    pub fn isIpv4Compatible(): bool {
        if (_.segments[0] != 0 || _.segments[1] != 0 || _.segments[2] != 0 ||
            _.segments[3] != 0 || _.segments[4] != 0 || _.segments[5] != 0)
        {
            return false;
        }
        if (_.segments[6] == 0 && (_.segments[7] == 0 || _.segments[7] == 1)) return false;
        return true;
    }

    pub fn toIpv4Mapped(): Ipv4Addr? {
        if (_.segments[0] == 0 && _.segments[1] == 0 && _.segments[2] == 0 &&
            _.segments[3] == 0 && _.segments[4] == 0 && _.segments[5] == 0xFFFF)
        {
            return Ipv4Addr {
                octets: [
                    @as(u8, (_.segments[6] >> 8) & 0xFF),
                    @as(u8, _.segments[6] & 0xFF),
                    @as(u8, (_.segments[7] >> 8) & 0xFF),
                    @as(u8, _.segments[7] & 0xFF),
                ],
            };
        }
        return null;
    }

    pub fn isIpv4Mapped(): bool {
        return _.toIpv4Mapped() != null;
    }

    /// RFC 6890 global unicast predicate for SSRF defense.
    /// Automatically unwraps IPv4-mapped IPv6 (`::ffff:127.0.0.1`, `::ffff:169.254.169.254` -> `false`).
    pub fn isGlobalUnicast(): bool {
        if (v4 := _.toIpv4Mapped()) {
            return v4.isGlobalUnicast();
        }
        if (_.isUnspecified()) return false;
        if (_.isLoopback()) return false;
        if (_.isIpv4Compatible()) return false; // deprecated ::a.b.c.d
        if (_.isMulticast()) return false;
        if (_.isLinkLocal()) return false;
        if (_.isUniqueLocal()) return false;
        if (_.isDocumentation()) return false;
        if (_.isDiscardOnly()) return false;
        if (_.isBenchmarking()) return false;
        // Deprecated site-local fec0::/10
        if ((_.segments[0] & 0xFFC0) == 0xFEC0) return false;
        // RFC 6052 IPv4/IPv6 translation 64:ff9b::/96 & 64:ff9b:1::/48
        if (_.segments[0] == 0x0064 && _.segments[1] == 0xFF9B) return false;
        return true;
    }

    /// 16-lane SIMD IPv6 subnet containment check.
    pub fn inSubnet(network: Ipv6Addr, prefix_len: u8): bool {
        return simdIpv6InSubnet(_.toBytes(), network.toBytes(), prefix_len);
    }

    /// Canonical RFC 5952 IPv6 formatter (longest >=2 zero run compressed to `::`, lowercase hex, IPv4-mapped suffix).
    pub fn format(out: []u8): ([]const u8)? {
        let pos: usize = 0;
        if (v4 := _.toIpv4Mapped()) {
            const prefix = "::ffff:";
            let pi: usize = 0;
            while (pi < prefix.len) {
                if (!ipaddrWriteByte(out, &pos, prefix[pi])) return null;
                pi += 1;
            }
            const rest = v4.format(out[pos..]) ?? return null;
            return out[0..pos + rest.len];
        }

        let best_start: usize = 8;
        let best_len: usize = 0;
        let i: usize = 0;
        while (i < 8) {
            if (_.segments[i] == 0) {
                const start = i;
                while (i < 8 && _.segments[i] == 0) {
                    i += 1;
                }
                const run_len = i - start;
                if (run_len >= 2 && run_len > best_len) {
                    best_start = start;
                    best_len = run_len;
                }
            } else {
                i += 1;
            }
        }

        let cur: usize = 0;
        while (cur < 8) {
            if (best_len >= 2 && cur == best_start) {
                if (!ipaddrWriteByte(out, &pos, ':')) return null;
                if (!ipaddrWriteByte(out, &pos, ':')) return null;
                cur += best_len;
                continue;
            }
            if (cur > 0 && !(best_len >= 2 && cur == best_start + best_len)) {
                if (!ipaddrWriteByte(out, &pos, ':')) return null;
            }
            if (!ipaddrWriteHexU16(out, &pos, _.segments[cur])) return null;
            cur += 1;
        }
        return out[0..pos];
    }
}

/// IPv4 CIDR network (`krisprice/ipnet::Ipv4Net` parity, zero-allocation).
export struct Ipv4Net {
    addr: Ipv4Addr,
    prefix_len: u8,

    pub static fn init(addr: Ipv4Addr, prefix_len: u8): Ipv4Net? {
        if (prefix_len > 32) return null;
        return Ipv4Net { addr: addr, prefix_len: prefix_len };
    }

    pub static fn parse(s: []const u8): Ipv4Net? {
        const slash = ipaddrFindSlash(s) ?? return null;
        const addr = Ipv4Addr.parse(s[0..slash]) ?? return null;
        const pfx = parsePrefixLen(s[slash + 1 ..], 32) ?? return null;
        return Ipv4Net { addr: addr, prefix_len: pfx };
    }

    pub fn netmask(): Ipv4Addr {
        if (_.prefix_len == 0) return Ipv4Addr.fromU32(0);
        if (_.prefix_len == 32) return Ipv4Addr.fromU32(0xFFFFFFFF);
        const shift: u5 = @as(u5, 32 - _.prefix_len);
        return Ipv4Addr.fromU32(@as(u32, 0xFFFFFFFF) << shift);
    }

    pub fn hostmask(): Ipv4Addr {
        return Ipv4Addr.fromU32(~_.netmask().toU32());
    }

    pub fn network(): Ipv4Addr {
        return Ipv4Addr.fromU32(_.addr.toU32() & _.netmask().toU32());
    }

    pub fn broadcast(): Ipv4Addr {
        return Ipv4Addr.fromU32(_.network().toU32() | _.hostmask().toU32());
    }

    pub fn trunc(): Ipv4Net {
        return Ipv4Net { addr: _.network(), prefix_len: _.prefix_len };
    }

    pub fn contains(ip: Ipv4Addr): bool {
        return ip.inSubnet(_.addr, _.prefix_len);
    }

    pub fn containsNet(other: Ipv4Net): bool {
        if (_.prefix_len > other.prefix_len) return false;
        return _.contains(other.network());
    }

    pub fn overlaps(other: Ipv4Net): bool {
        return _.contains(other.network()) || other.contains(_.network());
    }

    pub fn supernet(): Ipv4Net? {
        if (_.prefix_len == 0) return null;
        const new_pfx: u8 = _.prefix_len - 1;
        let sup = Ipv4Net { addr: _.addr, prefix_len: new_pfx };
        return sup.trunc();
    }

    pub fn eql(other: Ipv4Net): bool {
        return _.prefix_len == other.prefix_len && _.network().eql(other.network());
    }

    pub fn format(out: []u8): ([]const u8)? {
        const net = _.network();
        const ip_str = net.format(out) ?? return null;
        let pos: usize = ip_str.len;
        if (!ipaddrWriteByte(out, &pos, '/')) return null;
        if (!ipaddrWriteDecU8(out, &pos, _.prefix_len)) return null;
        return out[0..pos];
    }
}

/// IPv6 CIDR network (`krisprice/ipnet::Ipv6Net` parity, 16-lane SIMD containment, zero-allocation).
export struct Ipv6Net {
    addr: Ipv6Addr,
    prefix_len: u8,

    pub static fn init(addr: Ipv6Addr, prefix_len: u8): Ipv6Net? {
        if (prefix_len > 128) return null;
        return Ipv6Net { addr: addr, prefix_len: prefix_len };
    }

    pub static fn parse(s: []const u8): Ipv6Net? {
        const slash = ipaddrFindSlash(s) ?? return null;
        const addr = Ipv6Addr.parse(s[0..slash]) ?? return null;
        const pfx = parsePrefixLen(s[slash + 1 ..], 128) ?? return null;
        return Ipv6Net { addr: addr, prefix_len: pfx };
    }

    pub fn netmask(): Ipv6Addr {
        return Ipv6Addr.fromBytes(ipv6MaskBytes(_.prefix_len));
    }

    pub fn hostmask(): Ipv6Addr {
        const m_v: Ipv6Vec16u8 = ipv6MaskBytes(_.prefix_len);
        const h_arr: [16]u8 = ~m_v;
        return Ipv6Addr.fromBytes(h_arr);
    }

    pub fn network(): Ipv6Addr {
        const a_v: Ipv6Vec16u8 = _.addr.toBytes();
        const m_v: Ipv6Vec16u8 = ipv6MaskBytes(_.prefix_len);
        const n_arr: [16]u8 = a_v & m_v;
        return Ipv6Addr.fromBytes(n_arr);
    }

    pub fn broadcast(): Ipv6Addr {
        const a_v: Ipv6Vec16u8 = _.addr.toBytes();
        const m_v: Ipv6Vec16u8 = ipv6MaskBytes(_.prefix_len);
        const b_arr: [16]u8 = (a_v & m_v) | (~m_v);
        return Ipv6Addr.fromBytes(b_arr);
    }

    pub fn trunc(): Ipv6Net {
        return Ipv6Net { addr: _.network(), prefix_len: _.prefix_len };
    }

    pub fn contains(ip: Ipv6Addr): bool {
        return simdIpv6InSubnet(ip.toBytes(), _.addr.toBytes(), _.prefix_len);
    }

    pub fn containsNet(other: Ipv6Net): bool {
        if (_.prefix_len > other.prefix_len) return false;
        return _.contains(other.network());
    }

    pub fn overlaps(other: Ipv6Net): bool {
        return _.contains(other.network()) || other.contains(_.network());
    }

    pub fn supernet(): Ipv6Net? {
        if (_.prefix_len == 0) return null;
        const new_pfx: u8 = _.prefix_len - 1;
        let sup = Ipv6Net { addr: _.addr, prefix_len: new_pfx };
        return sup.trunc();
    }

    pub fn eql(other: Ipv6Net): bool {
        return _.prefix_len == other.prefix_len && _.network().eql(other.network());
    }

    pub fn format(out: []u8): ([]const u8)? {
        const net = _.network();
        const ip_str = net.format(out) ?? return null;
        let pos: usize = ip_str.len;
        if (!ipaddrWriteByte(out, &pos, '/')) return null;
        if (!ipaddrWriteDecU8(out, &pos, _.prefix_len)) return null;
        return out[0..pos];
    }
}

export enum IpVersion {
    v4,
    v6,
}

export struct IpAddr {
    version: IpVersion,
    v4: Ipv4Addr,
    v6: Ipv6Addr,

    pub static fn parse(s: []const u8): IpAddr? {
        if (a4 := Ipv4Addr.parse(s)) {
            return IpAddr {
                version: IpVersion.v4,
                v4: a4,
                v6: Ipv6Addr { segments: [0; 8] },
            };
        }
        if (a6 := Ipv6Addr.parse(s)) {
            return IpAddr {
                version: IpVersion.v6,
                v4: Ipv4Addr { octets: [0; 4] },
                v6: a6,
            };
        }
        return null;
    }

    pub fn isUnspecified(): bool {
        if (_.version == IpVersion.v4) return _.v4.isUnspecified();
        return _.v6.isUnspecified();
    }

    pub fn isLoopback(): bool {
        if (_.version == IpVersion.v4) return _.v4.isLoopback();
        return _.v6.isLoopback();
    }

    pub fn isPrivate(): bool {
        if (_.version == IpVersion.v4) return _.v4.isPrivate();
        return _.v6.isPrivate();
    }

    pub fn isLinkLocal(): bool {
        if (_.version == IpVersion.v4) return _.v4.isLinkLocal();
        return _.v6.isLinkLocal();
    }

    pub fn isCgnat(): bool {
        if (_.version == IpVersion.v4) return _.v4.isCgnat();
        return _.v6.isCgnat();
    }

    pub fn isMulticast(): bool {
        if (_.version == IpVersion.v4) return _.v4.isMulticast();
        return _.v6.isMulticast();
    }

    pub fn isDocumentation(): bool {
        if (_.version == IpVersion.v4) return _.v4.isDocumentation();
        return _.v6.isDocumentation();
    }

    /// RFC 6890 global unicast predicate (auto-unwraps IPv4-mapped IPv6 against SSRF bypass).
    pub fn isGlobalUnicast(): bool {
        if (_.version == IpVersion.v4) return _.v4.isGlobalUnicast();
        return _.v6.isGlobalUnicast();
    }

    pub fn format(out: []u8): ([]const u8)? {
        if (_.version == IpVersion.v4) return _.v4.format(out);
        return _.v6.format(out);
    }
}
