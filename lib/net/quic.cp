// Source: cp/lib/quic/quic.cp (RFC 9000 QUIC Varint, Long/Short Headers & Wire Frames Codec)

const std = @import("std");

/// Maximum value encodable in an RFC 9000 variable-length integer (2^62 - 1).
pub const MAX_VARINT: u64 = 0x3FFFFFFFFFFFFFFF;

/// Maximum length in bytes of a QUIC Connection ID per RFC 9000 §17.2.
pub const MAX_CID_LEN: usize = 20;

/// Safety cap on the number of ACK ranges decoded from a single ACK frame.
pub const MAX_ACK_RANGES: usize = 1024;

/// QUIC version 1 (RFC 9000).
pub const QUIC_V1: u32 = 0x00000001;

/// QUIC version 2 (RFC 9369).
pub const QUIC_V2: u32 = 0x6b3343cf;

/// Errors returned by `cap:quic` varint, header, and frame codecs.
pub err QuicError {
    UnexpectedEof,
    VarintOutOfRange,
    InvalidHeader,
    InvalidFrameType,
    InvalidConnectionId,
    TruncatedFrame,
    TooManyRanges,
    OutOfMemory,
}

/// Result of decoding an RFC 9000 §16 variable-length integer.
pub class VarintDecoded {
    pub value: u64,
    pub len: usize,
}

/// Returns the encoded byte length (1, 2, 4, or 8) for a QUIC varint value.
pub fn varintLen(v: u64): usize ! QuicError {
    if (v <= 63) return 1;
    if (v <= 16383) return 2;
    if (v <= 1073741823) return 4;
    if (v <= MAX_VARINT) return 8;
    return QuicError.VarintOutOfRange;
}

/// Appends an RFC 9000 §16 variable-length integer to `list`.
pub fn writeVarint(list: *std.ArrayList(u8), alloc: std.mem.Allocator, v: u64): void ! QuicError {
    const n = varintLen(v)?;
    if (n == 1) {
        list.append(alloc, @intCast(v)) catch return QuicError.OutOfMemory;
        return;
    }
    if (n == 2) {
        list.append(alloc, @as(u8, 0x40) | @as(u8, @intCast((v >> 8) & 0x3F))) catch return QuicError.OutOfMemory;
        list.append(alloc, @intCast(v & 0xFF)) catch return QuicError.OutOfMemory;
        return;
    }
    if (n == 4) {
        list.append(alloc, @as(u8, 0x80) | @as(u8, @intCast((v >> 24) & 0x3F))) catch return QuicError.OutOfMemory;
        list.append(alloc, @intCast((v >> 16) & 0xFF)) catch return QuicError.OutOfMemory;
        list.append(alloc, @intCast((v >> 8) & 0xFF)) catch return QuicError.OutOfMemory;
        list.append(alloc, @intCast(v & 0xFF)) catch return QuicError.OutOfMemory;
        return;
    }
    list.append(alloc, @as(u8, 0xC0) | @as(u8, @intCast((v >> 56) & 0x3F))) catch return QuicError.OutOfMemory;
    list.append(alloc, @intCast((v >> 48) & 0xFF)) catch return QuicError.OutOfMemory;
    list.append(alloc, @intCast((v >> 40) & 0xFF)) catch return QuicError.OutOfMemory;
    list.append(alloc, @intCast((v >> 32) & 0xFF)) catch return QuicError.OutOfMemory;
    list.append(alloc, @intCast((v >> 24) & 0xFF)) catch return QuicError.OutOfMemory;
    list.append(alloc, @intCast((v >> 16) & 0xFF)) catch return QuicError.OutOfMemory;
    list.append(alloc, @intCast((v >> 8) & 0xFF)) catch return QuicError.OutOfMemory;
    list.append(alloc, @intCast(v & 0xFF)) catch return QuicError.OutOfMemory;
}

/// Encodes `v` into a newly allocated RFC 9000 §16 variable-length integer buffer.
pub fn encodeVarint(allocator: std.mem.Allocator, v: u64): ([]u8) ! QuicError {
    let list: std.ArrayList(u8) = .empty;
    errdefer list.deinit(allocator);
    writeVarint(&list, allocator, v)?;
    return list.toOwnedSlice(allocator) catch return QuicError.OutOfMemory;
}

