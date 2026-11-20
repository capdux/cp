// Source: cp/lib/rtp/rtp.cp (RFC 3550 RTP/RTCP Packet Codec & Sequence Jitter Buffer)

const std = @import("std");

/// Fixed RTP v2 header size in bytes (without CSRC or extension).
pub const RTP_HEADER_SIZE: usize = 12;

/// Current RTP protocol version (RFC 3550 Section 5.1).
pub const RTP_VERSION: u8 = 2;

/// Errors returned by `cap:rtp` wire encoding, decoding, and jitter buffering.
pub err RtpError {
    InvalidVersion,
    TruncatedPacket,
    InvalidPadding,
    InvalidPayloadType,
    BufferFull,
    OutOfMemory,
}

/// RTP Header Extension (RFC 3550 Section 5.3.1).
pub class RtpExtension {
    pub profile: u16,
    pub data: []const u8,
}

/// Decoded or constructible RFC 3550 RTP Packet.
/// Note: `decode()` allocates `csrcs` when `cc > 0`; `extension.data` and `payload`
/// are zero-copy slices into the source wire buffer. Call `deinit(allocator)` to free `csrcs`.
pub class RtpPacket {
    pub version: u8 = 2,
    pub padding: bool = false,
    pub marker: bool = false,
    pub payload_type: u7 = 0,
    pub sequence_number: u16 = 0,
    pub timestamp: u32 = 0,
    pub ssrc: u32 = 0,
    pub csrcs: []const u32 = [],
    pub extension: RtpExtension? = null,
    pub payload: []const u8 = [],

