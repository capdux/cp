// Origin: Pure-CP Stdlib (Unbounded-Slice RFC 3550 RTP/RTCP, ITU-T G.711 PCMU/PCMA & Adaptive Jitter Buffer/PLC) | Created: 2026-11-23
// 100% pure CP, zero dynamic heap allocation, unbounded caller-slice real-time voice & media streaming engine:
//   - RFC 3550 zero-copy RTP packet parser & serializer (CSRC list, header extensions, padding stripping)
//   - Ambiguity-free 16-bit -> 64-bit `RtpSeqUnwrapper` handling wrap-around (65535 -> 0) and reordering
//   - RFC 3550 RTCP Sender Report (SR) / Receiver Report (RR) codec & §A.8 interarrival jitter / loss stats
//   - ITU-T G.711 u-law (PCMU) & A-law (PCMA) real-time speech codecs + batch slice encode/decode/transcode
//   - NetEQ-style `AdaptiveJitterBuffer` backed by caller-owned `[]JitterSlot` and `[]u8` payload pool,
//     with adaptive target delay scaling, duplicate/late rejection, and smooth fade-out `applyPcm16Plc`

const std = @import("std");

// ============================================================================
// 1. Constants & Error Set
// ============================================================================

/// RFC 3550 RTP protocol version (`2`).
pub const RTP_VERSION: u8 = 2;

/// Minimum fixed RTP header length in bytes (`12`).
pub const RTP_MIN_HEADER_LEN: usize = 12;

/// RTCP packet type: Sender Report (`200`).
pub const RTCP_PT_SR: u8 = 200;

/// RTCP packet type: Receiver Report (`201`).
pub const RTCP_PT_RR: u8 = 201;

/// RTCP packet type: Goodbye (`203`).
pub const RTCP_PT_BYE: u8 = 203;

/// Standard RTP static payload type: ITU-T G.711 u-law 8kHz (`0`).
pub const RTP_PT_PCMU: u8 = 0;

/// Standard RTP static payload type: ITU-T G.711 A-law 8kHz (`8`).
pub const RTP_PT_PCMA: u8 = 8;

/// Common dynamic RTP payload type for Opus (`111`).
pub const RTP_PT_OPUS: u8 = 111;

/// Error set returned by RTP/RTCP framing, G.711 transcoding, and Jitter Buffer routines.
pub err RtpMediaError {
    PacketTooShort,
    UnsupportedVersion,
    InvalidPadding,
    InvalidExtension,
    InvalidRtcpLength,
    BufferTooSmall,
    PayloadTooLarge,
    InvalidConfig,
}

// ============================================================================
// 2. Internal Big-Endian Binary Helpers
// ============================================================================

fn readU16Be(buf: []const u8, off: usize): u16 {
    return (@as(u16, buf[off]) << 8) | @as(u16, buf[off + 1]);
}

fn readU32Be(buf: []const u8, off: usize): u32 {
    return (@as(u32, buf[off]) << 24) |
        (@as(u32, buf[off + 1]) << 16) |
        (@as(u32, buf[off + 2]) << 8) |
        @as(u32, buf[off + 3]);
}

fn writeU16Be(dst: *[]u8, off: usize, v: u16) {
    dst[off] = @as(u8, @intCast((v >> 8) & 0xFF));
    dst[off + 1] = @as(u8, @intCast(v & 0xFF));
}

fn writeU32Be(dst: *[]u8, off: usize, v: u32) {
    dst[off] = @as(u8, @intCast((v >> 24) & 0xFF));
    dst[off + 1] = @as(u8, @intCast((v >> 16) & 0xFF));
    dst[off + 2] = @as(u8, @intCast((v >> 8) & 0xFF));
    dst[off + 3] = @as(u8, @intCast(v & 0xFF));
}

fn writeI24Be(dst: *[]u8, off: usize, v_in: i32) {
    let clamped: i32 = v_in;
    if (clamped > 8388607) clamped = 8388607;
    if (clamped < -8388608) clamped = -8388608;
    const raw: u32 = @as(u32, @bitCast(clamped)) & 0x00FF_FFFF;
    dst[off] = @as(u8, @intCast((raw >> 16) & 0xFF));
    dst[off + 1] = @as(u8, @intCast((raw >> 8) & 0xFF));
    dst[off + 2] = @as(u8, @intCast(raw & 0xFF));
}

fn readI24Be(buf: []const u8, off: usize): i32 {
    const raw: u32 = (@as(u32, buf[off]) << 16) |
        (@as(u32, buf[off + 1]) << 8) |
        @as(u32, buf[off + 2]);
    if ((raw & 0x0080_0000) != 0) {
        return @as(i32, @bitCast(raw | 0xFF00_0000));
    }
    return @as(i32, @bitCast(raw));
}

// ============================================================================
// 3. RFC 3550 RTP Packet Codec & 16-bit Sequence Unwrapper
// ============================================================================