/// Decodes an RFC 9000 §16 variable-length integer from the start of `bytes`.
pub fn decodeVarint(bytes: []const u8): VarintDecoded ! QuicError {
    if (bytes.len == 0) return QuicError.UnexpectedEof;
    const first = bytes[0];
    const prefix = first >> 6;
    const byte_len: usize = @as(usize, 1) << @intCast(prefix);
    if (bytes.len < byte_len) return QuicError.UnexpectedEof;

    let val: u64 = @as(u64, first & 0x3F);
    let i: usize = 1;
    while (i < byte_len) {
        val = (val << 8) | @as(u64, bytes[i]);
        i += 1;
    }
    return VarintDecoded {
        value: val,
        len: byte_len,
    };
}

/// RFC 9000 §17.2 Long Header packet types.
pub enum LongPacketType: u2 {
    Initial = 0,
    ZeroRtt = 1,
    Handshake = 2,
    Retry = 3,
}

/// RFC 9000 §17.2 QUIC Long Header.
pub class LongHeader {
    pub packet_type: LongPacketType,
    pub version: u32,
    pub dcid: []const u8,
    pub scid: []const u8,
    pub token: ([]const u8)? = null,
    pub length: u64 = 0,
    pub packet_number_len: u8 = 1,
}

/// RFC 9000 §17.3 QUIC Short Header (1-RTT).
pub class ShortHeader {
    pub spin_bit: bool = false,
    pub key_phase: bool = false,
    pub dcid: []const u8,
    pub packet_number_len: u8 = 1,
}

fn writeU32Be(list: *std.ArrayList(u8), alloc: std.mem.Allocator, v: u32): void ! QuicError {
    list.append(alloc, @intCast((v >> 24) & 0xFF)) catch return QuicError.OutOfMemory;
    list.append(alloc, @intCast((v >> 16) & 0xFF)) catch return QuicError.OutOfMemory;
    list.append(alloc, @intCast((v >> 8) & 0xFF)) catch return QuicError.OutOfMemory;
    list.append(alloc, @intCast(v & 0xFF)) catch return QuicError.OutOfMemory;
}

fn readU32Be(bytes: []const u8): u32 {
    return (@as(u32, bytes[0]) << 24) |
        (@as(u32, bytes[1]) << 16) |
        (@as(u32, bytes[2]) << 8) |
        @as(u32, bytes[3]);
}

/// Encodes a QUIC Long Header into a newly allocated wire buffer.
pub fn encodeLongHeader(allocator: std.mem.Allocator, hdr: LongHeader): ([]u8) ! QuicError {
    if (hdr.dcid.len > MAX_CID_LEN or hdr.scid.len > MAX_CID_LEN) {
        return QuicError.InvalidConnectionId;
    }
    if (hdr.packet_number_len < 1 or hdr.packet_number_len > 4) {
        return QuicError.InvalidHeader;
    }

    let list: std.ArrayList(u8) = .empty;
    errdefer list.deinit(allocator);

    const ptype_bits: u8 = @as(u8, @intFromEnum(hdr.packet_type)) << 4;
    let pn_bits: u8 = 0;
    if (hdr.packet_type != .Retry) {
        pn_bits = @intCast((hdr.packet_number_len - 1) & 0x03);
    }
    const first_byte: u8 = @as(u8, 0xC0) | ptype_bits | pn_bits;
    list.append(allocator, first_byte) catch return QuicError.OutOfMemory;

    writeU32Be(&list, allocator, hdr.version)?;

    list.append(allocator, @intCast(hdr.dcid.len)) catch return QuicError.OutOfMemory;
    list.appendSlice(allocator, hdr.dcid) catch return QuicError.OutOfMemory;

    list.append(allocator, @intCast(hdr.scid.len)) catch return QuicError.OutOfMemory;
    list.appendSlice(allocator, hdr.scid) catch return QuicError.OutOfMemory;

    switch (hdr.packet_type) {
        .Initial => {
            if (tok := hdr.token) {
                writeVarint(&list, allocator, @intCast(tok.len))?;
                list.appendSlice(allocator, tok) catch return QuicError.OutOfMemory;
            } else {
                writeVarint(&list, allocator, 0)?;
            }
            writeVarint(&list, allocator, hdr.length)?;
        },
        .ZeroRtt, .Handshake => {
            writeVarint(&list, allocator, hdr.length)?;
        },
        .Retry => {
            if (tok := hdr.token) {
                list.appendSlice(allocator, tok) catch return QuicError.OutOfMemory;
            }
        },
    }

    return list.toOwnedSlice(allocator) catch return QuicError.OutOfMemory;
}

