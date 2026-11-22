// Origin: Pure-CP Stdlib (RFC 4960 SCTP, RFC 3758 PR-SCTP & RFC 8832 WebRTC DCEP Engine) | Created: 2026-11-23
// 100% pure CP, zero dynamic heap allocation, zero external FFI dependencies, caller-slice WebRTC DataChannel engine:
//   - RFC 4960 Appendix B Castagnoli CRC32C (256-entry lookup table, polynomial 0x82F63B78, verified "123456789" == 0xE3069283)
//   - RFC 4960 12-byte Common Header, 4-byte padded zero-copy chunk iterator & zero-alloc packet builder
//   - Complete core chunk codecs: DATA, INIT, INIT_ACK, SACK, HEARTBEAT, HEARTBEAT_ACK, ABORT,
//     SHUTDOWN, SHUTDOWN_ACK, ERROR, COOKIE_ECHO, COOKIE_ACK, SHUTDOWN_COMPLETE, and RFC 3758 FORWARD_TSN
//   - Unbounded external-slice TSN receive tracker (`SctpRecvTracker`) with RFC 1982 wrap-around arithmetic,
//     multi-word bitmask shifting, duplicate TSN tracking, Gap Ack Block / SACK generation, and PR-SCTP FORWARD_TSN
//   - Unbounded external-slice multi-TSN fragment reassembler (`SctpFragmentReassembler`) for B/E/U/I flags
//   - RFC 8832 WebRTC DataChannel Establishment Protocol (DCEP) OPEN/ACK codecs for all 6 channel types
//   - Full 4-way SCTP handshake & graceful 3-way shutdown state machine (`SctpAssociationState` / `SctpAssociation`)

// ============================================================================
// 1. Protocol Constants, PPIDs, DCEP Types & Error Set
// ============================================================================

/// RFC 4960 §3.1 SCTP common header length in bytes (`12`).
pub const SCTP_COMMON_HEADER_LEN: usize = 12;

/// RFC 4960 §3.2 SCTP chunk header length in bytes (`4`: type + flags + length).
pub const SCTP_CHUNK_HDR_LEN: usize = 4;

/// RFC 4960 §3.3.1 DATA chunk fixed header size in bytes (`4 + 12 = 16`).
pub const SCTP_DATA_FIXED_LEN: usize = 16;

/// RFC 4960 §3.3.2 INIT / INIT_ACK chunk fixed header size in bytes (`4 + 16 = 20`).
pub const SCTP_INIT_FIXED_LEN: usize = 20;

/// RFC 4960 §3.3.4 SACK chunk fixed header size in bytes (`4 + 12 = 16`).
pub const SCTP_SACK_FIXED_LEN: usize = 16;

/// RFC 3758 §3.2 FORWARD_TSN chunk minimum size in bytes (`4 + 4 = 8`).
pub const SCTP_FORWARD_TSN_MIN_LEN: usize = 8;

/// RFC 4960 Appendix B reversed Castagnoli CRC32C polynomial (`0x82F63B78`).
pub const SCTP_CRC32C_REVERSED_POLY: u32 = 0x82F63B78;

// --- RFC 4960 & RFC 3758 Chunk Types ---
pub const SCTP_CHUNK_DATA: u8 = 0;
pub const SCTP_CHUNK_INIT: u8 = 1;
pub const SCTP_CHUNK_INIT_ACK: u8 = 2;
pub const SCTP_CHUNK_SACK: u8 = 3;
pub const SCTP_CHUNK_HEARTBEAT: u8 = 4;
pub const SCTP_CHUNK_HEARTBEAT_ACK: u8 = 5;
pub const SCTP_CHUNK_ABORT: u8 = 6;
pub const SCTP_CHUNK_SHUTDOWN: u8 = 7;
pub const SCTP_CHUNK_SHUTDOWN_ACK: u8 = 8;
pub const SCTP_CHUNK_ERROR: u8 = 9;
pub const SCTP_CHUNK_COOKIE_ECHO: u8 = 10;
pub const SCTP_CHUNK_COOKIE_ACK: u8 = 11;
pub const SCTP_CHUNK_SHUTDOWN_COMPLETE: u8 = 14;
pub const SCTP_CHUNK_FORWARD_TSN: u8 = 192;

// --- RFC 4960 §3.3.1 DATA Chunk Flag Bits ---
/// End fragment bit (`E = 0x01`).
pub const SCTP_DATA_FLAG_E: u8 = 0x01;
/// Beginning fragment bit (`B = 0x02`).
pub const SCTP_DATA_FLAG_B: u8 = 0x02;
/// Unordered delivery bit (`U = 0x04`).
pub const SCTP_DATA_FLAG_U: u8 = 0x04;
/// Immediate SACK request bit (`I = 0x08`, RFC 7053).
pub const SCTP_DATA_FLAG_I: u8 = 0x08;

// --- RFC 4960 / RFC 3758 TLV Parameter Types ---
pub const SCTP_PARAM_HEARTBEAT_INFO: u16 = 0x0001;
pub const SCTP_PARAM_STATE_COOKIE: u16 = 0x0007;
pub const SCTP_PARAM_SUPPORTED_EXTENSIONS: u16 = 0x8008;
pub const SCTP_PARAM_FORWARD_TSN_SUPPORTED: u16 = 0xC000;

// --- RFC 8831 §8 WebRTC DataChannel Payload Protocol Identifiers (PPID) ---
/// WebRTC DataChannel Establishment Protocol (DCEP) control messages (`50`).
pub const SCTP_PPID_DCEP: u32 = 50;
/// WebRTC UTF-8 string payload (`51`).
pub const SCTP_PPID_STRING: u32 = 51;
/// WebRTC binary payload (`53`).
pub const SCTP_PPID_BINARY: u32 = 53;
/// WebRTC empty UTF-8 string payload (`56`, RFC 8831 §6.6).
pub const SCTP_PPID_STRING_EMPTY: u32 = 56;
/// WebRTC empty binary payload (`57`, RFC 8831 §6.6).
pub const SCTP_PPID_BINARY_EMPTY: u32 = 57;

// --- RFC 8832 DCEP Message Types ---
pub const DCEP_MSG_ACK: u8 = 0x02;
pub const DCEP_MSG_OPEN: u8 = 0x03;

// --- RFC 8832 §5.1 DCEP Channel Types ---
pub const DCEP_CH_RELIABLE: u8 = 0x00;
pub const DCEP_CH_RELIABLE_UNORDERED: u8 = 0x80;
pub const DCEP_CH_PARTIAL_RELIABLE_REXMIT: u8 = 0x01;
pub const DCEP_CH_PARTIAL_RELIABLE_REXMIT_UNORDERED: u8 = 0x81;
pub const DCEP_CH_PARTIAL_RELIABLE_TIMED: u8 = 0x02;
pub const DCEP_CH_PARTIAL_RELIABLE_TIMED_UNORDERED: u8 = 0x82;

const SCTP_COOKIE_BODY_LEN: usize = 20;

/// Error set returned by SCTP, PR-SCTP, and WebRTC DCEP operations.
pub err SctpDcError {
    PacketTooShort,
    BufferTooSmall,
    ChecksumMismatch,
    InvalidChunkLength,
    InvalidChunkType,
    InvalidParamLength,
    InvalidVerificationTag,
    InvalidState,
    InvalidCookie,
    WindowOverflow,
    FragmentOutOfOrder,
    FragmentStreamMismatch,
    FragmentIncomplete,
    InvalidDcepMessage,
    InvalidChannelType,
    GapBlockOverflow,
    DuplicateTsnOverflow,
    StreamCountZero,
}

// ============================================================================
// 2. Byte-Order Helpers, RFC 1982 Serial Math & Castagnoli CRC32C Table
// ============================================================================

fn readBeU16(buf: []const u8, off: usize): u16 {
    return ((buf[off] as u16) << 8) | (buf[off + 1] as u16);
}

fn readBeU32(buf: []const u8, off: usize): u32 {
    return ((buf[off] as u32) << 24)
        | ((buf[off + 1] as u32) << 16)
        | ((buf[off + 2] as u32) << 8)
        | (buf[off + 3] as u32);
}

fn readLeU32(buf: []const u8, off: usize): u32 {
    return (buf[off] as u32)
        | ((buf[off + 1] as u32) << 8)
        | ((buf[off + 2] as u32) << 16)
        | ((buf[off + 3] as u32) << 24);
}

