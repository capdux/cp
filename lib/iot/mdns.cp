// Origin: Pure-CP Stdlib (Zero-Alloc RFC 6762 mDNS & RFC 6763 DNS-SD Wire Codec) | Created: 2026-11-22

pub const MDNS_UDP_PORT: u16 = 5353;
pub const MDNS_MULTICAST_V4: []const u8 = "224.0.0.251";
pub const MDNS_MULTICAST_V6: []const u8 = "ff02::fb";
pub const MDNS_HEADER_SIZE: usize = 12;
pub const MDNS_QU_BIT: u16 = 0x8000;
pub const MDNS_CACHE_FLUSH_BIT: u16 = 0x8000;
pub const CLASS_IN: u16 = 1;

// DNS / mDNS Resource Record Types
pub const TYPE_A: u16 = 1;
pub const TYPE_PTR: u16 = 12;
pub const TYPE_TXT: u16 = 16;
pub const TYPE_AAAA: u16 = 28;
pub const TYPE_SRV: u16 = 33;
pub const TYPE_NSEC: u16 = 47;
pub const TYPE_ANY: u16 = 255;

fn readU16Be(buf: []const u8, off: usize): u16 {
    const b0 = buf[off] as u16;
    const b1 = buf[off + 1] as u16;
    return (b0 << 8) | b1;
}

