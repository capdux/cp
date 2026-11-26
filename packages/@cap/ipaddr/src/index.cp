// @cap/ipaddr — Zero-allocation IPv4 & IPv6 address parser, RFC 5952 formatter,
// CIDR matcher, IPv4-mapped IPv6 converter, and address range classifier for CP.
// Clean-room MIT implementation with full parity to npm `ipaddr.js`.

const IPA_HEX_LOWER: string = "0123456789abcdef";

fn ipaHexVal(ch: u8): i16 {
    if (ch >= 48 && ch <= 57) {
        return (ch - 48) as i16;
    }
    if (ch >= 65 && ch <= 70) {
        return ((ch - 65) + 10) as i16;
    }
    if (ch >= 97 && ch <= 102) {
        return ((ch - 97) + 10) as i16;
    }
    return -1;
}

fn ipaMask32(prefix: u8): u32 {
    if (prefix == 0) {
        return 0;
    }
    if (prefix >= 32) {
        return 0xFFFFFFFF;
    }
    const shift: u5 = (32 - prefix) as u5;
    return (0xFFFFFFFF as u32) << shift;
}

fn ipaMask16(bits: u8): u16 {
    if (bits == 0) {
        return 0;
    }
    if (bits >= 16) {
        return 0xFFFF;
    }
    const shift: u4 = (16 - bits) as u4;
    return (0xFFFF as u16) << shift;
}

fn ipaWriteU8Dec(dst: []u8, pos: usize, valIn: u8): usize {
    let tmp: [4]u8 = [0, 0, 0, 0];
    let n: usize = 0;
    let v: u8 = valIn;
    if (v == 0) {
        tmp[0] = 48;
        n = 1;
    } else {
        while (v > 0) {
            tmp[n] = (v % 10) + 48;
            v = v / 10;
            n = n + 1;
        }
    }
    let p: usize = pos;
    let k: usize = n;
    while (k > 0) {
        k = k - 1;
        dst[p] = tmp[k];
        p = p + 1;
    }
    return p;
}

fn ipaWriteU16Hex(dst: []u8, pos: usize, valIn: u16): usize {
    let tmp: [4]u8 = [0, 0, 0, 0];
    let n: usize = 0;
    let v: u16 = valIn;
    if (v == 0) {
        tmp[0] = 48;
        n = 1;
    } else {
        while (v > 0) {
            tmp[n] = IPA_HEX_LOWER[(v & 0x0F) as usize];
            v = v >> (4 as u4);
            n = n + 1;
        }
    }
    let p: usize = pos;
    let k: usize = n;
    while (k > 0) {
        k = k - 1;
        dst[p] = tmp[k];
        p = p + 1;
    }
    return p;
}

fn ipaParsePrefix(s: string, maxPrefix: u8): i16 {
    if (s.len == 0 || s.len > 3) {
        return -1;
    }
    if (s.len > 1 && s[0] == 48) {
        return -1;
    }
    let val: u16 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const ch = s[i];
        if (ch < 48 || ch > 57) {
            return -1;
        }
        val = (val * 10) + ((ch - 48) as u16);
        if (val > (maxPrefix as u16)) {
            return -1;
        }
        i = i + 1;
    }
    return val as i16;
}

fn ipaInvalidV4(): Ipv4Addr {
    return Ipv4Addr {
        octets: [0, 0, 0, 0],
        ok: false,
    };
}

fn ipaInvalidV6(): Ipv6Addr {
    return Ipv6Addr {
        parts: [0, 0, 0, 0, 0, 0, 0, 0],
        ok: false,
    };
}

/// Parsed IPv4 address (`4` octets + validity flag).
pub struct Ipv4Addr {
    pub octets: [4]u8,
    pub ok: bool,