    /// Releases the owned `csrcs` slice allocated by `decode()`.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.csrcs.len > 0) {
            allocator.free(_.csrcs);
        }
        _.csrcs = _.csrcs[0..0];
    }

    /// Encodes this RTP packet into a newly allocated big-endian wire buffer.
    pub fn encode(allocator: std.mem.Allocator): ([]u8) ! RtpError {
        if (_.version != RTP_VERSION) {
            return RtpError.InvalidVersion;
        }
        if (_.csrcs.len > 15) {
            return RtpError.BufferFull;
        }

        const cc: usize = _.csrcs.len;
        let ext_words: usize = 0;
        let ext_padded_len: usize = 0;
        let has_ext: bool = false;
        if (ext := _.extension) {
            has_ext = true;
            ext_words = (ext.data.len + 3) / 4;
            if (ext_words > 65535) {
                return RtpError.BufferFull;
            }
            ext_padded_len = ext_words * 4;
        }

        const base_len: usize = RTP_HEADER_SIZE
            + (cc * 4)
            + (if (has_ext) (4 + ext_padded_len) else 0)
            + _.payload.len;

        let pad_len: usize = 0;
        if (_.padding) {
            // Pad to 4-byte boundary (1..4 bytes), last byte holds pad_len.
            pad_len = (4 as usize) - (base_len % (4 as usize));
        }

        const total_len = base_len + pad_len;
        let out = allocator.alloc(u8, total_len) catch return RtpError.OutOfMemory;
        errdefer allocator.free(out);

        const p_bit: u8 = if (_.padding) 1 else 0;
        const x_bit: u8 = if (has_ext) 1 else 0;
        const m_bit: u8 = if (_.marker) 1 else 0;

        out[0] = ((RTP_VERSION & 0x03) << 6)
            | ((p_bit & 0x01) << 5)
            | ((x_bit & 0x01) << 4)
            | ((cc as u8) & 0x0F);
        out[1] = ((m_bit & 0x01) << 7) | ((_.payload_type as u8) & 0x7F);
        std.mem.writeInt(u16, out[2..4], _.sequence_number, .big);
        std.mem.writeInt(u32, out[4..8], _.timestamp, .big);
        std.mem.writeInt(u32, out[8..12], _.ssrc, .big);

        let offset: usize = RTP_HEADER_SIZE;
        let i: usize = 0;
        while (i < cc) : (i += 1) {
            std.mem.writeInt(u32, out[offset .. offset + 4][0..4], _.csrcs[i], .big);
            offset += 4;
        }

        if (ext := _.extension) {
            std.mem.writeInt(u16, out[offset .. offset + 2][0..2], ext.profile, .big);
            std.mem.writeInt(u16, out[offset + 2 .. offset + 4][0..2], ext_words as u16, .big);
            offset += 4;
            if (ext.data.len > 0) {
                @memcpy(out[offset .. offset + ext.data.len], ext.data);
            }
            if (ext_padded_len > ext.data.len) {
                @memset(out[offset + ext.data.len .. offset + ext_padded_len], 0);
            }
            offset += ext_padded_len;
        }

        if (_.payload.len > 0) {
            @memcpy(out[offset .. offset + _.payload.len], _.payload);
            offset += _.payload.len;
        }

        if (_.padding) {
            if (pad_len > 1) {
                @memset(out[offset .. offset + pad_len - 1], 0);
            }
            out[total_len - 1] = pad_len as u8;
        }

        return out;
    }

    /// Decodes an RTP v2 packet from `bytes`, validating version, CSRC bounds,
    /// extension bounds, and padding count (`1..remaining`).
    pub static fn decode(allocator: std.mem.Allocator, bytes: []const u8): RtpPacket ! RtpError {
        if (bytes.len < RTP_HEADER_SIZE) {
            return RtpError.TruncatedPacket;
        }

        const version: u8 = (bytes[0] >> 6) & 0x03;
        if (version != RTP_VERSION) {
            return RtpError.InvalidVersion;
        }

        const padding: bool = ((bytes[0] >> 5) & 0x01) != 0;
        const has_ext: bool = ((bytes[0] >> 4) & 0x01) != 0;
        const cc: usize = (bytes[0] & 0x0F) as usize;
        const marker: bool = ((bytes[1] >> 7) & 0x01) != 0;
        const payload_type: u7 = (bytes[1] & 0x7F) as u7;
        const sequence_number = std.mem.readInt(u16, bytes[2..4], .big);
        const timestamp = std.mem.readInt(u32, bytes[4..8], .big);
        const ssrc = std.mem.readInt(u32, bytes[8..12], .big);

        let offset: usize = RTP_HEADER_SIZE;
        const csrc_bytes: usize = cc * 4;
        if (offset + csrc_bytes > bytes.len) {
            return RtpError.TruncatedPacket;
        }

        let csrcs_buf: []u32 = [];
        if (cc > 0) {
            csrcs_buf = allocator.alloc(u32, cc) catch return RtpError.OutOfMemory;
            let i: usize = 0;
            while (i < cc) : (i += 1) {
                csrcs_buf[i] = std.mem.readInt(u32, bytes[offset .. offset + 4][0..4], .big);
                offset += 4;
            }
        }
        errdefer {
            if (csrcs_buf.len > 0) {
                allocator.free(csrcs_buf);
            }
        }

        let ext_opt: RtpExtension? = null;
        if (has_ext) {
            if (offset + 4 > bytes.len) {
                return RtpError.TruncatedPacket;
            }
            const profile = std.mem.readInt(u16, bytes[offset .. offset + 2][0..2], .big);
            const ext_words = std.mem.readInt(u16, bytes[offset + 2 .. offset + 4][0..2], .big) as usize;
            offset += 4;
            const ext_len = ext_words * 4;
            if (offset + ext_len > bytes.len) {
                return RtpError.TruncatedPacket;
            }
            ext_opt = RtpExtension {
                profile: profile,
                data: bytes[offset .. offset + ext_len],
            };
            offset += ext_len;
        }

        let payload_end: usize = bytes.len;
        if (padding) {
            const remaining = bytes.len - offset;
            if (remaining == 0) {
                return RtpError.InvalidPadding;
            }
            const pad_len: usize = bytes[bytes.len - 1] as usize;
            if (pad_len == 0 or pad_len > remaining) {
                return RtpError.InvalidPadding;
            }
            payload_end = bytes.len - pad_len;
        }

        return RtpPacket {
            version: version,
            padding: padding,
            marker: marker,
            payload_type: payload_type,
            sequence_number: sequence_number,
            timestamp: timestamp,
            ssrc: ssrc,
            csrcs: csrcs_buf,
            extension: ext_opt,
            payload: bytes[offset .. payload_end],
        };
    }
}

