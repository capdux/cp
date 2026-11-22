// Reference Only: Functional overlap with lib/net/cidr.cp | Kept for comparison/analysis
// Source: cp/lib/cidr/cidr.cp (Pure CP IPv4/IPv6 CIDR Parsing, Subnetting, Summarization & LPM Router)
// Zero-C-dependency IPv4/IPv6 address, CIDR block, subnetting, summarization, and LPM routing engine.

const std = @import("std");

/// Unified error set returned by `cap:cidr` parsing, subnetting, and routing operations.
pub err CidrError {
    InvalidIpAddress,
    InvalidPrefixLength,
    InvalidCidrNotation,
    InvalidSubnetSplit,
    OutOfMemory,
}

fn mask4(prefix_len: u8): u32 {
    if (prefix_len == 0) return 0;
    if (prefix_len >= 32) return 0xFFFFFFFF;
    const shift: u5 = (32 - prefix_len) as u5;
    return (0xFFFFFFFF as u32) << shift;
}

fn mask6(prefix_len: u8): u128 {
    if (prefix_len == 0) return 0;
    if (prefix_len >= 128) return ~((0 as u128));
    const shift: u7 = (128 - prefix_len) as u7;
    return (~((0 as u128))) << shift;
}

fn parseDecU8(text: []const u8): u8? {
    if (text.len == 0 || text.len > 3) return null;
    if (text.len > 1 && text[0] == '0') return null;
    let acc: u16 = 0;
    let i: usize = 0;
    while (i < text.len) : (i += 1) {
        const ch = text[i];
        if (ch < '0' || ch > '9') return null;
        acc = acc * 10 + ((ch - '0') as u16);
    }
    if (acc > 255) return null;
    return acc as u8;
}

fn parsePrefixLen(text: []const u8, max_prefix: u8): u8 ! CidrError {
    const val = parseDecU8(text) orelse return CidrError.InvalidPrefixLength;
    if (val > max_prefix) return CidrError.InvalidPrefixLength;
    return val;
}

fn hexDigitVal(ch: u8): u16? {
    if (ch >= '0' && ch <= '9') return (ch - '0') as u16;
    if (ch >= 'a' && ch <= 'f') return (ch - 'a' + 10) as u16;
    if (ch >= 'A' && ch <= 'F') return (ch - 'A' + 10) as u16;
    return null;
}

fn parseHexGroup(text: []const u8): u16? {
    if (text.len == 0 || text.len > 4) return null;
    let acc: u16 = 0;
    let i: usize = 0;
    while (i < text.len) : (i += 1) {
        const d = hexDigitVal(text[i]) orelse return null;
        acc = (acc << 4) | d;
    }
    return acc;
}

fn writeDecU8(buf: []u8, pos: usize, val: u8): usize {
    if (val >= 100) {
        buf[pos] = ('0' as u8) + (val / 100);
        buf[pos + 1] = ('0' as u8) + ((val / 10) % 10);
        buf[pos + 2] = ('0' as u8) + (val % 10);
        return 3;
    }
    if (val >= 10) {
        buf[pos] = ('0' as u8) + (val / 10);
        buf[pos + 1] = ('0' as u8) + (val % 10);
        return 2;
    }
    buf[pos] = ('0' as u8) + val;
    return 1;
}

fn writeHexU16(buf: []u8, pos: usize, val: u16): usize {
    if (val == 0) {
        buf[pos] = '0';
        return 1;
    }
    let tmp: [4]u8 = [0; 4];
    let len: usize = 0;
    let v = val;
    while (v > 0) {
        const nib = (v & 0xF) as u8;
        tmp[len] = if (nib < 10) (('0' as u8) + nib) else (('a' as u8) + (nib - 10));
        len += 1;
        v >>= 4;
    }
    let i: usize = 0;
    while (i < len) : (i += 1) {
        buf[pos + i] = tmp[len - 1 - i];
    }
    return len;
}

/// IPv4 address stored in host-accessible 32-bit network byte order (MSB = first octet).
pub struct Ipv4Addr {
    addr: u32,

    /// Constructs an `Ipv4Addr` from a 32-bit network-order integer.
    pub static fn fromU32(val: u32): Ipv4Addr {
        return Ipv4Addr{ addr: val };
    }

    /// Returns the underlying 32-bit network-order integer.
    pub fn toU32(): u32 {
        return _.addr;
    }

    /// Checks equality with another `Ipv4Addr`.
    pub fn eql(other: Ipv4Addr): bool {
        return _.addr == other.addr;
    }

