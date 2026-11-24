// Reference-hardened against fjall-rs/fjall (MIT OR Apache-2.0)
// Origin: Pure-CP Stdlib (Zero-Alloc CRC32 Torn-Write-Safe WAL Framing, 16-Lane SIMD Copy & Crash Recovery Engine) | Created: 2026-11-22
// Deterministic little-endian binary WAL framing with IEEE 802.3 CRC32 integrity protection,
// atomic no-partial-write appends, monotonic LSN enforcement, 16-lane @Vector(16, u8) SIMD payload copy/compare,
// and torn-tail/corruption truncation recovery.
//
// Binary frame layout (24-byte fixed little-endian header + N-byte payload):
//   [0..4)   magic: u32 LE       (WAL_FRAME_MAGIC = 0x57414C46)
//   [4..12)  lsn: u64 LE         (strictly monotonic Log Sequence Number)
//   [12..13) kind: u8            (1=Put, 2=Delete, 3=Commit, 4=Checkpoint)
//   [13..16) reserved: [3]u8     (0, 0, 0)
//   [16..20) payload_len: u32 LE (byte length of payload)
//   [20..24) crc32: u32 LE       (IEEE 802.3 CRC32 over header[0..20] + payload[0..N])
//   [24..24+N) payload: [N]u8

type WalVec16u8 = @Vector(16, u8);

/// Magic header tag ("WALF" = 0x57414C46) identifying valid WAL recovery frames.
pub const WAL_FRAME_MAGIC: u32 = 0x57414C46;

/// Fixed little-endian binary frame header size in bytes.
pub const WAL_FRAME_HEADER_SIZE: usize = 24;

/// Maximum permitted single WAL frame payload length (64 MiB) preventing malformed-length DoS/overflow.
pub const WAL_FRAME_MAX_PAYLOAD_BYTES: u32 = 64 * 1024 * 1024;

const IEEE_CRC32_POLY: u32 = 0xEDB88320;

fn walLoadVec16(data: []const u8, off: usize): WalVec16u8 {
    const v: WalVec16u8 = [
        data[off], data[off + 1], data[off + 2], data[off + 3],
        data[off + 4], data[off + 5], data[off + 6], data[off + 7],
        data[off + 8], data[off + 9], data[off + 10], data[off + 11],
        data[off + 12], data[off + 13], data[off + 14], data[off + 15],
    ];
    return v;
}

/// 16-lane `@Vector(16, u8)` SIMD byte copy (`min(dst.len, src.len)` bytes) for WAL & WriteBatch hot paths.
pub fn simdWalCopyBytes(dst: []u8, src: []const u8): usize {
    const n = if (dst.len <= src.len) dst.len else src.len;
    let i: usize = 0;
    while (i + 16 <= n) {
        const v = walLoadVec16(src, i);
        dst[i] = v[0];
        dst[i + 1] = v[1];
        dst[i + 2] = v[2];
        dst[i + 3] = v[3];
        dst[i + 4] = v[4];
        dst[i + 5] = v[5];
        dst[i + 6] = v[6];
        dst[i + 7] = v[7];
        dst[i + 8] = v[8];
        dst[i + 9] = v[9];
        dst[i + 10] = v[10];
        dst[i + 11] = v[11];
        dst[i + 12] = v[12];
        dst[i + 13] = v[13];
        dst[i + 14] = v[14];
        dst[i + 15] = v[15];
        i += 16;
    }
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
    return n;
}

/// Scalar reference for `simdWalCopyBytes` (differential verification).
pub fn scalarWalCopyBytes(dst: []u8, src: []const u8): usize {
    const n = if (dst.len <= src.len) dst.len else src.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
    return n;
}

/// 16-lane `@Vector(16, u8)` SIMD byte slice equality check for WAL magics & WriteBatch keys.
pub fn simdWalBytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i + 16 <= a.len) {
        const va = walLoadVec16(a, i);
        const vb = walLoadVec16(b, i);
        if (!@reduce(.And, va == vb)) return false;
        i += 16;
    }
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/// Scalar reference for `simdWalBytesEqual` (differential verification).
pub fn scalarWalBytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
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

fn crc32Update(init_crc: u32, data: []const u8): u32 {
    let crc = init_crc;
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
    return crc;
}

/// Computes standard IEEE 802.3 CRC32 checksum over `data` with zero heap allocation.
pub fn walCrc32Ieee(data: []const u8): u32 {
    return ~crc32Update(0xFFFFFFFF, data);
}

fn computeFrameCrc32(header_prefix: []const u8, payload: []const u8): u32 {
    let crc: u32 = 0xFFFFFFFF;
    crc = crc32Update(crc, header_prefix);
    crc = crc32Update(crc, payload);
    return ~crc;
}