    /// Construct a valid `Ipv4Addr` from a big-endian 32-bit unsigned integer.
    pub static fn fromU32(val: u32): Ipv4Addr {
        return Ipv4Addr {
            octets: [
                ((val >> (24 as u5)) & 0xFF) as u8,
                ((val >> (16 as u5)) & 0xFF) as u8,
                ((val >> (8 as u5)) & 0xFF) as u8,
                (val & 0xFF) as u8,
            ],
            ok: true,
        };
    }

    /// Construct a subnet mask `Ipv4Addr` from a prefix length `0..32`.
    pub static fn subnetMaskFromPrefixLength(prefix: u8): Ipv4Addr {
        if (prefix > 32) {
            return ipaInvalidV4();
        }
        return Ipv4Addr.fromU32(ipaMask32(prefix));
    }

    /// Return the address kind string `"ipv4"`.
    pub fn kind(): string {
        return "ipv4";
    }

    /// Convert to big-endian 32-bit unsigned integer (`0` if invalid).
    pub fn toU32(): u32 {
        if (!_.ok) {
            return 0;
        }
        const b0: u32 = (_.octets[0] as u32) << (24 as u5);
        const b1: u32 = (_.octets[1] as u32) << (16 as u5);
        const b2: u32 = (_.octets[2] as u32) << (8 as u5);
        const b3: u32 = _.octets[3] as u32;
        return b0 | b1 | b2 | b3;
    }

    /// Return the raw 4-byte array representation.
    pub fn toByteArray(): [4]u8 {
        return _.octets;
    }

    /// Check exact equality with another `Ipv4Addr`.
    pub fn equals(other: Ipv4Addr): bool {
        if (!_.ok || !other.ok) {
            return false;
        }
        return _.octets[0] == other.octets[0]
            && _.octets[1] == other.octets[1]
            && _.octets[2] == other.octets[2]
            && _.octets[3] == other.octets[3];
    }

    /// Format as dotted decimal `"a.b.c.d"` into `out`.
    /// Returns bytes written on success, or `0` if invalid or `out` is too small.
    pub fn toStr(out: []u8): usize {
        if (!_.ok) {
            return 0;
        }
        let tmp: [16]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        let pos: usize = 0;
        pos = ipaWriteU8Dec(tmp[0..16], pos, _.octets[0]);
        tmp[pos] = 46;
        pos = pos + 1;
        pos = ipaWriteU8Dec(tmp[0..16], pos, _.octets[1]);
        tmp[pos] = 46;
        pos = pos + 1;
        pos = ipaWriteU8Dec(tmp[0..16], pos, _.octets[2]);
        tmp[pos] = 46;
        pos = pos + 1;
        pos = ipaWriteU8Dec(tmp[0..16], pos, _.octets[3]);

        if (out.len < pos) {
            return 0;
        }
        let i: usize = 0;
        while (i < pos) {
            out[i] = tmp[i];
            i = i + 1;
        }
        return pos;
    }

    /// Test whether this IPv4 address belongs to the CIDR block `net/prefix`.
    pub fn matchCidr(net: Ipv4Addr, prefix: u8): bool {
        if (!_.ok || !net.ok || prefix > 32) {
            return false;
        }
        if (prefix == 0) {
            return true;
        }
        const mask = ipaMask32(prefix);
        return (_.toU32() & mask) == (net.toU32() & mask);
    }

    /// Compute prefix length `0..32` if this address is a valid contiguous subnet mask, else `-1`.
    pub fn prefixLengthFromSubnetMask(): i8 {
        if (!_.ok) {
            return -1;
        }
        const val = _.toU32();
        let prefix: i8 = 0;
        let seenZero = false;
        let bit: u8 = 32;
        while (bit > 0) {
            bit = bit - 1;
            const isOne = ((val >> (bit as u5)) & 1) == 1;
            if (isOne) {
                if (seenZero) {
                    return -1;
                }
                prefix = prefix + 1;
            } else {
                seenZero = true;
            }
        }
        return prefix;
    }

