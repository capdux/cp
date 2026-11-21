// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RFC 2784/2890/2637 GRE Tunnel Codec)
// Pure CP zero-heap-allocation Generic Routing Encapsulation (GRE) codec:
// RFC 2784 / 2890 v0 standard GRE (Checksum, Key, Sequence), RFC 2637 v1 PPTP Enhanced GRE
// (Payload Length, Call ID, Sequence, Ack), RFC 1071 16-bit one's complement checksum,
// and zero-allocation encodeGre encoder.

/// IP protocol number for GRE (`47`).
pub const GRE_IP_PROTO: u8 = 47;

/// Minimum GRE header size in bytes (`4`).
pub const GRE_MIN_HEADER_SIZE: usize = 4;

// GRE Header Flags (2-byte big-endian flags_ver)
pub const GRE_FLAG_CHECKSUM: u16 = 0x8000;
pub const GRE_FLAG_ROUTING: u16 = 0x4000;
pub const GRE_FLAG_KEY: u16 = 0x2000;
pub const GRE_FLAG_SEQ: u16 = 0x1000;
pub const GRE_FLAG_STRICT_ROUTE: u16 = 0x0800;
pub const GRE_FLAG_ACK: u16 = 0x0080;

// Common GRE Protocol Types (EtherTypes)
pub const GRE_PROTO_IPV4: u16 = 0x0800;
pub const GRE_PROTO_IPV6: u16 = 0x86DD;
pub const GRE_PROTO_TEB: u16 = 0x6558;
pub const GRE_PROTO_MPLS_UC: u16 = 0x8847;
pub const GRE_PROTO_PPP: u16 = 0x880B;

/// Zero-copy decoded GRE packet (v0 RFC 2784/2890 or v1 RFC 2637 PPTP Enhanced GRE).
pub struct GrePacket {
    flags_ver: u16 = 0,
    version: u8 = 0,
    protocol_type: u16 = 0,
    checksum: u16? = null,
    key: u32? = null,
    seq_num: u32? = null,
    ack_num: u32? = null,
    payload_len: u16 = 0,
    call_id: u16 = 0,
    header_bytes: usize = 4,
    payload: []const u8 = "",

    fn hasChecksum(): bool {
        return (_.flags_ver & GRE_FLAG_CHECKSUM) != 0;
    }

    fn hasKey(): bool {
        return (_.flags_ver & GRE_FLAG_KEY) != 0;
    }

    fn hasSeq(): bool {
        return (_.flags_ver & GRE_FLAG_SEQ) != 0;
    }

    fn hasAck(): bool {
        return (_.flags_ver & GRE_FLAG_ACK) != 0;
    }
}

// ---------------------------------------------------------------------------
// Internal Big-Endian Helpers
// ---------------------------------------------------------------------------

fn readBeU16(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn readBeU32(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24)
        | ((data[off + 1] as u32) << 16)
        | ((data[off + 2] as u32) << 8)
        | (data[off + 3] as u32);
}

fn writeBeU16(buf: []u8, off: usize, val: u16) {
    buf[off] = ((val >> 8) & 0xFF) as u8;
    buf[off + 1] = (val & 0xFF) as u8;
}

fn writeBeU32(buf: []u8, off: usize, val: u32) {
    buf[off] = ((val >> 24) & 0xFF) as u8;
    buf[off + 1] = ((val >> 16) & 0xFF) as u8;
    buf[off + 2] = ((val >> 8) & 0xFF) as u8;
    buf[off + 3] = (val & 0xFF) as u8;
}

// ---------------------------------------------------------------------------
// RFC 1071 16-bit One's Complement Checksum
// ---------------------------------------------------------------------------

/// Computes the RFC 1071 16-bit one's complement checksum over `data`.
pub fn greChecksum(data: []const u8): u16 {
    let sum: u32 = 0;
    let i: usize = 0;
    while (i + 1 < data.len) {
        const word = (((data[i] as u32) << 8) | (data[i + 1] as u32));
        sum += word;
        i += 2;
    }
    if (i < data.len) {
        sum += (data[i] as u32) << 8;
    }
    while ((sum >> 16) != 0) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return ((~sum) & 0xFFFF) as u16;
}

