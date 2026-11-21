// Source: Absorbed from /data/dev/bun/src/sys.zig (IEEE 802 MAC-48 / EUI-48 Address Parser & EUI-64 Converter)
// Pure-CP zero-allocation MAC-48 parser (colon, hyphen, Cisco dot, bare hex), classifier, formatter, and EUI-64 derivation.

fn hexVal(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

fn hexLower(v: u8): u8 {
    if (v < 10) return @as(u8, '0') + v;
    return @as(u8, 'a') + (v - 10);
}

fn hexUpper(v: u8): u8 {
    if (v < 10) return @as(u8, '0') + v;
    return @as(u8, 'A') + (v - 10);
}

fn parseByteHex(hi_c: u8, lo_c: u8): u8? {
    const hi = hexVal(hi_c) ?? return null;
    const lo = hexVal(lo_c) ?? return null;
    return (hi << 4) | lo;
}

export struct MacAddr {
    octets: [6]u8,

    /// Parse IEEE 802 MAC-48 in any standard format:
    /// - Colon: `aa:bb:cc:dd:ee:ff` (17 chars)
    /// - Hyphen: `AA-BB-CC-DD-EE-FF` (17 chars)
    /// - Cisco dot: `aabb.ccdd.eeff` (14 chars)
    /// - Bare hex: `aabbccddeeff` (12 chars)
    pub fn parse(raw: []const u8): MacAddr? {
        let out: [6]u8 = [0; 6];
        if (raw.len == 17) {
            const sep = raw[2];
            if (sep != ':' && sep != '-') return null;
            if (raw[5] != sep || raw[8] != sep || raw[11] != sep || raw[14] != sep) return null;
            let i: usize = 0;
            while (i < 6) {
                const off = i * 3;
                out[i] = parseByteHex(raw[off], raw[off + 1]) ?? return null;
                i += 1;
            }
            return MacAddr { octets: out };
        }
        if (raw.len == 14) {
            if (raw[4] != '.' || raw[9] != '.') return null;
            out[0] = parseByteHex(raw[0], raw[1]) ?? return null;
            out[1] = parseByteHex(raw[2], raw[3]) ?? return null;
            out[2] = parseByteHex(raw[5], raw[6]) ?? return null;
            out[3] = parseByteHex(raw[7], raw[8]) ?? return null;
            out[4] = parseByteHex(raw[10], raw[11]) ?? return null;
            out[5] = parseByteHex(raw[12], raw[13]) ?? return null;
            return MacAddr { octets: out };
        }
        if (raw.len == 12) {
            let i: usize = 0;
            while (i < 6) {
                const off = i * 2;
                out[i] = parseByteHex(raw[off], raw[off + 1]) ?? return null;
                i += 1;
            }
            return MacAddr { octets: out };
        }
        return null;
    }

    pub fn isZero(): bool {
        return _.octets[0] == 0 && _.octets[1] == 0 && _.octets[2] == 0 &&
            _.octets[3] == 0 && _.octets[4] == 0 && _.octets[5] == 0;
    }

    pub fn isBroadcast(): bool {
        return _.octets[0] == 0xFF && _.octets[1] == 0xFF && _.octets[2] == 0xFF &&
            _.octets[3] == 0xFF && _.octets[4] == 0xFF && _.octets[5] == 0xFF;
    }

    /// Least significant bit of first octet: 1 = multicast, 0 = unicast.
    pub fn isMulticast(): bool {
        return (_.octets[0] & 0x01) != 0;
    }

    pub fn isUnicast(): bool {
        return (_.octets[0] & 0x01) == 0;
    }

    /// Second least significant bit of first octet: 1 = locally administered (LAA), 0 = universally administered (OUI/UAA).
    pub fn isLocallyAdministered(): bool {
        return (_.octets[0] & 0x02) != 0;
    }

    /// Derive RFC 4291 Modified EUI-64 8-byte interface identifier (inserts `FF:FE` and flips U/L bit).
    pub fn toEui64(): [8]u8 {
        return [
            _.octets[0] ^ 0x02,
            _.octets[1],
            _.octets[2],
            0xFF,
            0xFE,
            _.octets[3],
            _.octets[4],
            _.octets[5],
        ];
    }

    /// Format as lowercase colon-separated `aa:bb:cc:dd:ee:ff` (17 bytes).
    pub fn formatColon(out: []u8): ([]const u8)? {
        if (out.len < 17) return null;
        let pos: usize = 0;
        let i: usize = 0;
        while (i < 6) {
            if (i > 0) {
                out[pos] = ':';
                pos += 1;
            }
            out[pos] = hexLower((_.octets[i] >> 4) & 0x0F);
            out[pos + 1] = hexLower(_.octets[i] & 0x0F);
            pos += 2;
            i += 1;
        }
        return out[0..17];
    }

    /// Format as uppercase hyphen-separated `AA-BB-CC-DD-EE-FF` (17 bytes).
    pub fn formatHyphenUpper(out: []u8): ([]const u8)? {
        if (out.len < 17) return null;
        let pos: usize = 0;
        let i: usize = 0;
        while (i < 6) {
            if (i > 0) {
                out[pos] = '-';
                pos += 1;
            }
            out[pos] = hexUpper((_.octets[i] >> 4) & 0x0F);
            out[pos + 1] = hexUpper(_.octets[i] & 0x0F);
            pos += 2;
            i += 1;
        }
        return out[0..17];
    }
}
