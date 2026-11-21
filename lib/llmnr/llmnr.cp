// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RFC 4795 LLMNR Wire Protocol Codec)

// ---------------------------------------------------------------------------
// Protocol Constants (RFC 4795 LLMNR)
// ---------------------------------------------------------------------------

pub const LLMNR_PORT: u16 = 5355;
pub const LLMNR_MULTICAST_V4: []const u8 = "224.0.0.252";
pub const LLMNR_MULTICAST_V6: []const u8 = "ff02::1:3";
pub const LLMNR_HEADER_SIZE: usize = 12;
pub const LLMNR_DEFAULT_TTL: u32 = 30;

pub const CLASS_IN: u16 = 1;
pub const TYPE_A: u16 = 1;
pub const TYPE_PTR: u16 = 12;
pub const TYPE_AAAA: u16 = 28;
pub const TYPE_ANY: u16 = 255;

// LLMNR Header Flag Masks (RFC 4795 §2.1.1)
pub const FLAG_QR: u16 = 0x8000;
pub const FLAG_CONFLICT: u16 = 0x0400;
pub const FLAG_TC: u16 = 0x0200;
pub const FLAG_TENTATIVE: u16 = 0x0100;

// ---------------------------------------------------------------------------
// Internal Big-Endian & DNS Name Helpers
// ---------------------------------------------------------------------------

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

fn writeU16Be(buf: []u8, off: usize, val: u16) {
    buf[off] = ((val >> 8) & 0xFF) as u8;
    buf[off + 1] = (val & 0xFF) as u8;
}

fn writeU32Be(buf: []u8, off: usize, val: u32) {
    buf[off] = ((val >> 24) & 0xFF) as u8;
    buf[off + 1] = ((val >> 16) & 0xFF) as u8;
    buf[off + 2] = ((val >> 8) & 0xFF) as u8;
    buf[off + 3] = (val & 0xFF) as u8;
}

/// Skips a wire-encoded DNS/LLMNR name starting at `start` and returns the next offset after it.
fn skipWireName(raw: []const u8, start: usize): usize? {
    let pos = start;
    let steps: usize = 0;
    while (pos < raw.len && steps < 128) {
        const len_byte = raw[pos];
        if (len_byte == 0) {
            return pos + 1;
        }
        const tag = len_byte & 0xC0;
        if (tag == 0xC0) {
            // Compression pointer consumes 2 bytes and ends the wire name field
            if (pos + 2 > raw.len) {
                return null;
            }
            return pos + 2;
        }
        if (tag != 0) {
            // Reserved label type (0x40 or 0x80) is invalid
            return null;
        }
        const label_len = len_byte as usize;
        if (pos + 1 + label_len > raw.len) {
            return null;
        }
        pos += 1 + label_len;
        steps += 1;
    }
    return null;
}

/// Encodes a dot-separated hostname (e.g. "printer" or "host.local") into DNS wire labels in `out`.
fn encodeWireName(name: []const u8, out: []u8): usize? {
    let trimmed_len = name.len;
    if (trimmed_len > 0 && name[trimmed_len - 1] == '.') {
        trimmed_len -= 1;
    }
    if (trimmed_len == 0 || trimmed_len > 253) {
        return null;
    }
    const s = name[0..trimmed_len];
    let in_pos: usize = 0;
    let out_pos: usize = 0;

    while (in_pos < s.len) {
        const label_start = in_pos;
        while (in_pos < s.len && s[in_pos] != '.') {
            in_pos += 1;
        }
        const label_len = in_pos - label_start;
        if (label_len == 0 || label_len > 63) {
            return null;
        }
        if (out_pos + 1 + label_len >= out.len) {
            return null;
        }
        out[out_pos] = label_len as u8;
        out_pos += 1;
        let i: usize = 0;
        while (i < label_len) {
            out[out_pos + i] = s[label_start + i];
            i += 1;
        }
        out_pos += label_len;
        if (in_pos < s.len && s[in_pos] == '.') {
            in_pos += 1;
            if (in_pos == s.len) {
                return null;
            }
        }
    }

    if (out_pos >= out.len) {
        return null;
    }
    out[out_pos] = 0;
    return out_pos + 1;
}

