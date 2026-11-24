// lib/rtc/index.cp — Pure CP Sans-IO WebRTC Core Protocol Stack (cap:rtc)
// Inspired by Rust str0m (MIT/Apache-2.0): zero I/O, zero heap allocation, zero external deps.
// Covers:
//   1. RTP / RTCP zero-copy codec (RFC 3550, RFC 4585 Generic NACK, RFC 7983 demux)
//   2. STUN codec (RFC 5389: Binding Request/Response, XOR-MAPPED-ADDRESS IPv4, CRC-32 FINGERPRINT)
//   3. SDP lightweight Offer/Answer parser & builder (RFC 4566 / RFC 8843 BUNDLE)
//   4. Sans-IO Jitter / NACK Sequence Tracker (16-bit wrap-around, reorder, NACK bitmask, loss stats)

pub err RtcError {
    Truncated,
    BufferTooSmall,
    InvalidVersion,
    InvalidHeaderLength,
    InvalidExtension,
    InvalidPadding,
    InvalidPacketType,
    InvalidCsrcCount,
    InvalidPayloadType,
    InvalidStunHeader,
    InvalidStunMagic,
    InvalidStunLength,
    InvalidStunAttribute,
    InvalidAddressFamily,
    InvalidFingerprint,
    InvalidSdp,
    TooManyMediaLines,
    InvalidValue,
}

// ---------------------------------------------------------------------------
// Protocol Constants
// ---------------------------------------------------------------------------

pub const RTP_VERSION: u8 = 2;
pub const RTP_MIN_HEADER_LEN: usize = 12;
pub const RTP_MAX_CSRC: usize = 15;

pub const RTCP_MIN_HEADER_LEN: usize = 4;
pub const RTCP_NACK_PACKET_LEN: usize = 16;
pub const RTCP_PT_SR: u8 = 200;
pub const RTCP_PT_RR: u8 = 201;
pub const RTCP_PT_SDES: u8 = 202;
pub const RTCP_PT_BYE: u8 = 203;
pub const RTCP_PT_APP: u8 = 204;
pub const RTCP_PT_RTPFB: u8 = 205;
pub const RTCP_PT_PSFB: u8 = 206;
pub const RTCP_FMT_NACK: u8 = 1;

pub const STUN_MAGIC_COOKIE: u32 = 0x2112A442;
pub const STUN_HEADER_LEN: usize = 20;
pub const STUN_BINDING_REQUEST: u16 = 0x0001;
pub const STUN_BINDING_SUCCESS: u16 = 0x0101;
pub const STUN_BINDING_ERROR: u16 = 0x0111;
pub const STUN_ATTR_MAPPED_ADDRESS: u16 = 0x0001;
pub const STUN_ATTR_USERNAME: u16 = 0x0006;
pub const STUN_ATTR_MESSAGE_INTEGRITY: u16 = 0x0008;
pub const STUN_ATTR_XOR_MAPPED_ADDRESS: u16 = 0x0020;
pub const STUN_ATTR_PRIORITY: u16 = 0x0024;
pub const STUN_ATTR_USE_CANDIDATE: u16 = 0x0025;
pub const STUN_ATTR_FINGERPRINT: u16 = 0x8028;
pub const STUN_FINGERPRINT_XOR: u32 = 0x5354554E;
pub const STUN_FAMILY_IPV4: u8 = 0x01;

pub const DEMUX_UNKNOWN: u8 = 0;
pub const DEMUX_STUN: u8 = 1;
pub const DEMUX_DTLS: u8 = 2;
pub const DEMUX_RTP: u8 = 3;
pub const DEMUX_RTCP: u8 = 4;

pub const SDP_MEDIA_UNKNOWN: u8 = 0;
pub const SDP_MEDIA_AUDIO: u8 = 1;
pub const SDP_MEDIA_VIDEO: u8 = 2;
pub const SDP_MEDIA_APPLICATION: u8 = 3;

pub const SDP_SETUP_NONE: u8 = 0;
pub const SDP_SETUP_ACTPASS: u8 = 1;
pub const SDP_SETUP_ACTIVE: u8 = 2;
pub const SDP_SETUP_PASSIVE: u8 = 3;

pub const SDP_MAX_MEDIA_LINES: usize = 4;
pub const SDP_MAX_PAYLOAD_TYPES: usize = 8;

// ---------------------------------------------------------------------------
// Internal Big-Endian & Buffer Helpers (zero heap, strict bounds)
// ---------------------------------------------------------------------------

fn readU16Be(buf: []const u8, off: usize): u16 {
    const b0: u16 = buf[off];
    const b1: u16 = buf[off + 1];
    return (b0 << 8) | b1;
}

