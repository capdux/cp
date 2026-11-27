// Source: cp/lib/dns/records.cp (RFC 1035/2782/8659 SOA, SRV, CAA, PTR & Safe Name Decoder)
// lib/dns/records.cp
// Production DNS Resource Record Codecs (SOA, SRV, CAA, PTR) & Loop-Safe Name Decompressor

const std = @import("std");
import { NSType } from "./_cares.cp";
import { DecodedDomain, DnsResourceRecord } from "./packet.cp";

pub err DnsRecordError {
    PointerLoop,
    InvalidPointer,
    InvalidLabel,
    OutOfBounds,
    BufferTooSmall,
    InvalidRecordType,
    InvalidRdata,
    InvalidIpAddress,
}

/// Safe RFC 1035 / RFC 3596 domain name wire codec and reverse PTR builder.
/// Enforces a hard compression pointer hop limit (MAX_JUMPS = 16) to defeat pointer loops.
pub struct DnsNameCodec {
    pub const MAX_JUMPS: usize = 16;
    pub const MAX_LABEL_LEN: usize = 63;
    pub const MAX_NAME_LEN: usize = 253;

    /// Decodes a wire-format DNS domain name starting at `start_offset`.
    /// Rejects compression pointer chains exceeding `MAX_JUMPS` (16 hops), out-of-bounds
    /// pointers, reserved label types (0x40/0x80), and oversized labels/names.
    pub static fn decode(packet: []const u8, start_offset: usize, out_name: []u8): DecodedDomain ! DnsRecordError {
        if (start_offset >= packet.len) {
            return DnsRecordError.OutOfBounds;
        }
        let cur = start_offset;
        let out_pos: usize = 0;
        let consumed: usize = 0;
        let jumped = false;
        let hops: usize = 0;

        while (cur < packet.len) {
            const len_byte = packet[cur];
            if (len_byte == 0) {
                if (!jumped) {
                    consumed = (cur - start_offset) + 1;
                }
                return DecodedDomain{
                    name_len: out_pos,
                    packet_bytes_consumed: consumed,
                };
            }

            const tag_bits = len_byte & 0xC0;
            if (tag_bits == 0xC0) {
                if (cur + 1 >= packet.len) {
                    return DnsRecordError.OutOfBounds;
                }
                if (hops >= MAX_JUMPS) {
                    return DnsRecordError.PointerLoop;
                }
                if (!jumped) {
                    consumed = (cur - start_offset) + 2;
                    jumped = true;
                }
                const target_offset: usize = (((len_byte & 0x3F) as usize) << 8) | (packet[cur + 1] as usize);
                if (target_offset >= packet.len) {
                    return DnsRecordError.InvalidPointer;
                }
                if (target_offset == cur) {
                    return DnsRecordError.PointerLoop;
                }
                cur = target_offset;
                hops += 1;
            } else if (tag_bits == 0x00) {
                const label_len = len_byte as usize;
                if (label_len > MAX_LABEL_LEN) {
                    return DnsRecordError.InvalidLabel;
                }
                cur += 1;
                if (cur + label_len > packet.len) {
                    return DnsRecordError.OutOfBounds;
                }
                let sep_len: usize = 0;
                if (out_pos > 0) {
                    sep_len = 1;
                }
                if (out_pos + sep_len + label_len > MAX_NAME_LEN) {
                    return DnsRecordError.InvalidLabel;
                }
                if (out_pos + sep_len + label_len > out_name.len) {
                    return DnsRecordError.BufferTooSmall;
                }
                if (out_pos > 0) {
                    out_name[out_pos] = '.';
                    out_pos += 1;
                }
                let k: usize = 0;
                while (k < label_len) {
                    out_name[out_pos + k] = packet[cur + k];
                    k += 1;
                }
                out_pos += label_len;
                cur += label_len;
            } else {
                // Reserved / extended label types (0x40, 0x80)
                return DnsRecordError.InvalidLabel;
            }
        }
        return DnsRecordError.OutOfBounds;
    }

