// Reference-hardened against krisprice/ipnet & smoltcp (MIT / 0BSD)
// Source: Absorbed from /data/dev/bun/src/runtime/node/net/BlockList.cp | Synced: 2026-11-22T16:14:43Z
// Pure-CP zero-allocation IPv4/IPv6 BlockList (Node/Bun `net.BlockList` semantics):
// single-address, inclusive range, and CIDR subnet rules with IPv4-mapped IPv6 interop & 16-lane SIMD.

import { Ipv4Addr, Ipv6Addr, IpVersion, IpAddr } from "./ipaddr.cp";
import { simdFwBytesEqual16 } from "./connTrack.cp";

type FwPrefixVec16u8 = @Vector(16, u8);

pub const BLOCKLIST_MAX_RULES: usize = 256;
pub const BLOCKLIST_IPV4_MAX_PREFIX: u8 = 32;
pub const BLOCKLIST_IPV6_MAX_PREFIX: u8 = 128;

pub enum BlockListFamily {
    Ipv4,
    Ipv6,

    static fn parse(s: []const u8): BlockListFamily? {
        if (s.len != 4) return null;
        const c0 = toLowerAscii(s[0]);
        const c1 = toLowerAscii(s[1]);
        const c2 = toLowerAscii(s[2]);
        const c3 = s[3];
        if (c0 == 'i' && c1 == 'p' && c2 == 'v') {
            if (c3 == '4') return BlockListFamily.Ipv4;
            if (c3 == '6') return BlockListFamily.Ipv6;
        }
        return null;
    }

    fn upperName(): []const u8 {
        if (_ == BlockListFamily.Ipv4) return "IPv4";
        return "IPv6";
    }

    fn lowerName(): []const u8 {
        if (_ == BlockListFamily.Ipv4) return "ipv4";
        return "ipv6";
    }

    fn maxPrefix(): u8 {
        if (_ == BlockListFamily.Ipv4) return BLOCKLIST_IPV4_MAX_PREFIX;
        return BLOCKLIST_IPV6_MAX_PREFIX;
    }
}

pub enum BlockListRuleKind {
    Address,
    Range,
    Subnet,
}

pub err BlockListError {
    InvalidAddress,
    InvalidFamily,
    InvalidPrefix,
    FamilyMismatch,
    InvalidRange,
    CapacityFull,
    BufferTooSmall,
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn writeSlice(out: []u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) return false;
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return true;
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

fn parsePrefixU8(s: []const u8): u8? {
    if (s.len == 0 || s.len > 3) return null;
    if (s.len > 1 && s[0] == '0') return null;
    let val: u16 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        val = val * 10 + @as(u16, c - '0');
        i += 1;
    }
    if (val > 255) return null;
    return @as(u8, val);
}

fn ipv6ToU128(a: Ipv6Addr): u128 {
    let acc: u128 = 0;
    let i: usize = 0;
    while (i < 8) {
        acc = (acc << 16) | @as(u128, a.segments[i]);
        i += 1;
    }
    return acc;
}

fn u128ToIpv6(v: u128): Ipv6Addr {
    let segs: [8]u16 = [0; 8];
    let i: usize = 0;
    while (i < 8) {
        const shift: u7 = @as(u7, (7 - i) * 16);
        segs[i] = @as(u16, (v >> shift) & 0xFFFF);
        i += 1;
    }
    return Ipv6Addr { segments: segs };
}

fn u128ToBeBytes16(v: u128): [16]u8 {
    let out: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        const shift: u7 = @as(u7, @intCast((15 - i) * 8));
        out[i] = @as(u8, @truncate(v >> shift));
    }
    return out;
}

fn beBytes16ToU128(bytes: [16]u8): u128 {
    let acc: u128 = 0;
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        acc = (acc << 8) | @as(u128, bytes[i]);
    }
    return acc;
}

/// 16-lane `@Vector(16, u8)` SIMD IPv6 CIDR prefix match (`0..=128`).
/// Hot path for `BlockList` IPv6 subnet matching.
pub fn simdFwIpv6MatchesPrefix(ip: [16]u8, net: [16]u8, prefix: u8): bool {
    if (prefix == 0) return true;
    if (prefix >= 128) return simdFwBytesEqual16(ip, net);
    let mask: [16]u8 = [0; 16];
    const full_bytes: usize = @as(usize, prefix / 8);
    const rem_bits: u8 = prefix % 8;
    let i: usize = 0;
    while (i < full_bytes) : (i += 1) {
        mask[i] = 0xFF;
    }
    if (rem_bits > 0 && full_bytes < 16) {
        const shift: u3 = @as(u3, @intCast(8 - rem_bits));
        mask[full_bytes] = @as(u8, 0xFF) << shift;
    }
    const v_ip: FwPrefixVec16u8 = ip;
    const v_net: FwPrefixVec16u8 = net;
    const v_mask: FwPrefixVec16u8 = mask;
    const diff: FwPrefixVec16u8 = (v_ip ^ v_net) & v_mask;
    return @reduce(.Or, diff) == 0;
}