/// RTCP Packet Types (RFC 3550 Section 12.1).
pub enum RtcpType: u8 {
    SenderReport = 200,
    ReceiverReport = 201,
    Sdes = 202,
    Bye = 203,
}

/// RTCP Sender Report (PT=200, RFC 3550 Section 6.4.1).
pub class RtcpSenderReport {
    pub ssrc: u32,
    pub ntp_msw: u32,
    pub ntp_lsw: u32,
    pub rtp_timestamp: u32,
    pub packet_count: u32,
    pub octet_count: u32,

    /// Encodes a 28-byte RTCP Sender Report (`RC=0`, `length=6` 32-bit words minus 1).
    pub fn encode(allocator: std.mem.Allocator): ([]u8) ! RtpError {
        let out = allocator.alloc(u8, 28) catch return RtpError.OutOfMemory;
        out[0] = (RTP_VERSION & 0x03) << 6;
        out[1] = RtcpType.SenderReport as u8;
        // (6 + 1) * 4 = 28 bytes
        std.mem.writeInt(u16, out[2..4], 6, .big);
        std.mem.writeInt(u32, out[4..8], _.ssrc, .big);
        std.mem.writeInt(u32, out[8..12], _.ntp_msw, .big);
        std.mem.writeInt(u32, out[12..16], _.ntp_lsw, .big);
        std.mem.writeInt(u32, out[16..20], _.rtp_timestamp, .big);
        std.mem.writeInt(u32, out[20..24], _.packet_count, .big);
        std.mem.writeInt(u32, out[24..28], _.octet_count, .big);
        return out;
    }

    /// Decodes an RTCP Sender Report from `bytes`, verifying `V=2`, `PT=200`, and `(len_words + 1) * 4`.
    pub static fn decode(bytes: []const u8): RtcpSenderReport ! RtpError {
        if (bytes.len < 4) {
            return RtpError.TruncatedPacket;
        }
        const version: u8 = (bytes[0] >> 6) & 0x03;
        if (version != RTP_VERSION) {
            return RtpError.InvalidVersion;
        }
        if (bytes[1] != (RtcpType.SenderReport as u8)) {
            return RtpError.InvalidPayloadType;
        }
        const len_words = std.mem.readInt(u16, bytes[2..4], .big) as usize;
        const total_bytes = (len_words + 1) * 4;
        if (total_bytes < 28 or bytes.len < total_bytes) {
            return RtpError.TruncatedPacket;
        }

        return RtcpSenderReport {
            ssrc: std.mem.readInt(u32, bytes[4..8], .big),
            ntp_msw: std.mem.readInt(u32, bytes[8..12], .big),
            ntp_lsw: std.mem.readInt(u32, bytes[12..16], .big),
            rtp_timestamp: std.mem.readInt(u32, bytes[16..20], .big),
            packet_count: std.mem.readInt(u32, bytes[20..24], .big),
            octet_count: std.mem.readInt(u32, bytes[24..28], .big),
        };
    }
}

/// RTCP BYE Packet (PT=203, RFC 3550 Section 6.6).
pub class RtcpBye {
    pub ssrc_list: []u32 = [],
    pub reason: ([]const u8)? = null,