/// Parsed RFC 3550 RTP packet header with zero-copy extension and payload byte spans.
pub struct RtpHeader {
    version: u8 = RTP_VERSION;
    padding: bool = false;
    extension: bool = false;
    csrc_count: u8 = 0;
    marker: bool = false;
    payload_type: u8 = 0;
    seq: u16 = 0;
    timestamp: u32 = 0;
    ssrc: u32 = 0;
    csrc: [15]u32 = [0; 15];
    ext_profile: u16 = 0;
    ext_offset: usize = 0;
    ext_len: usize = 0;
    header_len: usize = RTP_MIN_HEADER_LEN;
    payload_offset: usize = RTP_MIN_HEADER_LEN;
    payload_len: usize = 0;
}

/// Parses an RFC 3550 RTP packet (including CSRC list, header extension, and padding validation)
/// without allocating memory.
pub fn parseRtpPacket(raw: []const u8): RtpHeader ! RtpMediaError {
    if (raw.len < RTP_MIN_HEADER_LEN) {
        return RtpMediaError.PacketTooShort;
    }

    const b0: u8 = raw[0];
    const b1: u8 = raw[1];
    const ver: u8 = (b0 >> 6) & 0x03;
    if (ver != RTP_VERSION) {
        return RtpMediaError.UnsupportedVersion;
    }

    const pad: bool = ((b0 >> 5) & 0x01) != 0;
    const ext: bool = ((b0 >> 4) & 0x01) != 0;
    const cc: u8 = b0 & 0x0F;
    const mark: bool = ((b1 >> 7) & 0x01) != 0;
    const pt: u8 = b1 & 0x7F;

    let off: usize = RTP_MIN_HEADER_LEN;
    const csrc_bytes: usize = @as(usize, cc) * 4;
    if (off + csrc_bytes > raw.len) {
        return RtpMediaError.PacketTooShort;
    }

    let hdr = RtpHeader{
        version: ver,
        padding: pad,
        extension: ext,
        csrc_count: cc,
        marker: mark,
        payload_type: pt,
        seq: readU16Be(raw, 2),
        timestamp: readU32Be(raw, 4),
        ssrc: readU32Be(raw, 8),
    };

    let ci: usize = 0;
    while (ci < @as(usize, cc)) : (ci += 1) {
        hdr.csrc[ci] = readU32Be(raw, off);
        off += 4;
    }

    if (ext) {
        if (off + 4 > raw.len) {
            return RtpMediaError.InvalidExtension;
        }
        hdr.ext_profile = readU16Be(raw, off);
        const ext_words: usize = @as(usize, readU16Be(raw, off + 2));
        off += 4;
        const ext_bytes: usize = ext_words * 4;
        if (off + ext_bytes > raw.len) {
            return RtpMediaError.InvalidExtension;
        }
        hdr.ext_offset = off;
        hdr.ext_len = ext_bytes;
        off += ext_bytes;
    }

    hdr.header_len = off;
    hdr.payload_offset = off;

    let end_pos: usize = raw.len;
    if (pad) {
        if (raw.len <= off) {
            return RtpMediaError.InvalidPadding;
        }
        const pad_len: usize = @as(usize, raw[raw.len - 1]);
        if (pad_len == 0 || off + pad_len > raw.len) {
            return RtpMediaError.InvalidPadding;
        }
        end_pos = raw.len - pad_len;
    }

    hdr.payload_len = end_pos - off;
    return hdr;
}

/// Serializes an RFC 3550 RTP packet (with optional CSRC, extension header, and 4-byte aligned padding)
/// into `out_buf`. Returns total encoded byte length.
pub fn encodeRtpPacket(
    hdr: RtpHeader,
    payload: []const u8,
    out_buf: *[]u8,
): usize ! RtpMediaError {
    if (hdr.version != RTP_VERSION) {
        return RtpMediaError.UnsupportedVersion;
    }
    if (hdr.csrc_count > 15 || hdr.payload_type > 127) {
        return RtpMediaError.InvalidConfig;
    }
    if (hdr.extension && (hdr.ext_len % 4) != 0) {
        return RtpMediaError.InvalidExtension;
    }

    const csrc_bytes: usize = @as(usize, hdr.csrc_count) * 4;
    const ext_total: usize = if (hdr.extension) hdr.ext_len + 4 else 0;
    const unpadded_len: usize = RTP_MIN_HEADER_LEN + csrc_bytes + ext_total + payload.len;
    let pad_len: usize = 0;
    if (hdr.padding) {
        const rem: usize = unpadded_len % 4;
        pad_len = if (rem == 0) @as(usize, 4) else @as(usize, 4) - rem;
    }
    const total_len: usize = unpadded_len + pad_len;
    if (out_buf.len < total_len) {
        return RtpMediaError.BufferTooSmall;
    }

    let b0: u8 = (RTP_VERSION << 6) | (hdr.csrc_count & 0x0F);
    if (hdr.padding) b0 |= 0x20;
    if (hdr.extension) b0 |= 0x10;
    let b1: u8 = hdr.payload_type & 0x7F;
    if (hdr.marker) b1 |= 0x80;

    out_buf[0] = b0;
    out_buf[1] = b1;
    writeU16Be(out_buf, 2, hdr.seq);
    writeU32Be(out_buf, 4, hdr.timestamp);
    writeU32Be(out_buf, 8, hdr.ssrc);

    let off: usize = RTP_MIN_HEADER_LEN;
    let ci: usize = 0;
    while (ci < @as(usize, hdr.csrc_count)) : (ci += 1) {
        writeU32Be(out_buf, off, hdr.csrc[ci]);
        off += 4;
    }

    if (hdr.extension) {
        writeU16Be(out_buf, off, hdr.ext_profile);
        writeU16Be(out_buf, off + 2, @as(u16, @intCast(hdr.ext_len / 4)));
        off += 4;
        let ei: usize = 0;
        while (ei < hdr.ext_len) : (ei += 1) {
            out_buf[off + ei] = 0;
        }
        off += hdr.ext_len;
    }

    let pi: usize = 0;
    while (pi < payload.len) : (pi += 1) {
        out_buf[off + pi] = payload[pi];
    }
    off += payload.len;

    if (hdr.padding) {
        let zi: usize = 0;
        while (zi + 1 < pad_len) : (zi += 1) {
            out_buf[off + zi] = 0;
        }
        out_buf[off + pad_len - 1] = @as(u8, @intCast(pad_len));
        off += pad_len;
    }

    return off;
}