/// Scalar reference for `simdFwIpv6MatchesPrefix` (0-diff differential verification).
pub fn scalarFwIpv6MatchesPrefix(ip: [16]u8, net: [16]u8, prefix: u8): bool {
    if (prefix == 0) return true;
    const clamped: u8 = if (prefix > 128) 128 else prefix;
    const full_bytes: usize = @as(usize, clamped / 8);
    const rem_bits: u8 = clamped % 8;
    let i: usize = 0;
    while (i < full_bytes) : (i += 1) {
        if (ip[i] != net[i]) return false;
    }
    if (rem_bits > 0 && full_bytes < 16) {
        const shift: u3 = @as(u3, @intCast(8 - rem_bits));
        const m: u8 = @as(u8, 0xFF) << shift;
        if ((ip[full_bytes] & m) != (net[full_bytes] & m)) return false;
    }
    return true;
}

fn maskMatchV4(ip: u32, net: u32, prefix: u8): bool {
    if (prefix == 0) return true;
    if (prefix >= 32) return ip == net;
    const shift: u5 = @as(u5, 32 - prefix);
    return (ip >> shift) == (net >> shift);
}

fn maskMatchV6(ip: u128, net: u128, prefix: u8): bool {
    if (prefix == 0) return true;
    return simdFwIpv6MatchesPrefix(u128ToBeBytes16(ip), u128ToBeBytes16(net), prefix);
}

/// Normalized IPv4 / IPv6 address stored in host-comparable big-endian integer order.
pub struct BlockListAddr {
    family: BlockListFamily = BlockListFamily.Ipv4,
    v4: u32 = 0,
    v6: u128 = 0,

    static fn fromV4(addr: Ipv4Addr): BlockListAddr {
        return BlockListAddr {
            family: BlockListFamily.Ipv4,
            v4: addr.toU32(),
            v6: 0,
        };
    }

    static fn fromV6(addr: Ipv6Addr): BlockListAddr {
        return BlockListAddr {
            family: BlockListFamily.Ipv6,
            v4: 0,
            v6: ipv6ToU128(addr),
        };
    }

    static fn fromV6Bytes(bytes: [16]u8): BlockListAddr {
        return BlockListAddr {
            family: BlockListFamily.Ipv6,
            v4: 0,
            v6: beBytes16ToU128(bytes),
        };
    }

    fn toV6Bytes(): [16]u8 {
        if (_.family == BlockListFamily.Ipv4) {
            return u128ToBeBytes16((@as(u128, 0xFFFF) << 32) | @as(u128, _.v4));
        }
        return u128ToBeBytes16(_.v6);
    }

    static fn fromIpAddr(ip: IpAddr): BlockListAddr {
        if (ip.version == IpVersion.v4) {
            return BlockListAddr.fromV4(ip.v4);
        }
        return BlockListAddr.fromV6(ip.v6);
    }

    static fn parseWithFamily(s: []const u8, family: BlockListFamily): BlockListAddr? {
        if (family == BlockListFamily.Ipv4) {
            if (a4 := Ipv4Addr.parse(s)) {
                return BlockListAddr.fromV4(a4);
            }
            return null;
        }
        if (a6 := Ipv6Addr.parse(s)) {
            return BlockListAddr.fromV6(a6);
        }
        return null;
    }

    static fn parseAuto(s: []const u8): BlockListAddr? {
        if (a4 := Ipv4Addr.parse(s)) {
            return BlockListAddr.fromV4(a4);
        }
        if (a6 := Ipv6Addr.parse(s)) {
            return BlockListAddr.fromV6(a6);
        }
        return null;
    }

    /// Returns IPv4 value for native IPv4 or IPv4-mapped IPv6 (`::ffff:a.b.c.d`).
    fn asV4(): u32? {
        if (_.family == BlockListFamily.Ipv4) return _.v4;
        const prefix_hi: u128 = _.v6 >> 32;
        if (prefix_hi == 0x0000_FFFF) {
            return @as(u32, _.v6 & 0xFFFF_FFFF);
        }
        return null;
    }