    pub static fn decodeOrNull(packet: []const u8, start_offset: usize, out_name: []u8): DecodedDomain? {
        return DnsNameCodec.decode(packet, start_offset, out_name) catch null;
    }

    /// Encodes a domain name (including root `"."` and optional trailing dot) into DNS wire format.
    pub static fn encode(domain: []const u8, out: []u8): usize {
        if (out.len == 0) return 0;
        if (domain.len == 0 || (domain.len == 1 && domain[0] == '.')) {
            out[0] = 0;
            return 1;
        }
        let clean = domain;
        if (clean[clean.len - 1] == '.') {
            clean = clean[0 .. clean.len - 1];
        }
        if (clean.len == 0) {
            out[0] = 0;
            return 1;
        }
        if (clean.len > MAX_NAME_LEN) {
            return 0;
        }
        let pos: usize = 0;
        let label_start: usize = 0;
        let i: usize = 0;
        while (i <= clean.len) {
            if (i == clean.len || clean[i] == '.') {
                const label_len = i - label_start;
                if (label_len == 0 || label_len > MAX_LABEL_LEN || pos + 1 + label_len + 1 > out.len) {
                    return 0;
                }
                out[pos] = label_len as u8;
                pos += 1;
                let k: usize = 0;
                while (k < label_len) {
                    out[pos + k] = clean[label_start + k];
                    k += 1;
                }
                pos += label_len;
                label_start = i + 1;
            }
            i += 1;
        }
        out[pos] = 0;
        return pos + 1;
    }

    /// Parses a dotted-quad IPv4 string into 4 raw octets.
    pub static fn parseIPv4(ip_str: []const u8): ([4]u8) ! DnsRecordError {
        if (ip_str.len < 7 || ip_str.len > 15) return DnsRecordError.InvalidIpAddress;
        let octets: [4]u8 = [0; 4];
        let oct_idx: usize = 0;
        let acc: u16 = 0;
        let digits: usize = 0;
        for (c of ip_str) {
            if (c == '.') {
                if (digits == 0 || oct_idx >= 3) return DnsRecordError.InvalidIpAddress;
                octets[oct_idx] = acc as u8;
                oct_idx += 1;
                acc = 0;
                digits = 0;
            } else if (c >= '0' && c <= '9') {
                if (digits == 1 && acc == 0) return DnsRecordError.InvalidIpAddress;
                acc = acc * 10 + ((c - '0') as u16);
                if (acc > 255) return DnsRecordError.InvalidIpAddress;
                digits += 1;
            } else {
                return DnsRecordError.InvalidIpAddress;
            }
        }
        if (oct_idx != 3 || digits == 0) return DnsRecordError.InvalidIpAddress;
        octets[3] = acc as u8;
        return octets;
    }