    /// Parses a strict dotted-decimal IPv4 address (e.g. `"192.168.1.1"`).
    pub static fn parse(str: []const u8): Ipv4Addr ! CidrError {
        if (str.len < 7 || str.len > 15) return CidrError.InvalidIpAddress;
        let octets: [4]u8 = [0; 4];
        let oct_idx: usize = 0;
        let start: usize = 0;
        let i: usize = 0;
        while (i <= str.len) : (i += 1) {
            if (i == str.len || str[i] == '.') {
                if (oct_idx >= 4) return CidrError.InvalidIpAddress;
                const part = str[start..i];
                const oct = parseDecU8(part) orelse return CidrError.InvalidIpAddress;
                octets[oct_idx] = oct;
                oct_idx += 1;
                start = i + 1;
            }
        }
        if (oct_idx != 4) return CidrError.InvalidIpAddress;
        const val: u32 = ((octets[0] as u32) << 24)
            | ((octets[1] as u32) << 16)
            | ((octets[2] as u32) << 8)
            | (octets[3] as u32);
        return Ipv4Addr{ addr: val };
    }

    /// Formats the IPv4 address as dotted-decimal string. Caller owns the returned slice.
    pub fn format(allocator: std.mem.Allocator): []u8 ! CidrError {
        let tmp: [15]u8 = [0; 15];
        let pos: usize = 0;
        pos += writeDecU8(tmp[0..], pos, ((_.addr >> 24) & 0xFF) as u8);
        tmp[pos] = '.';
        pos += 1;
        pos += writeDecU8(tmp[0..], pos, ((_.addr >> 16) & 0xFF) as u8);
        tmp[pos] = '.';
        pos += 1;
        pos += writeDecU8(tmp[0..], pos, ((_.addr >> 8) & 0xFF) as u8);
        tmp[pos] = '.';
        pos += 1;
        pos += writeDecU8(tmp[0..], pos, (_.addr & 0xFF) as u8);

        const out = allocator.alloc(u8, pos) catch return CidrError.OutOfMemory;
        @memcpy(out, tmp[0..pos]);
        return out;
    }

    /// Returns true if the address falls in RFC 1918 private ranges (`10.0.0.0/8`, `172.16.0.0/12`, `192.168.0.0/16`).
    pub fn isPrivate(): bool {
        if ((_.addr & 0xFF000000) == 0x0A000000) return true;
        if ((_.addr & 0xFFF00000) == 0xAC100000) return true;
        if ((_.addr & 0xFFFF0000) == 0xC0A80000) return true;
        return false;
    }

    /// Returns true if the address falls in the IPv4 loopback range (`127.0.0.0/8`).
    pub fn isLoopback(): bool {
        return (_.addr & 0xFF000000) == 0x7F000000;
    }

    /// Returns true if the address falls in the IPv4 link-local range (`169.254.0.0/16`).
    pub fn isLinkLocal(): bool {
        return (_.addr & 0xFFFF0000) == 0xA9FE0000;
    }
}

/// IPv6 address stored as a 128-bit unsigned integer (MSB = first hextet).
pub struct Ipv6Addr {
    addr: u128,

    /// Constructs an `Ipv6Addr` from a 128-bit integer.
    pub static fn fromU128(val: u128): Ipv6Addr {
        return Ipv6Addr{ addr: val };
    }

    /// Returns the underlying 128-bit integer.
    pub fn toU128(): u128 {
        return _.addr;
    }

    /// Checks equality with another `Ipv6Addr`.
    pub fn eql(other: Ipv6Addr): bool {
        return _.addr == other.addr;
    }

