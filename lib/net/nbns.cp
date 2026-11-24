// Origin: Pure-CP Stdlib (Zero-Alloc RFC 1001 / RFC 1002 NetBIOS Name Service Codec) | Created: 2026-11-22

// Standard NBNS Port, Sizes, Record Types & Class
pub const NBNS_UDP_PORT: u16 = 137;
pub const NBNS_HEADER_SIZE: usize = 12;
pub const NBNS_ENCODED_NAME_LEN: usize = 32;
pub const NBNS_RAW_NAME_LEN: usize = 16;
pub const NBNS_TYPE_NB: u16 = 0x0020;
pub const NBNS_TYPE_NBSTAT: u16 = 0x0021;
pub const NBNS_CLASS_IN: u16 = 0x0001;

// NBNS Opcodes (RFC 1002 §4.2.1.1)
pub const OP_QUERY: u8 = 0;
pub const OP_REGISTRATION: u8 = 5;
pub const OP_RELEASE: u8 = 6;
pub const OP_WACK: u8 = 7;
pub const OP_REFRESH: u8 = 8;

// Common 16th-Byte NetBIOS Name Suffixes
pub const SUFFIX_WORKSTATION: u8 = 0x00;
pub const SUFFIX_MESSENGER: u8 = 0x03;
pub const SUFFIX_FILE_SERVER: u8 = 0x20;
pub const SUFFIX_DOMAIN_MASTER: u8 = 0x1B;
pub const SUFFIX_DOMAIN_CTRL: u8 = 0x1C;
pub const SUFFIX_MASTER_BROWSER: u8 = 0x1D;
pub const SUFFIX_BROWSER_ELECTION: u8 = 0x1E;

fn readU16Be(data: []const u8, off: usize): u16 {
    const hi = (data[off] as u16) << 8;
    const lo = data[off + 1] as u16;
    return hi | lo;
}

fn readU32Be(data: []const u8, off: usize): u32 {
    const b0 = (data[off] as u32) << 24;
    const b1 = (data[off + 1] as u32) << 16;
    const b2 = (data[off + 2] as u32) << 8;
    const b3 = data[off + 3] as u32;
    return b0 | b1 | b2 | b3;
}

fn writeU16Be(out: []u8, off: usize, val: u16) {
    out[off] = ((val >> 8) & 0xFF) as u8;
    out[off + 1] = (val & 0xFF) as u8;
}

fn toUpperAscii(c: u8): u8 {
    if (c >= 'a' && c <= 'z') {
        return c - 32;
    }
    return c;
}

/// Decoded 16-byte NetBIOS name (`raw[0..15]` space-padded name, `raw[15]` == `suffix`).
pub struct NbnsName {
    raw: [16]u8,
    suffix: u8,

    /// Returns the length of the NetBIOS name without trailing ASCII spaces (`0x20`).
    fn trimmedLen(): usize {
        let end: usize = 15;
        while (end > 0 && _.raw[end - 1] == 0x20) {
            end -= 1;
        }
        return end;
    }

    /// Returns a zero-copy slice of the trimmed NetBIOS name (without trailing spaces or suffix).
    fn nameSlice(): []const u8 {
        return _.raw[0.._.trimmedLen()];
    }
}

/// RFC 1001 §14.1 First-Level Encoding: uppercases `name`, right-pads with spaces to 15B + 1B `suffix`,
/// and encodes each byte into two `'A'..'P'` nibble characters (32 bytes total).
pub fn encodeNbnsName(name: []const u8, suffix: u8, out: []u8): usize? {
    if (name.len > 15 || out.len < NBNS_ENCODED_NAME_LEN) {
        return null;
    }
    let i: usize = 0;
    while (i < 16) {
        let b: u8 = 0x20;
        if (i < name.len) {
            b = toUpperAscii(name[i]);
        } else if (i == 15) {
            b = suffix;
        }
        const hi = ((b >> 4) & 0x0F) + 'A';
        const lo = (b & 0x0F) + 'A';
        out[i * 2] = hi;
        out[i * 2 + 1] = lo;
        i += 1;
    }
    return NBNS_ENCODED_NAME_LEN;
}

/// RFC 1001 §14.1 First-Level Decoding: validates 32 bytes in `'A'..'P'` and decodes into `NbnsName`.
pub fn decodeNbnsName(encoded: []const u8): NbnsName? {
    if (encoded.len != NBNS_ENCODED_NAME_LEN) {
        return null;
    }
    let raw: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i < 16) {
        const hi_c = encoded[i * 2];
        const lo_c = encoded[i * 2 + 1];
        if (hi_c < 'A' || hi_c > 'P' || lo_c < 'A' || lo_c > 'P') {
            return null;
        }
        const hi = (hi_c - 'A') << 4;
        const lo = lo_c - 'A';
        raw[i] = hi | lo;
        i += 1;
    }
    return NbnsName{
        .raw = raw,
        .suffix = raw[15],
    };
}

/// RFC 1002 §4.2.1 12-byte NBNS packet header.
pub struct NbnsHeader {
    tr_id: u16,
    flags: u16,
    qd_count: u16,
    an_count: u16,
    ns_count: u16,
    ar_count: u16,