    /// Parses an RFC 4291 IPv6 text address (including `::` compression) into 16 raw bytes.
    pub static fn parseIPv6(ip_str: []const u8): ([16]u8) ! DnsRecordError {
        if (ip_str.len < 2 || ip_str.len > 64) return DnsRecordError.InvalidIpAddress;
        // Strip optional zone id (%eth0)
        let s = ip_str;
        if (pct := std.mem.indexOfScalar(u8, s, '%')) {
            s = s[0..pct];
        }
        if (s.len < 2) return DnsRecordError.InvalidIpAddress;

        let groups: [8]u16 = [0; 8];
        let group_count: usize = 0;
        let compress_idx: usize? = null;
        let i: usize = 0;

        if (s[0] == ':') {
            if (s.len < 2 || s[1] != ':') return DnsRecordError.InvalidIpAddress;
            compress_idx = 0;
            i = 2;
        }

        while (i < s.len) {
            if (group_count >= 8) return DnsRecordError.InvalidIpAddress;
            if (s[i] == ':') {
                if (compress_idx != null) return DnsRecordError.InvalidIpAddress;
                compress_idx = group_count;
                i += 1;
                if (i == s.len) break;
                continue;
            }
            // Check if tail is embedded IPv4 (e.g., ::ffff:192.0.2.1)
            const rest = s[i..];
            if (std.mem.indexOfScalar(u8, rest, ':') == null && std.mem.indexOfScalar(u8, rest, '.') != null) {
                if (group_count > 6) return DnsRecordError.InvalidIpAddress;
                const v4 = DnsNameCodec.parseIPv4(rest)?;
                groups[group_count] = ((v4[0] as u16) << 8) | (v4[1] as u16);
                groups[group_count + 1] = ((v4[2] as u16) << 8) | (v4[3] as u16);
                group_count += 2;
                i = s.len;
                break;
            }
            let val: u16 = 0;
            let hex_digits: usize = 0;
            while (i < s.len && s[i] != ':') {
                const c = s[i];
                let d: u16 = 0;
                if (c >= '0' && c <= '9') {
                    d = (c - '0') as u16;
                } else if (c >= 'a' && c <= 'f') {
                    d = (c - 'a' + 10) as u16;
                } else if (c >= 'A' && c <= 'F') {
                    d = (c - 'A' + 10) as u16;
                } else {
                    return DnsRecordError.InvalidIpAddress;
                }
                if (hex_digits >= 4) return DnsRecordError.InvalidIpAddress;
                val = (val << 4) | d;
                hex_digits += 1;
                i += 1;
            }
            if (hex_digits == 0) return DnsRecordError.InvalidIpAddress;
            groups[group_count] = val;
            group_count += 1;
            if (i < s.len && s[i] == ':') {
                i += 1;
                if (i == s.len) return DnsRecordError.InvalidIpAddress;
            }
        }

        let out_groups: [8]u16 = [0; 8];
        if (c_idx := compress_idx) {
            if (group_count >= 8) return DnsRecordError.InvalidIpAddress;
            const zeros = 8 - group_count;
            let k: usize = 0;
            while (k < c_idx) {
                out_groups[k] = groups[k];
                k += 1;
            }
            while (k < group_count) {
                out_groups[k + zeros] = groups[k];
                k += 1;
            }
        } else {
            if (group_count != 8) return DnsRecordError.InvalidIpAddress;
            out_groups = groups;
        }

        let addr: [16]u8 = [0; 16];
        let g: usize = 0;
        while (g < 8) {
            addr[g * 2] = (out_groups[g] >> 8) as u8;
            addr[g * 2 + 1] = (out_groups[g] & 0xFF) as u8;
            g += 1;
        }
        return addr;
    }

    /// Formats an IPv4 address (`[4]u8`) into its RFC 1035 reverse PTR name (`d.c.b.a.in-addr.arpa`).
    pub static fn ipv4ToArpa(addr: [4]u8, out: []u8): ([]const u8) ! DnsRecordError {
        const rendered = std.fmt.bufPrint(
            out,
            "{d}.{d}.{d}.{d}.in-addr.arpa",
            .{ addr[3], addr[2], addr[1], addr[0] },
        ) catch return DnsRecordError.BufferTooSmall;
        return rendered;
    }

    pub static fn ipv4StrToArpa(ip_str: []const u8, out: []u8): ([]const u8) ! DnsRecordError {
        const addr = DnsNameCodec.parseIPv4(ip_str)?;
        return DnsNameCodec.ipv4ToArpa(addr, out);
    }

    pub static fn ipv4ToArpaAlloc(alloc: std.mem.Allocator, addr: [4]u8): ([]u8)! {
        let stack_buf: [32]u8 = undefined;
        const slice = DnsNameCodec.ipv4ToArpa(addr, &stack_buf)?;
        return alloc.dupe(u8, slice);
    }