    /// Releases the owned `ssrc_list` slice allocated by `decode()`.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.ssrc_list.len > 0) {
            allocator.free(_.ssrc_list);
        }
        _.ssrc_list = _.ssrc_list[0..0];
    }

    /// Encodes an RTCP BYE packet with 32-bit word length `(len_words + 1) * 4`.
    pub fn encode(allocator: std.mem.Allocator): ([]u8) ! RtpError {
        if (_.ssrc_list.len > 31) {
            return RtpError.BufferFull;
        }
        const sc: usize = _.ssrc_list.len;
        let reason_bytes: usize = 0;
        if (r := _.reason) {
            if (r.len > 255) {
                return RtpError.BufferFull;
            }
            reason_bytes = (1 as usize) + r.len;
        }

        const raw_len: usize = (4 as usize) + (sc * (4 as usize)) + reason_bytes;
        const padded_len: usize = ((raw_len + (3 as usize)) / (4 as usize)) * (4 as usize);
        const len_words: u16 = ((padded_len / (4 as usize)) - (1 as usize)) as u16;

        let out = allocator.alloc(u8, padded_len) catch return RtpError.OutOfMemory;
        @memset(out, 0);

        out[0] = ((RTP_VERSION & 0x03) << 6) | ((sc as u8) & 0x1F);
        out[1] = RtcpType.Bye as u8;
        std.mem.writeInt(u16, out[2..4], len_words, .big);

        let offset: usize = 4;
        let i: usize = 0;
        while (i < sc) : (i += 1) {
            std.mem.writeInt(u32, out[offset .. offset + 4][0..4], _.ssrc_list[i], .big);
            offset += 4;
        }

        if (r := _.reason) {
            out[offset] = r.len as u8;
            offset += 1;
            if (r.len > 0) {
                @memcpy(out[offset .. offset + r.len], r);
            }
        }

        return out;
    }

    /// Decodes an RTCP BYE packet from `bytes`. Caller must call `deinit(allocator)`.
    pub static fn decode(allocator: std.mem.Allocator, bytes: []const u8): RtcpBye ! RtpError {
        if (bytes.len < 4) {
            return RtpError.TruncatedPacket;
        }
        const version: u8 = (bytes[0] >> 6) & 0x03;
        if (version != RTP_VERSION) {
            return RtpError.InvalidVersion;
        }
        if (bytes[1] != (RtcpType.Bye as u8)) {
            return RtpError.InvalidPayloadType;
        }

        const sc: usize = (bytes[0] & 0x1F) as usize;
        const len_words = std.mem.readInt(u16, bytes[2..4], .big) as usize;
        const total_bytes = (len_words + 1) * 4;
        if (bytes.len < total_bytes or 4 + (sc * 4) > total_bytes) {
            return RtpError.TruncatedPacket;
        }

        let ssrcs: []u32 = [];
        if (sc > 0) {
            ssrcs = allocator.alloc(u32, sc) catch return RtpError.OutOfMemory;
        }
        errdefer {
            if (ssrcs.len > 0) {
                allocator.free(ssrcs);
            }
        }

        let offset: usize = 4;
        let i: usize = 0;
        while (i < sc) : (i += 1) {
            ssrcs[i] = std.mem.readInt(u32, bytes[offset .. offset + 4][0..4], .big);
            offset += 4;
        }

        let reason_opt: ([]const u8)? = null;
        if (offset < total_bytes) {
            const rlen: usize = bytes[offset] as usize;
            offset += 1;
            if (offset + rlen > total_bytes) {
                return RtpError.TruncatedPacket;
            }
            reason_opt = bytes[offset .. offset + rlen];
        }

        return RtcpBye {
            ssrc_list: ssrcs,
            reason: reason_opt,
        };
    }
}

/// Computes signed 16-bit modular wrap-around sequence number difference (`a - b` mod 65536).
/// Positive if `a` is ahead of `b`, negative if `a` is behind `b`.
pub fn seqDiff(a: u16, b: u16): i32 {
    const raw: u16 = a -% b;
    const signed_diff: i16 = @as(i16, @bitCast(raw));
    return signed_diff as i32;
}

/// A single buffered RTP frame emitted by `JitterBuffer.popNext()`.
/// Call `deinit(allocator)` to release the owned `payload` slice.
pub class JitterItem {
    pub seq: u16,
    pub timestamp: u32,
    pub payload: []const u8,

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.payload.len > 0) {
            allocator.free(_.payload);
        }
        _.payload = _.payload[0..0];
    }
}

/// Snapshot of `JitterBuffer` reordering, drop, and loss counters.
pub class JitterStats {
    pub buffered: usize = 0,
    pub emitted: u64 = 0,
    pub duplicates: u64 = 0,
    pub late_drops: u64 = 0,
    pub lost_packets: u64 = 0,
}