fn readU32Be(buf: []const u8, off: usize): u32 {
    const b0 = buf[off] as u32;
    const b1 = buf[off + 1] as u32;
    const b2 = buf[off + 2] as u32;
    const b3 = buf[off + 3] as u32;
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

fn writeU16Be(buf: []u8, off: usize, v: u16) {
    buf[off] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

/// Skips a wire-encoded DNS domain name (supporting `0xC0` compression pointers) and returns the next offset.
fn skipDnsName(raw: []const u8, start: usize): usize? {
    let pos = start;
    let labels: usize = 0;
    while (labels < 128) {
        if (pos >= raw.len) {
            return null;
        }
        const b = raw[pos];
        if (b == 0) {
            return pos + 1;
        }
        const tag = b & 0xC0;
        if (tag == 0xC0) {
            if (pos + 2 > raw.len) {
                return null;
            }
            return pos + 2;
        }
        if (tag != 0) {
            return null;
        }
        const label_len = b as usize;
        pos += 1;
        if (pos + label_len > raw.len) {
            return null;
        }
        pos += label_len;
        labels += 1;
    }
    return null;
}

/// Decodes a wire-encoded DNS name into `out` (dot-separated, no trailing dot),
/// supporting RFC 1035 `0xC0` compression pointers with a max 16-jump loop guard.
fn decodeDnsName(raw: []const u8, offset: usize, out: []u8): usize? {
    if (offset >= raw.len) {
        return null;
    }
    let pos = offset;
    let out_pos: usize = 0;
    let jumps: usize = 0;
    let labels: usize = 0;

    while (labels < 128) {
        if (pos >= raw.len) {
            return null;
        }
        const b = raw[pos];
        if (b == 0) {
            return out_pos;
        }
        const tag = b & 0xC0;
        if (tag == 0xC0) {
            if (pos + 1 >= raw.len) {
                return null;
            }
            if (jumps >= 16) {
                return null;
            }
            const ptr = (((b & 0x3F) as usize) << 8) | (raw[pos + 1] as usize);
            if (ptr >= raw.len || ptr == pos) {
                return null;
            }
            pos = ptr;
            jumps += 1;
            continue;
        }
        if (tag != 0) {
            return null;
        }
        const label_len = b as usize;
        pos += 1;
        if (pos + label_len > raw.len) {
            return null;
        }
        if (out_pos > 0) {
            if (out_pos >= out.len) {
                return null;
            }
            out[out_pos] = '.';
            out_pos += 1;
        }
        if (out_pos + label_len > out.len) {
            return null;
        }
        let i: usize = 0;
        while (i < label_len) {
            out[out_pos + i] = raw[pos + i];
            i += 1;
        }
        out_pos += label_len;
        pos += label_len;
        labels += 1;
    }
    return null;
}

/// Parsed 12-byte mDNS header.
pub struct MdnsHeader {
    id: u16,
    flags: u16,
    qd_count: u16,
    an_count: u16,
    ns_count: u16,
    ar_count: u16,

    fn isResponse(): bool {
        return (_.flags & 0x8000) != 0;
    }

    fn isAuthoritative(): bool {
        return (_.flags & 0x0400) != 0;
    }

    fn isTruncated(): bool {
        return (_.flags & 0x0200) != 0;
    }
}

/// Parsed mDNS Question entry (with RFC 6762 QU unicast-response flag).
pub struct MdnsQuestion {
    name_offset: usize,
    qtype: u16,
    qclass: u16,
    unicast_response: bool,
}

/// Zero-copy iterator over mDNS Question entries.
pub struct MdnsQuestionIter {
    raw: []const u8,
    offset: usize,
    remaining: u16,
    failed: bool,

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): MdnsQuestion? {
        if (_.failed || _.remaining == 0) {
            return null;
        }
        const name_off = _.offset;
        let after_name: usize = 0;
        if (next_off := skipDnsName(_.raw, name_off)) {
            after_name = next_off;
        } else {
            _.failed = true;
            return null;
        }
        if (after_name + 4 > _.raw.len) {
            _.failed = true;
            return null;
        }
        const qtype = readU16Be(_.raw, after_name);
        const raw_qclass = readU16Be(_.raw, after_name + 2);
        _.offset = after_name + 4;
        _.remaining -= 1;
        return MdnsQuestion{
            .name_offset = name_off,
            .qtype = qtype,
            .qclass = raw_qclass & 0x7FFF,
            .unicast_response = (raw_qclass & MDNS_QU_BIT) != 0,
        };
    }
}

/// Parsed mDNS Resource Record (with RFC 6762 Cache-Flush flag and zero-copy rdata slice).
pub struct MdnsRecord {
    name_offset: usize,
    rtype: u16,
    rclass: u16,
    cache_flush: bool,
    ttl: u32,
    rdata_offset: usize,
    rdata: []const u8,
}

/// Zero-copy iterator over all Answer, Authority, and Additional Resource Records.
pub struct MdnsRecordIter {
    raw: []const u8,
    offset: usize,
    remaining: usize,
    failed: bool,

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): MdnsRecord? {
        if (_.failed || _.remaining == 0) {
            return null;
        }
        const name_off = _.offset;
        let after_name: usize = 0;
        if (next_off := skipDnsName(_.raw, name_off)) {
            after_name = next_off;
        } else {
            _.failed = true;
            return null;
        }
        if (after_name + 10 > _.raw.len) {
            _.failed = true;
            return null;
        }
        const rtype = readU16Be(_.raw, after_name);
        const raw_rclass = readU16Be(_.raw, after_name + 2);
        const ttl = readU32Be(_.raw, after_name + 4);
        const rdlength = readU16Be(_.raw, after_name + 8) as usize;
        const rdata_off = after_name + 10;
        if (rdata_off + rdlength > _.raw.len) {
            _.failed = true;
            return null;
        }
        _.offset = rdata_off + rdlength;
        _.remaining -= 1;
        return MdnsRecord{
            .name_offset = name_off,
            .rtype = rtype,
            .rclass = raw_rclass & 0x7FFF,
            .cache_flush = (raw_rclass & MDNS_CACHE_FLUSH_BIT) != 0,
            .ttl = ttl,
            .rdata_offset = rdata_off,
            .rdata = _.raw[rdata_off .. rdata_off + rdlength],
        };
    }
}

/// Parsed RFC 2782 / RFC 6763 SRV record payload.
pub struct MdnsSrv {
    priority: u16,
    weight: u16,
    port: u16,
    target_offset: usize,
}