/// Unwraps 16-bit cyclic RTP sequence numbers into a signed 64-bit monotonic space,
/// correctly handling wrap-around (`65535 -> 0`) and out-of-order late arrivals.
pub struct RtpSeqUnwrapper {
    initialized: bool = false;
    highest_ext_seq: i64 = 0;

    /// Initializes a fresh `RtpSeqUnwrapper`.
    static fn init(): RtpSeqUnwrapper {
        return RtpSeqUnwrapper{};
    }

    /// Unwraps `seq` relative to the highest seen sequence number.
    fn unwrap(*_, seq: u16): i64 {
        if (!initialized) {
            initialized = true;
            highest_ext_seq = @as(i64, seq);
            return highest_ext_seq;
        }

        const highest_u64: u64 = @as(u64, @bitCast(highest_ext_seq));
        const highest_u16: u16 = @as(u16, @intCast(highest_u64 & 0xFFFF));
        const diff_u16: u16 = seq -% highest_u16;
        const delta: i64 = if (diff_u16 < 32768)
            @as(i64, diff_u16)
        else
            @as(i64, diff_u16) - 65536;

        const ext: i64 = highest_ext_seq + delta;
        if (ext > highest_ext_seq) {
            highest_ext_seq = ext;
        }
        return ext;
    }
}

// ============================================================================
// 4. RFC 3550 RTCP Sender Report (SR), Receiver Report (RR) & Recv Stats
// ============================================================================

/// RFC 3550 §6.4.1 RTCP Sender Report (SR) payload (28 bytes on wire).
pub struct RtcpSenderReport {
    ssrc: u32 = 0;
    ntp_sec: u32 = 0;
    ntp_frac: u32 = 0;
    rtp_timestamp: u32 = 0;
    packet_count: u32 = 0;
    octet_count: u32 = 0;
}

/// RFC 3550 §6.4.2 RTCP Reception Report Block (24 bytes).
pub struct RtcpReportBlock {
    ssrc: u32 = 0;
    fraction_lost: u8 = 0;
    cumulative_lost: i32 = 0;
    ext_highest_seq: u32 = 0;
    jitter: u32 = 0;
    lsr: u32 = 0;
    dlsr: u32 = 0;
}

/// Encodes a 28-byte RTCP Sender Report (`PT = 200, RC = 0`) into `out_buf`.
pub fn encodeRtcpSr(sr: RtcpSenderReport, out_buf: *[]u8): usize ! RtpMediaError {
    if (out_buf.len < 28) {
        return RtpMediaError.BufferTooSmall;
    }
    out_buf[0] = RTP_VERSION << 6;
    out_buf[1] = RTCP_PT_SR;
    writeU16Be(out_buf, 2, 6); // 7 32-bit words - 1 = 6
    writeU32Be(out_buf, 4, sr.ssrc);
    writeU32Be(out_buf, 8, sr.ntp_sec);
    writeU32Be(out_buf, 12, sr.ntp_frac);
    writeU32Be(out_buf, 16, sr.rtp_timestamp);
    writeU32Be(out_buf, 20, sr.packet_count);
    writeU32Be(out_buf, 24, sr.octet_count);
    return 28;
}

/// Parses an RTCP Sender Report (`PT = 200`) from `raw`.
pub fn parseRtcpSr(raw: []const u8): RtcpSenderReport ! RtpMediaError {
    if (raw.len < 28) {
        return RtpMediaError.PacketTooShort;
    }
    if (((raw[0] >> 6) & 0x03) != RTP_VERSION) {
        return RtpMediaError.UnsupportedVersion;
    }
    if (raw[1] != RTCP_PT_SR) {
        return RtpMediaError.InvalidRtcpLength;
    }
    const declared_bytes: usize = (@as(usize, readU16Be(raw, 2)) + 1) * 4;
    if (declared_bytes < 28 || declared_bytes > raw.len) {
        return RtpMediaError.InvalidRtcpLength;
    }
    return RtcpSenderReport{
        ssrc: readU32Be(raw, 4),
        ntp_sec: readU32Be(raw, 8),
        ntp_frac: readU32Be(raw, 12),
        rtp_timestamp: readU32Be(raw, 16),
        packet_count: readU32Be(raw, 20),
        octet_count: readU32Be(raw, 24),
    };
}

