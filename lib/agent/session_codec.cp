// Origin: Pure-CP Stdlib (Zero-Alloc Versioned & CRC32-Verified Agent Session Snapshot Codec) | Created: 2026-11-22
// Deterministic little-endian binary codec for persisting and resuming long-running AI Agent
// sessions across process restarts, BEAM Actor handoffs, and crash boundaries with zero heap allocation.
//
// Binary Layout (64-byte fixed LE header + 4 length-prefixed UTF-8 strings + 4-byte IEEE CRC32 footer):
//   [0..4)   magic: u32 LE                 (AGENT_SNAPSHOT_MAGIC = 0x41475353 "AGSS")
//   [4..6)   version: u16 LE               (AGENT_SNAPSHOT_VERSION = 1)
//   [6..7)   flags: u8                     (bit 0 = is_paused; upper bits must be 0)
//   [7..8)   reserved: u8                  (must be 0)
//   [8..12)  turn_count: u32 LE
//   [12..16) tool_call_count: u32 LE
//   [16..24) input_tokens: u64 LE
//   [24..32) output_tokens: u64 LE
//   [32..40) cost_micro_usd: u64 LE
//   [40..48) updated_ms: u64 LE            (ZigZag-encoded signed i64)
//   [48..52) session_id_len: u32 LE
//   [52..56) agent_name_len: u32 LE
//   [56..60) goal_summary_len: u32 LE
//   [60..64) last_checkpoint_note_len: u32 LE
//   [64..end-4) UTF-8 string payloads concatenated in order
//   [end-4..end) crc32: u32 LE             (IEEE 802.3 CRC32 over bytes [0..end-4))

/// Binary magic header tag ("AGSS" = 0x41475353) for `AgentSessionSnapshot`.
pub const AGENT_SNAPSHOT_MAGIC: u32 = 0x41475353;

/// Current binary wire format version supported by this codec.
pub const AGENT_SNAPSHOT_VERSION: u16 = 1;

const SNAPSHOT_HEADER_LEN: usize = 64;
const SNAPSHOT_FOOTER_LEN: usize = 4;
const SNAPSHOT_MIN_LEN: usize = SNAPSHOT_HEADER_LEN + SNAPSHOT_FOOTER_LEN;
const IEEE_CRC32_POLY: u32 = 0xEDB88320;

/// Explicit error variants returned by `decodeSessionSnapshot`.
pub err SnapshotDecodeError {
    TruncatedInput,
    InvalidMagic,
    UnsupportedVersion,
    InvalidFlags,
    ChecksumMismatch,
    TrailingBytes,
}

/// Persistent snapshot state for a long-running AI Agent session.
/// On decode, all string fields are zero-copy slices backed by the input buffer.
pub struct AgentSessionSnapshot {
    session_id: string = "";
    agent_name: string = "";
    turn_count: u32 = 0;
    tool_call_count: u32 = 0;
    input_tokens: u64 = 0;
    output_tokens: u64 = 0;
    cost_micro_usd: u64 = 0;
    updated_ms: i64 = 0;
    is_paused: bool = false;
    goal_summary: string = "";
    last_checkpoint_note: string = "";
}

// ============================================================================
// Internal Little-Endian, ZigZag & IEEE 802.3 CRC32 Helpers
// ============================================================================

fn writeU16LE(buf: []u8, off: usize, v: u16) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn writeU32LE(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn writeU64LE(buf: []u8, off: usize, v: u64) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
    buf[off + 4] = ((v >> 32) & 0xFF) as u8;
    buf[off + 5] = ((v >> 40) & 0xFF) as u8;
    buf[off + 6] = ((v >> 48) & 0xFF) as u8;
    buf[off + 7] = ((v >> 56) & 0xFF) as u8;
}

fn readU16LE(data: []const u8, off: usize): u16 {
    return (data[off] as u16) | ((data[off + 1] as u16) << 8);
}

fn readU32LE(data: []const u8, off: usize): u32 {
    return (data[off] as u32) |
        ((data[off + 1] as u32) << 8) |
        ((data[off + 2] as u32) << 16) |
        ((data[off + 3] as u32) << 24);
}

fn readU64LE(data: []const u8, off: usize): u64 {
    return (data[off] as u64) |
        ((data[off + 1] as u64) << 8) |
        ((data[off + 2] as u64) << 16) |
        ((data[off + 3] as u64) << 24) |
        ((data[off + 4] as u64) << 32) |
        ((data[off + 5] as u64) << 40) |
        ((data[off + 6] as u64) << 48) |
        ((data[off + 7] as u64) << 56);
}

fn encodeZigZag(v: i64): u64 {
    if (v >= 0) {
        return (v as u64) << 1;
    }
    const mag = (-(v + 1)) as u64;
    return (mag << 1) | 1;
}

fn decodeZigZag(u: u64): i64 {
    if ((u & 1) == 0) {
        return (u >> 1) as i64;
    }
    return -((u >> 1) as i64) - 1;
}