    /// Parses an IPv6 address supporting `::` zero compression, leading/trailing `::`,
    /// uniqueness of `::`, and mixed-case hexadecimal hextets.
    pub static fn parse(str: []const u8): Ipv6Addr ! CidrError {
        if (str.len < 2 || str.len > 39) return CidrError.InvalidIpAddress;

        // Locate "::" and verify at most one occurrence
        let dcolon_pos: usize? = null;
        let i: usize = 0;
        while (i + 1 < str.len) : (i += 1) {
            if (str[i] == ':' && str[i + 1] == ':') {
                if (dcolon_pos != null) return CidrError.InvalidIpAddress;
                dcolon_pos = i;
                // Reject ":::"
                if (i + 2 < str.len && str[i + 2] == ':') return CidrError.InvalidIpAddress;
            }
        }

        let groups: [8]u16 = [0; 8];

        if (dc := dcolon_pos) {
            const left_str = str[0..dc];
            const right_str = str[dc + 2 .. str.len];

            let left_groups: [8]u16 = [0; 8];
            let left_count: usize = 0;
            if (left_str.len > 0) {
                if (left_str[0] == ':' || left_str[left_str.len - 1] == ':') {
                    return CidrError.InvalidIpAddress;
                }
                let start: usize = 0;
                let idx: usize = 0;
                while (idx <= left_str.len) : (idx += 1) {
                    if (idx == left_str.len || left_str[idx] == ':') {
                        if (left_count >= 8) return CidrError.InvalidIpAddress;
                        const val = parseHexGroup(left_str[start..idx]) orelse return CidrError.InvalidIpAddress;
                        left_groups[left_count] = val;
                        left_count += 1;
                        start = idx + 1;
                    }
                }
            }

            let right_groups: [8]u16 = [0; 8];
            let right_count: usize = 0;
            if (right_str.len > 0) {
                if (right_str[0] == ':' || right_str[right_str.len - 1] == ':') {
                    return CidrError.InvalidIpAddress;
                }
                let start: usize = 0;
                let idx: usize = 0;
                while (idx <= right_str.len) : (idx += 1) {
                    if (idx == right_str.len || right_str[idx] == ':') {
                        if (right_count >= 8) return CidrError.InvalidIpAddress;
                        const val = parseHexGroup(right_str[start..idx]) orelse return CidrError.InvalidIpAddress;
                        right_groups[right_count] = val;
                        right_count += 1;
                        start = idx + 1;
                    }
                }
            }

            // "::" must compress at least one zero group
            if (left_count + right_count >= 8) return CidrError.InvalidIpAddress;

            let k: usize = 0;
            while (k < left_count) : (k += 1) {
                groups[k] = left_groups[k];
            }
            const right_start: usize = (8 as usize) - right_count;
            let r: usize = 0;
            while (r < right_count) : (r += 1) {
                groups[right_start + r] = right_groups[r];
            }
        } else {
            if (str[0] == ':' || str[str.len - 1] == ':') {
                return CidrError.InvalidIpAddress;
            }
            let count: usize = 0;
            let start: usize = 0;
            let idx: usize = 0;
            while (idx <= str.len) : (idx += 1) {
                if (idx == str.len || str[idx] == ':') {
                    if (count >= 8) return CidrError.InvalidIpAddress;
                    const val = parseHexGroup(str[start..idx]) orelse return CidrError.InvalidIpAddress;
                    groups[count] = val;
                    count += 1;
                    start = idx + 1;
                }
            }
            if (count != 8) return CidrError.InvalidIpAddress;
        }

        let acc: u128 = 0;
        let g: usize = 0;
        while (g < 8) : (g += 1) {
            acc = (acc << 16) | (groups[g] as u128);
        }
        return Ipv6Addr{ addr: acc };
    }

    /// Formats the IPv6 address in RFC 5952 canonical lowercase compressed form.
    /// Caller owns the returned slice.
    pub fn formatCanonical(allocator: std.mem.Allocator): []u8 ! CidrError {
        let groups: [8]u16 = [0; 8];
        let g: usize = 0;
        while (g < 8) : (g += 1) {
            const shift: u7 = ((7 - g) * 16) as u7;
            groups[g] = ((_.addr >> shift) & 0xFFFF) as u16;
        }

        // Find longest run of >= 2 consecutive zero hextets (leftmost wins ties)
        let best_start: usize = 8;
        let best_len: usize = 0;
        let cur_start: usize = 0;
        let cur_len: usize = 0;
        let i: usize = 0;
        while (i < 8) : (i += 1) {
            if (groups[i] == 0) {
                if (cur_len == 0) cur_start = i;
                cur_len += 1;
                if (cur_len > best_len) {
                    best_len = cur_len;
                    best_start = cur_start;
                }
            } else {
                cur_len = 0;
            }
        }
        if (best_len < 2) {
            best_start = 8;
            best_len = 0;
        }

        let tmp: [39]u8 = [0; 39];
        let pos: usize = 0;
        let idx: usize = 0;
        while (idx < 8) {
            if (best_len > 0 && idx == best_start) {
                tmp[pos] = ':';
                tmp[pos + 1] = ':';
                pos += 2;
                idx += best_len;
                continue;
            }
            if (idx > 0 && !(best_len > 0 && idx == best_start + best_len)) {
                tmp[pos] = ':';
                pos += 1;
            }
            pos += writeHexU16(tmp[0..], pos, groups[idx]);
            idx += 1;
        }

        const out = allocator.alloc(u8, pos) catch return CidrError.OutOfMemory;
        @memcpy(out, tmp[0..pos]);
        return out;
    }