fn writeBeU16(buf: []u8, off: usize, v: u16) {
    buf[off] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

fn writeBeU32(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn writeLeU32(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn copyBytes(dst: []u8, dst_off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[dst_off + i] = src[i];
    }
}

/// Rounds `len` up to the next 4-byte boundary per RFC 4960 §3.2 chunk padding rules.
pub fn sctpPad4(len: usize): usize {
    return (len + 3) & (~@as(usize, 3));
}

/// RFC 1982 32-bit serial number comparison: returns `true` when `a < b` modulo `2^32`.
pub fn tsnLt(a: u32, b: u32): bool {
    const diff: u32 = b -% a;
    return diff != 0 && diff < 0x80000000;
}

/// RFC 1982 32-bit serial number comparison: returns `true` when `a <= b` modulo `2^32`.
pub fn tsnLe(a: u32, b: u32): bool {
    return a == b || tsnLt(a, b);
}

/// RFC 1982 32-bit serial number comparison: returns `true` when `a > b` modulo `2^32`.
pub fn tsnGt(a: u32, b: u32): bool {
    return tsnLt(b, a);
}

/// RFC 1982 32-bit serial number comparison: returns `true` when `a >= b` modulo `2^32`.
pub fn tsnGe(a: u32, b: u32): bool {
    return a == b || tsnLt(b, a);
}

/// Signed 32-bit modular distance `a - b` in `[-2^31, 2^31 - 1]`.
pub fn tsnDiff(a: u32, b: u32): i32 {
    return @as(i32, @bitCast(a -% b));
}

/// Precomputed 256-entry Castagnoli CRC32C lookup table (polynomial `0x82F63B78`).
const SCTP_CRC32C_TABLE: [256]u32 = [
    0x00000000, 0xF26B8303, 0xE13B70F7, 0x1350F3F4, 0xC79A971F, 0x35F1141C, 0x26A1E7E8, 0xD4CA64EB,
    0x8AD958CF, 0x78B2DBCC, 0x6BE22838, 0x9989AB3B, 0x4D43CFD0, 0xBF284CD3, 0xAC78BF27, 0x5E133C24,
    0x105EC76F, 0xE235446C, 0xF165B798, 0x030E349B, 0xD7C45070, 0x25AFD373, 0x36FF2087, 0xC494A384,
    0x9A879FA0, 0x68EC1CA3, 0x7BBCEF57, 0x89D76C54, 0x5D1D08BF, 0xAF768BBC, 0xBC267848, 0x4E4DFB4B,
    0x20BD8EDE, 0xD2D60DDD, 0xC186FE29, 0x33ED7D2A, 0xE72719C1, 0x154C9AC2, 0x061C6936, 0xF477EA35,
    0xAA64D611, 0x580F5512, 0x4B5FA6E6, 0xB93425E5, 0x6DFE410E, 0x9F95C20D, 0x8CC531F9, 0x7EAEB2FA,
    0x30E349B1, 0xC288CAB2, 0xD1D83946, 0x23B3BA45, 0xF779DEAE, 0x05125DAD, 0x1642AE59, 0xE4292D5A,
    0xBA3A117E, 0x4851927D, 0x5B016189, 0xA96AE28A, 0x7DA08661, 0x8FCB0562, 0x9C9BF696, 0x6EF07595,
    0x417B1DBC, 0xB3109EBF, 0xA0406D4B, 0x522BEE48, 0x86E18AA3, 0x748A09A0, 0x67DAFA54, 0x95B17957,
    0xCBA24573, 0x39C9C670, 0x2A993584, 0xD8F2B687, 0x0C38D26C, 0xFE53516F, 0xED03A29B, 0x1F682198,
    0x5125DAD3, 0xA34E59D0, 0xB01EAA24, 0x42752927, 0x96BF4DCC, 0x64D4CECF, 0x77843D3B, 0x85EFBE38,
    0xDBFC821C, 0x2997011F, 0x3AC7F2EB, 0xC8AC71E8, 0x1C661503, 0xEE0D9600, 0xFD5D65F4, 0x0F36E6F7,
    0x61C69362, 0x93AD1061, 0x80FDE395, 0x72966096, 0xA65C047D, 0x5437877E, 0x4767748A, 0xB50CF789,
    0xEB1FCBAD, 0x197448AE, 0x0A24BB5A, 0xF84F3859, 0x2C855CB2, 0xDEEEDFB1, 0xCDBE2C45, 0x3FD5AF46,
    0x7198540D, 0x83F3D70E, 0x90A324FA, 0x62C8A7F9, 0xB602C312, 0x44694011, 0x5739B3E5, 0xA55230E6,
    0xFB410CC2, 0x092A8FC1, 0x1A7A7C35, 0xE811FF36, 0x3CDB9BDD, 0xCEB018DE, 0xDDE0EB2A, 0x2F8B6829,
    0x82F63B78, 0x709DB87B, 0x63CD4B8F, 0x91A6C88C, 0x456CAC67, 0xB7072F64, 0xA457DC90, 0x563C5F93,
    0x082F63B7, 0xFA44E0B4, 0xE9141340, 0x1B7F9043, 0xCFB5F4A8, 0x3DDE77AB, 0x2E8E845F, 0xDCE5075C,
    0x92A8FC17, 0x60C37F14, 0x73938CE0, 0x81F80FE3, 0x55326B08, 0xA759E80B, 0xB4091BFF, 0x466298FC,
    0x1871A4D8, 0xEA1A27DB, 0xF94AD42F, 0x0B21572C, 0xDFEB33C7, 0x2D80B0C4, 0x3ED04330, 0xCCBBC033,
    0xA24BB5A6, 0x502036A5, 0x4370C551, 0xB11B4652, 0x65D122B9, 0x97BAA1BA, 0x84EA524E, 0x7681D14D,
    0x2892ED69, 0xDAF96E6A, 0xC9A99D9E, 0x3BC21E9D, 0xEF087A76, 0x1D63F975, 0x0E330A81, 0xFC588982,
    0xB21572C9, 0x407EF1CA, 0x532E023E, 0xA145813D, 0x758FE5D6, 0x87E466D5, 0x94B49521, 0x66DF1622,
    0x38CC2A06, 0xCAA7A905, 0xD9F75AF1, 0x2B9CD9F2, 0xFF56BD19, 0x0D3D3E1A, 0x1E6DCDEE, 0xEC064EED,
    0xC38D26C4, 0x31E6A5C7, 0x22B65633, 0xD0DDD530, 0x0417B1DB, 0xF67C32D8, 0xE52CC12C, 0x1747422F,
    0x49547E0B, 0xBB3FFD08, 0xA86F0EFC, 0x5A048DFF, 0x8ECEE914, 0x7CA56A17, 0x6FF599E3, 0x9D9E1AE0,
    0xD3D3E1AB, 0x21B862A8, 0x32E8915C, 0xC083125F, 0x144976B4, 0xE622F5B7, 0xF5720643, 0x07198540,
    0x590AB964, 0xAB613A67, 0xB831C993, 0x4A5A4A90, 0x9E902E7B, 0x6CFBAD78, 0x7FAB5E8C, 0x8DC0DD8F,
    0xE330A81A, 0x115B2B19, 0x020BD8ED, 0xF0605BEE, 0x24AA3F05, 0xD6C1BC06, 0xC5914FF2, 0x37FACCF1,
    0x69E9F0D5, 0x9B8273D6, 0x88D28022, 0x7AB90321, 0xAE7367CA, 0x5C18E4C9, 0x4F48173D, 0xBD23943E,
    0xF36E6F75, 0x0105EC76, 0x12551F82, 0xE03E9C81, 0x34F4F86A, 0xC69F7B69, 0xD5CF889D, 0x27A40B9E,
    0x79B737BA, 0x8BDCB4B9, 0x988C474D, 0x6AE7C44E, 0xBE2DA0A5, 0x4C4623A6, 0x5F16D052, 0xAD7D5351,
];

/// Incrementally updates a raw (pre-inverted) Castagnoli CRC32C accumulator over `data`.
pub fn updateSctpCrc32c(init_crc: u32, data: []const u8): u32 {
    let crc: u32 = init_crc;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        const idx: usize = ((crc ^ (data[i] as u32)) & 0xFF) as usize;
        crc = SCTP_CRC32C_TABLE[idx] ^ (crc >> 8);
    }
    return crc;
}

/// Computes the RFC 4960 Appendix B Castagnoli CRC32C checksum over `data`.
/// Verified test vector: `computeSctpCrc32c("123456789") == 0xE3069283`.
pub fn computeSctpCrc32c(data: []const u8): u32 {
    return updateSctpCrc32c(0xFFFFFFFF, data) ^ 0xFFFFFFFF;
}

/// Computes the RFC 4960 §6.8 SCTP packet CRC32C treating bytes `[8..12]` as zero without mutating `pkt`.
pub fn computeSctpPacketCrc32c(pkt: []const u8): u32 ! SctpDcError {
    if (pkt.len < SCTP_COMMON_HEADER_LEN) {
        return SctpDcError.PacketTooShort;
    }
    const zero4: [4]u8 = [0, 0, 0, 0];
    let crc: u32 = updateSctpCrc32c(0xFFFFFFFF, pkt[0..8]);
    crc = updateSctpCrc32c(crc, zero4[0..4]);
    if (pkt.len > SCTP_COMMON_HEADER_LEN) {
        crc = updateSctpCrc32c(crc, pkt[SCTP_COMMON_HEADER_LEN..pkt.len]);
    }
    return crc ^ 0xFFFFFFFF;
}

/// Computes and writes the little-endian Castagnoli CRC32C checksum into `pkt[8..12]`.
pub fn stampSctpPacketCrc32c(pkt: []u8): u32 ! SctpDcError {
    if (pkt.len < SCTP_COMMON_HEADER_LEN) {
        return SctpDcError.PacketTooShort;
    }
    writeLeU32(pkt, 8, 0);
    const crc = computeSctpCrc32c(pkt);
    writeLeU32(pkt, 8, crc);
    return crc;
}

/// Verifies the little-endian Castagnoli CRC32C checksum stored at `pkt[8..12]`.
pub fn verifySctpPacketCrc32c(pkt: []const u8): bool ! SctpDcError {
    if (pkt.len < SCTP_COMMON_HEADER_LEN) {
        return SctpDcError.PacketTooShort;
    }
    const stored: u32 = readLeU32(pkt, 8);
    const computed = computeSctpPacketCrc32c(pkt)?;
    return stored == computed;
}

// ============================================================================
// 3. RFC 4960 Common Header, Zero-Copy Chunk Iterator & Packet Builder
// ============================================================================

/// RFC 4960 §3.1 12-byte SCTP common packet header.
pub struct SctpCommonHeader {
    src_port: u16 = 0;
    dst_port: u16 = 0;
    verification_tag: u32 = 0;
    checksum: u32 = 0;

    /// Decodes the 12-byte SCTP common header from `buf` (`checksum` decoded in little-endian).
    static fn decode(buf: []const u8): SctpCommonHeader ! SctpDcError {
        if (buf.len < SCTP_COMMON_HEADER_LEN) {
            return SctpDcError.PacketTooShort;
        }
        return SctpCommonHeader{
            src_port: readBeU16(buf, 0),
            dst_port: readBeU16(buf, 2),
            verification_tag: readBeU32(buf, 4),
            checksum: readLeU32(buf, 8),
        };
    }

    /// Encodes the 12-byte SCTP common header into `out` (`checksum` encoded in little-endian).
    fn encode(out: []u8): usize ! SctpDcError {
        if (out.len < SCTP_COMMON_HEADER_LEN) {
            return SctpDcError.BufferTooSmall;
        }
        writeBeU16(out, 0, src_port);
        writeBeU16(out, 2, dst_port);
        writeBeU32(out, 4, verification_tag);
        writeLeU32(out, 8, checksum);
        return SCTP_COMMON_HEADER_LEN;
    }
}

/// Zero-copy view of a single SCTP chunk within a received packet buffer.
pub struct SctpChunkView {
    chunk_type: u8 = 0;
    flags: u8 = 0;
    length: u16 = 0;
    value: []const u8 = "";
    raw: []const u8 = "";
}

/// Zero-copy RFC 4960 chunk iterator handling 4-byte alignment padding and strict length guards.
pub struct SctpWireChunkIter {
    body: []const u8 = "";
    offset: usize = 0;

    /// Creates a chunk iterator over a full SCTP packet (skips the 12-byte common header).
    static fn fromPacket(pkt: []const u8): SctpWireChunkIter ! SctpDcError {
        if (pkt.len < SCTP_COMMON_HEADER_LEN) {
            return SctpDcError.PacketTooShort;
        }
        return SctpWireChunkIter{
            body: pkt[SCTP_COMMON_HEADER_LEN..pkt.len],
            offset: 0,
        };
    }

    /// Creates a chunk iterator directly over the chunk payload region (after the common header).
    static fn fromBody(chunk_body: []const u8): SctpWireChunkIter {
        return SctpWireChunkIter{
            body: chunk_body,
            offset: 0,
        };
    }

    /// Returns `true` when all chunks have been consumed.
    fn isDone(): bool {
        return offset >= body.len;
    }

    /// Parses and returns the next chunk, advancing past 4-byte trailing padding.
    fn next(*_): SctpChunkView? ! SctpDcError {
        if (offset >= body.len) {
            return null;
        }
        if (body.len - offset < SCTP_CHUNK_HDR_LEN) {
            return SctpDcError.InvalidChunkLength;
        }
        const ctype: u8 = body[offset];
        const cflags: u8 = body[offset + 1];
        const clen_u16: u16 = readBeU16(body, offset + 2);
        const clen: usize = clen_u16 as usize;
        if (clen < SCTP_CHUNK_HDR_LEN || offset + clen > body.len) {
            return SctpDcError.InvalidChunkLength;
        }
        const view = SctpChunkView{
            chunk_type: ctype,
            flags: cflags,
            length: clen_u16,
            value: body[(offset + SCTP_CHUNK_HDR_LEN)..(offset + clen)],
            raw: body[offset..(offset + clen)],
        };
        const padded: usize = sctpPad4(clen);
        if (offset + padded <= body.len) {
            offset += padded;
        } else {
            // RFC 4960 §3.2 permits omitting trailing padding after the final chunk in a packet.
            offset += clen;
        }
        return view;
    }
}

/// Zero-allocation SCTP packet builder writing common header, 4-byte padded chunks, and CRC32C.
pub struct SctpPacketBuilder {
    buf: []u8 = [];
    pos: usize = 0;

    /// Initializes a builder on caller-owned `out` with an SCTP common header.
    static fn init(out: []u8, src_port: u16, dst_port: u16, verification_tag: u32): SctpPacketBuilder ! SctpDcError {
        const hdr = SctpCommonHeader{
            src_port: src_port,
            dst_port: dst_port,
            verification_tag: verification_tag,
            checksum: 0,
        };
        * = hdr.encode(out)?;
        return SctpPacketBuilder{
            buf: out,
            pos: SCTP_COMMON_HEADER_LEN,
        };
    }

    /// Appends a pre-encoded chunk slice and pads to a 4-byte boundary with zero bytes.
    fn appendRawChunk(*_, chunk: []const u8): usize ! SctpDcError {
        if (chunk.len < SCTP_CHUNK_HDR_LEN) {
            return SctpDcError.InvalidChunkLength;
        }
        const padded: usize = sctpPad4(chunk.len);
        if (pos + padded > buf.len) {
            return SctpDcError.BufferTooSmall;
        }
        copyBytes(buf, pos, chunk);
        let p: usize = chunk.len;
        while (p < padded) : (p += 1) {
            buf[pos + p] = 0;
        }
        pos += padded;
        return pos;
    }

    /// Returns the writable tail slice starting at the current write cursor.
    fn tailSlice(*_): []u8 {
        return buf[pos..buf.len];
    }

    /// Commits `chunk_len` bytes already written directly into `tailSlice()`, adding zero padding.
    fn commitChunk(*_, chunk_len: usize): usize ! SctpDcError {
        if (chunk_len < SCTP_CHUNK_HDR_LEN) {
            return SctpDcError.InvalidChunkLength;
        }
        const padded: usize = sctpPad4(chunk_len);
        if (pos + padded > buf.len) {
            return SctpDcError.BufferTooSmall;
        }
        let p: usize = chunk_len;
        while (p < padded) : (p += 1) {
            buf[pos + p] = 0;
        }
        pos += padded;
        return pos;
    }

    /// Stamps the Castagnoli CRC32C checksum into bytes `[8..12]` and returns the total packet length.
    fn finish(*_): usize ! SctpDcError {
        * = stampSctpPacketCrc32c(buf[0..pos])?;
        return pos;
    }
}

// ============================================================================
// 4. TLV Parameters & Core RFC 4960 / RFC 3758 Chunk Codecs
// ============================================================================

/// Zero-copy view of a 4-byte aligned SCTP TLV parameter (`type` + `length` + `value`).
pub struct SctpTlvParam {
    param_type: u16 = 0;
    value: []const u8 = "";
}

/// Zero-copy iterator over RFC 4960 §3.2.1 TLV parameters inside INIT / INIT_ACK / HEARTBEAT chunks.
pub struct SctpTlvIter {
    data: []const u8 = "";
    offset: usize = 0;

    static fn init(params_raw: []const u8): SctpTlvIter {
        return SctpTlvIter{
            data: params_raw,
            offset: 0,
        };
    }

    fn next(*_): SctpTlvParam? ! SctpDcError {
        if (offset >= data.len) {
            return null;
        }
        if (data.len - offset < 4) {
            return SctpDcError.InvalidParamLength;
        }
        const ptype: u16 = readBeU16(data, offset);
        const plen: usize = readBeU16(data, offset + 2) as usize;
        if (plen < 4 || offset + plen > data.len) {
            return SctpDcError.InvalidParamLength;
        }
        const param = SctpTlvParam{
            param_type: ptype,
            value: data[(offset + 4)..(offset + plen)],
        };
        const padded: usize = sctpPad4(plen);
        if (offset + padded <= data.len) {
            offset += padded;
        } else {
            offset += plen;
        }
        return param;
    }
}

/// Encodes a single TLV parameter with 4-byte zero padding into `out`, returning padded length.
pub fn encodeSctpTlvParam(param: SctpTlvParam, out: []u8): usize ! SctpDcError {
    const raw_len: usize = param.value.len + 4;
    if (raw_len > 65535) {
        return SctpDcError.InvalidParamLength;
    }
    const padded: usize = sctpPad4(raw_len);
    if (out.len < padded) {
        return SctpDcError.BufferTooSmall;
    }
    writeBeU16(out, 0, param.param_type);
    writeBeU16(out, 2, raw_len as u16);
    copyBytes(out, 4, param.value);
    let p: usize = raw_len;
    while (p < padded) : (p += 1) {
        out[p] = 0;
    }
    return padded;
}

/// Decoded RFC 4960 §3.3.1 DATA chunk payload and header fields.
pub struct SctpDataPayload {
    flags: u8 = 0;
    tsn: u32 = 0;
    stream_id: u16 = 0;
    stream_seq: u16 = 0;
    ppid: u32 = 0;
    user_data: []const u8 = "";

    fn isEnd(): bool {
        return (flags & SCTP_DATA_FLAG_E) != 0;
    }

    fn isBegin(): bool {
        return (flags & SCTP_DATA_FLAG_B) != 0;
    }

    fn isUnordered(): bool {
        return (flags & SCTP_DATA_FLAG_U) != 0;
    }

    fn isImmediate(): bool {
        return (flags & SCTP_DATA_FLAG_I) != 0;
    }

    fn isSingleFragment(): bool {
        return isBegin() && isEnd();
    }
}

/// Encodes an RFC 4960 §3.3.1 DATA chunk into `out`, returning the unpadded chunk length.
pub fn encodeSctpDataChunk(data: SctpDataPayload, out: []u8): usize ! SctpDcError {
    const clen: usize = SCTP_DATA_FIXED_LEN + data.user_data.len;
    if (clen > 65535) {
        return SctpDcError.InvalidChunkLength;
    }
    if (out.len < clen) {
        return SctpDcError.BufferTooSmall;
    }
    out[0] = SCTP_CHUNK_DATA;
    out[1] = data.flags & 0x0F;
    writeBeU16(out, 2, clen as u16);
    writeBeU32(out, 4, data.tsn);
    writeBeU16(out, 8, data.stream_id);
    writeBeU16(out, 10, data.stream_seq);
    writeBeU32(out, 12, data.ppid);
    copyBytes(out, 16, data.user_data);
    return clen;
}

/// Decodes an RFC 4960 §3.3.1 DATA chunk from `chunk_raw` (including 4-byte chunk header).
pub fn decodeSctpDataChunk(chunk_raw: []const u8): SctpDataPayload ! SctpDcError {
    if (chunk_raw.len < SCTP_DATA_FIXED_LEN) {
        return SctpDcError.InvalidChunkLength;
    }
    if (chunk_raw[0] != SCTP_CHUNK_DATA) {
        return SctpDcError.InvalidChunkType;
    }
    const clen: usize = readBeU16(chunk_raw, 2) as usize;
    if (clen < SCTP_DATA_FIXED_LEN || clen > chunk_raw.len) {
        return SctpDcError.InvalidChunkLength;
    }
    return SctpDataPayload{
        flags: chunk_raw[1],
        tsn: readBeU32(chunk_raw, 4),
        stream_id: readBeU16(chunk_raw, 8),
        stream_seq: readBeU16(chunk_raw, 10),
        ppid: readBeU32(chunk_raw, 12),
        user_data: chunk_raw[SCTP_DATA_FIXED_LEN..clen],
    };
}

/// Decoded RFC 4960 §3.3.2 / §3.3.3 INIT or INIT_ACK chunk body.
pub struct SctpInitBody {
    is_ack: bool = false;
    initiate_tag: u32 = 0;
    a_rwnd: u32 = 0;
    num_outbound_streams: u16 = 0;
    num_inbound_streams: u16 = 0;
    initial_tsn: u32 = 0;
    params_raw: []const u8 = "";

    /// Searches optional TLV parameters for a `State Cookie` (`0x0007`) value.
    fn findStateCookie(): []const u8 ! SctpDcError {
        let it = SctpTlvIter.init(params_raw);
        while (true) {
            const opt = it.next()?;
            if (p := opt) {
                if (p.param_type == SCTP_PARAM_STATE_COOKIE) {
                    return p.value;
                }
            } else {
                break;
            }
        }
        return SctpDcError.InvalidCookie;
    }

    /// Returns `true` when `Forward-TSN-Supported` (`0xC000`) or `Supported Extensions` contains `FORWARD_TSN`.
    fn supportsForwardTsn(): bool ! SctpDcError {
        let it = SctpTlvIter.init(params_raw);
        while (true) {
            const opt = it.next()?;
            if (p := opt) {
                if (p.param_type == SCTP_PARAM_FORWARD_TSN_SUPPORTED) {
                    return true;
                }
                if (p.param_type == SCTP_PARAM_SUPPORTED_EXTENSIONS) {
                    let i: usize = 0;
                    while (i < p.value.len) : (i += 1) {
                        if (p.value[i] == SCTP_CHUNK_FORWARD_TSN) {
                            return true;
                        }
                    }
                }
            } else {
                break;
            }
        }
        return false;
    }
}

/// Encodes an RFC 4960 INIT (`1`) or INIT_ACK (`2`) chunk with optional TLV parameters into `out`.
pub fn encodeSctpInitChunk(body: SctpInitBody, params: []const SctpTlvParam, out: []u8): usize ! SctpDcError {
    if (body.initiate_tag == 0) {
        return SctpDcError.InvalidVerificationTag;
    }
    if (body.num_outbound_streams == 0 || body.num_inbound_streams == 0) {
        return SctpDcError.StreamCountZero;
    }
    if (out.len < SCTP_INIT_FIXED_LEN) {
        return SctpDcError.BufferTooSmall;
    }
    let pos: usize = SCTP_INIT_FIXED_LEN;
    let i: usize = 0;
    while (i < params.len) : (i += 1) {
        const written = encodeSctpTlvParam(params[i], out[pos..out.len])?;
        pos += written;
    }
    if (pos > 65535) {
        return SctpDcError.InvalidChunkLength;
    }
    out[0] = if (body.is_ack) SCTP_CHUNK_INIT_ACK else SCTP_CHUNK_INIT;
    out[1] = 0;
    writeBeU16(out, 2, pos as u16);
    writeBeU32(out, 4, body.initiate_tag);
    writeBeU32(out, 8, body.a_rwnd);
    writeBeU16(out, 12, body.num_outbound_streams);
    writeBeU16(out, 14, body.num_inbound_streams);
    writeBeU32(out, 16, body.initial_tsn);
    return pos;
}

/// Decodes an RFC 4960 INIT (`1`) or INIT_ACK (`2`) chunk from `chunk_raw`.
pub fn decodeSctpInitChunk(chunk_raw: []const u8): SctpInitBody ! SctpDcError {
    if (chunk_raw.len < SCTP_INIT_FIXED_LEN) {
        return SctpDcError.InvalidChunkLength;
    }
    const ctype = chunk_raw[0];
    if (ctype != SCTP_CHUNK_INIT && ctype != SCTP_CHUNK_INIT_ACK) {
        return SctpDcError.InvalidChunkType;
    }
    const clen: usize = readBeU16(chunk_raw, 2) as usize;
    if (clen < SCTP_INIT_FIXED_LEN || clen > chunk_raw.len) {
        return SctpDcError.InvalidChunkLength;
    }
    const init_tag = readBeU32(chunk_raw, 4);
    const out_s = readBeU16(chunk_raw, 12);
    const in_s = readBeU16(chunk_raw, 14);
    if (init_tag == 0) {
        return SctpDcError.InvalidVerificationTag;
    }
    if (out_s == 0 || in_s == 0) {
        return SctpDcError.StreamCountZero;
    }
    return SctpInitBody{
        is_ack: ctype == SCTP_CHUNK_INIT_ACK,
        initiate_tag: init_tag,
        a_rwnd: readBeU32(chunk_raw, 8),
        num_outbound_streams: out_s,
        num_inbound_streams: in_s,
        initial_tsn: readBeU32(chunk_raw, 16),
        params_raw: chunk_raw[SCTP_INIT_FIXED_LEN..clen],
    };
}

/// RFC 4960 §3.3.4 SACK Gap Ack Block (`start` and `end` are 1-based offsets from `cumulative_tsn_ack`).
pub struct GapAckBlock {
    start: u16 = 0;
    end: u16 = 0;
}

/// Decoded RFC 4960 §3.3.4 SACK chunk header fields.
pub struct SctpSackBody {
    cumulative_tsn_ack: u32 = 0;
    a_rwnd: u32 = 0;
    num_gap_blocks: u16 = 0;
    num_dup_tsns: u16 = 0;
}

/// Encodes an RFC 4960 §3.3.4 SACK chunk using caller-provided `gaps` and `dups` slices.
pub fn encodeSctpSackChunk(
    cumulative_tsn_ack: u32,
    a_rwnd: u32,
    gaps: []const GapAckBlock,
    dups: []const u32,
    out: []u8,
): usize ! SctpDcError {
    if (gaps.len > 65535 || dups.len > 65535) {
        return SctpDcError.InvalidChunkLength;
    }
    const clen: usize = SCTP_SACK_FIXED_LEN + gaps.len * 4 + dups.len * 4;
    if (clen > 65535) {
        return SctpDcError.InvalidChunkLength;
    }
    if (out.len < clen) {
        return SctpDcError.BufferTooSmall;
    }
    out[0] = SCTP_CHUNK_SACK;
    out[1] = 0;
    writeBeU16(out, 2, clen as u16);
    writeBeU32(out, 4, cumulative_tsn_ack);
    writeBeU32(out, 8, a_rwnd);
    writeBeU16(out, 12, gaps.len as u16);
    writeBeU16(out, 14, dups.len as u16);

    let pos: usize = SCTP_SACK_FIXED_LEN;
    let i: usize = 0;
    while (i < gaps.len) : (i += 1) {
        writeBeU16(out, pos, gaps[i].start);
        writeBeU16(out, pos + 2, gaps[i].end);
        pos += 4;
    }
    let j: usize = 0;
    while (j < dups.len) : (j += 1) {
        writeBeU32(out, pos, dups[j]);
        pos += 4;
    }
    return clen;
}

/// Decodes an RFC 4960 §3.3.4 SACK chunk into caller-provided `out_gaps` and `out_dups` slices.
pub fn decodeSctpSackChunk(
    chunk_raw: []const u8,
    out_gaps: []GapAckBlock,
    out_dups: []u32,
): SctpSackBody ! SctpDcError {
    if (chunk_raw.len < SCTP_SACK_FIXED_LEN) {
        return SctpDcError.InvalidChunkLength;
    }
    if (chunk_raw[0] != SCTP_CHUNK_SACK) {
        return SctpDcError.InvalidChunkType;
    }
    const clen: usize = readBeU16(chunk_raw, 2) as usize;
    if (clen < SCTP_SACK_FIXED_LEN || clen > chunk_raw.len) {
        return SctpDcError.InvalidChunkLength;
    }
    const n_gaps: usize = readBeU16(chunk_raw, 12) as usize;
    const n_dups: usize = readBeU16(chunk_raw, 14) as usize;
    const expected: usize = SCTP_SACK_FIXED_LEN + n_gaps * 4 + n_dups * 4;
    if (clen < expected) {
        return SctpDcError.InvalidChunkLength;
    }
    if (out_gaps.len < n_gaps) {
        return SctpDcError.GapBlockOverflow;
    }
    if (out_dups.len < n_dups) {
        return SctpDcError.DuplicateTsnOverflow;
    }
    let pos: usize = SCTP_SACK_FIXED_LEN;
    let i: usize = 0;
    while (i < n_gaps) : (i += 1) {
        const s = readBeU16(chunk_raw, pos);
        const e = readBeU16(chunk_raw, pos + 2);
        if (s == 0 || e < s) {
            return SctpDcError.InvalidChunkLength;
        }
        out_gaps[i] = GapAckBlock{ start: s, end: e };
        pos += 4;
    }
    let j: usize = 0;
    while (j < n_dups) : (j += 1) {
        out_dups[j] = readBeU32(chunk_raw, pos);
        pos += 4;
    }
    return SctpSackBody{
        cumulative_tsn_ack: readBeU32(chunk_raw, 4),
        a_rwnd: readBeU32(chunk_raw, 8),
        num_gap_blocks: n_gaps as u16,
        num_dup_tsns: n_dups as u16,
    };
}

/// Encodes an RFC 4960 §3.3.5 HEARTBEAT (`4`) or §3.3.6 HEARTBEAT_ACK (`5`) chunk wrapping TLV `0x0001`.
pub fn encodeSctpHeartbeatChunk(is_ack: bool, info: []const u8, out: []u8): usize ! SctpDcError {
    if (out.len < SCTP_CHUNK_HDR_LEN) {
        return SctpDcError.BufferTooSmall;
    }
    const tlv = SctpTlvParam{
        param_type: SCTP_PARAM_HEARTBEAT_INFO,
        value: info,
    };
    const tlv_len = encodeSctpTlvParam(tlv, out[SCTP_CHUNK_HDR_LEN..out.len])?;
    const clen: usize = SCTP_CHUNK_HDR_LEN + tlv_len;
    if (clen > 65535) {
        return SctpDcError.InvalidChunkLength;
    }
    out[0] = if (is_ack) SCTP_CHUNK_HEARTBEAT_ACK else SCTP_CHUNK_HEARTBEAT;
    out[1] = 0;
    writeBeU16(out, 2, clen as u16);
    return clen;
}

/// Decodes a HEARTBEAT or HEARTBEAT_ACK chunk and returns its zero-copy heartbeat info payload.
pub fn decodeSctpHeartbeatChunk(chunk_raw: []const u8): []const u8 ! SctpDcError {
    if (chunk_raw.len < SCTP_CHUNK_HDR_LEN) {
        return SctpDcError.InvalidChunkLength;
    }
    const ctype = chunk_raw[0];
    if (ctype != SCTP_CHUNK_HEARTBEAT && ctype != SCTP_CHUNK_HEARTBEAT_ACK) {
        return SctpDcError.InvalidChunkType;
    }
    const clen: usize = readBeU16(chunk_raw, 2) as usize;
    if (clen < SCTP_CHUNK_HDR_LEN + 4 || clen > chunk_raw.len) {
        return SctpDcError.InvalidChunkLength;
    }
    let it = SctpTlvIter.init(chunk_raw[SCTP_CHUNK_HDR_LEN..clen]);
    const opt = it.next()?;
    if (p := opt) {
        if (p.param_type != SCTP_PARAM_HEARTBEAT_INFO) {
            return SctpDcError.InvalidParamLength;
        }
        return p.value;
    }
    return SctpDcError.InvalidParamLength;
}

/// Encodes an RFC 4960 §3.3.7 ABORT (`6`) chunk with optional `t_bit` and raw error-causes bytes.
pub fn encodeSctpAbortChunk(t_bit: bool, error_causes: []const u8, out: []u8): usize ! SctpDcError {
    const clen: usize = SCTP_CHUNK_HDR_LEN + error_causes.len;
    if (clen > 65535) {
        return SctpDcError.InvalidChunkLength;
    }
    if (out.len < clen) {
        return SctpDcError.BufferTooSmall;
    }
    out[0] = SCTP_CHUNK_ABORT;
    out[1] = if (t_bit) 0x01 else 0x00;
    writeBeU16(out, 2, clen as u16);
    copyBytes(out, SCTP_CHUNK_HDR_LEN, error_causes);
    return clen;
}

/// Encodes an RFC 4960 §3.3.8 SHUTDOWN (`7`) chunk carrying `cumulative_tsn_ack`.
pub fn encodeSctpShutdownChunk(cumulative_tsn_ack: u32, out: []u8): usize ! SctpDcError {
    if (out.len < 8) {
        return SctpDcError.BufferTooSmall;
    }
    out[0] = SCTP_CHUNK_SHUTDOWN;
    out[1] = 0;
    writeBeU16(out, 2, 8);
    writeBeU32(out, 4, cumulative_tsn_ack);
    return 8;
}

/// Decodes an RFC 4960 §3.3.8 SHUTDOWN (`7`) chunk and returns `cumulative_tsn_ack`.
pub fn decodeSctpShutdownChunk(chunk_raw: []const u8): u32 ! SctpDcError {
    if (chunk_raw.len < 8) {
        return SctpDcError.InvalidChunkLength;
    }
    if (chunk_raw[0] != SCTP_CHUNK_SHUTDOWN) {
        return SctpDcError.InvalidChunkType;
    }
    const clen: usize = readBeU16(chunk_raw, 2) as usize;
    if (clen != 8) {
        return SctpDcError.InvalidChunkLength;
    }
    return readBeU32(chunk_raw, 4);
}

/// Encodes an RFC 4960 §3.3.9 SHUTDOWN_ACK (`8`) 4-byte chunk.
pub fn encodeSctpShutdownAckChunk(out: []u8): usize ! SctpDcError {
    if (out.len < 4) {
        return SctpDcError.BufferTooSmall;
    }
    out[0] = SCTP_CHUNK_SHUTDOWN_ACK;
    out[1] = 0;
    writeBeU16(out, 2, 4);
    return 4;
}

/// Encodes an RFC 4960 §3.3.13 SHUTDOWN_COMPLETE (`14`) 4-byte chunk.
pub fn encodeSctpShutdownCompleteChunk(t_bit: bool, out: []u8): usize ! SctpDcError {
    if (out.len < 4) {
        return SctpDcError.BufferTooSmall;
    }
    out[0] = SCTP_CHUNK_SHUTDOWN_COMPLETE;
    out[1] = if (t_bit) 0x01 else 0x00;
    writeBeU16(out, 2, 4);
    return 4;
}

/// Encodes an RFC 4960 §3.3.11 COOKIE_ECHO (`10`) chunk.
pub fn encodeSctpCookieEchoChunk(cookie: []const u8, out: []u8): usize ! SctpDcError {
    const clen: usize = SCTP_CHUNK_HDR_LEN + cookie.len;
    if (clen > 65535 || cookie.len == 0) {
        return SctpDcError.InvalidChunkLength;
    }
    if (out.len < clen) {
        return SctpDcError.BufferTooSmall;
    }
    out[0] = SCTP_CHUNK_COOKIE_ECHO;
    out[1] = 0;
    writeBeU16(out, 2, clen as u16);
    copyBytes(out, SCTP_CHUNK_HDR_LEN, cookie);
    return clen;
}

/// Decodes an RFC 4960 §3.3.11 COOKIE_ECHO (`10`) chunk and returns its zero-copy cookie slice.
pub fn decodeSctpCookieEchoChunk(chunk_raw: []const u8): []const u8 ! SctpDcError {
    if (chunk_raw.len < SCTP_CHUNK_HDR_LEN) {
        return SctpDcError.InvalidChunkLength;
    }
    if (chunk_raw[0] != SCTP_CHUNK_COOKIE_ECHO) {
        return SctpDcError.InvalidChunkType;
    }
    const clen: usize = readBeU16(chunk_raw, 2) as usize;
    if (clen <= SCTP_CHUNK_HDR_LEN || clen > chunk_raw.len) {
        return SctpDcError.InvalidChunkLength;
    }
    return chunk_raw[SCTP_CHUNK_HDR_LEN..clen];
}

/// Encodes an RFC 4960 §3.3.12 COOKIE_ACK (`11`) 4-byte chunk.
pub fn encodeSctpCookieAckChunk(out: []u8): usize ! SctpDcError {
    if (out.len < 4) {
        return SctpDcError.BufferTooSmall;
    }
    out[0] = SCTP_CHUNK_COOKIE_ACK;
    out[1] = 0;
    writeBeU16(out, 2, 4);
    return 4;
}

/// RFC 3758 §3.2 per-stream sequence entry inside a `FORWARD_TSN` chunk.
pub struct ForwardTsnStreamEntry {
    stream_id: u16 = 0;
    stream_seq: u16 = 0;
}

/// Decoded RFC 3758 §3.2 `FORWARD_TSN` (`192`) chunk summary.
pub struct SctpForwardTsnBody {
    new_cumulative_tsn: u32 = 0;
    num_streams: usize = 0;
}

/// Encodes an RFC 3758 §3.2 PR-SCTP `FORWARD_TSN` (`192`) chunk.
pub fn encodeSctpForwardTsnChunk(
    new_cumulative_tsn: u32,
    streams: []const ForwardTsnStreamEntry,
    out: []u8,
): usize ! SctpDcError {
    const clen: usize = SCTP_FORWARD_TSN_MIN_LEN + streams.len * 4;
    if (clen > 65535) {
        return SctpDcError.InvalidChunkLength;
    }
    if (out.len < clen) {
        return SctpDcError.BufferTooSmall;
    }
    out[0] = SCTP_CHUNK_FORWARD_TSN;
    out[1] = 0;
    writeBeU16(out, 2, clen as u16);
    writeBeU32(out, 4, new_cumulative_tsn);
    let pos: usize = SCTP_FORWARD_TSN_MIN_LEN;
    let i: usize = 0;
    while (i < streams.len) : (i += 1) {
        writeBeU16(out, pos, streams[i].stream_id);
        writeBeU16(out, pos + 2, streams[i].stream_seq);
        pos += 4;
    }
    return clen;
}

/// Decodes an RFC 3758 §3.2 PR-SCTP `FORWARD_TSN` (`192`) chunk into caller-provided `out_streams`.
pub fn decodeSctpForwardTsnChunk(
    chunk_raw: []const u8,
    out_streams: []ForwardTsnStreamEntry,
): SctpForwardTsnBody ! SctpDcError {
    if (chunk_raw.len < SCTP_FORWARD_TSN_MIN_LEN) {
        return SctpDcError.InvalidChunkLength;
    }
    if (chunk_raw[0] != SCTP_CHUNK_FORWARD_TSN) {
        return SctpDcError.InvalidChunkType;
    }
    const clen: usize = readBeU16(chunk_raw, 2) as usize;
    if (clen < SCTP_FORWARD_TSN_MIN_LEN || clen > chunk_raw.len) {
        return SctpDcError.InvalidChunkLength;
    }
    const rem: usize = clen - SCTP_FORWARD_TSN_MIN_LEN;
    if ((rem & 3) != 0) {
        return SctpDcError.InvalidChunkLength;
    }
    const n_streams: usize = rem / 4;
    if (out_streams.len < n_streams) {
        return SctpDcError.BufferTooSmall;
    }
    let pos: usize = SCTP_FORWARD_TSN_MIN_LEN;
    let i: usize = 0;
    while (i < n_streams) : (i += 1) {
        out_streams[i] = ForwardTsnStreamEntry{
            stream_id: readBeU16(chunk_raw, pos),
            stream_seq: readBeU16(chunk_raw, pos + 2),
        };
        pos += 4;
    }
    return SctpForwardTsnBody{
        new_cumulative_tsn: readBeU32(chunk_raw, 4),
        num_streams: n_streams,
    };
}

// ============================================================================
// 5. Unbounded External-Slice TSN Receive Tracker & SACK Generator
// ============================================================================

/// Outcome classification when recording an incoming TSN into `SctpRecvTracker`.
pub enum SctpTsnAcceptResult {
    in_order,
    out_of_order,
    duplicate,
}

/// Unbounded external-slice RFC 4960 TSN receive window, Gap Ack Block builder,
/// duplicate TSN recorder, and RFC 3758 PR-SCTP `FORWARD_TSN` consumer.
///
/// Memory model: zero dynamic heap allocation. Caller supplies `bitmap: []u64`
/// (supporting `bitmap.len * 64` out-of-order TSNs beyond `cumulative_tsn_ack`)
/// and `dup_buf: []u32` for duplicate TSNs reported in the next SACK.
pub struct SctpRecvTracker {
    cumulative_tsn_ack: u32 = 0;
    a_rwnd: u32 = 131072;
    bitmap: []u64 = [];
    dup_buf: []u32 = [];
    dup_count: usize = 0;
    total_received: u64 = 0;
    total_duplicates: u64 = 0;
    total_forwarded: u64 = 0;

    /// Initializes the tracker expecting `initial_peer_tsn` as the first peer DATA TSN.
    static fn init(
        initial_peer_tsn: u32,
        a_rwnd_val: u32,
        bitmap_slice: []u64,
        dup_slice: []u32,
    ): SctpRecvTracker ! SctpDcError {
        if (bitmap_slice.len == 0) {
            return SctpDcError.BufferTooSmall;
        }
        let i: usize = 0;
        while (i < bitmap_slice.len) : (i += 1) {
            bitmap_slice[i] = 0;
        }
        return SctpRecvTracker{
            cumulative_tsn_ack: initial_peer_tsn -% 1,
            a_rwnd: a_rwnd_val,
            bitmap: bitmap_slice,
            dup_buf: dup_slice,
            dup_count: 0,
            total_received: 0,
            total_duplicates: 0,
            total_forwarded: 0,
        };
    }

    /// Maximum out-of-order TSN window span supported by the external bitmap (`bitmap.len * 64`).
    fn maxWindowBits(): usize {
        return bitmap.len * 64;
    }

    fn getBit(bit_idx: usize): bool {
        const w: usize = bit_idx >> 6;
        const b: u6 = @as(u6, @intCast(bit_idx & 63));
        return ((bitmap[w] >> b) & 1) != 0;
    }

    fn setBit(*_, bit_idx: usize) {
        const w: usize = bit_idx >> 6;
        const b: u6 = @as(u6, @intCast(bit_idx & 63));
        bitmap[w] |= (@as(u64, 1) << b);
    }

    /// Shifts the entire multi-word bitmap right by `shift_bits` bit positions.
    fn shiftBitmapRight(*_, shift_bits: usize) {
        const total_bits: usize = bitmap.len * 64;
        if (shift_bits == 0) {
            return;
        }
        if (shift_bits >= total_bits) {
            let z: usize = 0;
            while (z < bitmap.len) : (z += 1) {
                bitmap[z] = 0;
            }
            return;
        }
        const word_shift: usize = shift_bits >> 6;
        const bit_shift: usize = shift_bits & 63;
        const n: usize = bitmap.len;

        if (bit_shift == 0) {
            let i: usize = 0;
            while (i + word_shift < n) : (i += 1) {
                bitmap[i] = bitmap[i + word_shift];
            }
            while (i < n) : (i += 1) {
                bitmap[i] = 0;
            }
            return;
        }

        const rshift: u6 = @as(u6, @intCast(bit_shift));
        const lshift: u6 = @as(u6, @intCast(64 - bit_shift));
        let i: usize = 0;
        while (i + word_shift < n) : (i += 1) {
            const lo: u64 = bitmap[i + word_shift] >> rshift;
            const hi: u64 = if (i + word_shift + 1 < n) (bitmap[i + word_shift + 1] << lshift) else 0;
            bitmap[i] = lo | hi;
        }
        while (i < n) : (i += 1) {
            bitmap[i] = 0;
        }
    }

    /// Counts contiguous leading 1-bits starting from bit 0, advances `cumulative_tsn_ack`, and shifts bitmap.
    fn advanceContiguous(*_): u32 {
        let contiguous: usize = 0;
        let w: usize = 0;
        while (w < bitmap.len) : (w += 1) {
            const word = bitmap[w];
            if (word == 0xFFFFFFFFFFFFFFFF) {
                contiguous += 64;
            } else {
                let b: usize = 0;
                while (b < 64) : (b += 1) {
                    const mask: u64 = @as(u64, 1) << (@as(u6, @intCast(b)));
                    if ((word & mask) != 0) {
                        contiguous += 1;
                    } else {
                        break;
                    }
                }
                break;
            }
        }
        if (contiguous > 0) {
            shiftBitmapRight(contiguous);
            cumulative_tsn_ack = cumulative_tsn_ack +% (contiguous as u32);
        }
        return contiguous as u32;
    }

    fn recordDuplicate(*_, tsn: u32) {
        total_duplicates += 1;
        if (dup_count < dup_buf.len) {
            dup_buf[dup_count] = tsn;
            dup_count += 1;
        }
    }

    /// Records a received TSN using RFC 1982 wrap-around comparison.
    /// Automatically advances `cumulative_tsn_ack` across contiguous TSNs and logs duplicates.
    fn recordTsn(*_, tsn: u32): SctpTsnAcceptResult ! SctpDcError {
        if (tsnLe(tsn, cumulative_tsn_ack)) {
            recordDuplicate(tsn);
            return SctpTsnAcceptResult.duplicate;
        }
        const delta: usize = (tsn -% cumulative_tsn_ack) as usize;
        if (delta > maxWindowBits()) {
            return SctpDcError.WindowOverflow;
        }
        const bit_idx: usize = delta - 1;
        if (getBit(bit_idx)) {
            recordDuplicate(tsn);
            return SctpTsnAcceptResult.duplicate;
        }
        setBit(bit_idx);
        total_received += 1;
        if (delta == 1) {
            * = advanceContiguous();
            return SctpTsnAcceptResult.in_order;
        }
        return SctpTsnAcceptResult.out_of_order;
    }

    /// Applies an RFC 3758 §3.6 `FORWARD_TSN` chunk, skipping expired TSNs up to `new_cumulative_tsn`
    /// and continuing across any contiguous already-received TSNs beyond it.
    fn applyForwardTsn(*_, new_cumulative_tsn: u32): u32 {
        if (tsnLe(new_cumulative_tsn, cumulative_tsn_ack)) {
            return 0;
        }
        const skip: u32 = new_cumulative_tsn -% cumulative_tsn_ack;
        shiftBitmapRight(skip as usize);
        cumulative_tsn_ack = new_cumulative_tsn;
        total_forwarded += skip as u64;
        const cascaded: u32 = advanceContiguous();
        return skip +% cascaded;
    }

    /// Scans the receive bitmap and emits RFC 4960 §3.3.4 `GapAckBlock` ranges into `out_gaps`.
    fn buildGapAckBlocks(out_gaps: []GapAckBlock): usize ! SctpDcError {
        let count: usize = 0;
        let in_run: bool = false;
        let run_start: usize = 0;
        let w: usize = 0;
        while (w < bitmap.len) : (w += 1) {
            const word = bitmap[w];
            if (word == 0 && !in_run) {
                continue;
            }
            let b: usize = 0;
            while (b < 64) : (b += 1) {
                const mask: u64 = @as(u64, 1) << (@as(u6, @intCast(b)));
                const is_set: bool = (word & mask) != 0;
                const offset_1: usize = w * 64 + b + 1;
                if (is_set) {
                    if (!in_run) {
                        in_run = true;
                        run_start = offset_1;
                    }
                } else if (in_run) {
                    if (count >= out_gaps.len) {
                        return SctpDcError.GapBlockOverflow;
                    }
                    const run_end: usize = offset_1 - 1;
                    if (run_end > 65535) {
                        return SctpDcError.GapBlockOverflow;
                    }
                    out_gaps[count] = GapAckBlock{
                        start: run_start as u16,
                        end: run_end as u16,
                    };
                    count += 1;
                    in_run = false;
                }
            }
        }
        if (in_run) {
            if (count >= out_gaps.len) {
                return SctpDcError.GapBlockOverflow;
            }
            const run_end: usize = bitmap.len * 64;
            if (run_end > 65535) {
                return SctpDcError.GapBlockOverflow;
            }
            out_gaps[count] = GapAckBlock{
                start: run_start as u16,
                end: run_end as u16,
            };
            count += 1;
        }
        return count;
    }

    /// Encodes a SACK chunk reflecting current cumulative TSN, gap blocks, and duplicate TSNs,
    /// then clears the pending duplicate TSN list per RFC 4960 §6.2.
    fn buildSackChunk(*_, out: []u8, scratch_gaps: []GapAckBlock): usize ! SctpDcError {
        const n_gaps = buildGapAckBlocks(scratch_gaps)?;
        const written = encodeSctpSackChunk(
            cumulative_tsn_ack,
            a_rwnd,
            scratch_gaps[0..n_gaps],
            dup_buf[0..dup_count],
            out,
        )?;
        dup_count = 0;
        return written;
    }
}

// ============================================================================
// 6. Unbounded External-Slice Multi-TSN Fragment Reassembler
// ============================================================================

/// Outcome returned when feeding a DATA chunk into `SctpFragmentReassembler`.
pub enum SctpReassembleOutcome {
    in_progress,
    complete,
}

/// Unbounded external-slice RFC 4960 §6.9 user-message fragment reassembler.
/// Supports `B=1,E=1` (single), `B=1,E=0` (first), `B=0,E=0` (middle), and `B=0,E=1` (last)
/// across consecutive TSNs with zero dynamic heap allocation and no hardcoded payload cap.
pub struct SctpFragmentReassembler {
    buf: []u8 = [];
    written: usize = 0;
    active: bool = false;
    stream_id: u16 = 0;
    stream_seq: u16 = 0;
    ppid: u32 = 0;
    unordered: bool = false;
    expected_tsn: u32 = 0;
    fragment_count: usize = 0;

    /// Initializes the reassembler over caller-provided external `storage` slice.
    static fn init(storage: []u8): SctpFragmentReassembler ! SctpDcError {
        if (storage.len == 0) {
            return SctpDcError.BufferTooSmall;
        }
        return SctpFragmentReassembler{
            buf: storage,
            written: 0,
            active: false,
            stream_id: 0,
            stream_seq: 0,
            ppid: 0,
            unordered: false,
            expected_tsn: 0,
            fragment_count: 0,
        };
    }

    /// Aborts any in-progress fragmented message (e.g., upon PR-SCTP `FORWARD_TSN`).
    fn abort(*_) {
        active = false;
        written = 0;
        fragment_count = 0;
    }

    /// Returns `true` while a multi-fragment message is actively being assembled.
    fn isInProgress(): bool {
        return active;
    }

    /// Returns the reassembled message bytes (valid immediately after `.complete`).
    fn message(): []const u8 {
        return buf[0..written];
    }

    /// Feeds a decoded `SctpDataPayload` fragment into the reassembler.
    fn pushFragment(*_, chunk: SctpDataPayload): SctpReassembleOutcome ! SctpDcError {
        const b = chunk.isBegin();
        const e = chunk.isEnd();
        const u = chunk.isUnordered();

        if (b && e) {
            // Single-fragment complete message
            if (active) {
                return SctpDcError.FragmentIncomplete;
            }
            if (chunk.user_data.len > buf.len) {
                return SctpDcError.BufferTooSmall;
            }
            copyBytes(buf, 0, chunk.user_data);
            written = chunk.user_data.len;
            stream_id = chunk.stream_id;
            stream_seq = chunk.stream_seq;
            ppid = chunk.ppid;
            unordered = u;
            fragment_count = 1;
            active = false;
            return SctpReassembleOutcome.complete;
        }

        if (b && !e) {
            // Beginning fragment
            if (active) {
                return SctpDcError.FragmentIncomplete;
            }
            if (chunk.user_data.len > buf.len) {
                return SctpDcError.BufferTooSmall;
            }
            copyBytes(buf, 0, chunk.user_data);
            written = chunk.user_data.len;
            stream_id = chunk.stream_id;
            stream_seq = chunk.stream_seq;
            ppid = chunk.ppid;
            unordered = u;
            expected_tsn = chunk.tsn +% 1;
            fragment_count = 1;
            active = true;
            return SctpReassembleOutcome.in_progress;
        }

        // Middle (B=0,E=0) or Ending (B=0,E=1) fragment
        if (!active) {
            return SctpDcError.FragmentOutOfOrder;
        }
        if (chunk.stream_id != stream_id || u != unordered) {
            return SctpDcError.FragmentStreamMismatch;
        }
        if (!unordered && chunk.stream_seq != stream_seq) {
            return SctpDcError.FragmentStreamMismatch;
        }
        if (chunk.tsn != expected_tsn) {
            return SctpDcError.FragmentOutOfOrder;
        }
        if (written + chunk.user_data.len > buf.len) {
            return SctpDcError.BufferTooSmall;
        }
        copyBytes(buf, written, chunk.user_data);
        written += chunk.user_data.len;
        expected_tsn = expected_tsn +% 1;
        fragment_count += 1;

        if (e) {
            active = false;
            return SctpReassembleOutcome.complete;
        }
        return SctpReassembleOutcome.in_progress;
    }
}

// ============================================================================
// 7. RFC 8832 WebRTC DataChannel Establishment Protocol (DCEP)
// ============================================================================

/// All 6 RFC 8832 §5.1 WebRTC DataChannel reliability & ordering modes.
pub enum DcepChannelType {
    reliable,
    reliable_unordered,
    partial_reliable_rexmit,
    partial_reliable_rexmit_unordered,
    partial_reliable_timed,
    partial_reliable_timed_unordered,

    /// Converts the channel type enum to its RFC 8832 wire byte.
    fn toU8(): u8 {
        return match (_) {
            DcepChannelType.reliable => DCEP_CH_RELIABLE,
            DcepChannelType.reliable_unordered => DCEP_CH_RELIABLE_UNORDERED,
            DcepChannelType.partial_reliable_rexmit => DCEP_CH_PARTIAL_RELIABLE_REXMIT,
            DcepChannelType.partial_reliable_rexmit_unordered => DCEP_CH_PARTIAL_RELIABLE_REXMIT_UNORDERED,
            DcepChannelType.partial_reliable_timed => DCEP_CH_PARTIAL_RELIABLE_TIMED,
            DcepChannelType.partial_reliable_timed_unordered => DCEP_CH_PARTIAL_RELIABLE_TIMED_UNORDERED,
        };
    }

    /// Parses an RFC 8832 wire byte into a `DcepChannelType`.
    static fn fromU8(raw: u8): DcepChannelType ! SctpDcError {
        if (raw == DCEP_CH_RELIABLE) return DcepChannelType.reliable;
        if (raw == DCEP_CH_RELIABLE_UNORDERED) return DcepChannelType.reliable_unordered;
        if (raw == DCEP_CH_PARTIAL_RELIABLE_REXMIT) return DcepChannelType.partial_reliable_rexmit;
        if (raw == DCEP_CH_PARTIAL_RELIABLE_REXMIT_UNORDERED) return DcepChannelType.partial_reliable_rexmit_unordered;
        if (raw == DCEP_CH_PARTIAL_RELIABLE_TIMED) return DcepChannelType.partial_reliable_timed;
        if (raw == DCEP_CH_PARTIAL_RELIABLE_TIMED_UNORDERED) return DcepChannelType.partial_reliable_timed_unordered;
        return SctpDcError.InvalidChannelType;
    }

    /// Returns `true` when unordered delivery (`0x80` bit) is enabled.
    fn isUnordered(): bool {
        return (_.toU8() & 0x80) != 0;
    }

    /// Returns `true` for fully reliable channels (`RELIABLE` / `RELIABLE_UNORDERED`).
    fn isReliable(): bool {
        return (_.toU8() & 0x7F) == 0;
    }
}

/// RFC 8832 §5.1 `DATA_CHANNEL_OPEN` (`0x03`) control message.
pub struct DcepOpenMessage {
    channel_type: DcepChannelType = DcepChannelType.reliable;
    priority: u16 = 0;
    reliability_parameter: u32 = 0;
    label: []const u8 = "";
    protocol: []const u8 = "";
}

/// Encodes an RFC 8832 §5.1 `DATA_CHANNEL_OPEN` (`0x03`) message into `out`.
pub fn encodeDcepOpen(msg: DcepOpenMessage, out: []u8): usize ! SctpDcError {
    if (msg.label.len > 65535 || msg.protocol.len > 65535) {
        return SctpDcError.InvalidDcepMessage;
    }
    const total: usize = msg.label.len + msg.protocol.len + 12;
    if (out.len < total) {
        return SctpDcError.BufferTooSmall;
    }
    out[0] = DCEP_MSG_OPEN;
    out[1] = msg.channel_type.toU8();
    writeBeU16(out, 2, msg.priority);
    writeBeU32(out, 4, msg.reliability_parameter);
    writeBeU16(out, 8, msg.label.len as u16);
    writeBeU16(out, 10, msg.protocol.len as u16);
    copyBytes(out, 12, msg.label);
    copyBytes(out, 12 + msg.label.len, msg.protocol);
    return total;
}

/// Decodes an RFC 8832 §5.1 `DATA_CHANNEL_OPEN` (`0x03`) message with zero-copy `label`/`protocol` views.
pub fn decodeDcepOpen(data: []const u8): DcepOpenMessage ! SctpDcError {
    if (data.len < 12) {
        return SctpDcError.InvalidDcepMessage;
    }
    if (data[0] != DCEP_MSG_OPEN) {
        return SctpDcError.InvalidDcepMessage;
    }
    const ch_type = DcepChannelType.fromU8(data[1])?;
    const priority: u16 = readBeU16(data, 2);
    const rel_param: u32 = readBeU32(data, 4);
    const label_len: usize = readBeU16(data, 8) as usize;
    const proto_len: usize = readBeU16(data, 10) as usize;
    if (12 + label_len + proto_len != data.len) {
        return SctpDcError.InvalidDcepMessage;
    }
    return DcepOpenMessage{
        channel_type: ch_type,
        priority: priority,
        reliability_parameter: rel_param,
        label: data[12..(12 + label_len)],
        protocol: data[(12 + label_len)..(12 + label_len + proto_len)],
    };
}

/// Encodes an RFC 8832 §5.2 `DATA_CHANNEL_ACK` (`0x02`) 1-byte control message into `out`.
pub fn encodeDcepAck(out: []u8): usize ! SctpDcError {
    if (out.len < 1) {
        return SctpDcError.BufferTooSmall;
    }
    out[0] = DCEP_MSG_ACK;
    return 1;
}

/// Validates an RFC 8832 §5.2 `DATA_CHANNEL_ACK` (`0x02`) control message.
pub fn decodeDcepAck(data: []const u8): bool ! SctpDcError {
    if (data.len != 1 || data[0] != DCEP_MSG_ACK) {
        return SctpDcError.InvalidDcepMessage;
    }
    return true;
}

// ============================================================================
// 8. RFC 4960 4-Way Handshake & Graceful Shutdown Association State Machine
// ============================================================================

/// RFC 4960 §4 SCTP association lifecycle states.
pub enum SctpAssociationState {
    closed,
    cookie_wait,
    cookie_echoed,
    established,
    shutdown_sent,
    shutdown_received,
    shutdown_ack_sent,
}

/// Configuration for initializing an `SctpAssociation` endpoint.
pub struct SctpAssociationConfig {
    local_port: u16 = 5000;
    remote_port: u16 = 5000;
    local_tag: u32 = 0x11223344;
    initial_tsn: u32 = 1000;
    a_rwnd: u32 = 262144;
    num_outbound_streams: u16 = 1024;
    num_inbound_streams: u16 = 1024;
    forward_tsn_supported: bool = true;
    cookie_secret: u32 = 0xA55A5AA5;
}

fn computeCookieMac(
    secret: u32,
    local_tag: u32,
    peer_tag: u32,
    peer_init_tsn: u32,
    ports_word: u32,
): u32 {
    let raw: [20]u8 = [0; 20];
    writeBeU32(raw[0..20], 0, secret);
    writeBeU32(raw[0..20], 4, local_tag);
    writeBeU32(raw[0..20], 8, peer_tag);
    writeBeU32(raw[0..20], 12, peer_init_tsn);
    writeBeU32(raw[0..20], 16, ports_word);
    return computeSctpCrc32c(raw[0..20]);
}

/// Zero-heap RFC 4960 + RFC 3758 + RFC 8832 SCTP association state machine.
/// Drives complete 4-way handshake (`INIT -> INIT_ACK -> COOKIE_ECHO -> COOKIE_ACK`),
/// DATA / DCEP / SACK / FORWARD_TSN / HEARTBEAT packet construction, and 3-way graceful shutdown.
pub struct SctpAssociation {
    state: SctpAssociationState = SctpAssociationState.closed;
    local_port: u16 = 5000;
    remote_port: u16 = 5000;
    local_tag: u32 = 0;
    peer_tag: u32 = 0;
    initial_tsn: u32 = 0;
    next_tsn: u32 = 0;
    peer_initial_tsn: u32 = 0;
    a_rwnd: u32 = 262144;
    peer_a_rwnd: u32 = 0;
    outbound_streams: u16 = 1024;
    inbound_streams: u16 = 1024;
    negotiated_outbound_streams: u16 = 0;
    negotiated_inbound_streams: u16 = 0;
    forward_tsn_supported: bool = true;
    peer_forward_tsn_supported: bool = false;
    cookie_secret: u32 = 0xA55A5AA5;

    /// Creates a new association in `SctpAssociationState.closed`.
    static fn init(cfg: SctpAssociationConfig): SctpAssociation ! SctpDcError {
        if (cfg.local_tag == 0) {
            return SctpDcError.InvalidVerificationTag;
        }
        if (cfg.num_outbound_streams == 0 || cfg.num_inbound_streams == 0) {
            return SctpDcError.StreamCountZero;
        }
        return SctpAssociation{
            state: SctpAssociationState.closed,
            local_port: cfg.local_port,
            remote_port: cfg.remote_port,
            local_tag: cfg.local_tag,
            peer_tag: 0,
            initial_tsn: cfg.initial_tsn,
            next_tsn: cfg.initial_tsn,
            peer_initial_tsn: 0,
            a_rwnd: cfg.a_rwnd,
            peer_a_rwnd: 0,
            outbound_streams: cfg.num_outbound_streams,
            inbound_streams: cfg.num_inbound_streams,
            negotiated_outbound_streams: 0,
            negotiated_inbound_streams: 0,
            forward_tsn_supported: cfg.forward_tsn_supported,
            peer_forward_tsn_supported: false,
            cookie_secret: cfg.cookie_secret,
        };
    }

    /// Returns `true` when the association has completed the 4-way handshake.
    fn isEstablished(): bool {
        return state == SctpAssociationState.established;
    }

    // --- Client-Side 4-Way Handshake ---

    /// Step 1 (Client): transitions `closed -> cookie_wait` and builds a CRC32C-stamped `INIT` packet.
    fn buildInit(*_, out: []u8): usize ! SctpDcError {
        if (state != SctpAssociationState.closed) {
            return SctpDcError.InvalidState;
        }
        let b = SctpPacketBuilder.init(out, local_port, remote_port, 0)?;
        const init_body = SctpInitBody{
            is_ack: false,
            initiate_tag: local_tag,
            a_rwnd: a_rwnd,
            num_outbound_streams: outbound_streams,
            num_inbound_streams: inbound_streams,
            initial_tsn: initial_tsn,
            params_raw: "",
        };
        let params: [1]SctpTlvParam = [
            SctpTlvParam{ param_type: SCTP_PARAM_FORWARD_TSN_SUPPORTED, value: "" },
        ];
        const param_slice: []const SctpTlvParam = if (forward_tsn_supported) params[0..1] else params[0..0];
        const clen = encodeSctpInitChunk(init_body, param_slice, b.tailSlice())?;
        * = b.commitChunk(clen)?;
        const pkt_len = b.finish()?;
        state = SctpAssociationState.cookie_wait;
        return pkt_len;
    }

    /// Step 3 (Client): processes `INIT_ACK`, transitions `cookie_wait -> cookie_echoed`,
    /// and builds a CRC32C-stamped `COOKIE_ECHO` packet.
    fn onInitAck(*_, pkt: []const u8, out_cookie_echo: []u8): usize ! SctpDcError {
        if (state != SctpAssociationState.cookie_wait) {
            return SctpDcError.InvalidState;
        }
        if (!(verifySctpPacketCrc32c(pkt)?)) {
            return SctpDcError.ChecksumMismatch;
        }
        const hdr = SctpCommonHeader.decode(pkt)?;
        if (hdr.verification_tag != local_tag) {
            return SctpDcError.InvalidVerificationTag;
        }
        let it = SctpWireChunkIter.fromPacket(pkt)?;
        const opt_chunk = it.next()?;
        const chunk = opt_chunk orelse return SctpDcError.InvalidChunkType;
        if (chunk.chunk_type != SCTP_CHUNK_INIT_ACK) {
            return SctpDcError.InvalidChunkType;
        }
        const ack = decodeSctpInitChunk(chunk.raw)?;
        const cookie = ack.findStateCookie()?;

        peer_tag = ack.initiate_tag;
        peer_initial_tsn = ack.initial_tsn;
        peer_a_rwnd = ack.a_rwnd;
        negotiated_outbound_streams = if (outbound_streams < ack.num_inbound_streams) outbound_streams else ack.num_inbound_streams;
        negotiated_inbound_streams = if (inbound_streams < ack.num_outbound_streams) inbound_streams else ack.num_outbound_streams;
        peer_forward_tsn_supported = ack.supportsForwardTsn()?;

        let b = SctpPacketBuilder.init(out_cookie_echo, local_port, remote_port, peer_tag)?;
        const clen = encodeSctpCookieEchoChunk(cookie, b.tailSlice())?;
        * = b.commitChunk(clen)?;
        const pkt_len = b.finish()?;
        state = SctpAssociationState.cookie_echoed;
        return pkt_len;
    }

    /// Step 4 (Client): validates `COOKIE_ACK` and transitions `cookie_echoed -> established`.
    fn onCookieAck(*_, pkt: []const u8): bool ! SctpDcError {
        if (state != SctpAssociationState.cookie_echoed) {
            return SctpDcError.InvalidState;
        }
        if (!(verifySctpPacketCrc32c(pkt)?)) {
            return SctpDcError.ChecksumMismatch;
        }
        const hdr = SctpCommonHeader.decode(pkt)?;
        if (hdr.verification_tag != local_tag) {
            return SctpDcError.InvalidVerificationTag;
        }
        let it = SctpWireChunkIter.fromPacket(pkt)?;
        const opt_chunk = it.next()?;
        const chunk = opt_chunk orelse return SctpDcError.InvalidChunkType;
        if (chunk.chunk_type != SCTP_CHUNK_COOKIE_ACK) {
            return SctpDcError.InvalidChunkType;
        }
        state = SctpAssociationState.established;
        return true;
    }

    // --- Server-Side Stateless 4-Way Handshake ---

    /// Step 2 (Server): validates `INIT` (RFC 4960 §5.1 statelessly, remaining in `closed`)
    /// and builds a CRC32C-stamped `INIT_ACK` with an authenticated 20-byte State Cookie.
    fn onInit(*_, pkt: []const u8, out_init_ack: []u8): usize ! SctpDcError {
        if (state != SctpAssociationState.closed) {
            return SctpDcError.InvalidState;
        }
        if (!(verifySctpPacketCrc32c(pkt)?)) {
            return SctpDcError.ChecksumMismatch;
        }
        const hdr = SctpCommonHeader.decode(pkt)?;
        if (hdr.verification_tag != 0) {
            return SctpDcError.InvalidVerificationTag;
        }
        let it = SctpWireChunkIter.fromPacket(pkt)?;
        const opt_chunk = it.next()?;
        const chunk = opt_chunk orelse return SctpDcError.InvalidChunkType;
        if (chunk.chunk_type != SCTP_CHUNK_INIT) {
            return SctpDcError.InvalidChunkType;
        }
        const req = decodeSctpInitChunk(chunk.raw)?;
        const peer_fwd = req.supportsForwardTsn()?;

        // Pack 20-byte authenticated State Cookie:
        // [0..4] peer_tag, [4..8] peer_initial_tsn, [8..12] peer_a_rwnd,
        // [12..14] peer_out_streams, [14..16] flags_and_ports_hash, [16..20] cookie_mac
        let cookie_buf: [SCTP_COOKIE_BODY_LEN]u8 = [0; SCTP_COOKIE_BODY_LEN];
        const ports_word: u32 = ((hdr.src_port as u32) << 16) | (hdr.dst_port as u32);
        const mac: u32 = computeCookieMac(cookie_secret, local_tag, req.initiate_tag, req.initial_tsn, ports_word);
        writeBeU32(cookie_buf[0..SCTP_COOKIE_BODY_LEN], 0, req.initiate_tag);
        writeBeU32(cookie_buf[0..SCTP_COOKIE_BODY_LEN], 4, req.initial_tsn);
        writeBeU32(cookie_buf[0..SCTP_COOKIE_BODY_LEN], 8, req.a_rwnd);
        writeBeU16(cookie_buf[0..SCTP_COOKIE_BODY_LEN], 12, req.num_outbound_streams);
        writeBeU16(cookie_buf[0..SCTP_COOKIE_BODY_LEN], 14, if (peer_fwd) 1 else 0);
        writeBeU32(cookie_buf[0..SCTP_COOKIE_BODY_LEN], 16, mac);

        let b = SctpPacketBuilder.init(out_init_ack, local_port, hdr.src_port, req.initiate_tag)?;
        const ack_body = SctpInitBody{
            is_ack: true,
            initiate_tag: local_tag,
            a_rwnd: a_rwnd,
            num_outbound_streams: outbound_streams,
            num_inbound_streams: inbound_streams,
            initial_tsn: initial_tsn,
            params_raw: "",
        };
        let params: [2]SctpTlvParam = [
            SctpTlvParam{ param_type: SCTP_PARAM_STATE_COOKIE, value: cookie_buf[0..SCTP_COOKIE_BODY_LEN] },
            SctpTlvParam{ param_type: SCTP_PARAM_FORWARD_TSN_SUPPORTED, value: "" },
        ];
        const param_slice: []const SctpTlvParam = if (forward_tsn_supported) params[0..2] else params[0..1];
        const clen = encodeSctpInitChunk(ack_body, param_slice, b.tailSlice())?;
        * = b.commitChunk(clen)?;
        return b.finish();
    }

    /// Step 4 (Server): verifies `COOKIE_ECHO` + State Cookie MAC, transitions `closed -> established`,
    /// and emits a CRC32C-stamped `COOKIE_ACK` packet.
    fn onCookieEcho(*_, pkt: []const u8, out_cookie_ack: []u8): usize ! SctpDcError {
        if (state != SctpAssociationState.closed) {
            return SctpDcError.InvalidState;
        }
        if (!(verifySctpPacketCrc32c(pkt)?)) {
            return SctpDcError.ChecksumMismatch;
        }
        const hdr = SctpCommonHeader.decode(pkt)?;
        if (hdr.verification_tag != local_tag) {
            return SctpDcError.InvalidVerificationTag;
        }
        let it = SctpWireChunkIter.fromPacket(pkt)?;
        const opt_chunk = it.next()?;
        const chunk = opt_chunk orelse return SctpDcError.InvalidChunkType;
        if (chunk.chunk_type != SCTP_CHUNK_COOKIE_ECHO) {
            return SctpDcError.InvalidChunkType;
        }
        const cookie = decodeSctpCookieEchoChunk(chunk.raw)?;
        if (cookie.len != SCTP_COOKIE_BODY_LEN) {
            return SctpDcError.InvalidCookie;
        }
        const ck_peer_tag = readBeU32(cookie, 0);
        const ck_peer_tsn = readBeU32(cookie, 4);
        const ck_peer_rwnd = readBeU32(cookie, 8);
        const ck_peer_out = readBeU16(cookie, 12);
        const ck_peer_fwd = readBeU16(cookie, 14) != 0;
        const ck_mac = readBeU32(cookie, 16);

        const ports_word: u32 = ((hdr.src_port as u32) << 16) | (hdr.dst_port as u32);
        const expected_mac = computeCookieMac(cookie_secret, local_tag, ck_peer_tag, ck_peer_tsn, ports_word);
        if (ck_mac != expected_mac) {
            return SctpDcError.InvalidCookie;
        }

        remote_port = hdr.src_port;
        peer_tag = ck_peer_tag;
        peer_initial_tsn = ck_peer_tsn;
        peer_a_rwnd = ck_peer_rwnd;
        negotiated_outbound_streams = outbound_streams;
        negotiated_inbound_streams = if (inbound_streams < ck_peer_out) inbound_streams else ck_peer_out;
        peer_forward_tsn_supported = ck_peer_fwd;

        let b = SctpPacketBuilder.init(out_cookie_ack, local_port, remote_port, peer_tag)?;
        const clen = encodeSctpCookieAckChunk(b.tailSlice())?;
        * = b.commitChunk(clen)?;
        const pkt_len = b.finish()?;
        state = SctpAssociationState.established;
        return pkt_len;
    }

    // --- Established-State Packet Construction & Graceful 3-Way Shutdown ---

    /// Builds a CRC32C-stamped SCTP `DATA` packet, auto-incrementing `next_tsn`.
    fn buildDataPacket(
        *_,
        stream_id: u16,
        stream_seq: u16,
        ppid: u32,
        flags: u8,
        payload: []const u8,
        out: []u8,
    ): usize ! SctpDcError {
        if (state != SctpAssociationState.established) {
            return SctpDcError.InvalidState;
        }
        let b = SctpPacketBuilder.init(out, local_port, remote_port, peer_tag)?;
        const tsn: u32 = next_tsn;
        const data = SctpDataPayload{
            flags: flags,
            tsn: tsn,
            stream_id: stream_id,
            stream_seq: stream_seq,
            ppid: ppid,
            user_data: payload,
        };
        const clen = encodeSctpDataChunk(data, b.tailSlice())?;
        * = b.commitChunk(clen)?;
        const pkt_len = b.finish()?;
        next_tsn = next_tsn +% 1;
        return pkt_len;
    }

    /// Builds an SCTP `DATA` packet carrying an RFC 8832 `DATA_CHANNEL_OPEN` (`PPID = 50`).
    fn buildDcepOpenPacket(
        *_,
        stream_id: u16,
        msg: DcepOpenMessage,
        scratch_dcep: []u8,
        out: []u8,
    ): usize ! SctpDcError {
        const dcep_len = encodeDcepOpen(msg, scratch_dcep)?;
        return buildDataPacket(
            stream_id,
            0,
            SCTP_PPID_DCEP,
            SCTP_DATA_FLAG_B | SCTP_DATA_FLAG_E,
            scratch_dcep[0..dcep_len],
            out,
        );
    }

    /// Builds an SCTP `DATA` packet carrying an RFC 8832 `DATA_CHANNEL_ACK` (`PPID = 50`).
    fn buildDcepAckPacket(*_, stream_id: u16, out: []u8): usize ! SctpDcError {
        let ack_buf: [1]u8 = [0];
        const ack_len = encodeDcepAck(ack_buf[0..1])?;
        return buildDataPacket(
            stream_id,
            0,
            SCTP_PPID_DCEP,
            SCTP_DATA_FLAG_B | SCTP_DATA_FLAG_E,
            ack_buf[0..ack_len],
            out,
        );
    }

    /// Builds a CRC32C-stamped SCTP `SACK` packet from `tracker`.
    fn buildSackPacket(
        *_,
        tracker: *SctpRecvTracker,
        scratch_gaps: []GapAckBlock,
        out: []u8,
    ): usize ! SctpDcError {
        if (state != SctpAssociationState.established && state != SctpAssociationState.shutdown_sent) {
            return SctpDcError.InvalidState;
        }
        let b = SctpPacketBuilder.init(out, local_port, remote_port, peer_tag)?;
        const clen = tracker.buildSackChunk(b.tailSlice(), scratch_gaps)?;
        * = b.commitChunk(clen)?;
        return b.finish();
    }

    /// Builds a CRC32C-stamped RFC 3758 `FORWARD_TSN` packet.
    fn buildForwardTsnPacket(
        *_,
        new_cumulative_tsn: u32,
        streams: []const ForwardTsnStreamEntry,
        out: []u8,
    ): usize ! SctpDcError {
        if (state != SctpAssociationState.established) {
            return SctpDcError.InvalidState;
        }
        let b = SctpPacketBuilder.init(out, local_port, remote_port, peer_tag)?;
        const clen = encodeSctpForwardTsnChunk(new_cumulative_tsn, streams, b.tailSlice())?;
        * = b.commitChunk(clen)?;
        return b.finish();
    }

    /// Builds a CRC32C-stamped `HEARTBEAT` packet carrying `info`.
    fn buildHeartbeatPacket(*_, info: []const u8, out: []u8): usize ! SctpDcError {
        if (state != SctpAssociationState.established) {
            return SctpDcError.InvalidState;
        }
        let b = SctpPacketBuilder.init(out, local_port, remote_port, peer_tag)?;
        const clen = encodeSctpHeartbeatChunk(false, info, b.tailSlice())?;
        * = b.commitChunk(clen)?;
        return b.finish();
    }

    /// Validates an incoming `HEARTBEAT` packet and builds a matching `HEARTBEAT_ACK` packet.
    fn onHeartbeat(*_, pkt: []const u8, out_ack: []u8): usize ! SctpDcError {
        if (state != SctpAssociationState.established) {
            return SctpDcError.InvalidState;
        }
        if (!(verifySctpPacketCrc32c(pkt)?)) {
            return SctpDcError.ChecksumMismatch;
        }
        const hdr = SctpCommonHeader.decode(pkt)?;
        if (hdr.verification_tag != local_tag) {
            return SctpDcError.InvalidVerificationTag;
        }
        let it = SctpWireChunkIter.fromPacket(pkt)?;
        const opt_chunk = it.next()?;
        const chunk = opt_chunk orelse return SctpDcError.InvalidChunkType;
        if (chunk.chunk_type != SCTP_CHUNK_HEARTBEAT) {
            return SctpDcError.InvalidChunkType;
        }
        const info = decodeSctpHeartbeatChunk(chunk.raw)?;
        let b = SctpPacketBuilder.init(out_ack, local_port, remote_port, peer_tag)?;
        const clen = encodeSctpHeartbeatChunk(true, info, b.tailSlice())?;
        * = b.commitChunk(clen)?;
        return b.finish();
    }

    /// Initiates graceful shutdown: transitions `established -> shutdown_sent` and builds `SHUTDOWN`.
    fn buildShutdownPacket(*_, cumulative_tsn_ack: u32, out: []u8): usize ! SctpDcError {
        if (state != SctpAssociationState.established) {
            return SctpDcError.InvalidState;
        }
        let b = SctpPacketBuilder.init(out, local_port, remote_port, peer_tag)?;
        const clen = encodeSctpShutdownChunk(cumulative_tsn_ack, b.tailSlice())?;
        * = b.commitChunk(clen)?;
        const pkt_len = b.finish()?;
        state = SctpAssociationState.shutdown_sent;
        return pkt_len;
    }

    /// Handles peer `SHUTDOWN`: transitions `established -> shutdown_ack_sent` and builds `SHUTDOWN_ACK`.
    fn onShutdown(*_, pkt: []const u8, out_ack: []u8): usize ! SctpDcError {
        if (state != SctpAssociationState.established) {
            return SctpDcError.InvalidState;
        }
        if (!(verifySctpPacketCrc32c(pkt)?)) {
            return SctpDcError.ChecksumMismatch;
        }
        const hdr = SctpCommonHeader.decode(pkt)?;
        if (hdr.verification_tag != local_tag) {
            return SctpDcError.InvalidVerificationTag;
        }
        let it = SctpWireChunkIter.fromPacket(pkt)?;
        const opt_chunk = it.next()?;
        const chunk = opt_chunk orelse return SctpDcError.InvalidChunkType;
        * = decodeSctpShutdownChunk(chunk.raw)?;
        let b = SctpPacketBuilder.init(out_ack, local_port, remote_port, peer_tag)?;
        const clen = encodeSctpShutdownAckChunk(b.tailSlice())?;
        * = b.commitChunk(clen)?;
        const pkt_len = b.finish()?;
        state = SctpAssociationState.shutdown_ack_sent;
        return pkt_len;
    }

    /// Handles peer `SHUTDOWN_ACK`: transitions `shutdown_sent -> closed` and builds `SHUTDOWN_COMPLETE`.
    fn onShutdownAck(*_, pkt: []const u8, out_complete: []u8): usize ! SctpDcError {
        if (state != SctpAssociationState.shutdown_sent) {
            return SctpDcError.InvalidState;
        }
        if (!(verifySctpPacketCrc32c(pkt)?)) {
            return SctpDcError.ChecksumMismatch;
        }
        const hdr = SctpCommonHeader.decode(pkt)?;
        if (hdr.verification_tag != local_tag) {
            return SctpDcError.InvalidVerificationTag;
        }
        let it = SctpWireChunkIter.fromPacket(pkt)?;
        const opt_chunk = it.next()?;
        const chunk = opt_chunk orelse return SctpDcError.InvalidChunkType;
        if (chunk.chunk_type != SCTP_CHUNK_SHUTDOWN_ACK) {
            return SctpDcError.InvalidChunkType;
        }
        let b = SctpPacketBuilder.init(out_complete, local_port, remote_port, peer_tag)?;
        const clen = encodeSctpShutdownCompleteChunk(false, b.tailSlice())?;
        * = b.commitChunk(clen)?;
        const pkt_len = b.finish()?;
        state = SctpAssociationState.closed;
        return pkt_len;
    }

    /// Handles peer `SHUTDOWN_COMPLETE`: transitions `shutdown_ack_sent -> closed`.
    fn onShutdownComplete(*_, pkt: []const u8): bool ! SctpDcError {
        if (state != SctpAssociationState.shutdown_ack_sent) {
            return SctpDcError.InvalidState;
        }
        if (!(verifySctpPacketCrc32c(pkt)?)) {
            return SctpDcError.ChecksumMismatch;
        }
        const hdr = SctpCommonHeader.decode(pkt)?;
        if (hdr.verification_tag != local_tag) {
            return SctpDcError.InvalidVerificationTag;
        }
        let it = SctpWireChunkIter.fromPacket(pkt)?;
        const opt_chunk = it.next()?;
        const chunk = opt_chunk orelse return SctpDcError.InvalidChunkType;
        if (chunk.chunk_type != SCTP_CHUNK_SHUTDOWN_COMPLETE) {
            return SctpDcError.InvalidChunkType;
        }
        state = SctpAssociationState.closed;
        return true;
    }
}