    /// Compares two addresses; returns -1, 0, 1, or null if incompatible families.
    /// Aligns with Bun `_compare`: native IPv4 and IPv4-mapped IPv6 compare as IPv4.
    fn compare(other: BlockListAddr): i32? {
        if (l4 := _.asV4()) {
            if (r4 := other.asV4()) {
                if (l4 < r4) return -1;
                if (l4 > r4) return 1;
                return 0;
            }
        }
        if (_.family == BlockListFamily.Ipv6 && other.family == BlockListFamily.Ipv6) {
            if (_.v6 < other.v6) return -1;
            if (_.v6 > other.v6) return 1;
            return 0;
        }
        return null;
    }

    fn format(out: []u8): ([]const u8)? {
        if (_.family == BlockListFamily.Ipv4) {
            return Ipv4Addr.fromU32(_.v4).format(out);
        }
        return u128ToIpv6(_.v6).format(out);
    }
}

/// A single BlockList rule: Address, Range (`[start, end]`), or Subnet (`network/prefix`).
pub struct BlockListRule {
    kind: BlockListRuleKind = BlockListRuleKind.Address,
    family: BlockListFamily = BlockListFamily.Ipv4,
    start: BlockListAddr = BlockListAddr {},
    end: BlockListAddr = BlockListAddr {},
    prefix: u8 = 0,

    fn matches(addr: BlockListAddr): bool {
        if (_.kind == BlockListRuleKind.Address) {
            if (l4 := addr.asV4()) {
                if (r4 := _.start.asV4()) {
                    return l4 == r4;
                }
            }
            if (addr.family == BlockListFamily.Ipv6 && _.start.family == BlockListFamily.Ipv6) {
                return simdFwBytesEqual16(u128ToBeBytes16(addr.v6), u128ToBeBytes16(_.start.v6));
            }
            return false;
        }
        if (_.kind == BlockListRuleKind.Range) {
            const os = addr.compare(_.start) ?? return false;
            const oe = addr.compare(_.end) ?? return false;
            return os >= 0 && oe <= 0;
        }
        // Subnet rule: check IPv4 (including IPv4-mapped IPv6) first, then native IPv6.
        if (ip4 := addr.asV4()) {
            if (net4 := _.start.asV4()) {
                let effective_prefix: u8 = _.prefix;
                if (_.start.family == BlockListFamily.Ipv6) {
                    if (_.prefix < 96) {
                        return maskMatchV6(
                            (@as(u128, 0xFFFF) << 32) | @as(u128, ip4),
                            _.start.v6,
                            _.prefix,
                        );
                    }
                    effective_prefix = _.prefix - 96;
                }
                if (maskMatchV4(ip4, net4, effective_prefix)) return true;
            }
        }
        if (addr.family == BlockListFamily.Ipv6 && _.start.family == BlockListFamily.Ipv6) {
            return maskMatchV6(addr.v6, _.start.v6, _.prefix);
        }
        return false;
    }

    /// Formats rule identically to Node/Bun `BlockList.prototype.rules`:
    /// - `"Address: IPv4 127.0.0.1"`
    /// - `"Range: IPv4 10.0.0.1-10.0.0.10"`
    /// - `"Subnet: IPv6 2001:db8::/32"`
    fn format(out: []u8): ([]const u8)? {
        let pos: usize = 0;
        let tmp1: [64]u8 = [0; 64];
        let tmp2: [64]u8 = [0; 64];

        if (_.kind == BlockListRuleKind.Address) {
            if (!writeSlice(out, &pos, "Address: ")) return null;
            if (!writeSlice(out, &pos, _.start.family.upperName())) return null;
            if (!writeByte(out, &pos, ' ')) return null;
            const s1 = _.start.format(tmp1[0..]) ?? return null;
            if (!writeSlice(out, &pos, s1)) return null;
            return out[0..pos];
        }
        if (_.kind == BlockListRuleKind.Range) {
            if (!writeSlice(out, &pos, "Range: ")) return null;
            if (!writeSlice(out, &pos, _.start.family.upperName())) return null;
            if (!writeByte(out, &pos, ' ')) return null;
            const s1 = _.start.format(tmp1[0..]) ?? return null;
            if (!writeSlice(out, &pos, s1)) return null;
            if (!writeByte(out, &pos, '-')) return null;
            const s2 = _.end.format(tmp2[0..]) ?? return null;
            if (!writeSlice(out, &pos, s2)) return null;
            return out[0..pos];
        }
        if (!writeSlice(out, &pos, "Subnet: ")) return null;
        if (!writeSlice(out, &pos, _.start.family.upperName())) return null;
        if (!writeByte(out, &pos, ' ')) return null;
        const s1 = _.start.format(tmp1[0..]) ?? return null;
        if (!writeSlice(out, &pos, s1)) return null;
        if (!writeByte(out, &pos, '/')) return null;
        if (!writeDecU8(out, &pos, _.prefix)) return null;
        return out[0..pos];
    }
}