fn readU32Be(buf: []const u8, off: usize): u32 {
    const b0: u32 = buf[off];
    const b1: u32 = buf[off + 1];
    const b2: u32 = buf[off + 2];
    const b3: u32 = buf[off + 3];
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
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

fn copyIntoFixed(dst: []u8, src: []const u8): usize {
    let n = src.len;
    if (n > dst.len) {
        n = dst.len;
    }
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
    return n;
}

fn appendBytes(out: []u8, pos: usize, data: []const u8): usize! {
    if (pos > out.len || data.len > out.len - pos) {
        return error.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        out[pos + i] = data[i];
    }
    return pos + data.len;
}

fn appendByte(out: []u8, pos: usize, b: u8): usize! {
    if (pos >= out.len) {
        return error.BufferTooSmall;
    }
    out[pos] = b;
    return pos + 1;
}

fn appendU64Dec(out: []u8, pos: usize, val: u64): usize! {
    let tmp: [20]u8 = [0; 20];
    let len: usize = 0;
    let v = val;
    if (v == 0) {
        tmp[0] = 0x30;
        len = 1;
    } else {
        while (v > 0) {
            tmp[len] = (0x30 + (v % 10)) as u8;
            v = v / 10;
            len += 1;
        }
    }
    if (pos > out.len || len > out.len - pos) {
        return error.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < len) : (i += 1) {
        out[pos + i] = tmp[len - 1 - i];
    }
    return pos + len;
}

fn startsWith(hay: []const u8, prefix: []const u8): bool {
    if (hay.len < prefix.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (hay[i] != prefix[i]) {
            return false;
        }
    }
    return true;
}

fn sliceEquals(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn parseU64Dec(s: []const u8): u64! {
    if (s.len == 0) {
        return error.InvalidValue;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const ch = s[i];
        if (ch < 0x30 || ch > 0x39) {
            return error.InvalidValue;
        }
        const digit: u64 = ch - 0x30;
        if (acc > 1844674407370955161 || (acc == 1844674407370955161 && digit > 5)) {
            return error.InvalidValue;
        }
        acc = acc * 10 + digit;
    }
    return acc;
}

fn parseU16Dec(s: []const u8): u16! {
    const v = parseU64Dec(s)?;
    if (v > 65535) {
        return error.InvalidValue;
    }
    return v as u16;
}

fn parseU8Dec(s: []const u8): u8! {
    const v = parseU64Dec(s)?;
    if (v > 255) {
        return error.InvalidValue;
    }
    return v as u8;
}

// ---------------------------------------------------------------------------
// RFC 7983 Packet Demultiplexing (STUN / DTLS / RTP / RTCP)
// ---------------------------------------------------------------------------

pub fn demuxPacketKind(pkt: []const u8): u8 {
    if (pkt.len == 0) {
        return DEMUX_UNKNOWN;
    }
    const b0 = pkt[0];
    if (b0 <= 3) {
        return DEMUX_STUN;
    }
    if (b0 >= 20 && b0 <= 63) {
        return DEMUX_DTLS;
    }
    if (b0 >= 128 && b0 <= 191) {
        if (pkt.len >= 2) {
            const pt = pkt[1] & 0x7F;
            if (pt >= 64 && pt <= 95) {
                return DEMUX_RTCP;
            }
        }
        return DEMUX_RTP;
    }
    return DEMUX_UNKNOWN;
}

// ---------------------------------------------------------------------------
// 1. RTP / RTCP Zero-Copy Codec (RFC 3550 + RFC 4585 NACK)
// ---------------------------------------------------------------------------

pub struct RtpHeader {
    pub version: u8 = RTP_VERSION;
    pub padding: bool = false;
    pub extension: bool = false;
    pub csrc_count: u8 = 0;
    pub marker: bool = false;
    pub payload_type: u8 = 0;
    pub seq: u16 = 0;
    pub timestamp: u32 = 0;
    pub ssrc: u32 = 0;
    pub csrc: [15]u32 = [0; 15];
    pub ext_profile: u16 = 0;
    pub ext_offset: usize = 0;
    pub ext_len: usize = 0;
    pub header_len: usize = RTP_MIN_HEADER_LEN;
    pub payload_len: usize = 0;
    pub padding_len: u8 = 0;

    pub fn payloadSlice(pkt: []const u8): []const u8 {
        if (_.header_len > pkt.len || _.header_len + _.payload_len > pkt.len) {
            return pkt[0..0];
        }
        return pkt[_.header_len.._.header_len + _.payload_len];
    }

    pub fn csrcAt(idx: usize): u32 {
        if (idx >= usize(_.csrc_count) || idx >= RTP_MAX_CSRC) {
            return 0;
        }
        return _.csrc[idx];
    }
}

pub fn parseRtpHeader(pkt: []const u8): RtpHeader! {
    if (pkt.len < RTP_MIN_HEADER_LEN) {
        return error.Truncated;
    }
    const b0 = pkt[0];
    const version: u8 = (b0 >> 6) & 0x03;
    if (version != RTP_VERSION) {
        return error.InvalidVersion;
    }
    const has_padding = ((b0 >> 5) & 0x01) != 0;
    const has_extension = ((b0 >> 4) & 0x01) != 0;
    const cc: u8 = b0 & 0x0F;

    const b1 = pkt[1];
    const marker = ((b1 >> 7) & 0x01) != 0;
    const pt: u8 = b1 & 0x7F;

    const seq = readU16Be(pkt, 2);
    const ts = readU32Be(pkt, 4);
    const ssrc = readU32Be(pkt, 8);

    let off: usize = RTP_MIN_HEADER_LEN;
    const cc_bytes: usize = usize(cc) * 4;
    if (off + cc_bytes > pkt.len) {
        return error.Truncated;
    }

    let hdr = RtpHeader{
        version: version,
        padding: has_padding,
        extension: has_extension,
        csrc_count: cc,
        marker: marker,
        payload_type: pt,
        seq: seq,
        timestamp: ts,
        ssrc: ssrc,
    };

    let i: usize = 0;
    while (i < usize(cc)) : (i += 1) {
        hdr.csrc[i] = readU32Be(pkt, off + i * 4);
    }
    off += cc_bytes;

    if (has_extension) {
        if (off + 4 > pkt.len) {
            return error.InvalidExtension;
        }
        const ext_profile = readU16Be(pkt, off);
        const ext_words: usize = readU16Be(pkt, off + 2);
        const ext_bytes: usize = ext_words * 4;
        off += 4;
        if (ext_bytes > pkt.len - off) {
            return error.InvalidExtension;
        }
        hdr.ext_profile = ext_profile;
        hdr.ext_offset = off;
        hdr.ext_len = ext_bytes;
        off += ext_bytes;
    }

    hdr.header_len = off;

    if (has_padding) {
        if (pkt.len <= off) {
            return error.InvalidPadding;
        }
        const pad_len = pkt[pkt.len - 1];
        if (pad_len == 0 || usize(pad_len) > pkt.len - off) {
            return error.InvalidPadding;
        }
        hdr.padding_len = pad_len;
        hdr.payload_len = pkt.len - off - usize(pad_len);
    } else {
        hdr.padding_len = 0;
        hdr.payload_len = pkt.len - off;
    }

    return hdr;
}

pub fn writeRtpHeader(hdr: RtpHeader, out: []u8): usize! {
    if (hdr.version != RTP_VERSION) {
        return error.InvalidVersion;
    }
    if (usize(hdr.csrc_count) > RTP_MAX_CSRC) {
        return error.InvalidCsrcCount;
    }
    if (hdr.payload_type > 127) {
        return error.InvalidPayloadType;
    }
    if (hdr.extension && (hdr.ext_len % 4) != 0) {
        return error.InvalidExtension;
    }

    let needed: usize = RTP_MIN_HEADER_LEN + usize(hdr.csrc_count) * 4;
    if (hdr.extension) {
        needed += 4 + hdr.ext_len;
    }
    if (out.len < needed) {
        return error.BufferTooSmall;
    }

    let b0: u8 = (hdr.version & 0x03) << 6;
    if (hdr.padding) {
        b0 = b0 | 0x20;
    }
    if (hdr.extension) {
        b0 = b0 | 0x10;
    }
    b0 = b0 | (hdr.csrc_count & 0x0F);

    let b1: u8 = hdr.payload_type & 0x7F;
    if (hdr.marker) {
        b1 = b1 | 0x80;
    }

    out[0] = b0;
    out[1] = b1;
    writeU16Be(out, 2, hdr.seq);
    writeU32Be(out, 4, hdr.timestamp);
    writeU32Be(out, 8, hdr.ssrc);

    let off: usize = RTP_MIN_HEADER_LEN;
    let i: usize = 0;
    while (i < usize(hdr.csrc_count)) : (i += 1) {
        writeU32Be(out, off, hdr.csrc[i]);
        off += 4;
    }

    if (hdr.extension) {
        const ext_words = (hdr.ext_len / 4) as u16;
        writeU16Be(out, off, hdr.ext_profile);
        writeU16Be(out, off + 2, ext_words);
        off += 4;
        let k: usize = 0;
        while (k < hdr.ext_len) : (k += 1) {
            out[off + k] = 0;
        }
        off += hdr.ext_len;
    }

    return off;
}

pub struct RtcpHeader {
    pub version: u8 = RTP_VERSION;
    pub padding: bool = false;
    pub count_or_fmt: u8 = 0;
    pub packet_type: u8 = 0;
    pub length_words: u16 = 0;
    pub byte_len: usize = 0;
    pub sender_ssrc: u32 = 0;
    pub media_ssrc: u32 = 0;

    pub fn isSr(): bool {
        return _.packet_type == RTCP_PT_SR;
    }

    pub fn isRr(): bool {
        return _.packet_type == RTCP_PT_RR;
    }

    pub fn isSdes(): bool {
        return _.packet_type == RTCP_PT_SDES;
    }

    pub fn isBye(): bool {
        return _.packet_type == RTCP_PT_BYE;
    }

    pub fn isNack(): bool {
        return _.packet_type == RTCP_PT_RTPFB && _.count_or_fmt == RTCP_FMT_NACK;
    }
}

pub fn parseRtcpHeader(pkt: []const u8): RtcpHeader! {
    if (pkt.len < RTCP_MIN_HEADER_LEN) {
        return error.Truncated;
    }
    const b0 = pkt[0];
    const version: u8 = (b0 >> 6) & 0x03;
    if (version != RTP_VERSION) {
        return error.InvalidVersion;
    }
    const has_padding = ((b0 >> 5) & 0x01) != 0;
    const count_or_fmt: u8 = b0 & 0x1F;
    const pt = pkt[1];
    if (pt < 200 || pt > 211) {
        return error.InvalidPacketType;
    }
    const length_words = readU16Be(pkt, 2);
    const byte_len: usize = (usize(length_words) + 1) * 4;
    if (byte_len > pkt.len) {
        return error.Truncated;
    }
    if (has_padding) {
        if (byte_len <= RTCP_MIN_HEADER_LEN) {
            return error.InvalidPadding;
        }
        const pad_len = pkt[byte_len - 1];
        if (pad_len == 0 || usize(pad_len) > byte_len - RTCP_MIN_HEADER_LEN) {
            return error.InvalidPadding;
        }
    }

    let sender_ssrc: u32 = 0;
    let media_ssrc: u32 = 0;
    if (byte_len >= 8) {
        sender_ssrc = readU32Be(pkt, 4);
    }
    if ((pt == RTCP_PT_RTPFB || pt == RTCP_PT_PSFB) && byte_len >= 12) {
        media_ssrc = readU32Be(pkt, 8);
    }

    return RtcpHeader{
        version: version,
        padding: has_padding,
        count_or_fmt: count_or_fmt,
        packet_type: pt,
        length_words: length_words,
        byte_len: byte_len,
        sender_ssrc: sender_ssrc,
        media_ssrc: media_ssrc,
    };
}

pub struct RtcpNackFci {
    pub sender_ssrc: u32 = 0;
    pub media_ssrc: u32 = 0;
    pub base_seq: u16 = 0;
    pub bitmask: u16 = 0;
}

pub fn buildRtcpNack(ssrc: u32, media_ssrc: u32, base_seq: u16, bitmask: u16, out: []u8): usize! {
    if (out.len < RTCP_NACK_PACKET_LEN) {
        return error.BufferTooSmall;
    }
    // V=2, P=0, FMT=1 (Generic NACK) -> 0x81
    out[0] = ((RTP_VERSION & 0x03) << 6) | (RTCP_FMT_NACK & 0x1F);
    out[1] = RTCP_PT_RTPFB;
    // length = 3 (in 32-bit words minus 1) => 16 bytes total
    writeU16Be(out, 2, 3);
    writeU32Be(out, 4, ssrc);
    writeU32Be(out, 8, media_ssrc);
    writeU16Be(out, 12, base_seq);
    writeU16Be(out, 14, bitmask);
    return RTCP_NACK_PACKET_LEN;
}

pub fn parseRtcpNack(pkt: []const u8): RtcpNackFci! {
    const hdr = parseRtcpHeader(pkt)?;
    if (!hdr.isNack()) {
        return error.InvalidPacketType;
    }
    if (hdr.byte_len < RTCP_NACK_PACKET_LEN) {
        return error.Truncated;
    }
    return RtcpNackFci{
        sender_ssrc: hdr.sender_ssrc,
        media_ssrc: hdr.media_ssrc,
        base_seq: readU16Be(pkt, 12),
        bitmask: readU16Be(pkt, 14),
    };
}

// ---------------------------------------------------------------------------
// 2. STUN Protocol Codec (RFC 5389)
// ---------------------------------------------------------------------------

pub fn stunCrc32(data: []const u8): u32 {
    let crc: u32 = 0xFFFFFFFF;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        crc = crc ^ u32(data[i]);
        let bit: u8 = 0;
        while (bit < 8) : (bit += 1) {
            if ((crc & 1) != 0) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return crc ^ 0xFFFFFFFF;
}

pub fn stunFingerprint(prefix: []const u8): u32 {
    return stunCrc32(prefix) ^ STUN_FINGERPRINT_XOR;
}

pub struct XorMappedAddr {
    pub ip4: [4]u8 = [0; 4];
    pub port: u16 = 0;
}

pub fn encodeXorMappedAddr(ip4: [4]u8, port: u16, out: []u8): usize! {
    if (out.len < 8) {
        return error.BufferTooSmall;
    }
    const xport: u16 = port ^ ((STUN_MAGIC_COOKIE >> 16) as u16);
    out[0] = 0x00;
    out[1] = STUN_FAMILY_IPV4;
    writeU16Be(out, 2, xport);
    out[4] = ip4[0] ^ ((STUN_MAGIC_COOKIE >> 24) & 0xFF) as u8;
    out[5] = ip4[1] ^ ((STUN_MAGIC_COOKIE >> 16) & 0xFF) as u8;
    out[6] = ip4[2] ^ ((STUN_MAGIC_COOKIE >> 8) & 0xFF) as u8;
    out[7] = ip4[3] ^ (STUN_MAGIC_COOKIE & 0xFF) as u8;
    return 8;
}

pub fn decodeXorMappedAddr(attr: []const u8): XorMappedAddr! {
    if (attr.len < 8) {
        return error.Truncated;
    }
    if (attr[1] != STUN_FAMILY_IPV4) {
        return error.InvalidAddressFamily;
    }
    const xport = readU16Be(attr, 2);
    const port: u16 = xport ^ ((STUN_MAGIC_COOKIE >> 16) as u16);
    let ip4: [4]u8 = [0; 4];
    ip4[0] = attr[4] ^ ((STUN_MAGIC_COOKIE >> 24) & 0xFF) as u8;
    ip4[1] = attr[5] ^ ((STUN_MAGIC_COOKIE >> 16) & 0xFF) as u8;
    ip4[2] = attr[6] ^ ((STUN_MAGIC_COOKIE >> 8) & 0xFF) as u8;
    ip4[3] = attr[7] ^ (STUN_MAGIC_COOKIE & 0xFF) as u8;
    return XorMappedAddr{
        ip4: ip4,
        port: port,
    };
}

pub struct StunMessage {
    pub msg_type: u16 = 0;
    pub msg_len: u16 = 0;
    pub magic_cookie: u32 = STUN_MAGIC_COOKIE;
    pub txn_id: [12]u8 = [0; 12];
    pub total_len: usize = STUN_HEADER_LEN;
    pub attr_count: usize = 0;
    pub has_xor_mapped: bool = false;
    pub xor_mapped_ip4: [4]u8 = [0; 4];
    pub xor_mapped_port: u16 = 0;
    pub has_fingerprint: bool = false;
    pub fingerprint_valid: bool = false;

    pub fn isBindingRequest(): bool {
        return _.msg_type == STUN_BINDING_REQUEST;
    }

    pub fn isBindingSuccess(): bool {
        return _.msg_type == STUN_BINDING_SUCCESS;
    }
}

pub fn buildStunBindingRequest(txn_id: [12]u8, out: []u8): usize! {
    // 20-byte header + 8-byte FINGERPRINT attribute = 28 bytes
    const total: usize = STUN_HEADER_LEN + 8;
    if (out.len < total) {
        return error.BufferTooSmall;
    }
    writeU16Be(out, 0, STUN_BINDING_REQUEST);
    writeU16Be(out, 2, 8);
    writeU32Be(out, 4, STUN_MAGIC_COOKIE);
    let i: usize = 0;
    while (i < 12) : (i += 1) {
        out[8 + i] = txn_id[i];
    }
    const fp = stunFingerprint(out[0..STUN_HEADER_LEN]);
    writeU16Be(out, 20, STUN_ATTR_FINGERPRINT);
    writeU16Be(out, 22, 4);
    writeU32Be(out, 24, fp);
    return total;
}

pub fn buildStunBindingSuccess(txn_id: [12]u8, ip4: [4]u8, port: u16, out: []u8): usize! {
    // 20-byte header + 12-byte XOR-MAPPED-ADDRESS + 8-byte FINGERPRINT = 40 bytes
    const total: usize = STUN_HEADER_LEN + 12 + 8;
    if (out.len < total) {
        return error.BufferTooSmall;
    }
    writeU16Be(out, 0, STUN_BINDING_SUCCESS);
    writeU16Be(out, 2, 20);
    writeU32Be(out, 4, STUN_MAGIC_COOKIE);
    let i: usize = 0;
    while (i < 12) : (i += 1) {
        out[8 + i] = txn_id[i];
    }
    writeU16Be(out, 20, STUN_ATTR_XOR_MAPPED_ADDRESS);
    writeU16Be(out, 22, 8);
    * = encodeXorMappedAddr(ip4, port, out[24..32])?;

    const fp = stunFingerprint(out[0..32]);
    writeU16Be(out, 32, STUN_ATTR_FINGERPRINT);
    writeU16Be(out, 34, 4);
    writeU32Be(out, 36, fp);
    return total;
}

pub fn parseStunHeader(pkt: []const u8): StunMessage! {
    if (pkt.len < STUN_HEADER_LEN) {
        return error.Truncated;
    }
    // RFC 5389 §6: most significant 2 bits of STUN message MUST be zero
    if ((pkt[0] & 0xC0) != 0) {
        return error.InvalidStunHeader;
    }
    const msg_type = readU16Be(pkt, 0);
    const msg_len = readU16Be(pkt, 2);
    if ((msg_len & 0x03) != 0) {
        return error.InvalidStunLength;
    }
    const magic = readU32Be(pkt, 4);
    if (magic != STUN_MAGIC_COOKIE) {
        return error.InvalidStunMagic;
    }
    const total_len: usize = STUN_HEADER_LEN + usize(msg_len);
    if (total_len > pkt.len) {
        return error.Truncated;
    }

    let msg = StunMessage{
        msg_type: msg_type,
        msg_len: msg_len,
        magic_cookie: magic,
        total_len: total_len,
    };
    let i: usize = 0;
    while (i < 12) : (i += 1) {
        msg.txn_id[i] = pkt[8 + i];
    }

    // Walk STUN TLV attributes with 4-byte padding alignment
    let off: usize = STUN_HEADER_LEN;
    while (off < total_len) {
        if (total_len - off < 4) {
            return error.InvalidStunAttribute;
        }
        const attr_type = readU16Be(pkt, off);
        const attr_len: usize = readU16Be(pkt, off + 2);
        const val_start = off + 4;
        if (attr_len > total_len - val_start) {
            return error.InvalidStunAttribute;
        }
        const padded_len: usize = (attr_len + 3) & ~usize(3);
        if (padded_len > total_len - val_start) {
            return error.InvalidStunAttribute;
        }

        msg.attr_count += 1;

        if (attr_type == STUN_ATTR_XOR_MAPPED_ADDRESS) {
            if (attr_len >= 8 && pkt[val_start + 1] == STUN_FAMILY_IPV4) {
                const decoded = decodeXorMappedAddr(pkt[val_start..val_start + attr_len])?;
                msg.has_xor_mapped = true;
                msg.xor_mapped_ip4 = decoded.ip4;
                msg.xor_mapped_port = decoded.port;
            }
        } else if (attr_type == STUN_ATTR_FINGERPRINT) {
            if (attr_len != 4) {
                return error.InvalidFingerprint;
            }
            const stored_fp = readU32Be(pkt, val_start);
            const expected_fp = stunFingerprint(pkt[0..off]);
            msg.has_fingerprint = true;
            msg.fingerprint_valid = (stored_fp == expected_fp);
        }

        off = val_start + padded_len;
    }

    return msg;
}

// ---------------------------------------------------------------------------
// 3. SDP Lightweight Offer/Answer Parser & Builder (RFC 4566 / RFC 8843)
// ---------------------------------------------------------------------------

pub struct SdpMediaLine {
    pub kind: u8 = SDP_MEDIA_UNKNOWN;
    pub port: u16 = 9;
    pub mid_buf: [32]u8 = [0; 32];
    pub mid_len: usize = 0;
    pub ufrag_buf: [64]u8 = [0; 64];
    pub ufrag_len: usize = 0;
    pub pwd_buf: [64]u8 = [0; 64];
    pub pwd_len: usize = 0;
    pub fingerprint_buf: [128]u8 = [0; 128];
    pub fingerprint_len: usize = 0;
    pub setup: u8 = SDP_SETUP_ACTPASS;
    pub payload_types: [8]u8 = [0; 8];
    pub payload_type_count: usize = 0;

    pub fn mid(): []const u8 {
        return _.mid_buf[0.._.mid_len];
    }

    pub fn ufrag(): []const u8 {
        return _.ufrag_buf[0.._.ufrag_len];
    }

    pub fn pwd(): []const u8 {
        return _.pwd_buf[0.._.pwd_len];
    }

    pub fn fingerprint(): []const u8 {
        return _.fingerprint_buf[0.._.fingerprint_len];
    }

    pub fn setMid(*_, s: []const u8) {
        _.mid_len = copyIntoFixed(_.mid_buf[0..], s);
    }

    pub fn setUfrag(*_, s: []const u8) {
        _.ufrag_len = copyIntoFixed(_.ufrag_buf[0..], s);
    }

    pub fn setPwd(*_, s: []const u8) {
        _.pwd_len = copyIntoFixed(_.pwd_buf[0..], s);
    }

    pub fn setFingerprint(*_, s: []const u8) {
        _.fingerprint_len = copyIntoFixed(_.fingerprint_buf[0..], s);
    }

    pub fn addPayloadType(*_, pt: u8): bool {
        if (_.payload_type_count >= SDP_MAX_PAYLOAD_TYPES) {
            return false;
        }
        _.payload_types[_.payload_type_count] = pt;
        _.payload_type_count += 1;
        return true;
    }
}

pub struct SdpSession {
    pub version: u8 = 0;
    pub session_id: u64 = 1;
    pub ufrag_buf: [64]u8 = [0; 64];
    pub ufrag_len: usize = 0;
    pub pwd_buf: [64]u8 = [0; 64];
    pub pwd_len: usize = 0;
    pub fingerprint_buf: [128]u8 = [0; 128];
    pub fingerprint_len: usize = 0;
    pub setup: u8 = SDP_SETUP_ACTPASS;
    pub media: [4]SdpMediaLine = [SdpMediaLine{}, SdpMediaLine{}, SdpMediaLine{}, SdpMediaLine{}];
    pub media_count: usize = 0;

    pub fn ufrag(): []const u8 {
        return _.ufrag_buf[0.._.ufrag_len];
    }

    pub fn pwd(): []const u8 {
        return _.pwd_buf[0.._.pwd_len];
    }

    pub fn fingerprint(): []const u8 {
        return _.fingerprint_buf[0.._.fingerprint_len];
    }

    pub fn setUfrag(*_, s: []const u8) {
        _.ufrag_len = copyIntoFixed(_.ufrag_buf[0..], s);
    }

    pub fn setPwd(*_, s: []const u8) {
        _.pwd_len = copyIntoFixed(_.pwd_buf[0..], s);
    }

    pub fn setFingerprint(*_, s: []const u8) {
        _.fingerprint_len = copyIntoFixed(_.fingerprint_buf[0..], s);
    }

    pub fn addMedia(*_, m: SdpMediaLine): usize! {
        if (_.media_count >= SDP_MAX_MEDIA_LINES) {
            return error.TooManyMediaLines;
        }
        const idx = _.media_count;
        _.media[idx] = m;
        _.media_count += 1;
        return idx;
    }
}

fn parseSetupRole(s: []const u8): u8 {
    if (sliceEquals(s, "actpass")) {
        return SDP_SETUP_ACTPASS;
    }
    if (sliceEquals(s, "active")) {
        return SDP_SETUP_ACTIVE;
    }
    if (sliceEquals(s, "passive")) {
        return SDP_SETUP_PASSIVE;
    }
    return SDP_SETUP_NONE;
}

fn setupRoleName(role: u8): []const u8 {
    if (role == SDP_SETUP_ACTIVE) {
        return "active";
    }
    if (role == SDP_SETUP_PASSIVE) {
        return "passive";
    }
    return "actpass";
}

fn mediaKindName(kind: u8): []const u8 {
    if (kind == SDP_MEDIA_AUDIO) {
        return "audio";
    }
    if (kind == SDP_MEDIA_VIDEO) {
        return "video";
    }
    return "application";
}

fn parseMediaLine(body: []const u8): SdpMediaLine! {
    // Format: <media> <port> <proto> <fmt>...
    let m = SdpMediaLine{};
    let pos: usize = 0;
    let tok_idx: usize = 0;

    while (pos < body.len) {
        while (pos < body.len && body[pos] == 0x20) : (pos += 1) {}
        if (pos >= body.len) {
            break;
        }
        const start = pos;
        while (pos < body.len && body[pos] != 0x20) : (pos += 1) {}
        const tok = body[start..pos];

        if (tok_idx == 0) {
            if (sliceEquals(tok, "audio")) {
                m.kind = SDP_MEDIA_AUDIO;
            } else if (sliceEquals(tok, "video")) {
                m.kind = SDP_MEDIA_VIDEO;
            } else if (sliceEquals(tok, "application")) {
                m.kind = SDP_MEDIA_APPLICATION;
            } else {
                m.kind = SDP_MEDIA_UNKNOWN;
            }
        } else if (tok_idx == 1) {
            m.port = parseU16Dec(tok) catch return error.InvalidSdp;
        } else if (tok_idx >= 3) {
            if (m.kind == SDP_MEDIA_AUDIO || m.kind == SDP_MEDIA_VIDEO) {
                const pt = parseU8Dec(tok) catch 255;
                if (pt <= 127) {
                    * = m.addPayloadType(pt);
                }
            }
        }
        tok_idx += 1;
    }

    if (tok_idx < 3) {
        return error.InvalidSdp;
    }
    return m;
}

pub fn parseSdp(text: []const u8): SdpSession! {
    if (text.len < 3) {
        return error.InvalidSdp;
    }
    let sess = SdpSession{};
    let saw_version = false;
    let in_media = false;
    let cur_media: usize = 0;

    let pos: usize = 0;
    while (pos < text.len) {
        const line_start = pos;
        while (pos < text.len && text[pos] != 0x0A) : (pos += 1) {}
        let line_end = pos;
        if (pos < text.len && text[pos] == 0x0A) {
            pos += 1;
        }
        if (line_end > line_start && text[line_end - 1] == 0x0D) {
            line_end -= 1;
        }
        if (line_end == line_start) {
            continue;
        }
        const line = text[line_start..line_end];
        if (line.len < 2 || line[1] != 0x3D) {
            return error.InvalidSdp;
        }
        const tag = line[0];
        const val = line[2..];

        if (tag == 0x76) {
            // v=
            const v = parseU8Dec(val) catch return error.InvalidSdp;
            if (v != 0) {
                return error.InvalidSdp;
            }
            sess.version = v;
            saw_version = true;
        } else if (tag == 0x6F) {
            // o=<user> <sess-id> ...
            let p: usize = 0;
            while (p < val.len && val[p] != 0x20) : (p += 1) {}
            while (p < val.len && val[p] == 0x20) : (p += 1) {}
            const id_start = p;
            while (p < val.len && val[p] != 0x20) : (p += 1) {}
            if (p > id_start) {
                sess.session_id = parseU64Dec(val[id_start..p]) catch sess.session_id;
            }
        } else if (tag == 0x6D) {
            // m=
            const m = parseMediaLine(val)?;
            cur_media = sess.addMedia(m)?;
            in_media = true;
        } else if (tag == 0x61) {
            // a=
            if (startsWith(val, "ice-ufrag:")) {
                const v = val[10..];
                if (in_media) {
                    sess.media[cur_media].setUfrag(v);
                }
                if (!in_media || sess.ufrag_len == 0) {
                    sess.setUfrag(v);
                }
            } else if (startsWith(val, "ice-pwd:")) {
                const v = val[8..];
                if (in_media) {
                    sess.media[cur_media].setPwd(v);
                }
                if (!in_media || sess.pwd_len == 0) {
                    sess.setPwd(v);
                }
            } else if (startsWith(val, "fingerprint:")) {
                const v = val[12..];
                if (in_media) {
                    sess.media[cur_media].setFingerprint(v);
                }
                if (!in_media || sess.fingerprint_len == 0) {
                    sess.setFingerprint(v);
                }
            } else if (startsWith(val, "setup:")) {
                const role = parseSetupRole(val[6..]);
                if (role != SDP_SETUP_NONE) {
                    if (in_media) {
                        sess.media[cur_media].setup = role;
                    }
                    if (!in_media || sess.setup == SDP_SETUP_NONE) {
                        sess.setup = role;
                    }
                }
            } else if (startsWith(val, "mid:")) {
                if (in_media) {
                    sess.media[cur_media].setMid(val[4..]);
                }
            }
        }
    }

    if (!saw_version) {
        return error.InvalidSdp;
    }
    return sess;
}

pub fn formatSdpOffer(sess: SdpSession, out: []u8): usize! {
    let pos: usize = 0;
    pos = appendBytes(out, pos, "v=0\r\n")?;
    pos = appendBytes(out, pos, "o=- ")?;
    pos = appendU64Dec(out, pos, sess.session_id)?;
    pos = appendBytes(out, pos, " 2 IN IP4 127.0.0.1\r\n")?;
    pos = appendBytes(out, pos, "s=-\r\n")?;
    pos = appendBytes(out, pos, "t=0 0\r\n")?;

    // Emit RFC 8843 BUNDLE group if any media lines carry a mid
    let has_mid = false;
    let i: usize = 0;
    while (i < sess.media_count) : (i += 1) {
        if (sess.media[i].mid_len > 0) {
            has_mid = true;
        }
    }
    if (has_mid) {
        pos = appendBytes(out, pos, "a=group:BUNDLE")?;
        let j: usize = 0;
        while (j < sess.media_count) : (j += 1) {
            if (sess.media[j].mid_len > 0) {
                pos = appendByte(out, pos, 0x20)?;
                pos = appendBytes(out, pos, sess.media[j].mid())?;
            }
        }
        pos = appendBytes(out, pos, "\r\n")?;
    }

    if (sess.media_count == 0) {
        if (sess.ufrag_len > 0) {
            pos = appendBytes(out, pos, "a=ice-ufrag:")?;
            pos = appendBytes(out, pos, sess.ufrag())?;
            pos = appendBytes(out, pos, "\r\n")?;
        }
        if (sess.pwd_len > 0) {
            pos = appendBytes(out, pos, "a=ice-pwd:")?;
            pos = appendBytes(out, pos, sess.pwd())?;
            pos = appendBytes(out, pos, "\r\n")?;
        }
        if (sess.fingerprint_len > 0) {
            pos = appendBytes(out, pos, "a=fingerprint:")?;
            pos = appendBytes(out, pos, sess.fingerprint())?;
            pos = appendBytes(out, pos, "\r\n")?;
        }
    }

    let m_idx: usize = 0;
    while (m_idx < sess.media_count) : (m_idx += 1) {
        const m = sess.media[m_idx];
        pos = appendBytes(out, pos, "m=")?;
        pos = appendBytes(out, pos, mediaKindName(m.kind))?;
        pos = appendByte(out, pos, 0x20)?;
        pos = appendU64Dec(out, pos, u64(m.port))?;
        if (m.kind == SDP_MEDIA_APPLICATION) {
            pos = appendBytes(out, pos, " UDP/DTLS/SCTP webrtc-datachannel\r\n")?;
        } else {
            pos = appendBytes(out, pos, " UDP/TLS/RTP/SAVPF")?;
            if (m.payload_type_count == 0) {
                pos = appendBytes(out, pos, " 96")?;
            } else {
                let p_idx: usize = 0;
                while (p_idx < m.payload_type_count) : (p_idx += 1) {
                    pos = appendByte(out, pos, 0x20)?;
                    pos = appendU64Dec(out, pos, u64(m.payload_types[p_idx]))?;
                }
            }
            pos = appendBytes(out, pos, "\r\n")?;
        }
        pos = appendBytes(out, pos, "c=IN IP4 0.0.0.0\r\n")?;

        const uf = if (m.ufrag_len > 0) m.ufrag() else sess.ufrag();
        if (uf.len > 0) {
            pos = appendBytes(out, pos, "a=ice-ufrag:")?;
            pos = appendBytes(out, pos, uf)?;
            pos = appendBytes(out, pos, "\r\n")?;
        }
        const pw = if (m.pwd_len > 0) m.pwd() else sess.pwd();
        if (pw.len > 0) {
            pos = appendBytes(out, pos, "a=ice-pwd:")?;
            pos = appendBytes(out, pos, pw)?;
            pos = appendBytes(out, pos, "\r\n")?;
        }
        const fp = if (m.fingerprint_len > 0) m.fingerprint() else sess.fingerprint();
        if (fp.len > 0) {
            pos = appendBytes(out, pos, "a=fingerprint:")?;
            pos = appendBytes(out, pos, fp)?;
            pos = appendBytes(out, pos, "\r\n")?;
        }
        const role = if (m.setup != SDP_SETUP_NONE) m.setup else sess.setup;
        pos = appendBytes(out, pos, "a=setup:")?;
        pos = appendBytes(out, pos, setupRoleName(role))?;
        pos = appendBytes(out, pos, "\r\n")?;
        if (m.mid_len > 0) {
            pos = appendBytes(out, pos, "a=mid:")?;
            pos = appendBytes(out, pos, m.mid())?;
            pos = appendBytes(out, pos, "\r\n")?;
        }
        pos = appendBytes(out, pos, "a=sendrecv\r\n")?;
        pos = appendBytes(out, pos, "a=rtcp-mux\r\n")?;
    }

    return pos;
}

// ---------------------------------------------------------------------------
// 4. Sans-IO Jitter / NACK Sequence Tracker (RFC 3550 + RFC 4585)
// ---------------------------------------------------------------------------

pub struct SeqObserveResult {
    pub is_new: bool = false;
    pub is_duplicate: bool = false;
    pub is_reordered: bool = false;
    pub is_late: bool = false;
    pub gap: u16 = 0;
    pub extended_seq: u64 = 0;
}

pub struct NackEntry {
    pub has_nack: bool = false;
    pub base_seq: u16 = 0;
    pub bitmask: u16 = 0;
}

pub struct SeqTracker {
    pub initialized: bool = false;
    pub highest_seq: u16 = 0;
    pub cycles: u32 = 0;
    pub base_ext_seq: u64 = 0;
    pub received_mask: u64 = 0;
    pub packets_received: u64 = 0;
    pub packets_duplicate: u64 = 0;
    pub packets_reordered: u64 = 0;
    pub packets_late: u64 = 0;

    pub static fn init(): SeqTracker {
        return SeqTracker{};
    }

    pub fn highestExtSeq(): u64 {
        return (u64(_.cycles) << 16) | u64(_.highest_seq);
    }

    pub fn observe(*_, seq: u16): SeqObserveResult {
        if (!_.initialized) {
            _.initialized = true;
            _.highest_seq = seq;
            _.cycles = 0;
            _.base_ext_seq = u64(seq);
            _.received_mask = 1;
            _.packets_received = 1;
            return SeqObserveResult{
                is_new: true,
                extended_seq: u64(seq),
            };
        }

        const diff_u16: u16 = ((u32(seq) + 65536 - u32(_.highest_seq)) & 0xFFFF) as u16;
        if (diff_u16 == 0) {
            _.packets_duplicate += 1;
            return SeqObserveResult{
                is_duplicate: true,
                extended_seq: _.highestExtSeq(),
            };
        }

        if (diff_u16 < 0x8000) {
            // Forward advance (possibly wrapping across 65535 -> 0)
            if (seq < _.highest_seq) {
                _.cycles += 1;
            }
            if (diff_u16 >= 64) {
                _.received_mask = 1;
            } else {
                const shift = diff_u16 as u6;
                _.received_mask = (_.received_mask << shift) | 1;
            }
            _.highest_seq = seq;
            _.packets_received += 1;
            return SeqObserveResult{
                is_new: true,
                gap: diff_u16 - 1,
                extended_seq: _.highestExtSeq(),
            };
        }

        // Backward packet (out-of-order / reordered)
        const age: u16 = ((u32(_.highest_seq) + 65536 - u32(seq)) & 0xFFFF) as u16;
        const high_ext = _.highestExtSeq();
        const ext_seq: u64 = if (high_ext >= u64(age)) high_ext - u64(age) else 0;

        if (age >= 64) {
            _.packets_late += 1;
            return SeqObserveResult{
                is_late: true,
                is_reordered: true,
                extended_seq: ext_seq,
            };
        }

        const bit: u64 = u64(1) << (age as u6);
        if ((_.received_mask & bit) != 0) {
            _.packets_duplicate += 1;
            return SeqObserveResult{
                is_duplicate: true,
                is_reordered: true,
                extended_seq: ext_seq,
            };
        }

        _.received_mask = _.received_mask | bit;
        _.packets_received += 1;
        _.packets_reordered += 1;
        return SeqObserveResult{
            is_new: true,
            is_reordered: true,
            extended_seq: ext_seq,
        };
    }

    pub fn buildNack(): NackEntry {
        if (!_.initialized) {
            return NackEntry{};
        }
        const span = _.highestExtSeq() - _.base_ext_seq;
        let max_age: u16 = 63;
        if (span < 63) {
            max_age = span as u16;
        }
        if (max_age == 0) {
            return NackEntry{};
        }

        // Find oldest missing sequence number in the 64-packet window
        let oldest_missing: u16 = 0;
        let age: u16 = max_age;
        while (age >= 1) : (age -= 1) {
            const bit: u64 = u64(1) << (age as u6);
            if ((_.received_mask & bit) == 0) {
                oldest_missing = age;
                break;
            }
        }

        if (oldest_missing == 0) {
            return NackEntry{};
        }

        const base_seq: u16 = ((u32(_.highest_seq) + 65536 - u32(oldest_missing)) & 0xFFFF) as u16;
        let mask: u16 = 0;
        let k: u16 = 1;
        while (k <= 16) : (k += 1) {
            if (k >= oldest_missing) {
                break;
            }
            const check_age: u16 = oldest_missing - k;
            const check_bit: u64 = u64(1) << (check_age as u6);
            if ((_.received_mask & check_bit) == 0) {
                mask = mask | (u16(1) << ((k - 1) as u4));
            }
        }

        return NackEntry{
            has_nack: true,
            base_seq: base_seq,
            bitmask: mask,
        };
    }

    pub fn expectedPackets(): u64 {
        if (!_.initialized) {
            return 0;
        }
        return _.highestExtSeq() - _.base_ext_seq + 1;
    }

    pub fn lostPackets(): u64 {
        const exp = _.expectedPackets();
        if (exp > _.packets_received) {
            return exp - _.packets_received;
        }
        return 0;
    }

    pub fn lossRatePermille(): u32 {
        const exp = _.expectedPackets();
        if (exp == 0) {
            return 0;
        }
        return ((_.lostPackets() * 1000) / exp) as u32;
    }

    pub fn lossRate(): f64 {
        const exp = _.expectedPackets();
        if (exp == 0) {
            return 0.0;
        }
        return f64(_.lostPackets()) / f64(exp);
    }
}