    /// Returns true if the address is the IPv6 loopback (`::1`).
    pub fn isLoopback(): bool {
        return _.addr == 1;
    }

    /// Returns true if the address is in the IPv6 Unique Local range (`fc00::/7`).
    pub fn isUniqueLocal(): bool {
        return (_.addr >> 121) == 0x7E;
    }
}

/// IPv4 CIDR block with normalized network address (`addr`) and `prefix_len` in `0..32`.
pub struct Cidr4 {
    addr: Ipv4Addr,
    prefix_len: u8,

    /// Parses an IPv4 CIDR string (e.g. `"192.168.1.42/24"`), automatically masking host bits.
    pub static fn parse(str: []const u8): Cidr4 ! CidrError {
        let slash_pos: usize? = null;
        let i: usize = 0;
        while (i < str.len) : (i += 1) {
            if (str[i] == '/') {
                if (slash_pos != null) return CidrError.InvalidCidrNotation;
                slash_pos = i;
            }
        }
        const sp = slash_pos orelse return CidrError.InvalidCidrNotation;
        if (sp == 0 || sp + 1 >= str.len) return CidrError.InvalidCidrNotation;

        const ip = Ipv4Addr.parse(str[0..sp])?;
        const plen = parsePrefixLen(str[sp + 1 .. str.len], 32)?;
        const m = mask4(plen);
        return Cidr4{
            addr: Ipv4Addr{ addr: ip.addr & m },
            prefix_len: plen,
        };
    }

    /// Strictly parses an IPv4 CIDR string, rejecting inputs where any host bit is non-zero.
    pub static fn parseStrict(str: []const u8): Cidr4 ! CidrError {
        let slash_pos: usize? = null;
        let i: usize = 0;
        while (i < str.len) : (i += 1) {
            if (str[i] == '/') {
                if (slash_pos != null) return CidrError.InvalidCidrNotation;
                slash_pos = i;
            }
        }
        const sp = slash_pos orelse return CidrError.InvalidCidrNotation;
        if (sp == 0 || sp + 1 >= str.len) return CidrError.InvalidCidrNotation;

        const ip = Ipv4Addr.parse(str[0..sp])?;
        const plen = parsePrefixLen(str[sp + 1 .. str.len], 32)?;
        const m = mask4(plen);
        if ((ip.addr & m) != ip.addr) return CidrError.InvalidCidrNotation;
        return Cidr4{
            addr: ip,
            prefix_len: plen,
        };
    }

    /// Checks equality of two `Cidr4` values.
    pub fn eql(other: Cidr4): bool {
        return _.prefix_len == other.prefix_len && _.addr.addr == other.addr.addr;
    }

    /// Returns true if `ip` belongs to this CIDR block.
    pub fn contains(ip: Ipv4Addr): bool {
        const m = mask4(_.prefix_len);
        return (ip.addr & m) == (_.addr.addr & m);
    }

    /// Returns true if this CIDR block overlaps with `other`.
    pub fn overlaps(other: Cidr4): bool {
        const min_len = if (_.prefix_len < other.prefix_len) _.prefix_len else other.prefix_len;
        const m = mask4(min_len);
        return (_.addr.addr & m) == (other.addr.addr & m);
    }

    /// Returns the network (first) address of this CIDR block.
    pub fn network(): Ipv4Addr {
        return Ipv4Addr{ addr: _.addr.addr & mask4(_.prefix_len) };
    }

    /// Returns the broadcast (last) address of this CIDR block.
    pub fn broadcast(): Ipv4Addr {
        const m = mask4(_.prefix_len);
        return Ipv4Addr{ addr: (_.addr.addr & m) | (~m) };
    }

    /// Returns the total number of IP addresses in this CIDR block (`2^(32 - prefix_len)`).
    pub fn hostCount(): u64 {
        const host_bits: u6 = (32 - _.prefix_len) as u6;
        return (1 as u64) << host_bits;
    }