    /// Classify the IPv4 address into its IANA special-purpose range name.
    pub fn range(): string {
        if (!_.ok) {
            return "invalid";
        }
        const o0 = _.octets[0];
        const o1 = _.octets[1];
        const o2 = _.octets[2];
        const o3 = _.octets[3];

        // 0.0.0.0/8
        if (o0 == 0) {
            return "unspecified";
        }
        // 255.255.255.255/32
        if (o0 == 255 && o1 == 255 && o2 == 255 && o3 == 255) {
            return "broadcast";
        }
        // 224.0.0.0/4
        if (o0 >= 224 && o0 <= 239) {
            return "multicast";
        }
        // 169.254.0.0/16
        if (o0 == 169 && o1 == 254) {
            return "linkLocal";
        }
        // 127.0.0.0/8
        if (o0 == 127) {
            return "loopback";
        }
        // 100.64.0.0/10
        if (o0 == 100 && o1 >= 64 && o1 <= 127) {
            return "carrierGradeNat";
        }
        // 10.0.0.0/8, 172.16.0.0/12, 192.168.0.0/16
        if (o0 == 10) {
            return "private";
        }
        if (o0 == 172 && o1 >= 16 && o1 <= 31) {
            return "private";
        }
        if (o0 == 192 && o1 == 168) {
            return "private";
        }
        // 240.0.0.0/4 (excluding 255.255.255.255 already handled above)
        if (o0 >= 240) {
            return "reserved";
        }
        return "unicast";
    }

    /// Convert this IPv4 address into an IPv4-mapped IPv6 address (`::ffff:a.b.c.d`).
    pub fn toIpv4MappedIpv6(): Ipv6Addr {
        if (!_.ok) {
            return ipaInvalidV6();
        }
        const hi: u16 = ((_.octets[0] as u16) << (8 as u4)) | (_.octets[1] as u16);
        const lo: u16 = ((_.octets[2] as u16) << (8 as u4)) | (_.octets[3] as u16);
        return Ipv6Addr {
            parts: [0, 0, 0, 0, 0, 0xFFFF, hi, lo],
            ok: true,
        };
    }
}

/// Parsed IPv6 address (`8` 16-bit groups + validity flag).
pub struct Ipv6Addr {
    pub parts: [8]u16,
    pub ok: bool,

    /// Return the address kind string `"ipv6"`.
    pub fn kind(): string {
        return "ipv6";
    }

    /// Return the raw 16-byte big-endian representation.
    pub fn toByteArray(): [16]u8 {
        let bytes: [16]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        let i: usize = 0;
        while (i < 8) {
            bytes[i * 2] = ((_.parts[i] >> (8 as u4)) & 0xFF) as u8;
            bytes[(i * 2) + 1] = (_.parts[i] & 0xFF) as u8;
            i = i + 1;
        }
        return bytes;
    }

    /// Check exact equality with another `Ipv6Addr`.
    pub fn equals(other: Ipv6Addr): bool {
        if (!_.ok || !other.ok) {
            return false;
        }
        let i: usize = 0;
        while (i < 8) {
            if (_.parts[i] != other.parts[i]) {
                return false;
            }
            i = i + 1;
        }
        return true;
    }

    /// Format in RFC 5952 canonical lowercase compressed form (`::` for longest run of >= 2 zeros).
    /// Returns bytes written on success, or `0` if invalid or `out` is too small.
    pub fn toStr(out: []u8): usize {
        if (!_.ok) {
            return 0;
        }
        // Find longest run of consecutive zero groups (length >= 2); tie goes to first run.
        let bestStart: usize = 8;
        let bestLen: usize = 0;
        let curStart: usize = 0;
        let curLen: usize = 0;
        let i: usize = 0;
        while (i < 8) {
            if (_.parts[i] == 0) {
                if (curLen == 0) {
                    curStart = i;
                }
                curLen = curLen + 1;
                if (curLen >= 2 && curLen > bestLen) {
                    bestStart = curStart;
                    bestLen = curLen;
                }
            } else {
                curLen = 0;
            }
            i = i + 1;
        }

        let tmp: [48]u8 = [
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        ];
        let pos: usize = 0;
        let idx: usize = 0;
        while (idx < 8) {
            if (bestLen >= 2 && idx == bestStart) {
                tmp[pos] = 58;
                pos = pos + 1;
                tmp[pos] = 58;
                pos = pos + 1;
                idx = idx + bestLen;
            } else {
                if (idx > 0 && !(bestLen >= 2 && idx == bestStart + bestLen)) {
                    tmp[pos] = 58;
                    pos = pos + 1;
                }
                pos = ipaWriteU16Hex(tmp[0..48], pos, _.parts[idx]);
                idx = idx + 1;
            }
        }

        if (out.len < pos) {
            return 0;
        }
        let k: usize = 0;
        while (k < pos) {
            out[k] = tmp[k];
            k = k + 1;
        }
        return pos;
    }