/// Decodes a QUIC Long Header from `bytes`. Slices in the returned header borrow from `bytes`.
pub fn decodeLongHeader(bytes: []const u8): LongHeader ! QuicError {
    if (bytes.len < 7) return QuicError.UnexpectedEof;
    const first = bytes[0];
    if ((first & 0x80) == 0 or (first & 0x40) == 0) {
        return QuicError.InvalidHeader;
    }

    const ptype_raw: u2 = @intCast((first >> 4) & 0x03);
    const packet_type: LongPacketType = @enumFromInt(ptype_raw);
    const pn_len: u8 = (first & 0x03) + 1;

    const version = readU32Be(bytes[1..5]);
    let pos: usize = 5;

    const dcid_len: usize = @as(usize, bytes[pos]);
    pos += 1;
    if (dcid_len > MAX_CID_LEN) return QuicError.InvalidConnectionId;
    if (bytes.len < pos + dcid_len) return QuicError.UnexpectedEof;
    const dcid = bytes[pos .. pos + dcid_len];
    pos += dcid_len;

    if (bytes.len < pos + 1) return QuicError.UnexpectedEof;
    const scid_len: usize = @as(usize, bytes[pos]);
    pos += 1;
    if (scid_len > MAX_CID_LEN) return QuicError.InvalidConnectionId;
    if (bytes.len < pos + scid_len) return QuicError.UnexpectedEof;
    const scid = bytes[pos .. pos + scid_len];
    pos += scid_len;

    let token: ([]const u8)? = null;
    let length: u64 = 0;

    switch (packet_type) {
        .Initial => {
            const tok_len_dec = decodeVarint(bytes[pos..])?;
            pos += tok_len_dec.len;
            const tok_len: usize = @intCast(tok_len_dec.value);
            if (bytes.len < pos + tok_len) return QuicError.UnexpectedEof;
            if (tok_len > 0) {
                token = bytes[pos .. pos + tok_len];
            }
            pos += tok_len;

            const pkt_len_dec = decodeVarint(bytes[pos..])?;
            length = pkt_len_dec.value;
        },
        .ZeroRtt, .Handshake => {
            const pkt_len_dec = decodeVarint(bytes[pos..])?;
            length = pkt_len_dec.value;
        },
        .Retry => {
            if (pos < bytes.len) {
                token = bytes[pos..bytes.len];
            }
        },
    }

    return LongHeader {
        packet_type: packet_type,
        version: version,
        dcid: dcid,
        scid: scid,
        token: token,
        length: length,
        packet_number_len: pn_len,
    };
}

/// Encodes a QUIC Short Header (1-RTT) into a newly allocated wire buffer.
pub fn encodeShortHeader(allocator: std.mem.Allocator, hdr: ShortHeader): ([]u8) ! QuicError {
    if (hdr.dcid.len > MAX_CID_LEN) {
        return QuicError.InvalidConnectionId;
    }
    if (hdr.packet_number_len < 1 or hdr.packet_number_len > 4) {
        return QuicError.InvalidHeader;
    }

    const total = 1 + hdr.dcid.len;
    let out = allocator.alloc(u8, total) catch return QuicError.OutOfMemory;
    let first: u8 = @as(u8, 0x40) | ((hdr.packet_number_len - 1) & 0x03);
    if (hdr.spin_bit) first |= 0x20;
    if (hdr.key_phase) first |= 0x04;
    out[0] = first;
    if (hdr.dcid.len > 0) {
        @memcpy(out[1..total], hdr.dcid);
    }
    return out;
}

/// Decodes a QUIC Short Header (1-RTT) from `bytes` given the connection's `dcid_len`.
pub fn decodeShortHeader(bytes: []const u8, dcid_len: usize): ShortHeader ! QuicError {
    if (dcid_len > MAX_CID_LEN) return QuicError.InvalidConnectionId;
    if (bytes.len < 1 + dcid_len) return QuicError.UnexpectedEof;

    const first = bytes[0];
    if ((first & 0x80) != 0 or (first & 0x40) == 0) {
        return QuicError.InvalidHeader;
    }

    return ShortHeader {
        spin_bit: (first & 0x20) != 0,
        key_phase: (first & 0x04) != 0,
        dcid: bytes[1 .. 1 + dcid_len],
        packet_number_len: (first & 0x03) + 1,
    };
}