// ---------------------------------------------------------------------------
// LLMNR Header, Question, Record & Packet Models
// ---------------------------------------------------------------------------

/// Decoded 12-byte RFC 4795 LLMNR header.
pub struct LlmnrHeader {
    id: u16,
    flags: u16,
    qd_count: u16,
    an_count: u16,
    ns_count: u16,
    ar_count: u16,

    fn isResponse(): bool {
        return (_.flags & FLAG_QR) != 0;
    }

    fn opcode(): u8 {
        return ((_.flags >> 11) & 0x0F) as u8;
    }

    fn isConflict(): bool {
        return (_.flags & FLAG_CONFLICT) != 0;
    }

    fn isTruncated(): bool {
        return (_.flags & FLAG_TC) != 0;
    }

    fn isTentative(): bool {
        return (_.flags & FLAG_TENTATIVE) != 0;
    }

    fn rcode(): u8 {
        return (_.flags & 0x000F) as u8;
    }
}

/// Decoded LLMNR Question entry referencing its wire name offset in the packet.
pub struct LlmnrQuestion {
    name_offset: usize,
    qtype: u16,
    qclass: u16,
}

/// Zero-copy LLMNR Resource Record view.
pub struct LlmnrRecord {
    name_offset: usize,
    rtype: u16,
    rclass: u16,
    ttl: u32,
    rdata: []const u8,

    /// Extracts an IPv4 address if this is a `TYPE_A` record with 4-byte `rdata`.
    fn asIpv4(): ([4]u8)? {
        if (_.rtype != TYPE_A || _.rdata.len != 4) {
            return null;
        }
        return [_.rdata[0], _.rdata[1], _.rdata[2], _.rdata[3]];
    }

    /// Extracts an IPv6 address if this is a `TYPE_AAAA` record with 16-byte `rdata`.
    fn asIpv6(): ([16]u8)? {
        if (_.rtype != TYPE_AAAA || _.rdata.len != 16) {
            return null;
        }
        let ip: [16]u8 = [0; 16];
        let i: usize = 0;
        while (i < 16) {
            ip[i] = _.rdata[i];
            i += 1;
        }
        return ip;
    }
}

/// Zero-allocation iterator over LLMNR Answer Resource Records.
pub struct LlmnrRecordIter {
    raw: []const u8,
    offset: usize,
    remaining: u16,
    failed: bool,

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): LlmnrRecord? {
        if (_.failed || _.remaining == 0) {
            return null;
        }
        const name_offset = _.offset;
        const after_opt = skipWireName(_.raw, _.offset);
        if (after_opt == null) {
            _.failed = true;
            return null;
        }
        const after_name = after_opt ?? 0;
        if (after_name + 10 > _.raw.len) {
            _.failed = true;
            return null;
        }
        const rtype = readU16Be(_.raw, after_name);
        const rclass = readU16Be(_.raw, after_name + 2);
        const ttl = readU32Be(_.raw, after_name + 4);
        const rdlen = readU16Be(_.raw, after_name + 8) as usize;
        const rdata_start = after_name + 10;
        if (rdata_start + rdlen > _.raw.len) {
            _.failed = true;
            return null;
        }
        _.offset = rdata_start + rdlen;
        _.remaining -= 1;
        return LlmnrRecord {
            name_offset: name_offset,
            rtype: rtype,
            rclass: rclass,
            ttl: ttl,
            rdata: _.raw[rdata_start..rdata_start + rdlen],
        };
    }
}

/// Zero-copy view of a parsed RFC 4795 LLMNR packet.
pub struct LlmnrPacket {
    raw: []const u8,
    header: LlmnrHeader,

