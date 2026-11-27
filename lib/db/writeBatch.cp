// Reference-hardened against fjall-rs/fjall (MIT OR Apache-2.0)
// Origin: Pure-CP Stdlib (LSM Atomic WriteBatch & Group Commit Coordinator + 16-Lane SIMD Copy) | Created: 2026-11-23
// Pure CP zero-heap-allocation RocksDB/fjall-style atomic WriteBatch binary codec + LSM Group Commit coordinator:
//   - 12-byte wire header (u64 LE sequence + u32 LE op count) + inline payload + CRC32-IEEE framing
//   - put / delete / singleDelete / merge mutations with nested SavePoint rollback support
//   - Zero-copy forward WriteBatchIter with strict truncation, varint overflow & checksum guards
//   - 16-lane @Vector(16, u8) SIMD payload copy & key equality matching
//   - Leader/Follower GroupCommitCoordinator coalescing concurrent WriteBatches into a single
//     contiguous disk write with monotonic sequence range assignment and per-writer receipts

const std = @import("std");
import { simdWalCopyBytes, simdWalBytesEqual } from "./walRecover.cp";

// ============================================================================
// 1. Wire Constants, CRC32-IEEE & Varint32 Codec
// ============================================================================

/// Fixed WriteBatch wire header size: 8-byte LE `seq` + 4-byte LE `count`.
pub const WRITE_BATCH_HEADER_SIZE: usize = 12;

/// Fixed WriteBatch framed trailer size: 4-byte LE CRC32-IEEE checksum.
pub const WRITE_BATCH_TRAILER_SIZE: usize = 4;

/// Maximum inline buffer capacity (header + payload) for a single zero-alloc `WriteBatch`.
pub const WRITE_BATCH_MAX_BYTES: usize = 4096;

/// Maximum nested savepoint depth per `WriteBatch`.
pub const WRITE_BATCH_MAX_SAVEPOINTS: usize = 16;

/// Maximum concurrent writers coalesced in a single Group Commit flush.
pub const GROUP_COMMIT_MAX_WRITERS: usize = 16;

/// Maximum coalesced buffer capacity (header + merged payloads + 4B trailer) in `GroupCommitCoordinator`.
pub const GROUP_COMMIT_MAX_BYTES: usize = 8192;