const EMPTY_ACK_RANGES: [0]AckRange = [];

/// A single Gap + ACK Range Length entry in an RFC 9000 §19.3 ACK frame.
pub class AckRange {
    pub gap: u64,
    pub ack_range_len: u64,
}

/// RFC 9000 §19.3 ACK Frame.
pub class AckFrame {
    pub largest_acked: u64,
    pub ack_delay: u64,
    pub first_ack_range: u64,
    pub ranges: []AckRange = @constCast(&EMPTY_ACK_RANGES),
}

/// RFC 9000 §19.6 CRYPTO Frame.
pub class CryptoFrame {
    pub offset: u64,
    pub data: []const u8,
}

/// RFC 9000 §19.8 STREAM Frame.
pub class StreamFrame {
    pub stream_id: u64,
    pub offset: u64 = 0,
    pub length: u64 = 0,
    pub fin: bool = false,
    pub data: []const u8,
}

/// RFC 9000 §19.4 RESET_STREAM Frame.
pub class ResetStreamFrame {
    pub stream_id: u64,
    pub error_code: u64,
    pub final_size: u64,
}

/// RFC 9000 §19.19 CONNECTION_CLOSE Frame.
pub class ConnectionCloseFrame {
    pub is_app_error: bool = false,
    pub error_code: u64,
    pub frame_type: u64 = 0,
    pub reason_phrase: []const u8 = "",
}

/// Tagged union of RFC 9000 §19 QUIC wire frames.
pub enum QuicFrame {
    padding: usize,
    ping: void,
    ack: AckFrame,
    crypto: CryptoFrame,
    stream: StreamFrame,
    reset_stream: ResetStreamFrame,
    max_data: u64,
    connection_close: ConnectionCloseFrame,
}

/// Encodes a single `QuicFrame` into a newly allocated wire buffer.
pub fn encodeFrame(allocator: std.mem.Allocator, frame: QuicFrame): ([]u8) ! QuicError {
    let list: std.ArrayList(u8) = .empty;
    errdefer list.deinit(allocator);

    switch (frame) {
        .padding => |count| {
            let i: usize = 0;
            while (i < count) {
                list.append(allocator, 0x00) catch return QuicError.OutOfMemory;
                i += 1;
            }
        },
        .ping => {
            list.append(allocator, 0x01) catch return QuicError.OutOfMemory;
        },
        .ack => |ack| {
            list.append(allocator, 0x02) catch return QuicError.OutOfMemory;
            writeVarint(&list, allocator, ack.largest_acked)?;
            writeVarint(&list, allocator, ack.ack_delay)?;
            writeVarint(&list, allocator, @intCast(ack.ranges.len))?;
            writeVarint(&list, allocator, ack.first_ack_range)?;
            for (r in ack.ranges) {
                writeVarint(&list, allocator, r.gap)?;
                writeVarint(&list, allocator, r.ack_range_len)?;
            }
        },
        .reset_stream => |rst| {
            list.append(allocator, 0x04) catch return QuicError.OutOfMemory;
            writeVarint(&list, allocator, rst.stream_id)?;
            writeVarint(&list, allocator, rst.error_code)?;
            writeVarint(&list, allocator, rst.final_size)?;
        },
        .crypto => |cry| {
            list.append(allocator, 0x06) catch return QuicError.OutOfMemory;
            writeVarint(&list, allocator, cry.offset)?;
            writeVarint(&list, allocator, @intCast(cry.data.len))?;
            list.appendSlice(allocator, cry.data) catch return QuicError.OutOfMemory;
        },
        .stream => |st| {
            let ftype: u8 = 0x08 | 0x02; // always set LEN bit for safe framing
            if (st.offset > 0) ftype |= 0x04;
            if (st.fin) ftype |= 0x01;
            list.append(allocator, ftype) catch return QuicError.OutOfMemory;
            writeVarint(&list, allocator, st.stream_id)?;
            if (st.offset > 0) {
                writeVarint(&list, allocator, st.offset)?;
            }
            writeVarint(&list, allocator, @intCast(st.data.len))?;
            list.appendSlice(allocator, st.data) catch return QuicError.OutOfMemory;
        },
        .max_data => |max_d| {
            list.append(allocator, 0x10) catch return QuicError.OutOfMemory;
            writeVarint(&list, allocator, max_d)?;
        },
        .connection_close => |cc| {
            const ftype: u8 = if (cc.is_app_error) 0x1d else 0x1c;
            list.append(allocator, ftype) catch return QuicError.OutOfMemory;
            writeVarint(&list, allocator, cc.error_code)?;
            if (!cc.is_app_error) {
                writeVarint(&list, allocator, cc.frame_type)?;
            }
            writeVarint(&list, allocator, @intCast(cc.reason_phrase.len))?;
            list.appendSlice(allocator, cc.reason_phrase) catch return QuicError.OutOfMemory;
        },
    }

    return list.toOwnedSlice(allocator) catch return QuicError.OutOfMemory;
}