/// Encodes a 32-byte RTCP Receiver Report (`PT = 201, RC = 1`) carrying one `RtcpReportBlock`.
pub fn encodeRtcpRr(
    sender_ssrc: u32,
    block: RtcpReportBlock,
    out_buf: *[]u8,
): usize ! RtpMediaError {
    if (out_buf.len < 32) {
        return RtpMediaError.BufferTooSmall;
    }
    out_buf[0] = (RTP_VERSION << 6) | 1; // RC = 1
    out_buf[1] = RTCP_PT_RR;
    writeU16Be(out_buf, 2, 7); // 8 32-bit words - 1 = 7
    writeU32Be(out_buf, 4, sender_ssrc);
    writeU32Be(out_buf, 8, block.ssrc);
    out_buf[12] = block.fraction_lost;
    writeI24Be(out_buf, 13, block.cumulative_lost);
    writeU32Be(out_buf, 16, block.ext_highest_seq);
    writeU32Be(out_buf, 20, block.jitter);
    writeU32Be(out_buf, 24, block.lsr);
    writeU32Be(out_buf, 28, block.dlsr);
    return 32;
}

/// Parses an RTCP Receiver Report (`PT = 201, RC >= 1`), writing `out_sender_ssrc` and returning the first report block.
pub fn parseRtcpRr(raw: []const u8, out_sender_ssrc: *u32): RtcpReportBlock ! RtpMediaError {
    if (raw.len < 32) {
        return RtpMediaError.PacketTooShort;
    }
    if (((raw[0] >> 6) & 0x03) != RTP_VERSION) {
        return RtpMediaError.UnsupportedVersion;
    }
    const rc: u8 = raw[0] & 0x1F;
    if (raw[1] != RTCP_PT_RR || rc == 0) {
        return RtpMediaError.InvalidRtcpLength;
    }
    const declared_bytes: usize = (@as(usize, readU16Be(raw, 2)) + 1) * 4;
    if (declared_bytes < 32 || declared_bytes > raw.len) {
        return RtpMediaError.InvalidRtcpLength;
    }
    out_sender_ssrc.* = readU32Be(raw, 4);
    return RtcpReportBlock{
        ssrc: readU32Be(raw, 8),
        fraction_lost: raw[12],
        cumulative_lost: readI24Be(raw, 13),
        ext_highest_seq: readU32Be(raw, 16),
        jitter: readU32Be(raw, 20),
        lsr: readU32Be(raw, 24),
        dlsr: readU32Be(raw, 28),
    };
}

/// RFC 3550 §A.3 & §A.8 real-time RTP reception statistics (interarrival jitter & packet loss).
pub struct RtpRecvStats {
    ssrc: u32 = 0;
    sample_rate_hz: u32 = 8000;
    unwrapper: RtpSeqUnwrapper = RtpSeqUnwrapper{};
    base_ext_seq: i64 = 0;
    max_ext_seq: i64 = 0;
    received_count: u64 = 0;
    expected_prior: u64 = 0;
    received_prior: u64 = 0;
    has_transit: bool = false;
    last_transit: i64 = 0;
    jitter_q: f64 = 0.0;

    /// Initializes reception stats for stream `ssrc` at RTP clock `sample_rate_hz`.
    static fn init(ssrc: u32, sample_rate_hz: u32): RtpRecvStats {
        return RtpRecvStats{
            ssrc: ssrc,
            sample_rate_hz: if (sample_rate_hz == 0) 8000 else sample_rate_hz,
            unwrapper: RtpSeqUnwrapper.init(),
        };
    }

    /// Updates sequence tracking and RFC 3550 §A.8 1/16-smoothed interarrival jitter.
    fn onPacket(*_, seq: u16, rtp_ts: u32, arrival_ms: u64) {
        const ext: i64 = unwrapper.unwrap(seq);
        if (received_count == 0) {
            base_ext_seq = ext;
            max_ext_seq = ext;
        } else if (ext > max_ext_seq) {
            max_ext_seq = ext;
        }
        received_count += 1;

        const arrival_rtp: i64 = @as(i64, @intCast((arrival_ms * @as(u64, sample_rate_hz)) / 1000));
        const transit: i64 = arrival_rtp - @as(i64, rtp_ts);
        if (!has_transit) {
            has_transit = true;
            last_transit = transit;
            return;
        }

        let d: i64 = transit - last_transit;
        last_transit = transit;
        if (d < 0) d = -d;
        const d_f: f64 = @as(f64, @floatFromInt(d));
        jitter_q += (d_f - jitter_q) / 16.0;
    }