/// Verifies the GRE packet checksum when `GRE_FLAG_CHECKSUM` is set.
/// Returns `false` if the packet is malformed or the checksum fails; returns `true` if valid
/// or if `GRE_FLAG_CHECKSUM` is not set.
pub fn verifyGreChecksum(packet: []const u8): bool {
    if (pkt := parseGre(packet)) {
        if (!pkt.hasChecksum()) {
            return true;
        }
        return greChecksum(packet) == 0;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Public GRE Parser & Zero-Alloc Encoder
// ---------------------------------------------------------------------------

/// Parses a GRE v0 (RFC 2784/2890) or v1 Enhanced GRE (RFC 2637 PPTP) packet.
pub fn parseGre(data: []const u8): GrePacket? {
    if (data.len < GRE_MIN_HEADER_SIZE) {
        return null;
    }
    const flags_ver = readBeU16(data, 0);
    const version = (flags_ver & 0x0007) as u8;
    if (version > 1) {
        return null;
    }
    const protocol_type = readBeU16(data, 2);

    let offset: usize = GRE_MIN_HEADER_SIZE;
    let checksum: u16? = null;
    let key: u32? = null;
    let seq_num: u32? = null;
    let ack_num: u32? = null;
    let payload_len: u16 = 0;
    let call_id: u16 = 0;

    if (version == 0) {
        // RFC 2784 / 2890 Standard GRE
        if ((flags_ver & GRE_FLAG_CHECKSUM) != 0) {
            if (data.len - offset < 4) {
                return null;
            }
            checksum = readBeU16(data, offset);
            offset += 4; // 2B checksum + 2B reserved0
        }
        if ((flags_ver & GRE_FLAG_KEY) != 0) {
            if (data.len - offset < 4) {
                return null;
            }
            key = readBeU32(data, offset);
            offset += 4;
        }
        if ((flags_ver & GRE_FLAG_SEQ) != 0) {
            if (data.len - offset < 4) {
                return null;
            }
            seq_num = readBeU32(data, offset);
            offset += 4;
        }
        return GrePacket {
            flags_ver: flags_ver,
            version: version,
            protocol_type: protocol_type,
            checksum: checksum,
            key: key,
            seq_num: seq_num,
            ack_num: null,
            payload_len: 0,
            call_id: 0,
            header_bytes: offset,
            payload: data[offset..data.len],
        };
    }

    // RFC 2637 v1 Enhanced GRE (PPTP): K must be set, C must be 0
    if ((flags_ver & GRE_FLAG_KEY) == 0 || (flags_ver & GRE_FLAG_CHECKSUM) != 0) {
        return null;
    }
    if (data.len - offset < 4) {
        return null;
    }
    payload_len = readBeU16(data, offset);
    call_id = readBeU16(data, offset + 2);
    offset += 4;

    if ((flags_ver & GRE_FLAG_SEQ) != 0) {
        if (data.len - offset < 4) {
            return null;
        }
        seq_num = readBeU32(data, offset);
        offset += 4;
    }
    if ((flags_ver & GRE_FLAG_ACK) != 0) {
        if (data.len - offset < 4) {
            return null;
        }
        ack_num = readBeU32(data, offset);
        offset += 4;
    }

    const plen = payload_len as usize;
    if (data.len - offset < plen) {
        return null;
    }

    return GrePacket {
        flags_ver: flags_ver,
        version: version,
        protocol_type: protocol_type,
        checksum: null,
        key: null,
        seq_num: seq_num,
        ack_num: ack_num,
        payload_len: payload_len,
        call_id: call_id,
        header_bytes: offset,
        payload: data[offset..(offset + plen)],
    };
}

/// Encodes a standard v0 GRE packet into `out` with optional Checksum, Key, and Sequence fields.
/// Automatically computes and backpatches the RFC 1071 16-bit checksum when `include_checksum` is `true`.
/// Returns total encoded packet length in bytes, or `null` if `out` is too small.
pub fn encodeGre(
    protocol_type: u16,
    include_checksum: bool,
    key: u32?,
    seq_num: u32?,
    inner_payload: []const u8,
    out: []u8,
): usize? {
    let flags_ver: u16 = 0;
    let hdr_len: usize = GRE_MIN_HEADER_SIZE;

    if (include_checksum) {
        flags_ver |= GRE_FLAG_CHECKSUM;
        hdr_len += 4;
    }
    if (k := key) {
        * = k;
        flags_ver |= GRE_FLAG_KEY;
        hdr_len += 4;
    }
    if (s := seq_num) {
        * = s;
        flags_ver |= GRE_FLAG_SEQ;
        hdr_len += 4;
    }

    const total_len = hdr_len + inner_payload.len;
    if (out.len < total_len) {
        return null;
    }

    writeBeU16(out, 0, flags_ver);
    writeBeU16(out, 2, protocol_type);
    let pos: usize = 4;

    if (include_checksum) {
        writeBeU16(out, pos, 0);     // checksum placeholder
        writeBeU16(out, pos + 2, 0); // reserved0
        pos += 4;
    }
    if (k := key) {
        writeBeU32(out, pos, k);
        pos += 4;
    }
    if (s := seq_num) {
        writeBeU32(out, pos, s);
        pos += 4;
    }

    let i: usize = 0;
    while (i < inner_payload.len) {
        out[pos + i] = inner_payload[i];
        i += 1;
    }

    if (include_checksum) {
        const csum = greChecksum(out[0..total_len]);
        writeBeU16(out, 4, csum);
    }

    return total_len;
}