    /// Splits this CIDR block into `2^(new_prefix_len - prefix_len)` equal subnets.
    /// Caller owns the returned slice.
    pub fn splitSubnets(allocator: std.mem.Allocator, new_prefix_len: u8): []Cidr4 ! CidrError {
        if (new_prefix_len < _.prefix_len || new_prefix_len > 32) {
            return CidrError.InvalidSubnetSplit;
        }
        const diff: u6 = (new_prefix_len - _.prefix_len) as u6;
        const count_u64: u64 = (1 as u64) << diff;
        if (count_u64 > (std.math.maxInt(usize) as u64)) {
            return CidrError.OutOfMemory;
        }
        const count: usize = count_u64 as usize;
        let out = allocator.alloc(Cidr4, count) catch return CidrError.OutOfMemory;
        const base = _.network().addr;
        const host_shift: u5 = (32 - new_prefix_len) as u5;
        let i: usize = 0;
        while (i < count) : (i += 1) {
            const offset: u32 = if (new_prefix_len == 0) 0 else ((i as u32) << host_shift);
            out[i] = Cidr4{
                addr: Ipv4Addr{ addr: base + offset },
                prefix_len: new_prefix_len,
            };
        }
        return out;
    }

    /// Summarizes a slice of IPv4 CIDRs by removing contained/duplicate subnets and
    /// recursively merging adjacent sibling prefixes into supernets.
    /// Caller owns the returned slice.
    pub static fn summarize(allocator: std.mem.Allocator, cidrs: []const Cidr4): []Cidr4 ! CidrError {
        if (cidrs.len == 0) {
            return allocator.alloc(Cidr4, 0) catch return CidrError.OutOfMemory;
        }

        let work = allocator.alloc(Cidr4, cidrs.len) catch return CidrError.OutOfMemory;
        let work_freed = false;
        defer if (!work_freed) allocator.free(work);

        let i: usize = 0;
        while (i < cidrs.len) : (i += 1) {
            const m = mask4(cidrs[i].prefix_len);
            work[i] = Cidr4{
                addr: Ipv4Addr{ addr: cidrs[i].addr.addr & m },
                prefix_len: cidrs[i].prefix_len,
            };
        }

        // Insertion / Shell-style or std.mem.sort: simple in-place sort by (addr ASC, prefix_len ASC)
        // Use std.sort.pdq with contextless comparator
        const Ctx = struct {
            fn lessThan(_: void, a: Cidr4, b: Cidr4): bool {
                if (a.addr.addr != b.addr.addr) return a.addr.addr < b.addr.addr;
                return a.prefix_len < b.prefix_len;
            }
        };
        std.sort.pdq(Cidr4, work, {}, Ctx.lessThan);

        // Stack-based deduplication, containment pruning, and sibling supernet merging
        let stack = allocator.alloc(Cidr4, cidrs.len) catch return CidrError.OutOfMemory;
        defer allocator.free(stack);
        let sp: usize = 0;

        i = 0;
        while (i < work.len) : (i += 1) {
            const cur = work[i];
            if (sp > 0 && stack[sp - 1].contains(cur.addr)) {
                // Contained by an already-kept broader or equal prefix
                continue;
            }
            stack[sp] = cur;
            sp += 1;

            while (sp >= 2) {
                const b = stack[sp - 1];
                const a = stack[sp - 2];
                if (a.prefix_len != b.prefix_len || a.prefix_len == 0) break;
                const parent_len = a.prefix_len - 1;
                const pm = mask4(parent_len);
                if ((a.addr.addr & pm) != (b.addr.addr & pm)) break;

                // Adjacent siblings under the same parent -> merge
                stack[sp - 2] = Cidr4{
                    addr: Ipv4Addr{ addr: a.addr.addr & pm },
                    prefix_len: parent_len,
                };
                sp -= 1;
            }
        }

        const out = allocator.alloc(Cidr4, sp) catch return CidrError.OutOfMemory;
        @memcpy(out, stack[0..sp]);
        return out;
    }
}

/// IPv6 CIDR block with normalized network address (`addr`) and `prefix_len` in `0..128`.
pub struct Cidr6 {
    addr: Ipv6Addr,
    prefix_len: u8,

    /// Parses an IPv6 CIDR string (e.g. `"2001:db8::1/32"`), normalizing host bits.
    pub static fn parse(str: []const u8): Cidr6 ! CidrError {
        let slash_pos: usize? = null;
        let i: usize = 0;
        while (i < str.len) : (i += 1) {
            if (str[i] == '/') {
                if (slash_pos != null) return CidrError.InvalidCidrNotation;
                slash_pos = i;
            }
        }
        const sp = slash_pos orelse return CidrError.InvalidCidrNotation;
        if (sp == 0 || sp + 1 >= str.len) return CidrError.InvalidCidrNotation;

        const ip = Ipv6Addr.parse(str[0..sp])?;
        const plen = parsePrefixLen(str[sp + 1 .. str.len], 128)?;
        const m = mask6(plen);
        return Cidr6{
            addr: Ipv6Addr{ addr: ip.addr & m },
            prefix_len: plen,
        };
    }