    /// Decodes a wire name at `offset` into `out`, supporting `0xC0` compression pointers with loop protection.
    /// Returns the number of bytes written, or `null` on loop, out-of-bounds, or insufficient `out`.
    fn decodeName(offset: usize, out: []u8): usize? {
        if (offset >= _.raw.len) {
            return null;
        }
        let pos = offset;
        let out_pos: usize = 0;
        let jumps: usize = 0;
        let labels: usize = 0;

        while (pos < _.raw.len) {
            if (labels >= 128) {
                return null;
            }
            const len_byte = _.raw[pos];
            if (len_byte == 0) {
                return out_pos;
            }
            const tag = len_byte & 0xC0;
            if (tag == 0xC0) {
                if (pos + 2 > _.raw.len) {
                    return null;
                }
                jumps += 1;
                if (jumps > 16) {
                    return null;
                }
                const ptr = ((((len_byte & 0x3F) as u16) << 8) | (_.raw[pos + 1] as u16)) as usize;
                if (ptr >= _.raw.len || ptr == pos) {
                    return null;
                }
                pos = ptr;
                continue;
            }
            if (tag != 0) {
                return null;
            }
            const label_len = len_byte as usize;
            if (pos + 1 + label_len > _.raw.len) {
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
                out[out_pos + i] = _.raw[pos + 1 + i];
                i += 1;
            }
            out_pos += label_len;
            pos += 1 + label_len;
            labels += 1;
        }
        return null;
    }

    /// Parses the primary Question section entry (offset 12).
    fn question(): LlmnrQuestion? {
        if (_.header.qd_count == 0) {
            return null;
        }
        const after_name = skipWireName(_.raw, LLMNR_HEADER_SIZE) ?? return null;
        if (after_name + 4 > _.raw.len) {
            return null;
        }
        return LlmnrQuestion {
            name_offset: LLMNR_HEADER_SIZE,
            qtype: readU16Be(_.raw, after_name),
            qclass: readU16Be(_.raw, after_name + 2),
        };
    }

    /// Returns a zero-allocation iterator over the Answer Resource Records.
    fn answers(): LlmnrRecordIter {
        let pos: usize = LLMNR_HEADER_SIZE;
        let q: u16 = 0;
        while (q < _.header.qd_count) {
            const next_opt = skipWireName(_.raw, pos);
            if (next_opt == null) {
                return LlmnrRecordIter {
                    raw: _.raw,
                    offset: _.raw.len,
                    remaining: 0,
                    failed: true,
                };
            }
            const next_pos = next_opt ?? 0;
            if (next_pos + 4 > _.raw.len) {
                return LlmnrRecordIter {
                    raw: _.raw,
                    offset: _.raw.len,
                    remaining: 0,
                    failed: true,
                };
            }
            pos = next_pos + 4;
            q += 1;
        }
        return LlmnrRecordIter {
            raw: _.raw,
            offset: pos,
            remaining: _.header.an_count,
            failed: false,
        };
    }
}

// ---------------------------------------------------------------------------
// LLMNR Packet Parser (Strict RFC 4795 Validation)
// ---------------------------------------------------------------------------

/// Parses and validates an RFC 4795 LLMNR packet.
/// Enforces: `len >= 12`, `opcode == 0`, and for queries (`QR == 0`):
/// `qd_count == 1`, `an_count == 0`, `ns_count == 0`, `C == 0`, `TC == 0`, `T == 0`, `rcode == 0`.
pub fn parseLlmnr(data: []const u8): LlmnrPacket? {
    if (data.len < LLMNR_HEADER_SIZE) {
        return null;
    }
    const hdr = LlmnrHeader {
        id: readU16Be(data, 0),
        flags: readU16Be(data, 2),
        qd_count: readU16Be(data, 4),
        an_count: readU16Be(data, 6),
        ns_count: readU16Be(data, 8),
        ar_count: readU16Be(data, 10),
    };

    if (hdr.opcode() != 0) {
        return null;
    }

    if (!hdr.isResponse()) {
        if (hdr.qd_count != 1 || hdr.an_count != 0 || hdr.ns_count != 0) {
            return null;
        }
        if (hdr.isConflict() || hdr.isTruncated() || hdr.isTentative() || hdr.rcode() != 0) {
            return null;
        }
    } else {
        if (hdr.qd_count != 1) {
            return null;
        }
    }

    return LlmnrPacket {
        raw: data,
        header: hdr,
    };
}