    /// Format in uncompressed normalized form (`h0:h1:h2:h3:h4:h5:h6:h7`, no `::`).
    /// Returns bytes written on success, or `0` if invalid or `out` is too small.
    pub fn toNormalizedStr(out: []u8): usize {
        if (!_.ok) {
            return 0;
        }
        let tmp: [48]u8 = [
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        ];
        let pos: usize = 0;
        let i: usize = 0;
        while (i < 8) {
            if (i > 0) {
                tmp[pos] = 58;
                pos = pos + 1;
            }
            pos = ipaWriteU16Hex(tmp[0..48], pos, _.parts[i]);
            i = i + 1;
        }
        if (out.len < pos) {
            return 0;
        }
        let k: usize = 0;
        while (k < pos) {
            out[k] = tmp[k];
            k = k + 1;
        }
        return pos;
    }

    /// Test whether this IPv6 address belongs to the CIDR block `net/prefix`.
    pub fn matchCidr(net: Ipv6Addr, prefix: u8): bool {
        if (!_.ok || !net.ok || prefix > 128) {
            return false;
        }
        if (prefix == 0) {
            return true;
        }
        const fullGroups: usize = (prefix / 16) as usize;
        const remBits: u8 = prefix % 16;
        let i: usize = 0;
        while (i < fullGroups) {
            if (_.parts[i] != net.parts[i]) {
                return false;
            }
            i = i + 1;
        }
        if (remBits > 0) {
            const mask = ipaMask16(remBits);
            if ((_.parts[fullGroups] & mask) != (net.parts[fullGroups] & mask)) {
                return false;
            }
        }
        return true;
    }

    /// Check whether this address is an IPv4-mapped IPv6 address (`::ffff:0:0/96`).
    pub fn isIpv4Mapped(): bool {
        if (!_.ok) {
            return false;
        }
        return _.parts[0] == 0
            && _.parts[1] == 0
            && _.parts[2] == 0
            && _.parts[3] == 0
            && _.parts[4] == 0
            && _.parts[5] == 0xFFFF;
    }

    /// Extract the embedded `Ipv4Addr` if this is an IPv4-mapped IPv6 address (`::ffff:0:0/96`).
    /// Returns an invalid `Ipv4Addr` (`ok: false`) otherwise.
    pub fn toIpv4(): Ipv4Addr {
        if (!_.isIpv4Mapped()) {
            return ipaInvalidV4();
        }
        const p6 = _.parts[6];
        const p7 = _.parts[7];
        return Ipv4Addr {
            octets: [
                ((p6 >> (8 as u4)) & 0xFF) as u8,
                (p6 & 0xFF) as u8,
                ((p7 >> (8 as u4)) & 0xFF) as u8,
                (p7 & 0xFF) as u8,
            ],
            ok: true,
        };
    }