    /// Formats an IPv6 address (`[16]u8`) into its RFC 3596 reverse PTR name (`<32 reversed nibbles>.ip6.arpa`).
    pub static fn ipv6ToArpa(addr: [16]u8, out: []u8): ([]const u8) ! DnsRecordError {
        const suffix: []const u8 = "ip6.arpa";
        const needed: usize = (64 as usize) + suffix.len; // 32 nibbles * 2 ("x.") + 8 = 72
        if (out.len < needed) return DnsRecordError.BufferTooSmall;
        const hex_l: []const u8 = "0123456789abcdef";
        let pos: usize = 0;
        let idx: usize = 16;
        while (idx > 0) {
            idx -= 1;
            const b = addr[idx];
            out[pos] = hex_l[(b & 0x0F) as usize];
            out[pos + 1] = '.';
            out[pos + 2] = hex_l[((b >> 4) & 0x0F) as usize];
            out[pos + 3] = '.';
            pos += 4;
        }
        @memcpy(out[pos .. pos + suffix.len], suffix);
        return out[0 .. pos + suffix.len];
    }

    pub static fn ipv6StrToArpa(ip_str: []const u8, out: []u8): ([]const u8) ! DnsRecordError {
        const addr = DnsNameCodec.parseIPv6(ip_str)?;
        return DnsNameCodec.ipv6ToArpa(addr, out);
    }

    pub static fn ipv6ToArpaAlloc(alloc: std.mem.Allocator, addr: [16]u8): ([]u8)! {
        let stack_buf: [80]u8 = undefined;
        const slice = DnsNameCodec.ipv6ToArpa(addr, &stack_buf)?;
        return alloc.dupe(u8, slice);
    }
}

/// RFC 2782 SRV Resource Record (Priority, Weight, Port, Target)
pub struct SrvRecord {
    priority: u16 = 0;
    weight: u16 = 0;
    port: u16 = 0;
    target_buf: [256]u8 = undefined;
    target_len: usize = 0;

    pub static fn init(priority: u16, weight: u16, port: u16, target_name: []const u8): SrvRecord {
        let rec = SrvRecord{
            priority: priority,
            weight: weight,
            port: port,
        };
        const copy_len = if (target_name.len <= 255) target_name.len else 255;
        if (copy_len > 0) {
            @memcpy(rec.target_buf[0..copy_len], target_name[0..copy_len]);
        }
        rec.target_len = copy_len;
        return rec;
    }

    pub fn target(*_): []const u8 {
        return _.target_buf[0.._.target_len];
    }

    pub static fn parseWire(packet: []const u8, rdata_offset: usize, rdlength: u16): SrvRecord ! DnsRecordError {
        const rdlen = rdlength as usize;
        if (rdlen < 7 || rdata_offset + rdlen > packet.len) {
            return DnsRecordError.InvalidRdata;
        }
        const prio: u16 = ((packet[rdata_offset] as u16) << 8) | (packet[rdata_offset + 1] as u16);
        const w: u16 = ((packet[rdata_offset + 2] as u16) << 8) | (packet[rdata_offset + 3] as u16);
        const port: u16 = ((packet[rdata_offset + 4] as u16) << 8) | (packet[rdata_offset + 5] as u16);
        let rec = SrvRecord{
            priority: prio,
            weight: w,
            port: port,
        };
        const dom = DnsNameCodec.decode(packet, rdata_offset + 6, rec.target_buf[0..256])?;
        if (6 + dom.packet_bytes_consumed != rdlen) {
            return DnsRecordError.InvalidRdata;
        }
        rec.target_len = dom.name_len;
        return rec;
    }

    pub static fn parseRdata(packet: []const u8, rr: DnsResourceRecord): SrvRecord ! DnsRecordError {
        if (rr.rtype != NSType.SRV) {
            return DnsRecordError.InvalidRecordType;
        }
        return SrvRecord.parseWire(packet, rr.rdata_offset, rr.rdlength);
    }

    pub fn encodeRdata(out: []u8): usize ! DnsRecordError {
        if (out.len < 7) return DnsRecordError.BufferTooSmall;
        out[0] = (_.priority >> 8) as u8;
        out[1] = (_.priority & 0xFF) as u8;
        out[2] = (_.weight >> 8) as u8;
        out[3] = (_.weight & 0xFF) as u8;
        out[4] = (_.port >> 8) as u8;
        out[5] = (_.port & 0xFF) as u8;
        const name_len = DnsNameCodec.encode(_.target_buf[0.._.target_len], out[6..out.len]);
        if (name_len == 0) return DnsRecordError.BufferTooSmall;
        return 6 + name_len;
    }
}