/// Fast-path RFC 4795 LLMNR packet validity check.
pub fn isLlmnr(data: []const u8): bool {
    return parseLlmnr(data) != null;
}

// ---------------------------------------------------------------------------
// Zero-Alloc LLMNR Encoders
// ---------------------------------------------------------------------------

/// Encodes an RFC 4795 LLMNR query (`QR=0`, `QDCOUNT=1`) into `out`.
pub fn encodeLlmnrQuery(id: u16, name: []const u8, qtype: u16, out: []u8): usize? {
    if (out.len < LLMNR_HEADER_SIZE + 6) {
        return null;
    }
    writeU16Be(out, 0, id);
    writeU16Be(out, 2, 0);
    writeU16Be(out, 4, 1);
    writeU16Be(out, 6, 0);
    writeU16Be(out, 8, 0);
    writeU16Be(out, 10, 0);

    const name_len = encodeWireName(name, out[LLMNR_HEADER_SIZE..]) ?? return null;
    const q_end = LLMNR_HEADER_SIZE + name_len;
    if (q_end + 4 > out.len) {
        return null;
    }
    writeU16Be(out, q_end, qtype);
    writeU16Be(out, q_end + 2, CLASS_IN);
    return q_end + 4;
}

fn encodeLlmnrResponseCommon(
    id: u16,
    name: []const u8,
    rtype: u16,
    ttl: u32,
    rdata: []const u8,
    out: []u8,
): usize? {
    if (out.len < LLMNR_HEADER_SIZE + 6) {
        return null;
    }
    writeU16Be(out, 0, id);
    writeU16Be(out, 2, FLAG_QR);
    writeU16Be(out, 4, 1);
    writeU16Be(out, 6, 1);
    writeU16Be(out, 8, 0);
    writeU16Be(out, 10, 0);

    const name_len = encodeWireName(name, out[LLMNR_HEADER_SIZE..]) ?? return null;
    const q_end = LLMNR_HEADER_SIZE + name_len;
    const need_after_q = 4 + 2 + 10 + rdata.len;
    if (q_end + need_after_q > out.len) {
        return null;
    }

    // Question section
    writeU16Be(out, q_end, rtype);
    writeU16Be(out, q_end + 2, CLASS_IN);

    // Answer section using 0xC00C compression pointer to offset 12
    const ans = q_end + 4;
    out[ans] = 0xC0;
    out[ans + 1] = LLMNR_HEADER_SIZE as u8;
    writeU16Be(out, ans + 2, rtype);
    writeU16Be(out, ans + 4, CLASS_IN);
    writeU32Be(out, ans + 6, ttl);
    writeU16Be(out, ans + 10, rdata.len as u16);

    const rdata_off = ans + 12;
    let i: usize = 0;
    while (i < rdata.len) {
        out[rdata_off + i] = rdata[i];
        i += 1;
    }
    return rdata_off + rdata.len;
}

/// Encodes an RFC 4795 LLMNR IPv4 `TYPE_A` response (`QR=1`, `QDCOUNT=1`, `ANCOUNT=1`) into `out`.
pub fn encodeLlmnrResponseA(id: u16, name: []const u8, ip: [4]u8, ttl: u32, out: []u8): usize? {
    return encodeLlmnrResponseCommon(id, name, TYPE_A, ttl, ip[0..], out);
}

/// Encodes an RFC 4795 LLMNR IPv6 `TYPE_AAAA` response (`QR=1`, `QDCOUNT=1`, `ANCOUNT=1`) into `out`.
pub fn encodeLlmnrResponseAaaa(id: u16, name: []const u8, ip: [16]u8, ttl: u32, out: []u8): usize? {
    return encodeLlmnrResponseCommon(id, name, TYPE_AAAA, ttl, ip[0..], out);
}