    /// Returns smoothed interarrival jitter converted to milliseconds.
    fn jitterMs(): f64 {
        return (jitter_q * 1000.0) / @as(f64, @floatFromInt(sample_rate_hz));
    }

    /// Generates an RFC 3550 `RtcpReportBlock` and updates interval loss baselines.
    fn buildReportBlock(*_, lsr: u32, dlsr: u32): RtcpReportBlock {
        let expected: u64 = 0;
        if (received_count > 0 && max_ext_seq >= base_ext_seq) {
            expected = @as(u64, @intCast(max_ext_seq - base_ext_seq + 1));
        }

        const cum_lost_i64: i64 = @as(i64, @intCast(expected)) - @as(i64, @intCast(received_count));
        let cum_lost: i32 = 0;
        if (cum_lost_i64 > 8388607) {
            cum_lost = 8388607;
        } else if (cum_lost_i64 < -8388608) {
            cum_lost = -8388608;
        } else {
            cum_lost = @as(i32, @intCast(cum_lost_i64));
        }

        const exp_interval: u64 = if (expected >= expected_prior) expected - expected_prior else 0;
        const rec_interval: u64 = if (received_count >= received_prior) received_count - received_prior else 0;
        expected_prior = expected;
        received_prior = received_count;

        let frac_lost: u8 = 0;
        if (exp_interval > 0 && exp_interval > rec_interval) {
            const lost_interval: u64 = exp_interval - rec_interval;
            const scaled: u64 = (lost_interval * 256) / exp_interval;
            frac_lost = if (scaled > 255) 255 else @as(u8, @intCast(scaled));
        }

        const ext_hi_u64: u64 = @as(u64, @bitCast(max_ext_seq));
        return RtcpReportBlock{
            ssrc: ssrc,
            fraction_lost: frac_lost,
            cumulative_lost: cum_lost,
            ext_highest_seq: @as(u32, @intCast(ext_hi_u64 & 0xFFFF_FFFF)),
            jitter: @as(u32, @intFromFloat(jitter_q)),
            lsr: lsr,
            dlsr: dlsr,
        };
    }
}

// ============================================================================
// 5. ITU-T G.711 u-law (PCMU) & A-law (PCMA) Real-Time Speech Codecs
// ============================================================================

const ULAW_BIAS: i32 = 0x84;
const ULAW_CLIP: i32 = 32635;

/// Encodes a single 16-bit signed linear PCM sample to ITU-T G.711 u-law (PCMU).
pub fn linear16ToPcmu(sample: i16): u8 {
    let pcm: i32 = @as(i32, sample);
    let sign: u8 = 0;
    if (pcm < 0) {
        sign = 0x80;
        pcm = -pcm;
    }
    if (pcm > ULAW_CLIP) {
        pcm = ULAW_CLIP;
    }
    pcm += ULAW_BIAS;

    let exponent: u8 = 7;
    let exp_mask: i32 = 0x4000;
    while (exponent > 0 && (pcm & exp_mask) == 0) {
        exponent -= 1;
        exp_mask >>= 1;
    }

    const shift: u5 = @as(u5, @intCast(exponent + 3));
    const mantissa: u8 = @as(u8, @intCast((pcm >> shift) & 0x0F));
    const uval: u8 = sign | (exponent << 4) | mantissa;
    return ~uval;
}

/// Decodes a single ITU-T G.711 u-law (PCMU) octet back to 16-bit signed linear PCM.
pub fn pcmuToLinear16(code: u8): i16 {
    const uval: u8 = ~code;
    const sign: bool = (uval & 0x80) != 0;
    const exponent: u5 = @as(u5, @intCast((uval >> 4) & 0x07));
    const mantissa: i32 = @as(i32, uval & 0x0F);

    let t: i32 = ((mantissa << 3) + ULAW_BIAS) << exponent;
    t -= ULAW_BIAS;
    if (sign) {
        return @as(i16, @intCast(-t));
    }
    return @as(i16, @intCast(t));
}

/// Encodes a single 16-bit signed linear PCM sample to ITU-T G.711 A-law (PCMA).
pub fn linear16ToPcma(sample: i16): u8 {
    let pcm: i32 = @as(i32, sample);
    let sign: u8 = 0x80;
    if (pcm < 0) {
        sign = 0x00;
        pcm = -pcm;
        if (pcm > 32767) pcm = 32767;
    }

    let aval: u8 = 0;
    if (pcm >= 256) {
        let exponent: u8 = 7;
        let exp_mask: i32 = 0x4000;
        while (exponent > 1 && (pcm & exp_mask) == 0) {
            exponent -= 1;
            exp_mask >>= 1;
        }
        const shift: u5 = @as(u5, @intCast(exponent + 3));
        const mantissa: u8 = @as(u8, @intCast((pcm >> shift) & 0x0F));
        aval = (exponent << 4) | mantissa;
    } else {
        aval = @as(u8, @intCast((pcm >> 4) & 0x0F));
    }

    return (sign | aval) ^ 0x55;
}