fn freeFramesList(allocator: std.mem.Allocator, frames: []QuicFrame) {
    for (f in frames) {
        switch (f) {
            .ack => |ack| {
                if (ack.ranges.len > 0) {
                    allocator.free(ack.ranges);
                }
            },
            else => {},
        }
    }
}

/// Frees a slice of `QuicFrame`s returned by `decodeFrames`.
pub fn freeFrames(allocator: std.mem.Allocator, frames: []QuicFrame) {
    freeFramesList(allocator, frames);
    allocator.free(frames);
}

fn decodeVarintTrunc(bytes: []const u8): VarintDecoded ! QuicError {
    return decodeVarint(bytes) catch |e| {
        if (e == QuicError.UnexpectedEof) return QuicError.TruncatedFrame;
        return e;
    };
}

/// Decodes all QUIC frames from `payload`. Data slices borrow from `payload`.
/// Call `freeFrames(allocator, frames)` to release allocated frame and ACK-range slices.
pub fn decodeFrames(allocator: std.mem.Allocator, payload: []const u8): ([]QuicFrame) ! QuicError {
    let frames: std.ArrayList(QuicFrame) = .empty;
    errdefer {
        freeFramesList(allocator, frames.items);
        frames.deinit(allocator);
    }

    let pos: usize = 0;
    while (pos < payload.len) {
        const ftype_dec = decodeVarint(payload[pos..])?;
        pos += ftype_dec.len;
        const ftype = ftype_dec.value;

        if (ftype == 0x00) {
            let pad_count: usize = 1;
            while (pos < payload.len and payload[pos] == 0x00) {
                pad_count += 1;
                pos += 1;
            }
            frames.append(allocator, QuicFrame { padding: pad_count }) catch return QuicError.OutOfMemory;
            continue;
        }

        if (ftype == 0x01) {
            frames.append(allocator, QuicFrame.ping) catch return QuicError.OutOfMemory;
            continue;
        }

        if (ftype == 0x02 or ftype == 0x03) {
            const largest_dec = decodeVarintTrunc(payload[pos..])?;
            pos += largest_dec.len;
            const delay_dec = decodeVarintTrunc(payload[pos..])?;
            pos += delay_dec.len;
            const count_dec = decodeVarintTrunc(payload[pos..])?;
            pos += count_dec.len;
            if (count_dec.value > @as(u64, MAX_ACK_RANGES)) {
                return QuicError.TooManyRanges;
            }
            const first_dec = decodeVarintTrunc(payload[pos..])?;
            pos += first_dec.len;

            const range_count: usize = @intCast(count_dec.value);
            let ranges: []AckRange = unsafe { @constCast(&EMPTY_ACK_RANGES) };
            if (range_count > 0) {
                ranges = allocator.alloc(AckRange, range_count) catch return QuicError.OutOfMemory;
                errdefer allocator.free(ranges);

                let i: usize = 0;
                while (i < range_count) {
                    const gap_dec = decodeVarintTrunc(payload[pos..])?;
                    pos += gap_dec.len;
                    const len_dec = decodeVarintTrunc(payload[pos..])?;
                    pos += len_dec.len;
                    ranges[i] = AckRange {
                        gap: gap_dec.value,
                        ack_range_len: len_dec.value,
                    };
                    i += 1;
                }
            }

            if (ftype == 0x03) {
                // Skip ECN counts (ECT0, ECT1, ECN-CE)
                const e0 = decodeVarintTrunc(payload[pos..])?;
                pos += e0.len;
                const e1 = decodeVarintTrunc(payload[pos..])?;
                pos += e1.len;
                const ece = decodeVarintTrunc(payload[pos..])?;
                pos += ece.len;
            }

            frames.append(allocator, QuicFrame {
                ack: AckFrame {
                    largest_acked: largest_dec.value,
                    ack_delay: delay_dec.value,
                    first_ack_range: first_dec.value,
                    ranges: ranges,
                },
            }) catch return QuicError.OutOfMemory;
            continue;
        }

        if (ftype == 0x04) {
            const sid_dec = decodeVarintTrunc(payload[pos..])?;
            pos += sid_dec.len;
            const err_dec = decodeVarintTrunc(payload[pos..])?;
            pos += err_dec.len;
            const fin_dec = decodeVarintTrunc(payload[pos..])?;
            pos += fin_dec.len;

            frames.append(allocator, QuicFrame {
                reset_stream: ResetStreamFrame {
                    stream_id: sid_dec.value,
                    error_code: err_dec.value,
                    final_size: fin_dec.value,
                },
            }) catch return QuicError.OutOfMemory;
            continue;
        }

        if (ftype == 0x06) {
            const off_dec = decodeVarintTrunc(payload[pos..])?;
            pos += off_dec.len;
            const len_dec = decodeVarintTrunc(payload[pos..])?;
            pos += len_dec.len;
            const dlen: usize = @intCast(len_dec.value);
            if (payload.len - pos < dlen) return QuicError.TruncatedFrame;
            const data = payload[pos .. pos + dlen];
            pos += dlen;

            frames.append(allocator, QuicFrame {
                crypto: CryptoFrame {
                    offset: off_dec.value,
                    data: data,
                },
            }) catch return QuicError.OutOfMemory;
            continue;
        }

        if (ftype >= 0x08 and ftype <= 0x0f) {
            const has_off = (ftype & 0x04) != 0;
            const has_len = (ftype & 0x02) != 0;
            const is_fin = (ftype & 0x01) != 0;

            const sid_dec = decodeVarintTrunc(payload[pos..])?;
            pos += sid_dec.len;

            let offset: u64 = 0;
            if (has_off) {
                const off_dec = decodeVarintTrunc(payload[pos..])?;
                pos += off_dec.len;
                offset = off_dec.value;
            }

            let data: []const u8 = "";
            let dlen_u64: u64 = 0;
            if (has_len) {
                const len_dec = decodeVarintTrunc(payload[pos..])?;
                pos += len_dec.len;
                dlen_u64 = len_dec.value;
                const dlen: usize = @intCast(dlen_u64);
                if (payload.len - pos < dlen) return QuicError.TruncatedFrame;
                data = payload[pos .. pos + dlen];
                pos += dlen;
            } else {
                data = payload[pos..payload.len];
                dlen_u64 = @intCast(data.len);
                pos = payload.len;
            }

            frames.append(allocator, QuicFrame {
                stream: StreamFrame {
                    stream_id: sid_dec.value,
                    offset: offset,
                    length: dlen_u64,
                    fin: is_fin,
                    data: data,
                },
            }) catch return QuicError.OutOfMemory;
            continue;
        }

        if (ftype == 0x10) {
            const max_dec = decodeVarintTrunc(payload[pos..])?;
            pos += max_dec.len;
            frames.append(allocator, QuicFrame { max_data: max_dec.value }) catch return QuicError.OutOfMemory;
            continue;
        }

        if (ftype == 0x1c or ftype == 0x1d) {
            const is_app = (ftype == 0x1d);
            const err_dec = decodeVarintTrunc(payload[pos..])?;
            pos += err_dec.len;
            let trigger_ftype: u64 = 0;
            if (!is_app) {
                const ft_dec = decodeVarintTrunc(payload[pos..])?;
                pos += ft_dec.len;
                trigger_ftype = ft_dec.value;
            }
            const rlen_dec = decodeVarintTrunc(payload[pos..])?;
            pos += rlen_dec.len;
            const rlen: usize = @intCast(rlen_dec.value);
            if (payload.len - pos < rlen) return QuicError.TruncatedFrame;
            const reason = payload[pos .. pos + rlen];
            pos += rlen;

            frames.append(allocator, QuicFrame {
                connection_close: ConnectionCloseFrame {
                    is_app_error: is_app,
                    error_code: err_dec.value,
                    frame_type: trigger_ftype,
                    reason_phrase: reason,
                },
            }) catch return QuicError.OutOfMemory;
            continue;
        }

        return QuicError.InvalidFrameType;
    }

    return frames.toOwnedSlice(allocator) catch return QuicError.OutOfMemory;
}