    /// Classify the IPv6 address into its IANA special-purpose range name.
    pub fn range(): string {
        if (!_.ok) {
            return "invalid";
        }
        const p0 = _.parts[0];
        const p1 = _.parts[1];
        const p2 = _.parts[2];
        const p3 = _.parts[3];
        const p4 = _.parts[4];
        const p5 = _.parts[5];
        const p6 = _.parts[6];
        const p7 = _.parts[7];

        // :: (unspecified)
        if (p0 == 0 && p1 == 0 && p2 == 0 && p3 == 0 && p4 == 0 && p5 == 0 && p6 == 0 && p7 == 0) {
            return "unspecified";
        }
        // ::1 (loopback)
        if (p0 == 0 && p1 == 0 && p2 == 0 && p3 == 0 && p4 == 0 && p5 == 0 && p6 == 0 && p7 == 1) {
            return "loopback";
        }
        // fe80::/10 (linkLocal)
        if ((p0 & 0xFFC0) == 0xFE80) {
            return "linkLocal";
        }
        // fc00::/7 (uniqueLocal)
        if ((p0 & 0xFE00) == 0xFC00) {
            return "uniqueLocal";
        }
        // ::ffff:0:0/96 (ipv4Mapped)
        if (p0 == 0 && p1 == 0 && p2 == 0 && p3 == 0 && p4 == 0 && p5 == 0xFFFF) {
            return "ipv4Mapped";
        }
        // ff00::/8 (multicast)
        if ((p0 & 0xFF00) == 0xFF00) {
            return "multicast";
        }
        return "unicast";
    }
}

/// Parsed IPv4 CIDR block (`addr` + `prefix` 0..32 + validity flag).
pub struct CidrV4 {
    pub addr: Ipv4Addr,
    pub prefix: u8,
    pub ok: bool,

    /// Test whether `ip` falls inside this IPv4 CIDR block.
    pub fn matches(ip: Ipv4Addr): bool {
        if (!_.ok) {
            return false;
        }
        return ip.matchCidr(_.addr, _.prefix);
    }
}

/// Parsed IPv6 CIDR block (`addr` + `prefix` 0..128 + validity flag).
pub struct CidrV6 {
    pub addr: Ipv6Addr,
    pub prefix: u8,
    pub ok: bool,

    /// Test whether `ip` falls inside this IPv6 CIDR block.
    pub fn matches(ip: Ipv6Addr): bool {
        if (!_.ok) {
            return false;
        }
        return ip.matchCidr(_.addr, _.prefix);
    }
}

/// Parse a strict dotted-decimal IPv4 address (`0..255` per octet).
/// Rejects leading zeros (`01.2.3.4`), out-of-range octets (`256.0.0.1`),
/// trailing characters, whitespace, or wrong octet count.
pub fn parseIpv4(s: string): Ipv4Addr {
    if (s.len < 7 || s.len > 15) {
        return ipaInvalidV4();
    }
    let octets: [4]u8 = [0, 0, 0, 0];
    let partIdx: usize = 0;
    let i: usize = 0;

    while (i < s.len) {
        if (partIdx >= 4) {
            return ipaInvalidV4();
        }
        const segStart = i;
        let val: u16 = 0;
        while (i < s.len && s[i] != 46) {
            const ch = s[i];
            if (ch < 48 || ch > 57) {
                return ipaInvalidV4();
            }
            if (i == segStart + 1 && s[segStart] == 48) {
                return ipaInvalidV4();
            }
            val = (val * 10) + ((ch - 48) as u16);
            if (val > 255) {
                return ipaInvalidV4();
            }
            i = i + 1;
        }
        if (i == segStart) {
            return ipaInvalidV4();
        }
        octets[partIdx] = val as u8;
        partIdx = partIdx + 1;

        if (i < s.len) {
            // Current byte is '.'; must not be trailing.
            i = i + 1;
            if (i >= s.len) {
                return ipaInvalidV4();
            }
        }
    }

    if (partIdx != 4) {
        return ipaInvalidV4();
    }
    return Ipv4Addr {
        octets: octets,
        ok: true,
    };
}

