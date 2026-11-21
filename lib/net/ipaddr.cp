// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-21T14:39:13Z (RFC 5952 IPv4 / IPv6 Address Parser & Canonical Formatter)
// Pure-CP zero-allocation IPv4 & IPv6 parser, RFC 5952 canonical formatter, classification, and CIDR matching.

fn hexVal(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

fn hexDigitLower(v: u8): u8 {
    if (v < 10) return @as(u8, '0') + v;
    return @as(u8, 'a') + (v - 10);
}

fn writeByte(out: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out.len) return false;
    out[pos.*] = b;
    pos.* += 1;
    return true;
}

fn writeDecU8(out: []u8, pos: *usize, v: u8): bool {
    if (v >= 100) {
        if (!writeByte(out, pos, @as(u8, '0') + (v / 100))) return false;
        if (!writeByte(out, pos, @as(u8, '0') + ((v / 10) % 10))) return false;
        return writeByte(out, pos, @as(u8, '0') + (v % 10));
    }
    if (v >= 10) {
        if (!writeByte(out, pos, @as(u8, '0') + (v / 10))) return false;
        return writeByte(out, pos, @as(u8, '0') + (v % 10));
    }
    return writeByte(out, pos, @as(u8, '0') + v);
}

fn writeHexU16(out: []u8, pos: *usize, v: u16): bool {
    if (v == 0) return writeByte(out, pos, '0');
    let started = false;
    const d0: u8 = @as(u8, (v >> 12) & 0xF);
    const d1: u8 = @as(u8, (v >> 8) & 0xF);
    const d2: u8 = @as(u8, (v >> 4) & 0xF);
    const d3: u8 = @as(u8, v & 0xF);
    if (d0 != 0) {
        if (!writeByte(out, pos, hexDigitLower(d0))) return false;
        started = true;
    }
    if (started || d1 != 0) {
        if (!writeByte(out, pos, hexDigitLower(d1))) return false;
        started = true;
    }
    if (started || d2 != 0) {
        if (!writeByte(out, pos, hexDigitLower(d2))) return false;
    }
    return writeByte(out, pos, hexDigitLower(d3));
}

export struct Ipv4Addr {
    octets: [4]u8,

    pub fn fromOctets(a: u8, b: u8, c: u8, d: u8): Ipv4Addr {
        return Ipv4Addr { octets: [a, b, c, d] };
    }

    pub fn toU32(): u32 {
        return (@as(u32, _.octets[0]) << 24) |
            (@as(u32, _.octets[1]) << 16) |
            (@as(u32, _.octets[2]) << 8) |
            @as(u32, _.octets[3]);
    }

    pub fn fromU32(val: u32): Ipv4Addr {
        return Ipv4Addr {
            octets: [
                @as(u8, (val >> 24) & 0xFF),
                @as(u8, (val >> 16) & 0xFF),
                @as(u8, (val >> 8) & 0xFF),
                @as(u8, val & 0xFF),
            ],
        };
    }