fn crc32Ieee(data: []const u8): u32 {
    let crc: u32 = 0xFFFFFFFF;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        crc = crc ^ (data[i] as u32);
        let bit: u8 = 0;
        while (bit < 8) : (bit += 1) {
            if ((crc & 1) != 0) {
                crc = (crc >> 1) ^ IEEE_CRC32_POLY;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return ~crc;
}

fn copyStr(dst: []u8, off: usize, s: string) {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        dst[off + i] = s[i];
    }
}

// ============================================================================
// Public Snapshot Codec Primitives
// ============================================================================

/// Returns the exact encoded byte length required to serialize `snap`.
pub fn encodedSnapshotSize(snap: AgentSessionSnapshot): usize {
    return SNAPSHOT_MIN_LEN +
        snap.session_id.len +
        snap.agent_name.len +
        snap.goal_summary.len +
        snap.last_checkpoint_note.len;
}

/// Encodes `snap` into `out_buf` with magic, version, deterministic little-endian fields,
/// length-prefixed strings, and a trailing IEEE 802.3 CRC32 checksum.
/// Returns the exact bytes written, or `null` if `out_buf` is too small or any string exceeds `u32` max.
pub fn encodeSessionSnapshot(snap: AgentSessionSnapshot, out_buf: []u8): usize? {
    if (snap.session_id.len > 0xFFFFFFFF ||
        snap.agent_name.len > 0xFFFFFFFF ||
        snap.goal_summary.len > 0xFFFFFFFF ||
        snap.last_checkpoint_note.len > 0xFFFFFFFF)
    {
        return null;
    }

    const total_len = encodedSnapshotSize(snap);
    if (out_buf.len < total_len) {
        return null;
    }

    writeU32LE(out_buf, 0, AGENT_SNAPSHOT_MAGIC);
    writeU16LE(out_buf, 4, AGENT_SNAPSHOT_VERSION);
    out_buf[6] = if (snap.is_paused) 1 else 0;
    out_buf[7] = 0;
    writeU32LE(out_buf, 8, snap.turn_count);
    writeU32LE(out_buf, 12, snap.tool_call_count);
    writeU64LE(out_buf, 16, snap.input_tokens);
    writeU64LE(out_buf, 24, snap.output_tokens);
    writeU64LE(out_buf, 32, snap.cost_micro_usd);
    writeU64LE(out_buf, 40, encodeZigZag(snap.updated_ms));
    writeU32LE(out_buf, 48, snap.session_id.len as u32);
    writeU32LE(out_buf, 52, snap.agent_name.len as u32);
    writeU32LE(out_buf, 56, snap.goal_summary.len as u32);
    writeU32LE(out_buf, 60, snap.last_checkpoint_note.len as u32);

    let pos: usize = SNAPSHOT_HEADER_LEN;
    copyStr(out_buf, pos, snap.session_id);
    pos += snap.session_id.len;
    copyStr(out_buf, pos, snap.agent_name);
    pos += snap.agent_name.len;
    copyStr(out_buf, pos, snap.goal_summary);
    pos += snap.goal_summary.len;
    copyStr(out_buf, pos, snap.last_checkpoint_note);
    pos += snap.last_checkpoint_note.len;

    const crc = crc32Ieee(out_buf[0..pos]);
    writeU32LE(out_buf, pos, crc);
    return total_len;
}

/// Decodes and verifies a binary `AgentSessionSnapshot` from `data` with zero heap allocation.
/// Validates magic, version, flag bits, trailing IEEE CRC32 integrity, exact payload length,
/// and absence of trailing garbage bytes.
pub fn decodeSessionSnapshot(data: []const u8): AgentSessionSnapshot ! SnapshotDecodeError {
    if (data.len < SNAPSHOT_MIN_LEN) {
        return SnapshotDecodeError.TruncatedInput;
    }

    const magic = readU32LE(data, 0);
    if (magic != AGENT_SNAPSHOT_MAGIC) {
        return SnapshotDecodeError.InvalidMagic;
    }

    const version = readU16LE(data, 4);
    if (version != AGENT_SNAPSHOT_VERSION) {
        return SnapshotDecodeError.UnsupportedVersion;
    }

    const flags = data[6];
    const reserved = data[7];
    if ((flags & 0xFE) != 0 || reserved != 0) {
        return SnapshotDecodeError.InvalidFlags;
    }

    const body_end = data.len - SNAPSHOT_FOOTER_LEN;
    const expected_crc = readU32LE(data, body_end);
    const actual_crc = crc32Ieee(data[0..body_end]);
    if (actual_crc != expected_crc) {
        return SnapshotDecodeError.ChecksumMismatch;
    }

    const sid_len = readU32LE(data, 48) as usize;
    const name_len = readU32LE(data, 52) as usize;
    const goal_len = readU32LE(data, 56) as usize;
    const note_len = readU32LE(data, 60) as usize;

    const payload_avail = body_end - SNAPSHOT_HEADER_LEN;
    if (sid_len > payload_avail) return SnapshotDecodeError.TruncatedInput;
    const rem1 = payload_avail - sid_len;
    if (name_len > rem1) return SnapshotDecodeError.TruncatedInput;
    const rem2 = rem1 - name_len;
    if (goal_len > rem2) return SnapshotDecodeError.TruncatedInput;
    const rem3 = rem2 - goal_len;
    if (note_len > rem3) return SnapshotDecodeError.TruncatedInput;
    if (note_len < rem3) return SnapshotDecodeError.TrailingBytes;

    let pos: usize = SNAPSHOT_HEADER_LEN;
    const session_id = data[pos .. pos + sid_len] as string;
    pos += sid_len;
    const agent_name = data[pos .. pos + name_len] as string;
    pos += name_len;
    const goal_summary = data[pos .. pos + goal_len] as string;
    pos += goal_len;
    const last_checkpoint_note = data[pos .. pos + note_len] as string;

    return AgentSessionSnapshot{
        session_id: session_id,
        agent_name: agent_name,
        turn_count: readU32LE(data, 8),
        tool_call_count: readU32LE(data, 12),
        input_tokens: readU64LE(data, 16),
        output_tokens: readU64LE(data, 24),
        cost_micro_usd: readU64LE(data, 32),
        updated_ms: decodeZigZag(readU64LE(data, 40)),
        is_paused: (flags & 1) != 0,
        goal_summary: goal_summary,
        last_checkpoint_note: last_checkpoint_note,
    };
}