/// Single key/value entry from an RFC 6763 §6 DNS-SD TXT record.
pub struct MdnsTxtEntry {
    key: []const u8,
    value: []const u8,
    has_value: bool,
}

/// Zero-copy iterator over length-prefixed RFC 6763 TXT key/value entries.
pub struct MdnsTxtIter {
    data: []const u8,
    offset: usize,
    failed: bool,

    static fn init(rdata: []const u8): MdnsTxtIter {
        return MdnsTxtIter{
            .data = rdata,
            .offset = 0,
            .failed = false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): MdnsTxtEntry? {
        while (!_.failed && _.offset < _.data.len) {
            const entry_len = _.data[_.offset] as usize;
            _.offset += 1;
            if (_.offset + entry_len > _.data.len) {
                _.failed = true;
                return null;
            }
            const entry = _.data[_.offset .. _.offset + entry_len];
            _.offset += entry_len;
            if (entry_len == 0) {
                continue;
            }
            // Split on first '=' (RFC 6763 §6.4: missing '=' denotes a boolean key)
            let eq_pos: usize = 0;
            let found_eq = false;
            let i: usize = 0;
            while (i < entry.len) {
                if (entry[i] == '=') {
                    eq_pos = i;
                    found_eq = true;
                    break;
                }
                i += 1;
            }
            if (found_eq) {
                return MdnsTxtEntry{
                    .key = entry[0..eq_pos],
                    .value = entry[eq_pos + 1 ..],
                    .has_value = true,
                };
            }
            return MdnsTxtEntry{
                .key = entry,
                .value = entry[entry.len..entry.len],
                .has_value = false,
            };
        }
        return null;
    }
}

/// Parsed mDNS packet with validated section boundaries.
pub struct MdnsPacket {
    raw: []const u8,
    header: MdnsHeader,
    records_offset: usize,

    fn decodeName(offset: usize, out: []u8): usize? {
        return decodeDnsName(_.raw, offset, out);
    }

    fn questions(): MdnsQuestionIter {
        return MdnsQuestionIter{
            .raw = _.raw,
            .offset = MDNS_HEADER_SIZE,
            .remaining = _.header.qd_count,
            .failed = false,
        };
    }