/// RFC 2782 SRV Selector & Sorter
pub struct SrvSelector {
    /// Sorts `records` in-place by priority ascending (lowest value = highest preference),
    /// and within equal priority by weight descending (weight 0 placed last per RFC 2782).
    pub static fn sortByPriorityAndWeight(records: []SrvRecord) {
        if (records.len <= 1) return;
        let i: usize = 1;
        while (i < records.len) {
            const key = records[i];
            let j = i;
            while (j > 0) {
                const prev = records[j - 1];
                let should_swap = false;
                if (key.priority < prev.priority) {
                    should_swap = true;
                } else if (key.priority == prev.priority && key.weight > prev.weight) {
                    should_swap = true;
                }
                if (!should_swap) break;
                records[j] = prev;
                j -= 1;
            }
            records[j] = key;
            i += 1;
        }
    }

    /// Performs RFC 2782 weighted randomized selection within each equal-priority tier
    /// using a deterministic SplitMix64 PRNG seeded by `seed`.
    pub static fn shuffleWeighted(records: []SrvRecord, seed: u64) {
        if (records.len <= 1) return;
        SrvSelector.sortByPriorityAndWeight(records);

        const gamma: u64 = ((0x9E3779B9 as u64) << 32) | (0x7F4A7C15 as u64);
        const mul1: u64 = ((0xBF58476D as u64) << 32) | (0x1CE4E5B9 as u64);
        const mul2: u64 = ((0x94D049BB as u64) << 32) | (0x133111EB as u64);
        let rng: u64 = if (seed == 0) gamma else seed;
        let group_start: usize = 0;
        while (group_start < records.len) {
            const prio = records[group_start].priority;
            let group_end = group_start + 1;
            while (group_end < records.len && records[group_end].priority == prio) {
                group_end += 1;
            }

            const group_len = group_end - group_start;
            if (group_len > 1) {
                let k: usize = group_start;
                while (k + 1 < group_end) {
                    let sum: u64 = 0;
                    let m: usize = k;
                    while (m < group_end) {
                        sum += records[m].weight as u64;
                        m += 1;
                    }
                    if (sum == 0) break;

                    // SplitMix64 step
                    rng = rng +% gamma;
                    let z: u64 = rng;
                    z = (z ^ (z >> 30)) *% mul1;
                    z = (z ^ (z >> 27)) *% mul2;
                    z = z ^ (z >> 31);

                    // Target in [1, sum] so weight-0 records are never picked while weight > 0 exists
                    const pick = (z % sum) + 1;
                    let running: u64 = 0;
                    let chosen = k;
                    m = k;
                    while (m < group_end) {
                        running += records[m].weight as u64;
                        if (running >= pick) {
                            chosen = m;
                            break;
                        }
                        m += 1;
                    }
                    if (chosen != k) {
                        const tmp = records[k];
                        records[k] = records[chosen];
                        records[chosen] = tmp;
                    }
                    k += 1;
                }
            }
            group_start = group_end;
        }
    }

    pub static fn selectBest(records: []const SrvRecord): usize? {
        if (records.len == 0) return null;
        let best_idx: usize = 0;
        let i: usize = 1;
        while (i < records.len) {
            if (records[i].priority < records[best_idx].priority ||
                (records[i].priority == records[best_idx].priority && records[i].weight > records[best_idx].weight))
            {
                best_idx = i;
            }
            i += 1;
        }
        return best_idx;
    }
}

/// RFC 1035 Section 3.3.13 SOA (Start of Authority) Resource Record
pub struct SoaRecord {
    mname_buf: [256]u8 = undefined;
    mname_len: usize = 0;
    rname_buf: [256]u8 = undefined;
    rname_len: usize = 0;
    serial: u32 = 0;
    refresh: u32 = 0;
    retry: u32 = 0;
    expire: u32 = 0;
    minimum: u32 = 0;