/// Zero-allocation IPv4/IPv6 BlockList aligned with Node.js / Bun `net.BlockList`.
/// Rules are prepended (LIFO order at index 0) to match Bun `da_rules.insert(0, ...)`.
pub struct BlockList {
    rules: [BLOCKLIST_MAX_RULES]BlockListRule = [BlockListRule {}; BLOCKLIST_MAX_RULES],
    count: usize = 0,

    static fn init(): BlockList {
        return BlockList {};
    }

    static fn parseFamily(s: []const u8): BlockListFamily? {
        return BlockListFamily.parse(s);
    }

    static fn ipv4ToMappedV6(v4: u32): u128 {
        return (@as(u128, 0xFFFF) << 32) | @as(u128, v4);
    }

    static fn mappedV6ToIpv4(v6: u128): u32? {
        if ((v6 >> 32) == 0x0000_FFFF) {
            return @as(u32, v6 & 0xFFFF_FFFF);
        }
        return null;
    }

    fn len(): usize {
        return _.count;
    }

    fn isEmpty(): bool {
        return _.count == 0;
    }

    fn capacity(): usize {
        return BLOCKLIST_MAX_RULES;
    }

    fn clear(*_) {
        _.count = 0;
    }

    fn prependRule(*_, rule: BlockListRule): ! {
        if (_.count >= BLOCKLIST_MAX_RULES) return BlockListError.CapacityFull;
        let i: usize = _.count;
        while (i > 0) {
            _.rules[i] = _.rules[i - 1];
            i -= 1;
        }
        _.rules[0] = rule;
        _.count += 1;
    }

    /// Adds a single IPv4 or IPv6 address rule with explicit family validation.
    fn addAddress(*_, address: []const u8, family: BlockListFamily): ! {
        const parsed = BlockListAddr.parseWithFamily(address, family) ?? return BlockListError.InvalidAddress;
        _.prependRule(BlockListRule {
            kind: BlockListRuleKind.Address,
            family: family,
            start: parsed,
            end: parsed,
            prefix: family.maxPrefix(),
        })?;
    }

    /// Adds a single IPv4 or IPv6 address rule, auto-detecting address family.
    fn addAddressAuto(*_, address: []const u8): ! {
        const parsed = BlockListAddr.parseAuto(address) ?? return BlockListError.InvalidAddress;
        _.prependRule(BlockListRule {
            kind: BlockListRuleKind.Address,
            family: parsed.family,
            start: parsed,
            end: parsed,
            prefix: parsed.family.maxPrefix(),
        })?;
    }

    /// Adds an inclusive IP range `[start, end]`; rejects `start > end`.
    fn addRange(*_, start: []const u8, end: []const u8, family: BlockListFamily): ! {
        const s_addr = BlockListAddr.parseWithFamily(start, family) ?? return BlockListError.InvalidAddress;
        const e_addr = BlockListAddr.parseWithFamily(end, family) ?? return BlockListError.InvalidAddress;
        if (ord := s_addr.compare(e_addr)) {
            if (ord > 0) return BlockListError.InvalidRange;
        } else {
            return BlockListError.FamilyMismatch;
        }
        _.prependRule(BlockListRule {
            kind: BlockListRuleKind.Range,
            family: family,
            start: s_addr,
            end: e_addr,
            prefix: 0,
        })?;
    }

    /// Adds an inclusive IP range `[start, end]`, auto-detecting family and rejecting cross-family pairs.
    fn addRangeAuto(*_, start: []const u8, end: []const u8): ! {
        const s_addr = BlockListAddr.parseAuto(start) ?? return BlockListError.InvalidAddress;
        const e_addr = BlockListAddr.parseAuto(end) ?? return BlockListError.InvalidAddress;
        if (s_addr.family != e_addr.family) return BlockListError.FamilyMismatch;
        if (ord := s_addr.compare(e_addr)) {
            if (ord > 0) return BlockListError.InvalidRange;
        } else {
            return BlockListError.FamilyMismatch;
        }
        _.prependRule(BlockListRule {
            kind: BlockListRuleKind.Range,
            family: s_addr.family,
            start: s_addr,
            end: e_addr,
            prefix: 0,
        })?;
    }