/// Computes standard IEEE 802.3 CRC32 checksum (polynomial 0xEDB88320).
pub fn writeBatchCrc32(data: []const u8): u32 {
    let crc: u32 = 0xFFFFFFFF;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        crc ^= (data[i] as u32);
        let bit: usize = 0;
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

fn writeU32Le(buf: *[]u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn readU32Le(buf: []const u8, off: usize): u32 {
    return (buf[off] as u32) |
        ((buf[off + 1] as u32) << 8) |
        ((buf[off + 2] as u32) << 16) |
        ((buf[off + 3] as u32) << 24);
}

fn writeU64Le(buf: *[]u8, off: usize, v: u64) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
    buf[off + 4] = ((v >> 32) & 0xFF) as u8;
    buf[off + 5] = ((v >> 40) & 0xFF) as u8;
    buf[off + 6] = ((v >> 48) & 0xFF) as u8;
    buf[off + 7] = ((v >> 56) & 0xFF) as u8;
}

fn readU64Le(buf: []const u8, off: usize): u64 {
    return (buf[off] as u64) |
        ((buf[off + 1] as u64) << 8) |
        ((buf[off + 2] as u64) << 16) |
        ((buf[off + 3] as u64) << 24) |
        ((buf[off + 4] as u64) << 32) |
        ((buf[off + 5] as u64) << 40) |
        ((buf[off + 6] as u64) << 48) |
        ((buf[off + 7] as u64) << 56);
}

fn varint32Len(val_in: u32): usize {
    let v: u32 = val_in;
    let n: usize = 1;
    while (v >= 0x80) {
        v = v >> 7;
        n += 1;
    }
    return n;
}

fn encodeVarint32(buf: *[]u8, off: usize, val_in: u32): usize {
    let v: u32 = val_in;
    let pos: usize = off;
    while (v >= 0x80) {
        buf[pos] = ((v & 0x7F) as u8) | 0x80;
        v = v >> 7;
        pos += 1;
    }
    buf[pos] = (v & 0x7F) as u8;
    return (pos + 1) - off;
}

struct DecodedVarint32 {
    value: u32 = 0;
    bytes_read: usize = 0;
}

fn decodeVarint32(data: []const u8, off: usize): DecodedVarint32? {
    let result: u32 = 0;
    let shift: u5 = 0;
    let i: usize = 0;
    while (i < 5) : (i += 1) {
        if (off + i >= data.len) return null;
        const b: u8 = data[off + i];
        if (i == 4 && (b & 0xF0) != 0) {
            // Reject 5th byte overflow (> 32 bits) or non-terminating varint.
            return null;
        }
        result |= ((b & 0x7F) as u32) << shift;
        if ((b & 0x80) == 0) {
            return DecodedVarint32{
                value: result,
                bytes_read: i + 1,
            };
        }
        shift += 7;
    }
    return null;
}

// ============================================================================
// 2. Public Enums, Record View & Integrity Verifiers
// ============================================================================

/// Mutation opcode stored in a `WriteBatch` wire stream.
pub enum WriteBatchOp : u8 {
    put = 1,
    delete = 2,
    single_delete = 3,
    merge = 4,
}

/// Operation and validation status codes for `WriteBatch`.
pub enum WriteBatchStatus {
    ok,
    buffer_full,
    empty_key,
    savepoint_stack_full,
    no_savepoint,
    truncated,
    invalid_tag,
    malformed_varint,
    count_mismatch,
    checksum_mismatch,
    invalid_batch,
}

/// Zero-copy decoded mutation view yielded by `WriteBatchIter`.
pub struct WriteBatchRecord {
    op: WriteBatchOp = WriteBatchOp.put;
    seq: u64 = 0;
    key: []const u8 = "";
    value: []const u8 = "";
}

fn decodeTag(tag: u8): WriteBatchOp? {
    if (tag == 1) return WriteBatchOp.put;
    if (tag == 2) return WriteBatchOp.delete;
    if (tag == 3) return WriteBatchOp.single_delete;
    if (tag == 4) return WriteBatchOp.merge;
    return null;
}

/// Validates a raw unframed WriteBatch `[12B header][payload]`:
/// checks header length, decodes every opcode/varint/key/value without out-of-bounds access,
/// and verifies that the exact decoded record count matches `header.count`.
pub fn verifyWriteBatchRaw(raw: []const u8): WriteBatchStatus {
    if (raw.len < WRITE_BATCH_HEADER_SIZE) {
        return WriteBatchStatus.truncated;
    }
    const expected_count: u32 = readU32Le(raw, 8);
    let pos: usize = WRITE_BATCH_HEADER_SIZE;
    let seen: u32 = 0;

    while (pos < raw.len) {
        const op = decodeTag(raw[pos]) orelse {
            return WriteBatchStatus.invalid_tag;
        };
        pos += 1;

        const k_dec = decodeVarint32(raw, pos) orelse {
            return WriteBatchStatus.malformed_varint;
        };
        pos += k_dec.bytes_read;
        const k_len: usize = k_dec.value as usize;
        if (k_len == 0) {
            return WriteBatchStatus.empty_key;
        }
        if (k_len > raw.len - pos) {
            return WriteBatchStatus.truncated;
        }
        pos += k_len;

        if (op == WriteBatchOp.put || op == WriteBatchOp.merge) {
            const v_dec = decodeVarint32(raw, pos) orelse {
                return WriteBatchStatus.malformed_varint;
            };
            pos += v_dec.bytes_read;
            const v_len: usize = v_dec.value as usize;
            if (v_len > raw.len - pos) {
                return WriteBatchStatus.truncated;
            }
            pos += v_len;
        }

        if (seen == std.math.maxInt(u32)) {
            return WriteBatchStatus.count_mismatch;
        }
        seen += 1;
    }

    if (seen != expected_count) {
        return WriteBatchStatus.count_mismatch;
    }
    return WriteBatchStatus.ok;
}

/// Validates a framed WriteBatch `[12B header][payload][4B LE CRC32-IEEE]`:
/// verifies CRC32 checksum before validating full payload structure.
pub fn verifyWriteBatchFramed(framed: []const u8): WriteBatchStatus {
    if (framed.len < WRITE_BATCH_HEADER_SIZE + WRITE_BATCH_TRAILER_SIZE) {
        return WriteBatchStatus.truncated;
    }
    const raw_len: usize = framed.len - WRITE_BATCH_TRAILER_SIZE;
    const stored_crc: u32 = readU32Le(framed, raw_len);
    const calc_crc: u32 = writeBatchCrc32(framed[0..raw_len]);
    if (stored_crc != calc_crc) {
        return WriteBatchStatus.checksum_mismatch;
    }
    return verifyWriteBatchRaw(framed[0..raw_len]);
}

// ============================================================================
// 3. Zero-Copy Forward Iterator
// ============================================================================

/// Zero-copy forward iterator decoding records directly from a WriteBatch byte slice.
pub struct WriteBatchIter {
    raw: []const u8 = "",
    pos: usize = 0,
    base_seq: u64 = 0,
    expected_count: u32 = 0,
    yielded: u32 = 0,
    err: WriteBatchStatus = WriteBatchStatus.ok;

    /// Creates an iterator over a raw unframed `[12B header][payload]` slice.
    static fn fromRaw(raw: []const u8): WriteBatchIter {
        if (raw.len < WRITE_BATCH_HEADER_SIZE) {
            return WriteBatchIter{
                raw: raw,
                pos: raw.len,
                base_seq: 0,
                expected_count: 0,
                yielded: 0,
                err: WriteBatchStatus.truncated,
            };
        }
        return WriteBatchIter{
            raw: raw,
            pos: WRITE_BATCH_HEADER_SIZE,
            base_seq: readU64Le(raw, 0),
            expected_count: readU32Le(raw, 8),
            yielded: 0,
            err: WriteBatchStatus.ok,
        };
    }

    /// Creates an iterator over a framed `[12B header][payload][4B LE CRC32]` slice.
    /// If checksum validation fails, `status()` returns `checksum_mismatch` and `next()` yields `null`.
    static fn fromFramed(framed: []const u8): WriteBatchIter {
        if (framed.len < WRITE_BATCH_HEADER_SIZE + WRITE_BATCH_TRAILER_SIZE) {
            return WriteBatchIter{
                raw: framed,
                pos: framed.len,
                err: WriteBatchStatus.truncated,
            };
        }
        const raw_len: usize = framed.len - WRITE_BATCH_TRAILER_SIZE;
        const stored_crc: u32 = readU32Le(framed, raw_len);
        const calc_crc: u32 = writeBatchCrc32(framed[0..raw_len]);
        if (stored_crc != calc_crc) {
            return WriteBatchIter{
                raw: framed[0..raw_len],
                pos: raw_len,
                err: WriteBatchStatus.checksum_mismatch,
            };
        }
        return WriteBatchIter.fromRaw(framed[0..raw_len]);
    }

    /// Returns current iterator status (`ok` if no decode error encountered).
    fn status(): WriteBatchStatus {
        return _.err;
    }

    /// Decodes and returns the next zero-copy `WriteBatchRecord`, or `null` at EOF / error.
    fn next(*_): WriteBatchRecord? {
        if (_.err != WriteBatchStatus.ok) return null;
        if (_.pos >= _.raw.len) {
            if (_.yielded != _.expected_count) {
                _.err = WriteBatchStatus.count_mismatch;
            }
            return null;
        }

        const op = decodeTag(_.raw[_.pos]) orelse {
            _.err = WriteBatchStatus.invalid_tag;
            return null;
        };
        _.pos += 1;

        const k_dec = decodeVarint32(_.raw, _.pos) orelse {
            _.err = WriteBatchStatus.malformed_varint;
            return null;
        };
        _.pos += k_dec.bytes_read;
        const k_len: usize = k_dec.value as usize;
        if (k_len == 0) {
            _.err = WriteBatchStatus.empty_key;
            return null;
        }
        if (k_len > _.raw.len - _.pos) {
            _.err = WriteBatchStatus.truncated;
            return null;
        }
        const key_slice: []const u8 = _.raw[_.pos .. _.pos + k_len];
        _.pos += k_len;

        let val_slice: []const u8 = "";
        if (op == WriteBatchOp.put || op == WriteBatchOp.merge) {
            const v_dec = decodeVarint32(_.raw, _.pos) orelse {
                _.err = WriteBatchStatus.malformed_varint;
                return null;
            };
            _.pos += v_dec.bytes_read;
            const v_len: usize = v_dec.value as usize;
            if (v_len > _.raw.len - _.pos) {
                _.err = WriteBatchStatus.truncated;
                return null;
            }
            val_slice = _.raw[_.pos .. _.pos + v_len];
            _.pos += v_len;
        }

        const rec_seq: u64 = _.base_seq + (_.yielded as u64);
        _.yielded += 1;
        return WriteBatchRecord{
            op: op,
            seq: rec_seq,
            key: key_slice,
            value: val_slice,
        };
    }
}

// ============================================================================
// 4. Atomic Zero-Alloc WriteBatch with Nested SavePoints
// ============================================================================

struct WriteBatchSavePoint {
    byte_len: usize = WRITE_BATCH_HEADER_SIZE;
    count: u32 = 0;
}

/// Zero-heap-allocation RocksDB-style atomic `WriteBatch` with 12-byte header,
/// inline payload storage, nested savepoint rollback, and CRC32 framing.
pub struct WriteBatch {
    buf: [4096]u8 = undefined,
    len: usize = WRITE_BATCH_HEADER_SIZE,
    seq: u64 = 0,
    count: u32 = 0,
    savepoints: [16]WriteBatchSavePoint = undefined,
    savepoint_depth: usize = 0;

    /// Initializes an empty `WriteBatch` with a zeroed 12-byte wire header.
    static fn init(): WriteBatch {
        let b = WriteBatch{};
        b.syncHeader();
        return b;
    }

    /// Writes `seq` and `count` into the 12-byte little-endian header.
    fn syncHeader(*_) {
        let hdr: []u8 = _.buf[0..WRITE_BATCH_HEADER_SIZE];
        writeU64Le(hdr, 0, _.seq);
        writeU32Le(hdr, 8, _.count);
    }

    /// Sets the starting sequence number stored in the 12-byte wire header.
    fn setSequence(*_, seq: u64) {
        _.seq = seq;
        _.syncHeader();
    }

    /// Returns the starting sequence number stored in the header.
    fn sequence(): u64 {
        return _.seq;
    }

    /// Returns the number of mutation records currently encoded in the batch.
    fn recordCount(): u32 {
        return _.count;
    }

    /// Returns `true` if the batch contains zero mutations.
    fn isEmpty(): bool {
        return _.count == 0;
    }

    /// Returns total raw encoded byte length (12-byte header + payload).
    fn byteSize(): usize {
        return _.len;
    }

    /// Returns remaining free payload bytes in the inline buffer.
    fn remainingBytes(): usize {
        return WRITE_BATCH_MAX_BYTES - _.len;
    }

    /// Returns the current nested savepoint stack depth.
    fn savePointDepth(): usize {
        return _.savepoint_depth;
    }

    /// Returns the raw wire slice `[12B header][payload]`.
    fn rawBytes(): []const u8 {
        return _.buf[0.._.len];
    }

    /// Returns only the payload slice excluding the 12-byte header.
    fn payloadBytes(): []const u8 {
        return _.buf[WRITE_BATCH_HEADER_SIZE.._.len];
    }

    /// Computes CRC32-IEEE checksum over the current `[12B header][payload]`.
    fn checksum(): u32 {
        return writeBatchCrc32(_.buf[0.._.len]);
    }

    /// Clears all records, resets sequence to 0, and clears all savepoints.
    fn clear(*_) {
        _.len = WRITE_BATCH_HEADER_SIZE;
        _.seq = 0;
        _.count = 0;
        _.savepoint_depth = 0;
        _.syncHeader();
    }

    /// Internal helper encoding a key-only or key-value mutation atomically.
    fn appendEntry(*_, op: WriteBatchOp, key: []const u8, val: []const u8, has_val: bool): WriteBatchStatus {
        if (key.len == 0) return WriteBatchStatus.empty_key;
        if (key.len > std.math.maxInt(u32) || val.len > std.math.maxInt(u32)) {
            return WriteBatchStatus.buffer_full;
        }
        if (_.count == std.math.maxInt(u32)) {
            return WriteBatchStatus.buffer_full;
        }

        const k_len32: u32 = key.len as u32;
        const v_len32: u32 = val.len as u32;
        let needed: usize = (1 as usize) + varint32Len(k_len32) + key.len;
        if (has_val) {
            needed += varint32Len(v_len32) + val.len;
        }
        if (needed > WRITE_BATCH_MAX_BYTES - _.len) {
            return WriteBatchStatus.buffer_full;
        }

        let pos: usize = _.len;
        let dst: []u8 = _.buf[0..WRITE_BATCH_MAX_BYTES];
        dst[pos] = op as u8;
        pos += 1;
        pos += encodeVarint32(dst, pos, k_len32);

        * = simdWalCopyBytes(dst[pos .. pos + key.len], key);
        pos += key.len;

        if (has_val) {
            pos += encodeVarint32(dst, pos, v_len32);
            * = simdWalCopyBytes(dst[pos .. pos + val.len], val);
            pos += val.len;
        }

        _.len = pos;
        _.count += 1;
        _.syncHeader();
        return WriteBatchStatus.ok;
    }

    /// Appends a `put(key, value)` mutation.
    fn put(*_, key: []const u8, val: []const u8): WriteBatchStatus {
        return _.appendEntry(WriteBatchOp.put, key, val, true);
    }

    /// Appends a `delete(key)` tombstone mutation.
    fn delete(*_, key: []const u8): WriteBatchStatus {
        return _.appendEntry(WriteBatchOp.delete, key, "", false);
    }

    /// Appends a `singleDelete(key)` tombstone mutation (for non-overwritten keys).
    fn singleDelete(*_, key: []const u8): WriteBatchStatus {
        return _.appendEntry(WriteBatchOp.single_delete, key, "", false);
    }

    /// Appends a `merge(key, operand)` read-modify-write operand mutation.
    fn merge(*_, key: []const u8, operand: []const u8): WriteBatchStatus {
        return _.appendEntry(WriteBatchOp.merge, key, operand, true);
    }

    /// Pushes a savepoint snapshot onto the nested rollback stack.
    fn setSavePoint(*_): WriteBatchStatus {
        if (_.savepoint_depth >= WRITE_BATCH_MAX_SAVEPOINTS) {
            return WriteBatchStatus.savepoint_stack_full;
        }
        _.savepoints[_.savepoint_depth] = WriteBatchSavePoint{
            byte_len: _.len,
            count: _.count,
        };
        _.savepoint_depth += 1;
        return WriteBatchStatus.ok;
    }

    /// Discards the most recent savepoint while keeping all mutations applied since it was set.
    fn popSavePoint(*_): WriteBatchStatus {
        if (_.savepoint_depth == 0) return WriteBatchStatus.no_savepoint;
        _.savepoint_depth -= 1;
        return WriteBatchStatus.ok;
    }

    /// Atomically rolls back all mutations appended after the most recent savepoint and pops it.
    fn rollbackToSavePoint(*_): WriteBatchStatus {
        if (_.savepoint_depth == 0) return WriteBatchStatus.no_savepoint;
        _.savepoint_depth -= 1;
        const sp = _.savepoints[_.savepoint_depth];
        _.len = sp.byte_len;
        _.count = sp.count;
        _.syncHeader();
        return WriteBatchStatus.ok;
    }

    /// Serializes `[12B header][payload][4B LE CRC32]` into `out_buf` via 16-lane SIMD copy.
    /// Returns total framed bytes written, or `null` if `out_buf` is too small.
    fn encodeFramed(out_buf: *[]u8): usize? {
        const total: usize = _.len + WRITE_BATCH_TRAILER_SIZE;
        if (out_buf.len < total) return null;
        * = simdWalCopyBytes(out_buf[0.._.len], _.buf[0.._.len]);
        const crc: u32 = writeBatchCrc32(_.buf[0.._.len]);
        writeU32Le(out_buf, _.len, crc);
        return total;
    }

    /// Scans the batch for a mutation targeting `target_key` using 16-lane SIMD key comparison.
    fn containsKey(target_key: []const u8): bool {
        let it = _.iter();
        while (rec := it.next()) {
            if (simdWalBytesEqual(rec.key, target_key)) {
                return true;
            }
        }
        return false;
    }

    /// Validates internal structure and record count of this batch.
    fn verify(): WriteBatchStatus {
        return verifyWriteBatchRaw(_.buf[0.._.len]);
    }

    /// Creates a zero-copy forward iterator over this batch's records.
    fn iter(): WriteBatchIter {
        return WriteBatchIter.fromRaw(_.buf[0.._.len]);
    }
}

// ============================================================================
// 5. Multi-Writer LSM Group Commit Coordinator
// ============================================================================

/// Configuration for `GroupCommitCoordinator`.
pub struct GroupCommitConfig {
    max_writers: usize = 8;
    max_batch_bytes: usize = 4096;
    initial_seq: u64 = 1;

    /// Returns production defaults (`max_writers=8`, `max_batch_bytes=4096`, `initial_seq=1`).
    static fn defaults(): GroupCommitConfig {
        return GroupCommitConfig{
            max_writers: 8,
            max_batch_bytes: 4096,
            initial_seq: 1,
        };
    }

    /// Validates capacity and sequence invariants.
    fn isValid(): bool {
        if (_.max_writers == 0 || _.max_writers > GROUP_COMMIT_MAX_WRITERS) return false;
        const min_bytes: usize = WRITE_BATCH_HEADER_SIZE + WRITE_BATCH_TRAILER_SIZE;
        if (_.max_batch_bytes < min_bytes || _.max_batch_bytes > GROUP_COMMIT_MAX_BYTES) return false;
        if (_.initial_seq == 0) return false;
        return true;
    }
}

/// Per-writer admission/commit status in a Group Commit cycle.
pub enum GroupCommitWriterStatus {
    committed,
    empty_noop,
    rejected_invalid,
    rejected_oversize,
    rejected_queue_full,
}

/// Commit receipt assigned to each writer participating in a Group Commit flush.
pub struct GroupCommitReceipt {
    writer_id: u32 = 0;
    status: GroupCommitWriterStatus = GroupCommitWriterStatus.committed;
    is_leader: bool = false;
    start_seq: u64 = 0;
    end_seq: u64 = 0;
    op_count: u32 = 0;
    payload_offset: usize = 0;
    payload_bytes: usize = 0;
}

/// Summary returned by `GroupCommitCoordinator.flush`.
pub struct GroupCommitFlushResult {
    leader_writer_id: u32 = 0;
    writer_count: usize = 0;
    total_ops: u32 = 0;
    start_seq: u64 = 0;
    end_seq: u64 = 0;
    merged_raw_bytes: usize = 0;
    merged_framed_bytes: usize = 0;
    crc32: u32 = 0;
}

/// Diagnostic telemetry snapshot for `GroupCommitCoordinator`.
pub struct GroupCommitStats {
    flushes: u64 = 0;
    total_writers_committed: u64 = 0;
    total_ops_committed: u64 = 0;
    total_bytes_flushed: u64 = 0;
    pending_writers: usize = 0;
    pending_bytes: usize = 0;
    next_seq: u64 = 1;
    rejected_count: u64 = 0;
}

/// Zero-heap-allocation Leader/Follower Group Commit coalescing coordinator.
/// Merges multiple concurrent writers' `WriteBatch` payloads into a single sequential
/// framed `WriteBatch` block, assigning contiguous monotonic sequence ranges.
pub struct GroupCommitCoordinator {
    cfg: GroupCommitConfig = GroupCommitConfig{},
    merged_buf: [8192]u8 = undefined,
    merged_len: usize = WRITE_BATCH_HEADER_SIZE,
    pending_receipts: [16]GroupCommitReceipt = undefined,
    pending_count: usize = 0,
    pending_ops: u32 = 0,
    next_seq: u64 = 1,
    flushes: u64 = 0,
    total_writers_committed: u64 = 0,
    total_ops_committed: u64 = 0,
    total_bytes_flushed: u64 = 0,
    rejected_count: u64 = 0;

    /// Initializes a Group Commit coordinator from `cfg`. Returns `null` if `cfg` is invalid.
    static fn init(cfg: GroupCommitConfig): GroupCommitCoordinator? {
        if (!cfg.isValid()) return null;
        return GroupCommitCoordinator{
            cfg: cfg,
            next_seq: cfg.initial_seq,
        };
    }

    /// Returns number of writers currently queued in the active commit group.
    fn pendingWriters(): usize {
        return _.pending_count;
    }

    /// Returns current coalesced raw byte length (including 12-byte header).
    fn pendingBytes(): usize {
        return _.merged_len;
    }

    /// Returns the next sequence number to be assigned on the upcoming flush.
    fn nextSeq(): u64 {
        return _.next_seq;
    }

    /// Returns a telemetry snapshot.
    fn stats(): GroupCommitStats {
        return GroupCommitStats{
            flushes: _.flushes,
            total_writers_committed: _.total_writers_committed,
            total_ops_committed: _.total_ops_committed,
            total_bytes_flushed: _.total_bytes_flushed,
            pending_writers: _.pending_count,
            pending_bytes: _.merged_len,
            next_seq: _.next_seq,
            rejected_count: _.rejected_count,
        };
    }

    /// Drops any pending un-flushed writers while preserving `next_seq` and historical counters.
    fn clearPending(*_) {
        _.merged_len = WRITE_BATCH_HEADER_SIZE;
        _.pending_count = 0;
        _.pending_ops = 0;
    }

    /// Enqueues a raw unframed `[12B header][payload]` WriteBatch from `writer_id`.
    /// Validates structural integrity before admission; first admitted writer becomes Leader.
    fn enqueueRaw(*_, writer_id: u32, raw_batch: []const u8): GroupCommitWriterStatus {
        const st = verifyWriteBatchRaw(raw_batch);
        if (st != WriteBatchStatus.ok) {
            _.rejected_count += 1;
            return GroupCommitWriterStatus.rejected_invalid;
        }

        const op_count: u32 = readU32Le(raw_batch, 8);
        if (op_count == 0) {
            return GroupCommitWriterStatus.empty_noop;
        }

        if (_.pending_count >= _.cfg.max_writers) {
            _.rejected_count += 1;
            return GroupCommitWriterStatus.rejected_queue_full;
        }

        const payload_len: usize = raw_batch.len - WRITE_BATCH_HEADER_SIZE;
        const max_raw: usize = _.cfg.max_batch_bytes - WRITE_BATCH_TRAILER_SIZE;
        if (payload_len > max_raw - _.merged_len) {
            _.rejected_count += 1;
            return GroupCommitWriterStatus.rejected_oversize;
        }
        if (op_count > std.math.maxInt(u32) - _.pending_ops) {
            _.rejected_count += 1;
            return GroupCommitWriterStatus.rejected_oversize;
        }

        const offset: usize = _.merged_len;
        * = simdWalCopyBytes(_.merged_buf[offset .. offset + payload_len], raw_batch[WRITE_BATCH_HEADER_SIZE .. WRITE_BATCH_HEADER_SIZE + payload_len]);
        _.merged_len += payload_len;

        const is_leader: bool = (_.pending_count == 0);
        _.pending_receipts[_.pending_count] = GroupCommitReceipt{
            writer_id: writer_id,
            status: GroupCommitWriterStatus.committed,
            is_leader: is_leader,
            start_seq: 0,
            end_seq: 0,
            op_count: op_count,
            payload_offset: offset,
            payload_bytes: payload_len,
        };
        _.pending_count += 1;
        _.pending_ops += op_count;
        return GroupCommitWriterStatus.committed;
    }

    /// Enqueues an in-memory `WriteBatch` from `writer_id` into the active commit group.
    fn enqueueBatch(*_, writer_id: u32, batch: WriteBatch): GroupCommitWriterStatus {
        return _.enqueueRaw(writer_id, batch.rawBytes());
    }

    /// Flushes all queued writers into a single contiguous framed WriteBatch `[12B hdr][payloads][4B CRC32]`,
    /// assigns contiguous monotonic sequence ranges to every writer in `out_receipts`, and resets the queue.
    ///
    /// Returns `null` if no writers are queued, or if `out_framed` / `out_receipts` is too small.
    fn flush(
        *_,
        out_framed: *[]u8,
        out_receipts: *[]GroupCommitReceipt,
    ): GroupCommitFlushResult? {
        if (_.pending_count == 0) return null;

        const framed_len: usize = _.merged_len + WRITE_BATCH_TRAILER_SIZE;
        if (out_framed.len < framed_len) return null;
        if (out_receipts.len < _.pending_count) return null;

        const group_start_seq: u64 = _.next_seq;
        if ((_.pending_ops as u64) > std.math.maxInt(u64) - group_start_seq) return null;
        const group_end_seq: u64 = group_start_seq + (_.pending_ops as u64);

        // Write coalesced 12-byte header.
        let hdr: []u8 = _.merged_buf[0..WRITE_BATCH_HEADER_SIZE];
        writeU64Le(hdr, 0, group_start_seq);
        writeU32Le(hdr, 8, _.pending_ops);

        const crc: u32 = writeBatchCrc32(_.merged_buf[0.._.merged_len]);

        // Copy coalesced raw bytes + 4-byte CRC32 trailer into caller output buffer via 16-lane SIMD.
        * = simdWalCopyBytes(out_framed[0.._.merged_len], _.merged_buf[0.._.merged_len]);
        writeU32Le(out_framed, _.merged_len, crc);

        // Finalize contiguous monotonic sequence intervals for Leader + Followers.
        let cur_seq: u64 = group_start_seq;
        let w: usize = 0;
        while (w < _.pending_count) : (w += 1) {
            let rec = _.pending_receipts[w];
            rec.start_seq = cur_seq;
            rec.end_seq = cur_seq + (rec.op_count as u64);
            cur_seq = rec.end_seq;
            out_receipts[w] = rec;
        }

        const res = GroupCommitFlushResult{
            leader_writer_id: _.pending_receipts[0].writer_id,
            writer_count: _.pending_count,
            total_ops: _.pending_ops,
            start_seq: group_start_seq,
            end_seq: group_end_seq,
            merged_raw_bytes: _.merged_len,
            merged_framed_bytes: framed_len,
            crc32: crc,
        };

        // Advance coordinator monotonic sequence & telemetry, then clear queue.
        _.next_seq = group_end_seq;
        _.flushes += 1;
        _.total_writers_committed += (_.pending_count as u64);
        _.total_ops_committed += (_.pending_ops as u64);
        _.total_bytes_flushed += (framed_len as u64);
        _.clearPending();

        return res;
    }
}