/// Logical operation type encoded in each WAL frame.
pub enum WalRecordKind {
    Put,
    Delete,
    Commit,
    Checkpoint,
}

fn kindToByte(kind: WalRecordKind): u8 {
    return switch (kind) {
        WalRecordKind.Put => 1,
        WalRecordKind.Delete => 2,
        WalRecordKind.Commit => 3,
        WalRecordKind.Checkpoint => 4,
    };
}

fn byteToKind(b: u8): WalRecordKind? {
    if (b == 1) return WalRecordKind.Put;
    if (b == 2) return WalRecordKind.Delete;
    if (b == 3) return WalRecordKind.Commit;
    if (b == 4) return WalRecordKind.Checkpoint;
    return null;
}

/// Zero-copy view of a validated WAL frame backed by the scanned log buffer.
pub struct WalFrameView {
    lsn: u64 = 0;
    kind: WalRecordKind = WalRecordKind.Put;
    offset: usize = 0;
    frame_len: usize = 0;
    crc32: u32 = 0;
    payload: []const u8 = "";
}

/// Termination classification produced by `WalRecoveryScanner.scanAndRecover`.
pub enum WalRecoveryStatus {
    Clean,
    TornTailTruncated,
    CorruptedTruncated,
    NonMonotonicLsnTruncated,
}

/// Crash recovery audit report returned by `WalRecoveryScanner.scanAndRecover`.
pub struct WalRecoveryReport {
    status: WalRecoveryStatus = WalRecoveryStatus.Clean;
    recovered_count: usize = 0;
    frames_stored: usize = 0;
    frames_truncated: bool = false;
    valid_end_offset: usize = 0;
    torn_tail_bytes: usize = 0;
    has_corruption: bool = false;
    corrupted_offset: usize = 0;
    has_valid_lsn: bool = false;
    last_valid_lsn: u64 = 0;
    has_checkpoint: bool = false;
    last_checkpoint_lsn: u64 = 0;

    /// Returns `true` when the entire log buffer was consumed without torn tail or corruption.
    fn isClean(): bool {
        return _.status == WalRecoveryStatus.Clean;
    }
}

/// Zero-allocation append-only WAL frame writer over a caller-provided byte buffer.
/// Guarantees atomic frame appends (never leaves half-written frames on buffer exhaustion)
/// and enforces strictly monotonic LSNs.
pub struct WalFrameWriter {
    buf: []u8;
    offset: usize = 0;
    last_lsn: u64 = 0;
    has_written: bool = false;
    frame_count: u32 = 0;

    /// Initializes a `WalFrameWriter` backed by caller-owned `buf`.
    static fn init(buf: []u8): WalFrameWriter {
        return WalFrameWriter{
            buf: buf,
            offset: 0,
            last_lsn: 0,
            has_written: false,
            frame_count: 0,
        };
    }

    /// Appends a CRC32-protected WAL frame atomically.
    /// Returns the total bytes written (`WAL_FRAME_HEADER_SIZE + payload.len`), or `null` if:
    ///   - `lsn` is not strictly greater than the previously written LSN;
    ///   - `payload.len > 0xFFFFFFFF`; or
    ///   - remaining buffer space is insufficient (buffer & offset remain untouched).
    fn appendFrame(*_, lsn: u64, kind: WalRecordKind, payload: []const u8): usize? {
        if (_.has_written && lsn <= _.last_lsn) {
            return null;
        }
        if (payload.len > (WAL_FRAME_MAX_PAYLOAD_BYTES as usize)) {
            return null;
        }
        const total_len = WAL_FRAME_HEADER_SIZE + payload.len;
        if (total_len > _.buf.len - _.offset) {
            return null;
        }

        const base = _.offset;
        writeU32LE(_.buf, base, WAL_FRAME_MAGIC);
        writeU64LE(_.buf, base + 4, lsn);
        _.buf[base + 12] = kindToByte(kind);
        _.buf[base + 13] = 0;
        _.buf[base + 14] = 0;
        _.buf[base + 15] = 0;
        writeU32LE(_.buf, base + 16, payload.len as u32);

        const crc = computeFrameCrc32(_.buf[base .. base + 20], payload);
        writeU32LE(_.buf, base + 20, crc);

        * = simdWalCopyBytes(_.buf[base + WAL_FRAME_HEADER_SIZE .. base + total_len], payload);

        _.offset += total_len;
        _.last_lsn = lsn;
        _.has_written = true;
        _.frame_count += 1;
        return total_len;
    }

    /// Returns a read-only slice of all bytes written so far.
    fn writtenBytes(): []const u8 {
        return _.buf[0.._.offset];
    }

    /// Returns the current byte write offset.
    fn position(): usize {
        return _.offset;
    }