    fn isResponse(): bool {
        return (_.flags & 0x8000) != 0;
    }

    fn opcode(): u8 {
        return ((_.flags >> 11) & 0x0F) as u8;
    }

    fn isAuthoritative(): bool {
        return (_.flags & 0x0400) != 0;
    }

    fn isTruncated(): bool {
        return (_.flags & 0x0200) != 0;
    }

    fn isBroadcast(): bool {
        return (_.flags & 0x0010) != 0;
    }

    fn rcode(): u8 {
        return (_.flags & 0x000F) as u8;
    }
}

/// Single 6-byte NB (`0x0020`) RDATA address entry (`NB_FLAGS: u16`, `NB_ADDRESS: [4]u8`).
pub struct NbnsAddrEntry {
    is_group: bool,
    ont: u8,
    ip: [4]u8,
}

/// Zero-copy iterator over 6-byte NB (`0x0020`) RDATA address entries.
pub struct NbnsAddrIter {
    rdata: []const u8,
    pos: usize,

    pub static fn init(rdata: []const u8): NbnsAddrIter {
        return NbnsAddrIter{
            .rdata = rdata,
            .pos = 0,
        };
    }

    fn next(*_): NbnsAddrEntry? {
        if (_.pos + 6 > _.rdata.len) {
            return null;
        }
        const nb_flags = readU16Be(_.rdata, _.pos);
        const ip: [4]u8 = [
            _.rdata[_.pos + 2],
            _.rdata[_.pos + 3],
            _.rdata[_.pos + 4],
            _.rdata[_.pos + 5],
        ];
        _.pos += 6;
        return NbnsAddrEntry{
            .is_group = (nb_flags & 0x8000) != 0,
            .ont = ((nb_flags >> 13) & 0x03) as u8,
            .ip = ip,
        };
    }
}

/// Single 18-byte NBSTAT (`0x0021`) node name entry (`raw: [16]u8`, `flags: u16`).
pub struct NbnsStatEntry {
    name: NbnsName,
    flags: u16,

    fn isGroup(): bool {
        return (_.flags & 0x8000) != 0;
    }

    fn isActive(): bool {
        return (_.flags & 0x0400) != 0;
    }
}

/// Zero-copy iterator over RFC 1002 §4.2.18 NBSTAT (`0x0021`) RDATA node table + 6-byte MAC `unit_id`.
pub struct NbnsStatIter {
    rdata: []const u8,
    num_names: u8,
    idx: u8,

    pub static fn init(rdata: []const u8): NbnsStatIter? {
        if (rdata.len < 1) {
            return null;
        }
        const num = rdata[0];
        const table_bytes = 1 + (num as usize) * 18;
        if (rdata.len < table_bytes) {
            return null;
        }
        return NbnsStatIter{
            .rdata = rdata,
            .num_names = num,
            .idx = 0,
        };
    }

    fn count(): u8 {
        return _.num_names;
    }

    fn next(*_): NbnsStatEntry? {
        if (_.idx >= _.num_names) {
            return null;
        }
        const off = 1 + (_.idx as usize) * 18;
        let raw: [16]u8 = [0; 16];
        let i: usize = 0;
        while (i < 16) {
            raw[i] = _.rdata[off + i];
            i += 1;
        }
        const flags = readU16Be(_.rdata, off + 16);
        _.idx += 1;
        return NbnsStatEntry{
            .name = NbnsName{ .raw = raw, .suffix = raw[15] },
            .flags = flags,
        };
    }

    /// Extracts the 6-byte MAC address (`UNIT_ID`) following the node name table, if present.
    fn unitId(): ([6]u8)? {
        const mac_off = 1 + (_.num_names as usize) * 18;
        if (mac_off + 6 > _.rdata.len) {
            return null;
        }
        return [
            _.rdata[mac_off],
            _.rdata[mac_off + 1],
            _.rdata[mac_off + 2],
            _.rdata[mac_off + 3],
            _.rdata[mac_off + 4],
            _.rdata[mac_off + 5],
        ];
    }
}

struct ParsedWireName {
    name: NbnsName,
    next_off: usize,
}

fn parseWireName(pkt: []const u8, start: usize): ParsedWireName? {
    if (start >= pkt.len) {
        return null;
    }
    let off = start;
    let jumped = false;
    let next_off: usize = 0;
    let name_opt: NbnsName? = null;
    let hops: usize = 0;

    while (off < pkt.len) {
        const label_len = pkt[off];
        if (label_len == 0) {
            if (!jumped) {
                next_off = off + 1;
            }
            const decoded = name_opt ?? return null;
            return ParsedWireName{
                .name = decoded,
                .next_off = next_off,
            };
        }
        if ((label_len & 0xC0) == 0xC0) {
            if (off + 2 > pkt.len || hops >= 4) {
                return null;
            }
            const ptr = (((label_len & 0x3F) as usize) << 8) | (pkt[off + 1] as usize);
            if (ptr >= pkt.len || ptr == off) {
                return null;
            }
            if (!jumped) {
                next_off = off + 2;
                jumped = true;
            }
            off = ptr;
            hops += 1;
        } else if ((label_len & 0xC0) == 0) {
            const llen = label_len as usize;
            if (off + 1 + llen > pkt.len) {
                return null;
            }
            if (name_opt == null) {
                if (llen != NBNS_ENCODED_NAME_LEN) {
                    return null;
                }
                name_opt = decodeNbnsName(pkt[(off + 1)..(off + 1 + llen)]) ?? return null;
            }
            off += 1 + llen;
        } else {
            return null;
        }
    }
    return null;
}