/// Decodes a single ITU-T G.711 A-law (PCMA) octet back to 16-bit signed linear PCM.
pub fn pcmaToLinear16(code: u8): i16 {
    const aval: u8 = code ^ 0x55;
    const sign_pos: bool = (aval & 0x80) != 0;
    const exponent: u8 = (aval >> 4) & 0x07;
    const mantissa: i32 = @as(i32, aval & 0x0F);

    let t: i32 = 0;
    if (exponent == 0) {
        t = (mantissa << 4) + 8;
    } else {
        const shift: u5 = @as(u5, @intCast(exponent + 3));
        t = ((mantissa << 4) + 0x108) << (shift - 4);
    }

    if (sign_pos) {
        return @as(i16, @intCast(t));
    }
    return @as(i16, @intCast(-t));
}

/// Zero-allocation batch PCM16 -> G.711 u-law (PCMU) encoder.
pub fn encodePcmuSlice(pcm: []const i16, out: *[]u8): usize ! RtpMediaError {
    if (out.len < pcm.len) return RtpMediaError.BufferTooSmall;
    let i: usize = 0;
    while (i < pcm.len) : (i += 1) {
        out[i] = linear16ToPcmu(pcm[i]);
    }
    return pcm.len;
}

/// Zero-allocation batch G.711 u-law (PCMU) -> PCM16 decoder.
pub fn decodePcmuSlice(pcmu: []const u8, out: *[]i16): usize ! RtpMediaError {
    if (out.len < pcmu.len) return RtpMediaError.BufferTooSmall;
    let i: usize = 0;
    while (i < pcmu.len) : (i += 1) {
        out[i] = pcmuToLinear16(pcmu[i]);
    }
    return pcmu.len;
}

/// Zero-allocation batch PCM16 -> G.711 A-law (PCMA) encoder.
pub fn encodePcmaSlice(pcm: []const i16, out: *[]u8): usize ! RtpMediaError {
    if (out.len < pcm.len) return RtpMediaError.BufferTooSmall;
    let i: usize = 0;
    while (i < pcm.len) : (i += 1) {
        out[i] = linear16ToPcma(pcm[i]);
    }
    return pcm.len;
}

/// Zero-allocation batch G.711 A-law (PCMA) -> PCM16 decoder.
pub fn decodePcmaSlice(pcma: []const u8, out: *[]i16): usize ! RtpMediaError {
    if (out.len < pcma.len) return RtpMediaError.BufferTooSmall;
    let i: usize = 0;
    while (i < pcma.len) : (i += 1) {
        out[i] = pcmaToLinear16(pcma[i]);
    }
    return pcma.len;
}

/// Direct zero-allocation G.711 u-law -> A-law transcoder.
pub fn transcodePcmuToPcma(pcmu: []const u8, out: *[]u8): usize ! RtpMediaError {
    if (out.len < pcmu.len) return RtpMediaError.BufferTooSmall;
    let i: usize = 0;
    while (i < pcmu.len) : (i += 1) {
        out[i] = linear16ToPcma(pcmuToLinear16(pcmu[i]));
    }
    return pcmu.len;
}

/// Direct zero-allocation G.711 A-law -> u-law transcoder.
pub fn transcodePcmaToPcmu(pcma: []const u8, out: *[]u8): usize ! RtpMediaError {
    if (out.len < pcma.len) return RtpMediaError.BufferTooSmall;
    let i: usize = 0;
    while (i < pcma.len) : (i += 1) {
        out[i] = linear16ToPcmu(pcmaToLinear16(pcma[i]));
    }
    return pcma.len;
}

// ============================================================================
// 6. Adaptive Jitter Buffer & Smooth Fade-Out Packet Loss Concealment (PLC)
// ============================================================================

/// Outcome classification emitted by `AdaptiveJitterBuffer.popNextFrame`.
pub enum JitterPopKind {
    audio_packet,
    plc_concealment,
    empty_underrun,
}

/// Metadata slot stored in caller's external `slots: []JitterSlot` slice.
pub struct JitterSlot {
    occupied: bool = false;
    ext_seq: i64 = 0;
    timestamp: u32 = 0;
    marker: bool = false;
    payload_len: usize = 0;
    arrival_ms: u64 = 0;
}

/// Result returned by `AdaptiveJitterBuffer.popNextFrame`.
pub struct JitterPopResult {
    kind: JitterPopKind = JitterPopKind.empty_underrun;
    ext_seq: i64 = 0;
    timestamp: u32 = 0;
    payload_len: usize = 0;
    marker: bool = false;
}

/// Aggregate jitter buffer telemetry.
pub struct JitterBufferStats {
    packets_pushed: u64 = 0;
    packets_popped: u64 = 0;
    duplicates_dropped: u64 = 0;
    late_dropped: u64 = 0;
    plc_concealed: u64 = 0;
    current_buffered: usize = 0;
    target_delay_ms: u64 = 0;
    jitter_ms: f64 = 0.0;
}

