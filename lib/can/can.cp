// Origin: Pure-CP Stdlib (Zero-Alloc ISO 11898 CAN 2.0/FD, SocketCAN, SAE J1939 & DBC Signal Codec) | Created: 2026-11-22

// ---------------------------------------------------------------------------
// Protocol Constants (ISO 11898 / SocketCAN / CAN FD)
// ---------------------------------------------------------------------------

pub const CAN_FRAME_SIZE: usize = 16;
pub const CANFD_FRAME_SIZE: usize = 72;
pub const CAN_MAX_DLEN: u8 = 8;
pub const CANFD_MAX_DLEN: u8 = 64;

pub const CAN_SFF_MASK: u32 = 0x000007FF;
pub const CAN_EFF_MASK: u32 = 0x1FFFFFFF;
pub const CAN_EFF_FLAG: u32 = 0x80000000;
pub const CAN_RTR_FLAG: u32 = 0x40000000;
pub const CAN_ERR_FLAG: u32 = 0x20000000;

// CAN FD Flags
pub const CANFD_BRS: u8 = 0x01;
pub const CANFD_ESI: u8 = 0x02;
pub const CANFD_FDF: u8 = 0x04;

// ---------------------------------------------------------------------------
// Internal Endian Helpers
// ---------------------------------------------------------------------------