    fn records(): MdnsRecordIter {
        const total: usize = (_.header.an_count as usize) + (_.header.ns_count as usize) + (_.header.ar_count as usize);
        return MdnsRecordIter{
            .raw = _.raw,
            .offset = _.records_offset,
            .remaining = total,
            .failed = false,
        };
    }
}

/// Strictly parses an mDNS packet and validates all question and resource record boundaries.
pub fn parseMdns(data: []const u8): MdnsPacket? {
    if (data.len < MDNS_HEADER_SIZE) {
        return null;
    }
    const hdr = MdnsHeader{
        .id = readU16Be(data, 0),
        .flags = readU16Be(data, 2),
        .qd_count = readU16Be(data, 4),
        .an_count = readU16Be(data, 6),
        .ns_count = readU16Be(data, 8),
        .ar_count = readU16Be(data, 10),
    };

    // Walk all questions to find records_offset and validate bounds
    let pos: usize = MDNS_HEADER_SIZE;
    let q: u16 = 0;
    while (q < hdr.qd_count) {
        pos = skipDnsName(data, pos) ?? return null;
        if (pos + 4 > data.len) {
            return null;
        }
        pos += 4;
        q += 1;
    }
    const records_offset = pos;

    // Validate all Answer + Authority + Additional records
    const total_rrs: usize = (hdr.an_count as usize) + (hdr.ns_count as usize) + (hdr.ar_count as usize);
    let r: usize = 0;
    while (r < total_rrs) {
        pos = skipDnsName(data, pos) ?? return null;
        if (pos + 10 > data.len) {
            return null;
        }
        const rdlength = readU16Be(data, pos + 8) as usize;
        pos += 10;
        if (pos + rdlength > data.len) {
            return null;
        }
        pos += rdlength;
        r += 1;
    }

    return MdnsPacket{
        .raw = data,
        .header = hdr,
        .records_offset = records_offset,
    };
}

/// Fast structural check for valid mDNS packets.
pub fn isMdns(data: []const u8): bool {
    return parseMdns(data) != null;
}

/// Parses an SRV record's rdata into `MdnsSrv`.
pub fn parseMdnsSrv(rec: MdnsRecord): MdnsSrv? {
    if (rec.rtype != TYPE_SRV || rec.rdata.len < 7) {
        return null;
    }
    return MdnsSrv{
        .priority = readU16Be(rec.rdata, 0),
        .weight = readU16Be(rec.rdata, 2),
        .port = readU16Be(rec.rdata, 4),
        .target_offset = rec.rdata_offset + 6,
    };
}

pub type MdnsIpv4 = [4]u8;
pub type MdnsIpv6 = [16]u8;

/// Extracts a 4-byte IPv4 address from an `A` record.
pub fn parseMdnsA(rec: MdnsRecord): MdnsIpv4? {
    if (rec.rtype != TYPE_A || rec.rdata.len != 4) {
        return null;
    }
    return [rec.rdata[0], rec.rdata[1], rec.rdata[2], rec.rdata[3]];
}

/// Extracts a 16-byte IPv6 address from an `AAAA` record.
pub fn parseMdnsAaaa(rec: MdnsRecord): MdnsIpv6? {
    if (rec.rtype != TYPE_AAAA || rec.rdata.len != 16) {
        return null;
    }
    let addr: [16]u8 = undefined;
    let i: usize = 0;
    while (i < 16) {
        addr[i] = rec.rdata[i];
        i += 1;
    }
    return addr;
}

/// Zero-alloc encoder for a single-question RFC 6762 mDNS Query packet.
/// Encodes dot-separated `qname` into DNS labels and sets the optional QU (unicast response) bit.
pub fn encodeMdnsQuery(qname: []const u8, qtype: u16, qu: bool, out: []u8): usize? {
    if (qname.len == 0 || out.len < MDNS_HEADER_SIZE + 5) {
        return null;
    }

    // Strip optional trailing dot
    let name_len = qname.len;
    if (name_len > 0 && qname[name_len - 1] == '.') {
        name_len -= 1;
    }
    if (name_len == 0) {
        return null;
    }

    // Write 12-byte mDNS Query header (id=0, flags=0, qd_count=1)
    let h: usize = 0;
    while (h < MDNS_HEADER_SIZE) {
        out[h] = 0;
        h += 1;
    }
    writeU16Be(out, 4, 1);

    let out_pos: usize = MDNS_HEADER_SIZE;
    let total_wire_name: usize = 0;
    let start: usize = 0;
    while (start < name_len) {
        let dot = start;
        while (dot < name_len && qname[dot] != '.') {
            dot += 1;
        }
        const label_len = dot - start;
        if (label_len == 0 || label_len > 63) {
            return null;
        }
        total_wire_name += 1 + label_len;
        if (total_wire_name + 1 > 255) {
            return null;
        }
        if (out_pos + 1 + label_len + 5 > out.len) {
            return null;
        }
        out[out_pos] = label_len as u8;
        out_pos += 1;
        let i: usize = 0;
        while (i < label_len) {
            out[out_pos + i] = qname[start + i];
            i += 1;
        }
        out_pos += label_len;
        start = dot + 1;
    }

    // Root label 0x00 + QTYPE + QCLASS (with optional QU bit)
    if (out_pos + 5 > out.len) {
        return null;
    }
    out[out_pos] = 0;
    out_pos += 1;
    writeU16Be(out, out_pos, qtype);
    out_pos += 2;
    const qclass: u16 = CLASS_IN | (if (qu) MDNS_QU_BIT else 0);
    writeU16Be(out, out_pos, qclass);
    out_pos += 2;
    return out_pos;
}