/// Parse a full RFC 4291 IPv6 address.
/// Supports `::` zero compression, embedded IPv4 suffix (e.g. `::ffff:192.0.2.128`),
/// and case-insensitive hex digits; rejects multiple `::` and invalid groups.
pub fn parseIpv6(s: string): Ipv6Addr {
    if (s.len < 2 || s.len > 45) {
        return ipaInvalidV6();
    }

    let leftParts: [8]u16 = [0, 0, 0, 0, 0, 0, 0, 0];
    let rightParts: [8]u16 = [0, 0, 0, 0, 0, 0, 0, 0];
    let leftCount: usize = 0;
    let rightCount: usize = 0;
    let hasCompression = false;
    let i: usize = 0;

    // Handle leading ':' (only valid if "::")
    if (s[0] == 58) {
        if (s[1] != 58) {
            return ipaInvalidV6();
        }
        hasCompression = true;
        i = 2;
        if (i == s.len) {
            return Ipv6Addr {
                parts: [0, 0, 0, 0, 0, 0, 0, 0],
                ok: true,
            };
        }
        // Reject ":::"
        if (s[i] == 58) {
            return ipaInvalidV6();
        }
    }

    while (i < s.len) {
        const segStart = i;
        let hasDot = false;
        while (i < s.len && s[i] != 58) {
            if (s[i] == 46) {
                hasDot = true;
            }
            i = i + 1;
        }
        const segEnd = i;
        const segLen = segEnd - segStart;

        if (segLen == 0) {
            return ipaInvalidV6();
        }

        if (hasDot) {
            // Embedded IPv4 must be the final segment of the string
            if (segEnd != s.len) {
                return ipaInvalidV6();
            }
            const v4 = parseIpv4(s[segStart..segEnd]);
            if (!v4.ok) {
                return ipaInvalidV6();
            }
            const hi: u16 = ((v4.octets[0] as u16) << (8 as u4)) | (v4.octets[1] as u16);
            const lo: u16 = ((v4.octets[2] as u16) << (8 as u4)) | (v4.octets[3] as u16);
            if (!hasCompression) {
                if (leftCount + 2 > 8) {
                    return ipaInvalidV6();
                }
                leftParts[leftCount] = hi;
                leftParts[leftCount + 1] = lo;
                leftCount = leftCount + 2;
            } else {
                if (rightCount + 2 > 8) {
                    return ipaInvalidV6();
                }
                rightParts[rightCount] = hi;
                rightParts[rightCount + 1] = lo;
                rightCount = rightCount + 2;
            }
            break;
        } else {
            if (segLen > 4) {
                return ipaInvalidV6();
            }
            let val: u16 = 0;
            let k: usize = segStart;
            while (k < segEnd) {
                const hv = ipaHexVal(s[k]);
                if (hv < 0) {
                    return ipaInvalidV6();
                }
                val = (val << (4 as u4)) | (hv as u16);
                k = k + 1;
            }
            if (!hasCompression) {
                if (leftCount >= 8) {
                    return ipaInvalidV6();
                }
                leftParts[leftCount] = val;
                leftCount = leftCount + 1;
            } else {
                if (rightCount >= 8) {
                    return ipaInvalidV6();
                }
                rightParts[rightCount] = val;
                rightCount = rightCount + 1;
            }
        }

        if (i < s.len) {
            // s[i] is ':'
            if (i + 1 < s.len && s[i + 1] == 58) {
                // Found "::"
                if (hasCompression) {
                    return ipaInvalidV6();
                }
                hasCompression = true;
                i = i + 2;
                if (i < s.len && s[i] == 58) {
                    // Reject ":::"
                    return ipaInvalidV6();
                }
            } else {
                // Single ':' must be followed by another segment
                i = i + 1;
                if (i >= s.len) {
                    return ipaInvalidV6();
                }
            }
        }
    }

    let parts: [8]u16 = [0, 0, 0, 0, 0, 0, 0, 0];
    if (!hasCompression) {
        if (leftCount != 8) {
            return ipaInvalidV6();
        }
        let j: usize = 0;
        while (j < 8) {
            parts[j] = leftParts[j];
            j = j + 1;
        }
    } else {
        // "::" must compress at least one 16-bit zero group
        if (leftCount + rightCount >= 8) {
            return ipaInvalidV6();
        }
        let j: usize = 0;
        while (j < leftCount) {
            parts[j] = leftParts[j];
            j = j + 1;
        }
        const rightStart = 8 - rightCount;
        let r: usize = 0;
        while (r < rightCount) {
            parts[rightStart + r] = rightParts[r];
            r = r + 1;
        }
    }

    return Ipv6Addr {
        parts: parts,
        ok: true,
    };
}