/// Smooth exponential-decay Packet Loss Concealment (PLC) for 16-bit PCM frames.
/// Gradually fades out repeated frames across consecutive losses and mutes cleanly at >= 6 losses.
pub fn applyPcm16Plc(
    prev_frame: []const i16,
    consecutive_losses: u32,
    out_frame: *[]i16,
): usize ! RtpMediaError {
    if (out_frame.len < prev_frame.len) {
        return RtpMediaError.BufferTooSmall;
    }
    if (prev_frame.len == 0) {
        return 0;
    }

    let gain_start: f64 = 0.0;
    let gain_end: f64 = 0.0;
    if (consecutive_losses <= 1) {
        gain_start = 0.90;
        gain_end = 0.75;
    } else if (consecutive_losses == 2) {
        gain_start = 0.75;
        gain_end = 0.55;
    } else if (consecutive_losses == 3) {
        gain_start = 0.55;
        gain_end = 0.35;
    } else if (consecutive_losses == 4) {
        gain_start = 0.35;
        gain_end = 0.15;
    } else if (consecutive_losses == 5) {
        gain_start = 0.15;
        gain_end = 0.0;
    } else {
        gain_start = 0.0;
        gain_end = 0.0;
    }

    const denom: f64 = if (prev_frame.len > 1) @as(f64, @floatFromInt(prev_frame.len - 1)) else 1.0;
    let i: usize = 0;
    while (i < prev_frame.len) : (i += 1) {
        const t: f64 = @as(f64, @floatFromInt(i)) / denom;
        const g: f64 = gain_start + (gain_end - gain_start) * t;
        const scaled: f64 = @as(f64, @floatFromInt(prev_frame[i])) * g;
        out_frame[i] = @as(i16, @intFromFloat(scaled));
    }
    return prev_frame.len;
}

/// NetEQ-style adaptive Jitter Buffer operating directly over caller-owned slot and payload slices.
pub struct AdaptiveJitterBuffer {
    slots: []JitterSlot = [],
    payload_pool: []u8 = [],
    slot_stride: usize = 0;
    sample_rate_hz: u32 = 8000;
    min_delay_ms: u64 = 20;
    max_delay_ms: u64 = 200;
    target_delay_ms: u64 = 20;

    unwrapper: RtpSeqUnwrapper = RtpSeqUnwrapper{};
    recv_stats: RtpRecvStats = RtpRecvStats{};

    started: bool = false;
    has_first_packet: bool = false;
    first_arrival_ms: u64 = 0;
    next_play_seq: i64 = 0;
    current_buffered: usize = 0;

    packets_pushed: u64 = 0;
    packets_popped: u64 = 0;
    duplicates_dropped: u64 = 0;
    late_dropped: u64 = 0;
    plc_concealed: u64 = 0;

    /// Initializes an `AdaptiveJitterBuffer` backed by caller-owned `slots` and `payload_pool`.
    static fn init(
        slot_stride: usize,
        sample_rate_hz: u32,
        min_delay_ms: u64,
        max_delay_ms: u64,
        slots: *[]JitterSlot,
        payload_pool: *[]u8,
    ): AdaptiveJitterBuffer ! RtpMediaError {
        if (slot_stride == 0 || sample_rate_hz == 0 || min_delay_ms > max_delay_ms) {
            return RtpMediaError.InvalidConfig;
        }
        if (slots.len == 0 || payload_pool.len < slots.len * slot_stride) {
            return RtpMediaError.BufferTooSmall;
        }

        let i: usize = 0;
        while (i < slots.len) : (i += 1) {
            slots[i] = JitterSlot{};
        }

        return AdaptiveJitterBuffer{
            slots: slots.*,
            payload_pool: payload_pool.*,
            slot_stride: slot_stride,
            sample_rate_hz: sample_rate_hz,
            min_delay_ms: min_delay_ms,
            max_delay_ms: max_delay_ms,
            target_delay_ms: min_delay_ms,
            unwrapper: RtpSeqUnwrapper.init(),
            recv_stats: RtpRecvStats.init(0, sample_rate_hz),
        };
    }

    fn updateAdaptiveDelay(*_) {
        const j_ms: f64 = recv_stats.jitterMs();
        const raw_target: u64 = min_delay_ms + @as(u64, @intFromFloat(std.math.ceil(j_ms * 2.5)));
        if (raw_target < min_delay_ms) {
            target_delay_ms = min_delay_ms;
        } else if (raw_target > max_delay_ms) {
            target_delay_ms = max_delay_ms;
        } else {
            target_delay_ms = raw_target;
        }
    }