    /// Returns the remaining free bytes in the backing buffer.
    fn remaining(): usize {
        return _.buf.len - _.offset;
    }

    /// Returns the number of frames successfully appended.
    fn frameCount(): u32 {
        return _.frame_count;
    }

    /// Returns the last appended LSN (`0` if no frames written).
    fn lastLsn(): u64 {
        return _.last_lsn;
    }

    /// Resets write offset and LSN tracking for reusing the backing buffer.
    fn reset(*_) {
        _.offset = 0;
        _.last_lsn = 0;
        _.has_written = false;
        _.frame_count = 0;
    }
}

/// Zero-allocation WAL crash recovery scanner.
/// Validates frame magic, reserved header bytes, payload bounds, IEEE CRC32 checksum,
/// and strictly monotonic LSNs, safely truncating at torn tails or corrupted offsets.
pub struct WalRecoveryScanner {
    /// Scans `buf` from offset 0, collecting validated frame views into `out_frames`.
    /// On encountering a torn tail, corrupted frame, or non-monotonic LSN, stops cleanly
    /// and sets `valid_end_offset` to the byte boundary right after the last valid frame.
    static fn scanAndRecover(buf: []const u8, out_frames: []WalFrameView): WalRecoveryReport {
        let report = WalRecoveryReport{};
        let pos: usize = 0;

        while (pos < buf.len) {
            const avail = buf.len - pos;
            if (avail < WAL_FRAME_HEADER_SIZE) {
                report.status = WalRecoveryStatus.TornTailTruncated;
                report.valid_end_offset = pos;
                report.torn_tail_bytes = avail;
                break;
            }

            const magic = readU32LE(buf, pos);
            if (magic != WAL_FRAME_MAGIC) {
                report.status = WalRecoveryStatus.CorruptedTruncated;
                report.has_corruption = true;
                report.corrupted_offset = pos;
                report.valid_end_offset = pos;
                break;
            }

            const lsn = readU64LE(buf, pos + 4);
            const kind_opt = byteToKind(buf[pos + 12]);
            const res_ok = buf[pos + 13] == 0 && buf[pos + 14] == 0 && buf[pos + 15] == 0;
            if (kind_opt == null || !res_ok) {
                report.status = WalRecoveryStatus.CorruptedTruncated;
                report.has_corruption = true;
                report.corrupted_offset = pos;
                report.valid_end_offset = pos;
                break;
            }
            const kind = kind_opt ?? WalRecordKind.Put;

            const payload_len_u32 = readU32LE(buf, pos + 16);
            if (payload_len_u32 > WAL_FRAME_MAX_PAYLOAD_BYTES) {
                report.status = WalRecoveryStatus.CorruptedTruncated;
                report.has_corruption = true;
                report.corrupted_offset = pos;
                report.valid_end_offset = pos;
                break;
            }
            const payload_len = payload_len_u32 as usize;
            const expected_crc = readU32LE(buf, pos + 20);
            const rem_payload = avail - WAL_FRAME_HEADER_SIZE;

            if (payload_len > rem_payload) {
                report.status = WalRecoveryStatus.TornTailTruncated;
                report.valid_end_offset = pos;
                report.torn_tail_bytes = avail;
                break;
            }

            const payload_start = pos + WAL_FRAME_HEADER_SIZE;
            const payload = buf[payload_start .. payload_start + payload_len];
            const actual_crc = computeFrameCrc32(buf[pos .. pos + 20], payload);
            if (actual_crc != expected_crc) {
                report.status = WalRecoveryStatus.CorruptedTruncated;
                report.has_corruption = true;
                report.corrupted_offset = pos;
                report.valid_end_offset = pos;
                break;
            }

            if (report.has_valid_lsn && lsn <= report.last_valid_lsn) {
                report.status = WalRecoveryStatus.NonMonotonicLsnTruncated;
                report.has_corruption = true;
                report.corrupted_offset = pos;
                report.valid_end_offset = pos;
                break;
            }

            const frame_len = WAL_FRAME_HEADER_SIZE + payload_len;
            report.recovered_count += 1;
            report.has_valid_lsn = true;
            report.last_valid_lsn = lsn;
            if (kind == WalRecordKind.Checkpoint) {
                report.has_checkpoint = true;
                report.last_checkpoint_lsn = lsn;
            }

            if (report.frames_stored < out_frames.len) {
                out_frames[report.frames_stored] = WalFrameView{
                    lsn: lsn,
                    kind: kind,
                    offset: pos,
                    frame_len: frame_len,
                    crc32: expected_crc,
                    payload: payload,
                };
                report.frames_stored += 1;
            } else {
                report.frames_truncated = true;
            }

            pos += frame_len;
            report.valid_end_offset = pos;
        }

        return report;
    }
}