    pub static fn init(
        mname_str: []const u8,
        rname_str: []const u8,
        serial: u32,
        refresh: u32,
        retry: u32,
        expire: u32,
        minimum: u32,
    ): SoaRecord {
        let rec = SoaRecord{
            serial: serial,
            refresh: refresh,
            retry: retry,
            expire: expire,
            minimum: minimum,
        };
        const mlen = if (mname_str.len <= 255) mname_str.len else 255;
        if (mlen > 0) @memcpy(rec.mname_buf[0..mlen], mname_str[0..mlen]);
        rec.mname_len = mlen;

        const rlen = if (rname_str.len <= 255) rname_str.len else 255;
        if (rlen > 0) @memcpy(rec.rname_buf[0..rlen], rname_str[0..rlen]);
        rec.rname_len = rlen;
        return rec;
    }

    pub fn mname(*_): []const u8 {
        return _.mname_buf[0.._.mname_len];
    }

    pub fn rname(*_): []const u8 {
        return _.rname_buf[0.._.rname_len];
    }

    static fn readU32BE(buf: []const u8, off: usize): u32 {
        return ((buf[off] as u32) << 24)
            | ((buf[off + 1] as u32) << 16)
            | ((buf[off + 2] as u32) << 8)
            | (buf[off + 3] as u32);
    }

    static fn writeU32BE(buf: []u8, off: usize, val: u32) {
        buf[off] = ((val >> 24) & 0xFF) as u8;
        buf[off + 1] = ((val >> 16) & 0xFF) as u8;
        buf[off + 2] = ((val >> 8) & 0xFF) as u8;
        buf[off + 3] = (val & 0xFF) as u8;
    }

    pub static fn parseWire(packet: []const u8, rdata_offset: usize, rdlength: u16): SoaRecord ! DnsRecordError {
        const rdlen = rdlength as usize;
        const rdata_end = rdata_offset + rdlen;
        if (rdlen < 22 || rdata_end > packet.len) {
            return DnsRecordError.InvalidRdata;
        }
        let rec = SoaRecord{};
        const m_dom = DnsNameCodec.decode(packet, rdata_offset, rec.mname_buf[0..256])?;
        rec.mname_len = m_dom.name_len;

        const rname_off = rdata_offset + m_dom.packet_bytes_consumed;
        if (rname_off >= rdata_end) return DnsRecordError.InvalidRdata;

        const r_dom = DnsNameCodec.decode(packet, rname_off, rec.rname_buf[0..256])?;
        rec.rname_len = r_dom.name_len;

        const nums_off = rname_off + r_dom.packet_bytes_consumed;
        if (nums_off + 20 != rdata_end) {
            return DnsRecordError.InvalidRdata;
        }

        rec.serial = SoaRecord.readU32BE(packet, nums_off);
        rec.refresh = SoaRecord.readU32BE(packet, nums_off + 4);
        rec.retry = SoaRecord.readU32BE(packet, nums_off + 8);
        rec.expire = SoaRecord.readU32BE(packet, nums_off + 12);
        rec.minimum = SoaRecord.readU32BE(packet, nums_off + 16);
        return rec;
    }

    pub static fn parseRdata(packet: []const u8, rr: DnsResourceRecord): SoaRecord ! DnsRecordError {
        if (rr.rtype != NSType.SOA) {
            return DnsRecordError.InvalidRecordType;
        }
        return SoaRecord.parseWire(packet, rr.rdata_offset, rr.rdlength);
    }

    pub fn encodeRdata(out: []u8): usize ! DnsRecordError {
        const m_len = DnsNameCodec.encode(_.mname_buf[0.._.mname_len], out);
        if (m_len == 0) return DnsRecordError.BufferTooSmall;
        const r_len = DnsNameCodec.encode(_.rname_buf[0.._.rname_len], out[m_len..out.len]);
        if (r_len == 0) return DnsRecordError.BufferTooSmall;
        const pos = m_len + r_len;
        if (pos + 20 > out.len) return DnsRecordError.BufferTooSmall;
        SoaRecord.writeU32BE(out, pos, _.serial);
        SoaRecord.writeU32BE(out, pos + 4, _.refresh);
        SoaRecord.writeU32BE(out, pos + 8, _.retry);
        SoaRecord.writeU32BE(out, pos + 12, _.expire);
        SoaRecord.writeU32BE(out, pos + 16, _.minimum);
        return pos + 20;
    }
}