    /// Inserts an arriving RTP packet, updating jitter estimation and adaptive target delay.
    /// Returns `true` if buffered, or `false` if dropped as duplicate or too late.
    fn pushPacket(
        *_,
        hdr: RtpHeader,
        payload: []const u8,
        arrival_ms: u64,
    ): bool ! RtpMediaError {
        if (payload.len > slot_stride) {
            return RtpMediaError.PayloadTooLarge;
        }

        const ext_seq: i64 = unwrapper.unwrap(hdr.seq);
        recv_stats.onPacket(hdr.seq, hdr.timestamp, arrival_ms);
        updateAdaptiveDelay();

        if (started && ext_seq < next_play_seq) {
            late_dropped += 1;
            return false;
        }

        // Check for duplicate and locate an empty slot (or oldest slot if full).
        let free_idx: usize = slots.len;
        let oldest_idx: usize = 0;
        let has_oldest: bool = false;
        let i: usize = 0;
        while (i < slots.len) : (i += 1) {
            if (slots[i].occupied) {
                if (slots[i].ext_seq == ext_seq) {
                    duplicates_dropped += 1;
                    return false;
                }
                if (!has_oldest || slots[i].ext_seq < slots[oldest_idx].ext_seq) {
                    oldest_idx = i;
                    has_oldest = true;
                }
            } else if (free_idx == slots.len) {
                free_idx = i;
            }
        }

        let dst_slot: usize = free_idx;
        if (dst_slot == slots.len) {
            // Buffer full: evict oldest packet to prevent unbounded latency buildup.
            dst_slot = oldest_idx;
            if (current_buffered > 0) {
                current_buffered -= 1;
            }
        }

        const pool_off: usize = dst_slot * slot_stride;
        let k: usize = 0;
        while (k < payload.len) : (k += 1) {
            payload_pool[pool_off + k] = payload[k];
        }

        slots[dst_slot] = JitterSlot{
            occupied: true,
            ext_seq: ext_seq,
            timestamp: hdr.timestamp,
            marker: hdr.marker,
            payload_len: payload.len,
            arrival_ms: arrival_ms,
        };
        current_buffered += 1;
        packets_pushed += 1;

        if (!has_first_packet) {
            has_first_packet = true;
            first_arrival_ms = arrival_ms;
            next_play_seq = ext_seq;
        } else if (!started && ext_seq < next_play_seq) {
            next_play_seq = ext_seq;
        }

        return true;
    }

    /// Pops the next in-order audio frame into `out_payload`, emits `.plc_concealment` if a gap is
    /// detected ahead of newer packets, or returns `.empty_underrun` while pre-buffering / drained.
    fn popNextFrame(
        *_,
        now_ms: u64,
        out_payload: *[]u8,
    ): JitterPopResult ! RtpMediaError {
        if (!has_first_packet || current_buffered == 0) {
            return JitterPopResult{ kind: JitterPopKind.empty_underrun };
        }

        if (!started) {
            if (now_ms < first_arrival_ms + target_delay_ms) {
                return JitterPopResult{ kind: JitterPopKind.empty_underrun };
            }
            // Start playout from the lowest buffered sequence number.
            let min_seq: i64 = 0;
            let found_any: bool = false;
            let m: usize = 0;
            while (m < slots.len) : (m += 1) {
                if (slots[m].occupied) {
                    if (!found_any || slots[m].ext_seq < min_seq) {
                        min_seq = slots[m].ext_seq;
                        found_any = true;
                    }
                }
            }
            if (!found_any) {
                return JitterPopResult{ kind: JitterPopKind.empty_underrun };
            }
            next_play_seq = min_seq;
            started = true;
        }

        // Look for exact `next_play_seq` and check if any future packet exists.
        let match_idx: usize = slots.len;
        let has_future: bool = false;
        let i: usize = 0;
        while (i < slots.len) : (i += 1) {
            if (!slots[i].occupied) continue;
            if (slots[i].ext_seq == next_play_seq) {
                match_idx = i;
                break;
            }
            if (slots[i].ext_seq > next_play_seq) {
                has_future = true;
            }
        }

        if (match_idx < slots.len) {
            const slot = slots[match_idx];
            if (out_payload.len < slot.payload_len) {
                return RtpMediaError.BufferTooSmall;
            }
            const pool_off: usize = match_idx * slot_stride;
            let k: usize = 0;
            while (k < slot.payload_len) : (k += 1) {
                out_payload[k] = payload_pool[pool_off + k];
            }
            slots[match_idx].occupied = false;
            if (current_buffered > 0) current_buffered -= 1;
            packets_popped += 1;
            next_play_seq += 1;

            return JitterPopResult{
                kind: JitterPopKind.audio_packet,
                ext_seq: slot.ext_seq,
                timestamp: slot.timestamp,
                payload_len: slot.payload_len,
                marker: slot.marker,
            };
        }

        if (has_future) {
            const missing_seq: i64 = next_play_seq;
            next_play_seq += 1;
            plc_concealed += 1;
            return JitterPopResult{
                kind: JitterPopKind.plc_concealment,
                ext_seq: missing_seq,
                timestamp: 0,
                payload_len: 0,
                marker: false,
            };
        }

        return JitterPopResult{ kind: JitterPopKind.empty_underrun };
    }

    /// Returns aggregate jitter buffer statistics and adaptive target delay.
    fn stats(): JitterBufferStats {
        return JitterBufferStats{
            packets_pushed: packets_pushed,
            packets_popped: packets_popped,
            duplicates_dropped: duplicates_dropped,
            late_dropped: late_dropped,
            plc_concealed: plc_concealed,
            current_buffered: current_buffered,
            target_delay_ms: target_delay_ms,
            jitter_ms: recv_stats.jitterMs(),
        };
    }
}