    /// Adds a CIDR subnet rule (`network`, `prefix`, `family`).
    /// Validates `prefix <= 32` for IPv4 and `prefix <= 128` for IPv6.
    fn addSubnet(*_, network: []const u8, prefix: u8, family: BlockListFamily): ! {
        if (prefix > family.maxPrefix()) return BlockListError.InvalidPrefix;
        const net_addr = BlockListAddr.parseWithFamily(network, family) ?? return BlockListError.InvalidAddress;
        _.prependRule(BlockListRule {
            kind: BlockListRuleKind.Subnet,
            family: family,
            start: net_addr,
            end: net_addr,
            prefix: prefix,
        })?;
    }

    /// Convenience helper accepting `"192.168.1.0/24"` or `"2001:db8::/32"`.
    fn addSubnetCidr(*_, cidr: []const u8): ! {
        let slash: usize = cidr.len;
        let i: usize = 0;
        while (i < cidr.len) {
            if (cidr[i] == '/') {
                slash = i;
                break;
            }
            i += 1;
        }
        if (slash == cidr.len || slash == 0 || slash + 1 >= cidr.len) {
            return BlockListError.InvalidPrefix;
        }
        const net_str = cidr[0..slash];
        const prefix_str = cidr[slash + 1..];
        const prefix = parsePrefixU8(prefix_str) ?? return BlockListError.InvalidPrefix;
        const net_addr = BlockListAddr.parseAuto(net_str) ?? return BlockListError.InvalidAddress;
        if (prefix > net_addr.family.maxPrefix()) return BlockListError.InvalidPrefix;
        _.prependRule(BlockListRule {
            kind: BlockListRuleKind.Subnet,
            family: net_addr.family,
            start: net_addr,
            end: net_addr,
            prefix: prefix,
        })?;
    }

    /// Checks whether `address` (parsed with `family`) is blocked.
    /// Aligns with Node/Bun: invalid address returns `false` without throwing.
    fn check(address: []const u8, family: BlockListFamily): bool {
        const parsed = BlockListAddr.parseWithFamily(address, family) ?? return false;
        return _.checkAddr(parsed);
    }

    /// Checks whether `address` (auto-detected IPv4 or IPv6) is blocked.
    fn checkAuto(address: []const u8): bool {
        const parsed = BlockListAddr.parseAuto(address) ?? return false;
        return _.checkAddr(parsed);
    }

    /// Checks a pre-parsed `IpAddr` from `cap:net`.
    fn checkIp(ip: IpAddr): bool {
        return _.checkAddr(BlockListAddr.fromIpAddr(ip));
    }

    fn checkAddr(addr: BlockListAddr): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (_.rules[i].matches(addr)) return true;
            i += 1;
        }
        return false;
    }

    fn ruleAt(idx: usize): BlockListRule? {
        if (idx >= _.count) return null;
        return _.rules[idx];
    }

    fn removeAt(*_, idx: usize): bool {
        if (idx >= _.count) return false;
        let i: usize = idx;
        while (i + 1 < _.count) {
            _.rules[i] = _.rules[i + 1];
            i += 1;
        }
        _.count -= 1;
        return true;
    }

    /// Formats the rule at `idx` into `out` and returns the written slice.
    fn formatRule(idx: usize, out: []u8): ([]const u8)? {
        if (idx >= _.count) return null;
        return _.rules[idx].format(out);
    }

    /// Formats all rules newline-separated (`'\n'`) into `out`, returning total bytes written.
    fn formatAllRules(out: []u8): usize? {
        let pos: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            if (i > 0) {
                if (!writeByte(out, &pos, '\n')) return null;
            }
            const slice = _.rules[i].format(out[pos..]) ?? return null;
            pos += slice.len;
            i += 1;
        }
        return pos;
    }

    /// Appends rules from `other` (preserving `other`'s priority order).
    fn mergeFrom(*_, other: BlockList): ! {
        if (other.count > BLOCKLIST_MAX_RULES || _.count > BLOCKLIST_MAX_RULES - other.count) {
            return BlockListError.CapacityFull;
        }
        let i: usize = 0;
        while (i < other.count) {
            _.rules[_.count] = other.rules[i];
            _.count += 1;
            i += 1;
        }
    }
}