/// Sequence-number-aware RTP jitter buffer supporting 16-bit wrap-around reordering,
/// deduplication, late-packet dropping, and missing-packet skip accounting.
pub class JitterBuffer {
    allocator: std.mem.Allocator,
    max_capacity: usize,
    initialized: bool = false,
    expected_seq: u16 = 0,
    items: std.ArrayList(JitterItem) = .empty,
    stat_emitted: u64 = 0,
    stat_duplicates: u64 = 0,
    stat_late_drops: u64 = 0,
    stat_lost_packets: u64 = 0,

    /// Initializes a `JitterBuffer` with the given `max_capacity`.
    pub static fn init(allocator: std.mem.Allocator, max_capacity: usize): JitterBuffer {
        return JitterBuffer {
            allocator: allocator,
            max_capacity: max_capacity,
            initialized: false,
            expected_seq: 0,
            items: .empty,
            stat_emitted: 0,
            stat_duplicates: 0,
            stat_late_drops: 0,
            stat_lost_packets: 0,
        };
    }

    /// Releases all buffered item payloads and internal storage.
    pub fn deinit(*_) {
        let i: usize = 0;
        while (i < _.items.items.len) : (i += 1) {
            if (_.items.items[i].payload.len > 0) {
                _.allocator.free(_.items.items[i].payload);
            }
        }
        _.items.deinit(_.allocator);
    }

    /// Pushes an incoming RTP payload into the jitter buffer.
    /// Drops packets older than `expected_seq`, deduplicates identical sequences,
    /// and rejects new packets with `RtpError.BufferFull` if capacity is reached.
    pub fn push(*_, seq: u16, timestamp: u32, payload: []const u8): void ! RtpError {
        if (!_.initialized) {
            _.initialized = true;
            _.expected_seq = seq;
        } else {
            const diff = seqDiff(seq, _.expected_seq);
            if (diff < 0) {
                _.stat_late_drops += 1;
                return;
            }
        }

        let i: usize = 0;
        while (i < _.items.items.len) : (i += 1) {
            if (_.items.items[i].seq == seq) {
                _.stat_duplicates += 1;
                return;
            }
        }

        if (_.items.items.len >= _.max_capacity) {
            return RtpError.BufferFull;
        }

        const copy = _.allocator.dupe(u8, payload) catch return RtpError.OutOfMemory;
        errdefer _.allocator.free(copy);

        _.items.append(_.allocator, JitterItem {
            seq: seq,
            timestamp: timestamp,
            payload: copy,
        }) catch return RtpError.OutOfMemory;
    }

    /// Pops the next in-order packet matching `expected_seq`, or returns `null` if missing.
    /// Caller owns the returned `JitterItem` and should call `item.deinit(allocator)`.
    pub fn popNext(*_): JitterItem? {
        if (!_.initialized or _.items.items.len == 0) {
            return null;
        }

        let i: usize = 0;
        while (i < _.items.items.len) : (i += 1) {
            if (_.items.items[i].seq == _.expected_seq) {
                const item = _.items.swapRemove(i);
                _.expected_seq = _.expected_seq +% 1;
                _.stat_emitted += 1;
                return item;
            }
        }
        return null;
    }

    /// Advances `expected_seq` to the earliest buffered packet ahead of it,
    /// incrementing `lost_packets` by the skipped gap size. Returns `true` if skipped.
    pub fn skipMissing(*_): bool {
        if (!_.initialized or _.items.items.len == 0) {
            return false;
        }

        let min_diff: i32 = seqDiff(_.items.items[0].seq, _.expected_seq);
        let min_seq: u16 = _.items.items[0].seq;
        let i: usize = 1;
        while (i < _.items.items.len) : (i += 1) {
            const d = seqDiff(_.items.items[i].seq, _.expected_seq);
            if (d < min_diff) {
                min_diff = d;
                min_seq = _.items.items[i].seq;
            }
        }

        if (min_diff > 0) {
            _.stat_lost_packets += (min_diff as u64);
            _.expected_seq = min_seq;
            return true;
        }
        return false;
    }

    /// Returns a snapshot of current jitter buffer statistics.
    pub fn stats(): JitterStats {
        return JitterStats {
            buffered: _.items.items.len,
            emitted: _.stat_emitted,
            duplicates: _.stat_duplicates,
            late_drops: _.stat_late_drops,
            lost_packets: _.stat_lost_packets,
        };
    }
}