    /// Strict dotted-quad IPv4 parser (rejects leading zeros like "01" and >255 octets).
    pub fn parse(s: []const u8): Ipv4Addr? {
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

    pub fn isLoopback(): bool {
        return _.octets[0] == 127;
    }

    pub fn isPrivate(): bool {
        if (_.octets[0] == 10) return true;
        if (_.octets[0] == 172 && _.octets[1] >= 16 && _.octets[1] <= 31) return true;
        if (_.octets[0] == 192 && _.octets[1] == 168) return true;
        return false;
    }

    pub fn isLinkLocal(): bool {
        return _.octets[0] == 169 && _.octets[1] == 254;
    }

    pub fn isMulticast(): bool {
        return _.octets[0] >= 224 && _.octets[0] <= 239;
    }

    pub fn isBroadcast(): bool {
        return _.octets[0] == 255 && _.octets[1] == 255 && _.octets[2] == 255 && _.octets[3] == 255;
    }

    pub fn inSubnet(network: Ipv4Addr, prefix_len: u8): bool {
        if (prefix_len > 32) return false;
        if (prefix_len == 0) return true;
        const shift: u5 = @as(u5, 32 - prefix_len);
        return (_.toU32() >> shift) == (network.toU32() >> shift);
    }

    pub fn format(out: []u8): ([]const u8)? {
        let pos: usize = 0;
        if (!writeDecU8(out, &pos, _.octets[0])) return null;
        if (!writeByte(out, &pos, '.')) return null;
        if (!writeDecU8(out, &pos, _.octets[1])) return null;
        if (!writeByte(out, &pos, '.')) return null;
        if (!writeDecU8(out, &pos, _.octets[2])) return null;
        if (!writeByte(out, &pos, '.')) return null;
        if (!writeDecU8(out, &pos, _.octets[3])) return null;
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

    /// Parse RFC 4291 IPv6 literal including `::` compression and embedded IPv4 suffix (e.g. `::ffff:192.0.2.128`).
    pub fn parse(raw: []const u8): Ipv6Addr? {
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
            // Check if the remainder contains '.' -> embedded IPv4 tail
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

            // Parse 1..4 hex digits
            const start = i;
            let val: u16 = 0;
            while (i < raw.len && raw[i] != ':') {
                const hv = hexVal(raw[i]) ?? return null;
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
                // Must be ':'
                if (raw[i] != ':') return null;
                i += 1;
                if (i >= raw.len) return null; // trailing single ':' invalid
                if (raw[i] == ':') {
                    if (has_compress) return null; // second '::' invalid
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
        let i: usize = 0;
        while (i < 7) {
            if (_.segments[i] != 0) return false;
            i += 1;
        }
        return _.segments[7] == 1;
    }

    pub fn isMulticast(): bool {
        return (_.segments[0] & 0xFF00) == 0xFF00;
    }

    pub fn isLinkLocal(): bool {
        return (_.segments[0] & 0xFFC0) == 0xFE80;
    }

    pub fn isUniqueLocal(): bool {
        return (_.segments[0] & 0xFE00) == 0xFC00;
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

    pub fn inSubnet(network: Ipv6Addr, prefix_len: u8): bool {
        if (prefix_len > 128) return false;
        let rem: u8 = prefix_len;
        let idx: usize = 0;
        while (idx < 8 && rem > 0) {
            if (rem >= 16) {
                if (_.segments[idx] != network.segments[idx]) return false;
                rem -= 16;
            } else {
                const shift: u4 = @as(u4, 16 - rem);
                if ((_.segments[idx] >> shift) != (network.segments[idx] >> shift)) return false;
                rem = 0;
            }
            idx += 1;
        }
        return true;
    }

    /// Canonical RFC 5952 IPv6 formatter (longest >=2 zero run compressed to `::`, lowercase hex, IPv4-mapped suffix).
    pub fn format(out: []u8): ([]const u8)? {
        let pos: usize = 0;
        if (v4 := _.toIpv4Mapped()) {
            const prefix = "::ffff:";
            let pi: usize = 0;
            while (pi < prefix.len) {
                if (!writeByte(out, &pos, prefix[pi])) return null;
                pi += 1;
            }
            const rest = v4.format(out[pos..]) ?? return null;
            return out[0..pos + rest.len];
        }

        // Find longest consecutive run of zero segments (length >= 2)
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
                if (!writeByte(out, &pos, ':')) return null;
                if (!writeByte(out, &pos, ':')) return null;
                cur += best_len;
                continue;
            }
            if (cur > 0 && !(best_len >= 2 && cur == best_start + best_len)) {
                if (!writeByte(out, &pos, ':')) return null;
            }
            if (!writeHexU16(out, &pos, _.segments[cur])) return null;
            cur += 1;
        }
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

    pub fn parse(s: []const u8): IpAddr? {
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

    pub fn isLoopback(): bool {
        if (_.version == IpVersion.v4) return _.v4.isLoopback();
        return _.v6.isLoopback();
    }

    pub fn format(out: []u8): ([]const u8)? {
        if (_.version == IpVersion.v4) return _.v4.format(out);
        return _.v6.format(out);
    }
}