/// Parse an IPv4 CIDR string `"a.b.c.d/prefix"` (`prefix` in `0..32`).
pub fn parseCidrV4(s: string): CidrV4 {
    let slashIdx: usize = s.len;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == 47) {
            if (slashIdx != s.len) {
                return CidrV4 { addr: ipaInvalidV4(), prefix: 0, ok: false };
            }
            slashIdx = i;
        }
        i = i + 1;
    }
    if (slashIdx == s.len || slashIdx == 0 || slashIdx + 1 >= s.len) {
        return CidrV4 { addr: ipaInvalidV4(), prefix: 0, ok: false };
    }
    const addr = parseIpv4(s[0..slashIdx]);
    if (!addr.ok) {
        return CidrV4 { addr: ipaInvalidV4(), prefix: 0, ok: false };
    }
    const p = ipaParsePrefix(s[(slashIdx + 1)..s.len], 32);
    if (p < 0) {
        return CidrV4 { addr: ipaInvalidV4(), prefix: 0, ok: false };
    }
    return CidrV4 {
        addr: addr,
        prefix: p as u8,
        ok: true,
    };
}

/// Parse an IPv6 CIDR string `"addr/prefix"` (`prefix` in `0..128`).
pub fn parseCidrV6(s: string): CidrV6 {
    let slashIdx: usize = s.len;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == 47) {
            if (slashIdx != s.len) {
                return CidrV6 { addr: ipaInvalidV6(), prefix: 0, ok: false };
            }
            slashIdx = i;
        }
        i = i + 1;
    }
    if (slashIdx == s.len || slashIdx == 0 || slashIdx + 1 >= s.len) {
        return CidrV6 { addr: ipaInvalidV6(), prefix: 0, ok: false };
    }
    const addr = parseIpv6(s[0..slashIdx]);
    if (!addr.ok) {
        return CidrV6 { addr: ipaInvalidV6(), prefix: 0, ok: false };
    }
    const p = ipaParsePrefix(s[(slashIdx + 1)..s.len], 128);
    if (p < 0) {
        return CidrV6 { addr: ipaInvalidV6(), prefix: 0, ok: false };
    }
    return CidrV6 {
        addr: addr,
        prefix: p as u8,
        ok: true,
    };
}

/// Return `true` if `s` is a valid IPv4 or IPv6 address.
pub fn isValidIp(s: string): bool {
    return parseIpv4(s).ok || parseIpv6(s).ok;
}

/// Return `true` if `s` is a valid IPv4 address.
pub fn isValidIpv4(s: string): bool {
    return parseIpv4(s).ok;
}

/// Return `true` if `s` is a valid IPv6 address.
pub fn isValidIpv6(s: string): bool {
    return parseIpv6(s).ok;
}

/// Detect the IP address family: `"ipv4"`, `"ipv6"`, or `"invalid"`.
pub fn ipKind(s: string): string {
    if (parseIpv4(s).ok) {
        return "ipv4";
    }
    if (parseIpv6(s).ok) {
        return "ipv6";
    }
    return "invalid";
}

export {
    Ipv4Addr,
    Ipv6Addr,
    CidrV4,
    CidrV6,
    parseIpv4,
    parseIpv6,
    parseCidrV4,
    parseCidrV6,
    isValidIp,
    isValidIpv4,
    isValidIpv6,
    ipKind,
};
