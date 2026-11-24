// Origin: Pure-CP Stdlib (Zero-Alloc RFC 4960 SCTP Packet & Chunk Parser) | Created: 2026-11-22

pub const SCTP_HEADER_SIZE: usize = 12;
pub const SCTP_CHUNK_HEADER_SIZE: usize = 4;

pub const CHUNK_DATA: u8 = 0;
pub const CHUNK_INIT: u8 = 1;
pub const CHUNK_INIT_ACK: u8 = 2;
pub const CHUNK_SACK: u8 = 3;
pub const CHUNK_HEARTBEAT: u8 = 4;
pub const CHUNK_HEARTBEAT_ACK: u8 = 5;
pub const CHUNK_ABORT: u8 = 6;
pub const CHUNK_SHUTDOWN: u8 = 7;
pub const CHUNK_SHUTDOWN_ACK: u8 = 8;
pub const CHUNK_ERROR: u8 = 9;
pub const CHUNK_COOKIE_ECHO: u8 = 10;
pub const CHUNK_COOKIE_ACK: u8 = 11;
pub const CHUNK_SHUTDOWN_COMPLETE: u8 = 14;

pub const DATA_FLAG_END: u8 = 0x01;
pub const DATA_FLAG_BEGIN: u8 = 0x02;
pub const DATA_FLAG_UNORDERED: u8 = 0x04;
pub const DATA_FLAG_IMMEDIATE: u8 = 0x08;

pub const SCTP_CRC32C_POLY: u32 = 0x82F6_3B78;

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