fn readU32Le(buf: []const u8, off: usize): u32 {
    const b0 = buf[off] as u32;
    const b1 = buf[off + 1] as u32;
    const b2 = buf[off + 2] as u32;
    const b3 = buf[off + 3] as u32;
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

fn readU32Be(buf: []const u8, off: usize): u32 {
    const b0 = buf[off] as u32;
    const b1 = buf[off + 1] as u32;
    const b2 = buf[off + 2] as u32;
    const b3 = buf[off + 3] as u32;
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

fn writeU32Le(buf: []u8, off: usize, val: u32) {
    buf[off] = (val & 0xFF) as u8;
    buf[off + 1] = ((val >> 8) & 0xFF) as u8;
    buf[off + 2] = ((val >> 16) & 0xFF) as u8;
    buf[off + 3] = ((val >> 24) & 0xFF) as u8;
}

fn writeU32Be(buf: []u8, off: usize, val: u32) {
    buf[off] = ((val >> 24) & 0xFF) as u8;
    buf[off + 1] = ((val >> 16) & 0xFF) as u8;
    buf[off + 2] = ((val >> 8) & 0xFF) as u8;
    buf[off + 3] = (val & 0xFF) as u8;
}

// ---------------------------------------------------------------------------
// CAN FD DLC <-> Payload Length Mapping (ISO 11898-1:2015)
// ---------------------------------------------------------------------------

/// Maps a 4-bit CAN FD DLC (`0..15`) to payload byte length (`0..64`).
pub fn canFdDlcToLen(dlc: u8): u8? {
    if (dlc <= 8) {
        return dlc;
    }
    if (dlc == 9) return 12;
    if (dlc == 10) return 16;
    if (dlc == 11) return 20;
    if (dlc == 12) return 24;
    if (dlc == 13) return 32;
    if (dlc == 14) return 48;
    if (dlc == 15) return 64;
    return null;
}

/// Maps an exact CAN FD payload byte length (`0..8, 12, 16, 20, 24, 32, 48, 64`) to its 4-bit DLC.
pub fn canFdLenToDlc(len: u8): u8? {
    if (len <= 8) {
        return len;
    }
    if (len == 12) return 9;
    if (len == 16) return 10;
    if (len == 20) return 11;
    if (len == 24) return 12;
    if (len == 32) return 13;
    if (len == 48) return 14;
    if (len == 64) return 15;
    return null;
}

// ---------------------------------------------------------------------------
// CAN Identifier & Frame Models
// ---------------------------------------------------------------------------

/// SocketCAN 32-bit identifier word (11-bit SFF or 29-bit EFF + EFF/RTR/ERR flags).
pub struct CanId {
    raw: u32,

    /// Constructs a standard 11-bit CAN 2.0A identifier.
    fn standard(id_11: u32): CanId {
        return CanId { raw: id_11 & CAN_SFF_MASK };
    }

    /// Constructs an extended 29-bit CAN 2.0B identifier with `CAN_EFF_FLAG` set.
    fn extended(id_29: u32): CanId {
        return CanId { raw: (id_29 & CAN_EFF_MASK) | CAN_EFF_FLAG };
    }

    fn isExtended(): bool {
        return (_.raw & CAN_EFF_FLAG) != 0;
    }

    fn isRtr(): bool {
        return (_.raw & CAN_RTR_FLAG) != 0;
    }

    fn isError(): bool {
        return (_.raw & CAN_ERR_FLAG) != 0;
    }

    /// Returns the masked numeric arbitration ID (29-bit if extended, 11-bit if standard).
    fn id(): u32 {
        if (_.isExtended()) {
            return _.raw & CAN_EFF_MASK;
        }
        return _.raw & CAN_SFF_MASK;
    }
}

/// Unified zero-allocation representation of a CAN 2.0 or CAN FD frame.
pub struct CanFrame {
    can_id: CanId,
    len: u8,
    flags: u8,
    is_fd: bool,
    data: [64]u8,

    /// Returns the active payload slice `data[0..len]`.
    fn payload(): []const u8 {
        const n: usize = if (_.len > CANFD_MAX_DLEN) CANFD_MAX_DLEN as usize else _.len as usize;
        return _.data[0..n];
    }

    fn hasBrs(): bool {
        return (_.flags & CANFD_BRS) != 0;
    }

    fn hasEsi(): bool {
        return (_.flags & CANFD_ESI) != 0;
    }
}

// ---------------------------------------------------------------------------
// SAE J1939 29-bit PGN Codec
// ---------------------------------------------------------------------------

/// Decoded SAE J1939 29-bit extended CAN identifier.
pub struct J1939Id {
    priority: u8,
    edp: u8,
    dp: u8,
    pf: u8,
    ps: u8,
    sa: u8,
    pgn: u32,

    /// Returns true if PDU1 (peer-to-peer, `pf < 240`, `ps` is Destination Address).
    fn isPdu1(): bool {
        return _.pf < 240;
    }

    /// Returns true if PDU2 (broadcast, `pf >= 240`, `ps` is Group Extension part of PGN).
    fn isPdu2(): bool {
        return _.pf >= 240;
    }
}

/// Decodes a 29-bit CAN ID (with or without `CAN_EFF_FLAG`) into its SAE J1939 fields and PGN.
pub fn decodeJ1939(can_id: u32): J1939Id {
    const eff = can_id & CAN_EFF_MASK;
    const priority = ((eff >> 26) & 0x07) as u8;
    const edp = ((eff >> 25) & 0x01) as u8;
    const dp = ((eff >> 24) & 0x01) as u8;
    const pf = ((eff >> 16) & 0xFF) as u8;
    const ps = ((eff >> 8) & 0xFF) as u8;
    const sa = (eff & 0xFF) as u8;

    let pgn: u32 = ((edp as u32) << 17) | ((dp as u32) << 16) | ((pf as u32) << 8);
    if (pf >= 240) {
        pgn = pgn | (ps as u32);
    }

    return J1939Id {
        priority: priority,
        edp: edp,
        dp: dp,
        pf: pf,
        ps: ps,
        sa: sa,
        pgn: pgn,
    };
}

/// Encodes SAE J1939 `priority` (0..7), `pgn` (18-bit), and `sa` (8-bit) into a 29-bit CAN ID.
pub fn encodeJ1939(priority: u8, pgn: u32, sa: u8): u32 {
    const p = ((priority as u32) & 0x07) << 26;
    const g = (pgn & 0x3FFFF) << 8;
    return p | g | (sa as u32);
}

/// Encodes a PDU1 (`pf < 240`) J1939 29-bit CAN ID including Destination Address `da`.
pub fn encodeJ1939Pdu1(priority: u8, pgn: u32, da: u8, sa: u8): u32 {
    const p = ((priority as u32) & 0x07) << 26;
    const g = ((pgn & 0x3FF00) | (da as u32)) << 8;
    return p | g | (sa as u32);
}

// ---------------------------------------------------------------------------
// SocketCAN Frame Codec (Little-Endian & Big-Endian / Network Byte Order)
// ---------------------------------------------------------------------------

fn parseSocketCanInternal(bytes: []const u8, big_endian: bool): CanFrame? {
    if (bytes.len < CAN_FRAME_SIZE) {
        return null;
    }
    const raw_id = if (big_endian) readU32Be(bytes, 0) else readU32Le(bytes, 0);
    const dlen = bytes[4];
    if (dlen > CAN_MAX_DLEN) {
        return null;
    }
    let data: [64]u8 = [0; 64];
    let i: usize = 0;
    while (i < (dlen as usize)) {
        data[i] = bytes[8 + i];
        i += 1;
    }
    return CanFrame {
        can_id: CanId { raw: raw_id },
        len: dlen,
        flags: 0,
        is_fd: false,
        data: data,
    };
}

/// Parses a 16-byte classic SocketCAN frame (little-endian `can_id`).
pub fn parseSocketCan(bytes: []const u8): CanFrame? {
    return parseSocketCanInternal(bytes, false);
}

/// Parses a 16-byte classic SocketCAN frame (big-endian / network byte order `can_id`).
pub fn parseSocketCanBe(bytes: []const u8): CanFrame? {
    return parseSocketCanInternal(bytes, true);
}

fn parseSocketCanFdInternal(bytes: []const u8, big_endian: bool): CanFrame? {
    if (bytes.len < CANFD_FRAME_SIZE) {
        return null;
    }
    const raw_id = if (big_endian) readU32Be(bytes, 0) else readU32Le(bytes, 0);
    const dlen = bytes[4];
    if (dlen > CANFD_MAX_DLEN) {
        return null;
    }
    const flags = bytes[5];
    let data: [64]u8 = [0; 64];
    let i: usize = 0;
    while (i < (dlen as usize)) {
        data[i] = bytes[8 + i];
        i += 1;
    }
    return CanFrame {
        can_id: CanId { raw: raw_id },
        len: dlen,
        flags: flags,
        is_fd: true,
        data: data,
    };
}

/// Parses a 72-byte SocketCAN FD frame (little-endian `can_id`).
pub fn parseSocketCanFd(bytes: []const u8): CanFrame? {
    return parseSocketCanFdInternal(bytes, false);
}

/// Parses a 72-byte SocketCAN FD frame (big-endian / network byte order `can_id`).
pub fn parseSocketCanFdBe(bytes: []const u8): CanFrame? {
    return parseSocketCanFdInternal(bytes, true);
}

fn encodeSocketCanInternal(frame: CanFrame, out: []u8, big_endian: bool): usize? {
    if (frame.len > CAN_MAX_DLEN || out.len < CAN_FRAME_SIZE) {
        return null;
    }
    if (big_endian) {
        writeU32Be(out, 0, frame.can_id.raw);
    } else {
        writeU32Le(out, 0, frame.can_id.raw);
    }
    out[4] = frame.len;
    out[5] = 0;
    out[6] = 0;
    out[7] = 0;
    let i: usize = 0;
    while (i < 8) {
        out[8 + i] = if (i < (frame.len as usize)) frame.data[i] else 0;
        i += 1;
    }
    return CAN_FRAME_SIZE;
}

/// Encodes a classic CAN 2.0 frame into a 16-byte SocketCAN struct (little-endian `can_id`).
pub fn encodeSocketCan(frame: CanFrame, out: []u8): usize? {
    return encodeSocketCanInternal(frame, out, false);
}

/// Encodes a classic CAN 2.0 frame into a 16-byte SocketCAN struct (big-endian `can_id`).
pub fn encodeSocketCanBe(frame: CanFrame, out: []u8): usize? {
    return encodeSocketCanInternal(frame, out, true);
}

fn encodeSocketCanFdInternal(frame: CanFrame, out: []u8, big_endian: bool): usize? {
    if (frame.len > CANFD_MAX_DLEN || out.len < CANFD_FRAME_SIZE) {
        return null;
    }
    if (big_endian) {
        writeU32Be(out, 0, frame.can_id.raw);
    } else {
        writeU32Le(out, 0, frame.can_id.raw);
    }
    out[4] = frame.len;
    out[5] = frame.flags;
    out[6] = 0;
    out[7] = 0;
    let i: usize = 0;
    while (i < 64) {
        out[8 + i] = if (i < (frame.len as usize)) frame.data[i] else 0;
        i += 1;
    }
    return CANFD_FRAME_SIZE;
}

/// Encodes a CAN FD frame into a 72-byte SocketCAN FD struct (little-endian `can_id`).
pub fn encodeSocketCanFd(frame: CanFrame, out: []u8): usize? {
    return encodeSocketCanFdInternal(frame, out, false);
}

/// Encodes a CAN FD frame into a 72-byte SocketCAN FD struct (big-endian `can_id`).
pub fn encodeSocketCanFdBe(frame: CanFrame, out: []u8): usize? {
    return encodeSocketCanFdInternal(frame, out, true);
}

// ---------------------------------------------------------------------------
// DBC Bitfield Signal Extraction (Intel Little-Endian & Motorola Big-Endian)
// ---------------------------------------------------------------------------

/// Extracts an unsigned 1..64 bit DBC Intel (little-endian) signal starting at LSB `start_bit`.
pub fn extractSignalIntel(payload: []const u8, start_bit: u16, bit_len: u8): u64? {
    if (bit_len == 0 || bit_len > 64) {
        return null;
    }
    const total_bits = payload.len * 8;
    const end_bit = (start_bit as usize) + (bit_len as usize);
    if (end_bit > total_bits) {
        return null;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < (bit_len as usize)) {
        const pos = (start_bit as usize) + i;
        const byte_idx = pos / 8;
        const bit_idx = (pos % 8) as u3;
        const bit = ((payload[byte_idx] >> bit_idx) & 0x01) as u64;
        acc = acc | (bit << (i as u6));
        i += 1;
    }
    return acc;
}

/// Extracts an unsigned 1..64 bit DBC Motorola (big-endian, sawtooth) signal starting at MSB `start_bit`.
pub fn extractSignalMotorola(payload: []const u8, start_bit: u16, bit_len: u8): u64? {
    if (bit_len == 0 || bit_len > 64) {
        return null;
    }
    let byte_idx = (start_bit as usize) / 8;
    let bit_in_byte = ((start_bit as usize) % 8) as u8;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < (bit_len as usize)) {
        if (byte_idx >= payload.len) {
            return null;
        }
        const bit = ((payload[byte_idx] >> (bit_in_byte as u3)) & 0x01) as u64;
        acc = (acc << 1) | bit;
        if (bit_in_byte > 0) {
            bit_in_byte -= 1;
        } else {
            byte_idx += 1;
            bit_in_byte = 7;
        }
        i += 1;
    }
    return acc;
}

/// Two's-complement sign extension of a `bit_len`-bit (1..64) unsigned value into `i64`.
pub fn signExtend(val: u64, bit_len: u8): i64 {
    if (bit_len == 0) {
        return 0;
    }
    if (bit_len >= 64) {
        const hi_bit: u64 = 0x8000000000000000;
        if ((val & hi_bit) == 0) {
            return val as i64;
        }
        if (val == hi_bit) {
            return (-9223372036854775807 as i64) - 1;
        }
        const all_ones: u64 = 0xFFFFFFFFFFFFFFFF;
        const mag: u64 = (all_ones - val) + 1;
        return -(mag as i64);
    }
    const sign_bit: u64 = (1 as u64) << ((bit_len - 1) as u6);
    const mask: u64 = ((1 as u64) << (bit_len as u6)) - 1;
    const masked = val & mask;
    if ((masked & sign_bit) == 0) {
        return masked as i64;
    }
    const mag: u64 = (mask - masked) + 1;
    return -(mag as i64);
}