/// Parsed NBNS Query or Response packet with zero-copy RDATA slice.
pub struct NbnsPacket {
    header: NbnsHeader,
    name: NbnsName,
    qtype: u16,
    qclass: u16,
    ttl: u32,
    rdata: []const u8,

    /// Iterates 6-byte NB (`0x0020`) address entries in `rdata`.
    fn addrs(): NbnsAddrIter {
        return NbnsAddrIter.init(_.rdata);
    }

    /// Parses NBSTAT (`0x0021`) node status table & MAC address in `rdata`.
    fn stats(): NbnsStatIter? {
        return NbnsStatIter.init(_.rdata);
    }
}

/// Parses an RFC 1001/1002 NBNS Query or Response packet.
pub fn parseNbns(data: []const u8): NbnsPacket? {
    if (data.len < NBNS_HEADER_SIZE) {
        return null;
    }
    const header = NbnsHeader{
        .tr_id = readU16Be(data, 0),
        .flags = readU16Be(data, 2),
        .qd_count = readU16Be(data, 4),
        .an_count = readU16Be(data, 6),
        .ns_count = readU16Be(data, 8),
        .ar_count = readU16Be(data, 10),
    };
    if (header.qd_count == 0 && header.an_count == 0) {
        return null;
    }

    let pos: usize = NBNS_HEADER_SIZE;
    let first_name = NbnsName{ .raw = [0; 16], .suffix = 0 };
    let has_name = false;
    let qtype: u16 = 0;
    let qclass: u16 = 0;
    let ttl: u32 = 0;
    let rdata: []const u8 = data[0..0];

    let qi: u16 = 0;
    while (qi < header.qd_count) {
        const pn = parseWireName(data, pos) ?? return null;
        pos = pn.next_off;
        if (pos + 4 > data.len) {
            return null;
        }
        const qt = readU16Be(data, pos);
        const qc = readU16Be(data, pos + 2);
        if ((qt != NBNS_TYPE_NB && qt != NBNS_TYPE_NBSTAT) || qc != NBNS_CLASS_IN) {
            return null;
        }
        if (!has_name) {
            first_name = pn.name;
            has_name = true;
            qtype = qt;
            qclass = qc;
        }
        pos += 4;
        qi += 1;
    }

    if (header.an_count > 0) {
        const pn = parseWireName(data, pos) ?? return null;
        pos = pn.next_off;
        if (pos + 10 > data.len) {
            return null;
        }
        const rt = readU16Be(data, pos);
        const rc = readU16Be(data, pos + 2);
        if ((rt != NBNS_TYPE_NB && rt != NBNS_TYPE_NBSTAT) || rc != NBNS_CLASS_IN) {
            return null;
        }
        ttl = readU32Be(data, pos + 4);
        const rdlen = readU16Be(data, pos + 8) as usize;
        pos += 10;
        if (rdlen > data.len - pos) {
            return null;
        }
        rdata = data[pos..(pos + rdlen)];
        if (rt == NBNS_TYPE_NB && (rdlen % 6) != 0) {
            return null;
        }
        if (!has_name) {
            first_name = pn.name;
            has_name = true;
        }
        qtype = rt;
        qclass = rc;
    }

    return NbnsPacket{
        .header = header,
        .name = first_name,
        .qtype = qtype,
        .qclass = qclass,
        .ttl = ttl,
        .rdata = rdata,
    };
}

/// Returns `true` if `data` is a well-formed NBNS Query or Response packet.
pub fn isNbns(data: []const u8): bool {
    return parseNbns(data) != null;
}

/// Zero-alloc encoder for an RFC 1002 NBNS Name Query packet (50 bytes).
pub fn encodeNbnsQuery(tr_id: u16, name: []const u8, suffix: u8, broadcast: bool, out: []u8): usize? {
    if (name.len > 15 || out.len < 50) {
        return null;
    }
    let flags: u16 = 0x0100; // RD=1, OP_QUERY=0
    if (broadcast) {
        flags = flags | 0x0010; // B=1
    }
    writeU16Be(out, 0, tr_id);
    writeU16Be(out, 2, flags);
    writeU16Be(out, 4, 1); // qd_count = 1
    writeU16Be(out, 6, 0);
    writeU16Be(out, 8, 0);
    writeU16Be(out, 10, 0);

    out[12] = NBNS_ENCODED_NAME_LEN as u8;
    * = encodeNbnsName(name, suffix, out[13..45]) ?? return null;
    out[45] = 0x00; // root label terminator
    writeU16Be(out, 46, NBNS_TYPE_NB);
    writeU16Be(out, 48, NBNS_CLASS_IN);
    return 50;
}