fn readU32Le(buf: []const u8, off: usize): u32 {
    const b0 = buf[off] as u32;
    const b1 = buf[off + 1] as u32;
    const b2 = buf[off + 2] as u32;
    const b3 = buf[off + 3] as u32;
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

fn writeU16Be(buf: []u8, off: usize, v: u16) {
    buf[off] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

fn writeU32Be(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn writeU32Le(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn crc32cUpdateByte(crc_in: u32, byte: u8): u32 {
    let crc = crc_in ^ (byte as u32);
    let bit: usize = 0;
    while (bit < 8) {
        if ((crc & 1) != 0) {
            crc = (crc >> 1) ^ SCTP_CRC32C_POLY;
        } else {
            crc = crc >> 1;
        }
        bit += 1;
    }
    return crc;
}

/// Computes raw Castagnoli CRC32C (reflected polynomial 0x82F63B78) over arbitrary bytes.
pub fn crc32c(data: []const u8): u32 {
    let crc: u32 = 0xFFFF_FFFF;
    let i: usize = 0;
    while (i < data.len) {
        crc = crc32cUpdateByte(crc, data[i]);
        i += 1;
    }
    return crc ^ 0xFFFF_FFFF;
}

/// Computes RFC 4960 SCTP CRC32C over a full SCTP packet, treating header checksum bytes [8..12] as 0x00000000.
pub fn sctpCrc32c(packet: []const u8): u32 {
    let crc: u32 = 0xFFFF_FFFF;
    let i: usize = 0;
    while (i < packet.len) {
        let b: u8 = packet[i];
        if (i >= 8 && i < 12) {
            b = 0;
        }
        crc = crc32cUpdateByte(crc, b);
        i += 1;
    }
    return crc ^ 0xFFFF_FFFF;
}

/// Verifies the Little-Endian CRC32C checksum stored at bytes [8..12] of an SCTP packet.
pub fn verifySctpChecksum(packet: []const u8): bool {
    if (packet.len < SCTP_HEADER_SIZE) {
        return false;
    }
    const stored = readU32Le(packet, 8);
    const computed = sctpCrc32c(packet);
    return stored == computed;
}

pub struct SctpHeader {
    src_port: u16,
    dst_port: u16,
    verification_tag: u32,
    checksum: u32,
}

pub struct SctpChunk {
    chunk_type: u8,
    flags: u8,
    length: u16,
    value: []const u8,
}

pub struct SctpChunkIter {
    data: []const u8,
    offset: usize,
    failed: bool,

    static fn init(chunks_slice: []const u8): SctpChunkIter {
        return SctpChunkIter{
            .data = chunks_slice,
            .offset = 0,
            .failed = false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn remaining(): usize {
        if (_.offset >= _.data.len) {
            return 0;
        }
        return _.data.len - _.offset;
    }

    fn next(*_): SctpChunk? {
        if (_.failed || _.offset >= _.data.len) {
            return null;
        }
        const avail = _.data.len - _.offset;
        if (avail < SCTP_CHUNK_HEADER_SIZE) {
            _.failed = true;
            return null;
        }

        const chunk_type = _.data[_.offset];
        const flags = _.data[_.offset + 1];
        const length = readU16Be(_.data, _.offset + 2);
        const chunk_len = length as usize;

        if (chunk_len < SCTP_CHUNK_HEADER_SIZE || chunk_len > avail) {
            _.failed = true;
            return null;
        }

        const value_start = _.offset + SCTP_CHUNK_HEADER_SIZE;
        const value_end = _.offset + chunk_len;
        const pad = (4 - (chunk_len % 4)) % 4;
        const padded_len = chunk_len + pad;

        if (padded_len <= avail) {
            _.offset += padded_len;
        } else if (value_end == _.data.len) {
            // RFC 4960 Section 3.2: last chunk may omit trailing padding bytes at EOF.
            _.offset = _.data.len;
        } else {
            // Truncated trailing padding before EOF.
            _.failed = true;
            return null;
        }

        return SctpChunk{
            .chunk_type = chunk_type,
            .flags = flags,
            .length = length,
            .value = _.data[value_start..value_end],
        };
    }
}

pub struct SctpPacket {
    header: SctpHeader,
    chunks_slice: []const u8,

    fn chunks(): SctpChunkIter {
        return SctpChunkIter.init(_.chunks_slice);
    }

    fn verifyChecksum(full_packet: []const u8): bool {
        if (full_packet.len < SCTP_HEADER_SIZE) {
            return false;
        }
        const computed = sctpCrc32c(full_packet);
        return computed == _.header.checksum && readU32Le(full_packet, 8) == computed;
    }
}

/// Parses an RFC 4960 12-byte SCTP common header and returns a zero-copy SctpPacket view.
pub fn parseSctpPacket(data: []const u8): SctpPacket? {
    if (data.len < SCTP_HEADER_SIZE) {
        return null;
    }
    const hdr = SctpHeader{
        .src_port = readU16Be(data, 0),
        .dst_port = readU16Be(data, 2),
        .verification_tag = readU32Be(data, 4),
        .checksum = readU32Le(data, 8),
    };
    return SctpPacket{
        .header = hdr,
        .chunks_slice = data[SCTP_HEADER_SIZE..],
    };
}

pub struct SctpDataChunk {
    flags: u8,
    tsn: u32,
    stream_id: u16,
    stream_seq: u16,
    ppid: u32,
    user_data: []const u8,

    fn isUnordered(): bool {
        return (_.flags & DATA_FLAG_UNORDERED) != 0;
    }

    fn isBegin(): bool {
        return (_.flags & DATA_FLAG_BEGIN) != 0;
    }

    fn isEnd(): bool {
        return (_.flags & DATA_FLAG_END) != 0;
    }

    fn isImmediate(): bool {
        return (_.flags & DATA_FLAG_IMMEDIATE) != 0;
    }
}

/// Decodes a DATA chunk (type 0). Requires at least 12 bytes of chunk value (16 bytes total chunk length).
pub fn parseSctpData(chunk: SctpChunk): SctpDataChunk? {
    if (chunk.chunk_type != CHUNK_DATA || chunk.value.len < 12) {
        return null;
    }
    return SctpDataChunk{
        .flags = chunk.flags,
        .tsn = readU32Be(chunk.value, 0),
        .stream_id = readU16Be(chunk.value, 4),
        .stream_seq = readU16Be(chunk.value, 6),
        .ppid = readU32Be(chunk.value, 8),
        .user_data = chunk.value[12..],
    };
}

pub struct SctpInitChunk {
    initiate_tag: u32,
    a_rwnd: u32,
    outbound_streams: u16,
    inbound_streams: u16,
    initial_tsn: u32,
    params_slice: []const u8,
}

/// Decodes an INIT (type 1) or INIT ACK (type 2) chunk. Requires at least 16 bytes of chunk value.
pub fn parseSctpInit(chunk: SctpChunk): SctpInitChunk? {
    if ((chunk.chunk_type != CHUNK_INIT && chunk.chunk_type != CHUNK_INIT_ACK) || chunk.value.len < 16) {
        return null;
    }
    return SctpInitChunk{
        .initiate_tag = readU32Be(chunk.value, 0),
        .a_rwnd = readU32Be(chunk.value, 4),
        .outbound_streams = readU16Be(chunk.value, 8),
        .inbound_streams = readU16Be(chunk.value, 10),
        .initial_tsn = readU32Be(chunk.value, 12),
        .params_slice = chunk.value[16..],
    };
}

pub struct SctpSackChunk {
    cumulative_tsn_ack: u32,
    a_rwnd: u32,
    num_gap_ack_blocks: u16,
    num_dup_tsns: u16,
    gap_blocks_slice: []const u8,
    dup_tsns_slice: []const u8,
}

/// Decodes a SACK chunk (type 3) and validates gap ack / duplicate TSN section bounds.
pub fn parseSctpSack(chunk: SctpChunk): SctpSackChunk? {
    if (chunk.chunk_type != CHUNK_SACK || chunk.value.len < 12) {
        return null;
    }
    const num_gaps = readU16Be(chunk.value, 8);
    const num_dups = readU16Be(chunk.value, 10);
    const gaps_bytes = (num_gaps as usize) * 4;
    const dups_bytes = (num_dups as usize) * 4;
    if (12 + gaps_bytes + dups_bytes > chunk.value.len) {
        return null;
    }
    const gaps_end = 12 + gaps_bytes;
    const dups_end = gaps_end + dups_bytes;
    return SctpSackChunk{
        .cumulative_tsn_ack = readU32Be(chunk.value, 0),
        .a_rwnd = readU32Be(chunk.value, 4),
        .num_gap_ack_blocks = num_gaps,
        .num_dup_tsns = num_dups,
        .gap_blocks_slice = chunk.value[12..gaps_end],
        .dup_tsns_slice = chunk.value[gaps_end..dups_end],
    };
}

pub struct SctpWriter {
    buf: []u8,
    pos: usize,
    valid: bool,

    fn init(buf: []u8, src_port: u16, dst_port: u16, vtag: u32): SctpWriter {
        if (buf.len < SCTP_HEADER_SIZE) {
            return SctpWriter{
                .buf = buf,
                .pos = 0,
                .valid = false,
            };
        }
        writeU16Be(buf, 0, src_port);
        writeU16Be(buf, 2, dst_port);
        writeU32Be(buf, 4, vtag);
        writeU32Le(buf, 8, 0);
        return SctpWriter{
            .buf = buf,
            .pos = SCTP_HEADER_SIZE,
            .valid = true,
        };
    }

    fn remaining(): usize {
        if (!_.valid || _.pos >= _.buf.len) {
            return 0;
        }
        return _.buf.len - _.pos;
    }

    fn writeChunk(*_, chunk_type: u8, flags: u8, value: []const u8): bool {
        if (!_.valid) {
            return false;
        }
        const chunk_len = SCTP_CHUNK_HEADER_SIZE + value.len;
        if (chunk_len > 65535) {
            return false;
        }
        const pad = (4 - (chunk_len % 4)) % 4;
        const total_len = chunk_len + pad;
        if (_.remaining() < total_len) {
            return false;
        }

        _.buf[_.pos] = chunk_type;
        _.buf[_.pos + 1] = flags;
        writeU16Be(_.buf, _.pos + 2, chunk_len as u16);

        let i: usize = 0;
        while (i < value.len) {
            _.buf[_.pos + SCTP_CHUNK_HEADER_SIZE + i] = value[i];
            i += 1;
        }
        let p: usize = 0;
        while (p < pad) {
            _.buf[_.pos + chunk_len + p] = 0;
            p += 1;
        }

        _.pos += total_len;
        return true;
    }

    fn writeDataChunk(
        *_,
        flags: u8,
        tsn: u32,
        stream_id: u16,
        stream_seq: u16,
        ppid: u32,
        user_data: []const u8,
    ): bool {
        if (!_.valid) {
            return false;
        }
        const chunk_len = SCTP_CHUNK_HEADER_SIZE + 12 + user_data.len;
        if (chunk_len > 65535) {
            return false;
        }
        const pad = (4 - (chunk_len % 4)) % 4;
        const total_len = chunk_len + pad;
        if (_.remaining() < total_len) {
            return false;
        }

        _.buf[_.pos] = CHUNK_DATA;
        _.buf[_.pos + 1] = flags;
        writeU16Be(_.buf, _.pos + 2, chunk_len as u16);
        writeU32Be(_.buf, _.pos + 4, tsn);
        writeU16Be(_.buf, _.pos + 8, stream_id);
        writeU16Be(_.buf, _.pos + 10, stream_seq);
        writeU32Be(_.buf, _.pos + 12, ppid);

        let i: usize = 0;
        while (i < user_data.len) {
            _.buf[_.pos + 16 + i] = user_data[i];
            i += 1;
        }
        let p: usize = 0;
        while (p < pad) {
            _.buf[_.pos + chunk_len + p] = 0;
            p += 1;
        }

        _.pos += total_len;
        return true;
    }

    fn finish(*_): ([]const u8)? {
        if (!_.valid || _.pos < SCTP_HEADER_SIZE) {
            return null;
        }
        writeU32Le(_.buf, 8, 0);
        const crc = sctpCrc32c(_.buf[0.._.pos]);
        writeU32Le(_.buf, 8, crc);
        return _.buf[0.._.pos];
    }
}