    /// Checks equality of two `Cidr6` values.
    pub fn eql(other: Cidr6): bool {
        return _.prefix_len == other.prefix_len && _.addr.addr == other.addr.addr;
    }

    /// Returns true if `ip` belongs to this IPv6 CIDR block.
    pub fn contains(ip: Ipv6Addr): bool {
        const m = mask6(_.prefix_len);
        return (ip.addr & m) == (_.addr.addr & m);
    }

    /// Returns true if this IPv6 CIDR block overlaps with `other`.
    pub fn overlaps(other: Cidr6): bool {
        const min_len = if (_.prefix_len < other.prefix_len) _.prefix_len else other.prefix_len;
        const m = mask6(min_len);
        return (_.addr.addr & m) == (other.addr.addr & m);
    }

    /// Returns the network (first) address of this IPv6 CIDR block.
    pub fn network(): Ipv6Addr {
        return Ipv6Addr{ addr: _.addr.addr & mask6(_.prefix_len) };
    }

    /// Returns the last address of this IPv6 CIDR block.
    pub fn lastAddr(): Ipv6Addr {
        const m = mask6(_.prefix_len);
        return Ipv6Addr{ addr: (_.addr.addr & m) | (~m) };
    }
}

const NULL_NODE: usize = std.math.maxInt(usize);

fn LpmNode(comptime V: type): type {
    return struct {
        children: [2]usize,
        has_value: bool,
        value: V,
    };
}

/// IPv4 Longest Prefix Match (LPM) routing table backed by a binary radix trie.
/// Supports `/0` default routes through `/32` host routes with O(32) lookup.
pub struct LpmTable4<V> {
    allocator: std.mem.Allocator,
    nodes: std.ArrayListUnmanaged(LpmNode(V)),

    /// Initializes an empty `LpmTable4<V>`.
    pub static fn init(allocator: std.mem.Allocator): LpmTable4<V> {
        return LpmTable4<V>{
            allocator: allocator,
            nodes: .empty,
        };
    }

    /// Frees all internal trie nodes.
    pub fn deinit(*_) {
        _.nodes.deinit(_.allocator);
    }

    fn allocNode(*_): usize ! CidrError {
        const idx = _.nodes.items.len;
        _.nodes.append(_.allocator, LpmNode(V){
            children: [NULL_NODE, NULL_NODE],
            has_value: false,
            value: undefined,
        }) catch return CidrError.OutOfMemory;
        return idx;
    }

    /// Inserts or updates a route mapping `cidr` to `value`.
    pub fn insert(*_, cidr: Cidr4, value: V): void ! CidrError {
        if (_.nodes.items.len == 0) {
            * = _.allocNode()?;
        }
        const net = cidr.network().addr;
        let cur: usize = 0;
        let bit_idx: u8 = 0;
        while (bit_idx < cidr.prefix_len) : (bit_idx += 1) {
            const shift: u5 = (31 - bit_idx) as u5;
            const bit: usize = ((net >> shift) & 1) as usize;
            let next = _.nodes.items[cur].children[bit];
            if (next == NULL_NODE) {
                next = _.allocNode()?;
                _.nodes.items[cur].children[bit] = next;
            }
            cur = next;
        }
        _.nodes.items[cur].has_value = true;
        _.nodes.items[cur].value = value;
    }

    /// Looks up the longest matching prefix for `ip`, returning its associated value or `null`.
    pub fn lookup(ip: Ipv4Addr): V? {
        if (_.nodes.items.len == 0) return null;
        let best: V? = null;
        let cur: usize = 0;
        if (_.nodes.items[0].has_value) {
            best = _.nodes.items[0].value;
        }
        let bit_idx: u8 = 0;
        while (bit_idx < 32) : (bit_idx += 1) {
            const shift: u5 = (31 - bit_idx) as u5;
            const bit: usize = ((ip.addr >> shift) & 1) as usize;
            const next = _.nodes.items[cur].children[bit];
            if (next == NULL_NODE) break;
            cur = next;
            if (_.nodes.items[cur].has_value) {
                best = _.nodes.items[cur].value;
            }
        }
        return best;
    }
}