/// RFC 8659 CAA (Certification Authority Authorization) Resource Record
pub struct CaaRecord {
    flags: u8 = 0;
    tag_buf: [16]u8 = [0; 16];
    tag_len: usize = 0;
    value_buf: [512]u8 = undefined;
    value_len: usize = 0;

    pub static fn init(flags: u8, tag_str: []const u8, val_str: []const u8): CaaRecord {
        let rec = CaaRecord{ flags: flags };
        const tlen = if (tag_str.len <= 15) tag_str.len else 15;
        if (tlen > 0) @memcpy(rec.tag_buf[0..tlen], tag_str[0..tlen]);
        rec.tag_len = tlen;
        const vlen = if (val_str.len <= 512) val_str.len else 512;
        if (vlen > 0) @memcpy(rec.value_buf[0..vlen], val_str[0..vlen]);
        rec.value_len = vlen;
        return rec;
    }

    /// Returns true if the Issuer Critical flag (bit 0 / 0x80) is set.
    pub fn isCritical(): bool {
        return (_.flags & 0x80) != 0;
    }

    pub fn tag(*_): []const u8 {
        return _.tag_buf[0.._.tag_len];
    }

    pub fn value(*_): []const u8 {
        return _.value_buf[0.._.value_len];
    }

    static fn isValidTagChar(c: u8): bool {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
    }

    pub static fn parseWire(packet: []const u8, rdata_offset: usize, rdlength: u16): CaaRecord ! DnsRecordError {
        const rdlen = rdlength as usize;
        if (rdlen < 3 || rdata_offset + rdlen > packet.len) {
            return DnsRecordError.InvalidRdata;
        }
        const flags = packet[rdata_offset];
        const tag_len = packet[rdata_offset + 1] as usize;
        if (tag_len == 0 || tag_len > 15 || 2 + tag_len > rdlen) {
            return DnsRecordError.InvalidRdata;
        }
        let rec = CaaRecord{
            flags: flags,
            tag_len: tag_len,
        };
        let i: usize = 0;
        while (i < tag_len) {
            const c = packet[rdata_offset + 2 + i];
            if (!CaaRecord.isValidTagChar(c)) {
                return DnsRecordError.InvalidRdata;
            }
            rec.tag_buf[i] = c;
            i += 1;
        }
        const val_len = rdlen - 2 - tag_len;
        if (val_len > 512) {
            return DnsRecordError.BufferTooSmall;
        }
        if (val_len > 0) {
            @memcpy(rec.value_buf[0..val_len], packet[rdata_offset + 2 + tag_len .. rdata_offset + rdlen]);
        }
        rec.value_len = val_len;
        return rec;
    }

    pub static fn parseRdata(packet: []const u8, rr: DnsResourceRecord): CaaRecord ! DnsRecordError {
        if (rr.rtype != NSType.CAA) {
            return DnsRecordError.InvalidRecordType;
        }
        return CaaRecord.parseWire(packet, rr.rdata_offset, rr.rdlength);
    }

    pub fn encodeRdata(out: []u8): usize ! DnsRecordError {
        if (_.tag_len == 0 || _.tag_len > 15) return DnsRecordError.InvalidRdata;
        const needed = 2 + _.tag_len + _.value_len;
        if (out.len < needed) return DnsRecordError.BufferTooSmall;
        out[0] = _.flags;
        out[1] = _.tag_len as u8;
        @memcpy(out[2 .. 2 + _.tag_len], _.tag_buf[0.._.tag_len]);
        if (_.value_len > 0) {
            @memcpy(out[2 + _.tag_len .. needed], _.value_buf[0.._.value_len]);
        }
        return needed;
    }
}
