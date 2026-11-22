// Origin: Pure-CP Stdlib (Unbounded Zero-Heap LSM-Tree: CRC32 WAL, MVCC MemTable, Block SSTable+Bloom & K-Way Compaction) | Created: 2026-11-23
// 100% pure CP, zero dynamic heap allocation, caller-slice-driven embedded KV storage core:
//   - Write-Ahead Log (`LsmWalCodec`): Put/Delete framing with magic, length prefix, IEEE CRC32,
//     and crash-recovery tail truncation scanner (`recoverLog`, `replayIntoMemTable`)
//   - Ordered MVCC MemTable (`LsmMemTable`): caller-owned external slices supporting arbitrary entry counts,
//     `(key ASC, seq DESC)` ordering, snapshot-isolated `getAt(key, snapshot_seq)` and `scanRange`
//   - Block-based SSTable (`SstBuilder` & `SstReader`): incremental prefix-compressed Data Blocks with
//     per-block IEEE CRC32, binary-searchable Index Block, double-hashed Bloom Filter (<2% FPR),
//     and 64-byte CRC32-verified Footer
//   - K-Way Leveled Compaction (`compactSstables`): unbounded external-cursor merge deduplicating
//     obsolete versions under `min_active_snapshot_seq` and purging bottommost tombstones

const std = @import("std");

// ============================================================================
// 1. Constants, Error Set & Low-Level Binary / Hashing / CRC32 Primitives
// ============================================================================

/// Little-endian magic tag `"LWAL"` (`0x4C41574C`) for WAL frames.
pub const LSM_WAL_MAGIC: u32 = 0x4C41574C;

/// Current WAL frame wire version.
pub const LSM_WAL_VERSION: u16 = 1;

/// Fixed WAL frame header byte size (magic:4 + ver:2 + op:1 + pad:1 + seq:8 + klen:4 + vlen:4).
pub const LSM_WAL_HEADER_BYTES: usize = 24;

/// Trailing IEEE CRC32 size for WAL frames and SSTable blocks.
pub const LSM_CRC32_BYTES: usize = 4;

/// 64-bit magic tag for SSTable footers (`"CPLSMSST"`).
pub const LSM_SST_MAGIC: u64 = 0x5453534D_534C5043;

/// Current SSTable binary format version.
pub const LSM_SST_VERSION: u32 = 1;

/// Fixed SSTable footer byte size.
pub const LSM_SST_FOOTER_BYTES: usize = 64;

/// Maximum per-key scratch slot size used by compaction cursors (bytes).
pub const LSM_CURSOR_KEY_STRIDE: usize = 256;

const FNV1A_64_OFFSET: u64 = 0xcbf29ce484222325;
const FNV1A_64_PRIME: u64 = 0x100000001b3;

/// Error set returned by WAL, MemTable, SSTable, and Compaction operations.
pub err LsmKvError {
    InvalidConfig,
    BufferTooSmall,
    CapacityFull,
    CorruptWal,
    CorruptSst,
    ChecksumMismatch,
    InvalidKeyOrder,
    NotFinalized,
    InvalidRange,
}

fn crc32Ieee(bytes: []const u8): u32 {
    let crc: u32 = 0xFFFF_FFFF;
    let i: usize = 0;
    while (i < bytes.len) : (i += 1) {
        crc ^= @as(u32, bytes[i]);
        let b: usize = 0;
        while (b < 8) : (b += 1) {
            if ((crc & 1) != 0) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    return ~crc;
}

fn hash64A(bytes: []const u8): u64 {
    let h: u64 = FNV1A_64_OFFSET;
    let i: usize = 0;
    while (i < bytes.len) : (i += 1) {
        h ^= @as(u64, bytes[i]);
        h *%= FNV1A_64_PRIME;
    }
    h ^= h >> 30;
    h *%= 0xBF58476D1CE4E5B9;
    h ^= h >> 27;
    h *%= 0x94D049BB133111EB;
    return h ^ (h >> 31);
}

fn hash64B(bytes: []const u8): u64 {
    let h: u64 = @as(u64, 0x9E3779B97F4A7C15) ^ @as(u64, @intCast(bytes.len));
    let i: usize = 0;
    while (i < bytes.len) : (i += 1) {
        h ^= @as(u64, bytes[i]) *% 0x100000001b3;
        h = (h << 13) | (h >> 51);
        h *%= 0xC6A4A7935BD1E995;
    }
    h ^= h >> 33;
    h *%= 0xFF51AFD7ED558CCD;
    h ^= h >> 33;
    h *%= 0xC4CEB9FE1A85EC53;
    return (h ^ (h >> 33)) | 1;
}

fn writeU16Le(dst: *[]u8, off: usize, v: u16) {
    dst[off + 0] = @as(u8, @intCast(v & 0xFF));
    dst[off + 1] = @as(u8, @intCast((v >> 8) & 0xFF));
}

fn writeU32Le(dst: *[]u8, off: usize, v: u32) {
    dst[off + 0] = @as(u8, @intCast(v & 0xFF));
    dst[off + 1] = @as(u8, @intCast((v >> 8) & 0xFF));
    dst[off + 2] = @as(u8, @intCast((v >> 16) & 0xFF));
    dst[off + 3] = @as(u8, @intCast((v >> 24) & 0xFF));
}

fn writeU64Le(dst: *[]u8, off: usize, v: u64) {
    writeU32Le(dst, off, @as(u32, @intCast(v & 0xFFFF_FFFF)));
    writeU32Le(dst, off + 4, @as(u32, @intCast((v >> 32) & 0xFFFF_FFFF)));
}

fn readU16Le(src: []const u8, off: usize): u16 {
    return @as(u16, src[off + 0]) | (@as(u16, src[off + 1]) << 8);
}

fn readU32Le(src: []const u8, off: usize): u32 {
    return @as(u32, src[off + 0]) |
        (@as(u32, src[off + 1]) << 8) |
        (@as(u32, src[off + 2]) << 16) |
        (@as(u32, src[off + 3]) << 24);
}

fn readU64Le(src: []const u8, off: usize): u64 {
    const lo: u64 = @as(u64, readU32Le(src, off));
    const hi: u64 = @as(u64, readU32Le(src, off + 4));
    return lo | (hi << 32);
}

/// Lexicographical comparison of two byte slices: `< 0`, `0`, or `> 0`.
pub fn compareBytes(a: []const u8, b: []const u8): i32 {
    const n: usize = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
    }
    if (a.len < b.len) return -1;
    if (a.len > b.len) return 1;
    return 0;
}

/// Compares internal MVCC keys `(key ASC, seq DESC)`.
/// Returns `< 0` if `(ka, sa)` precedes `(kb, sb)`, `0` if identical, `> 0` otherwise.
pub fn compareInternalKey(ka: []const u8, sa: u64, kb: []const u8, sb: u64): i32 {
    const cmp: i32 = compareBytes(ka, kb);
    if (cmp != 0) return cmp;
    if (sa > sb) return -1;
    if (sa < sb) return 1;
    return 0;
}

fn commonPrefixLen(a: []const u8, b: []const u8): usize {
    const n: usize = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < n && a[i] == b[i]) : (i += 1) {}
    return i;
}

fn copySlice(dst: *[]u8, dst_off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[dst_off + i] = src[i];
    }
}

// ============================================================================
// 2. Write-Ahead Log (`LsmWalCodec`) — Framing & Crash-Recovery Scanner
// ============================================================================

/// WAL operation kind.
pub enum LsmWalOp {
    put,
    delete,
}

/// Zero-copy view of a decoded WAL record.
pub struct LsmWalRecordView {
    op: LsmWalOp = LsmWalOp.put;
    seq: u64 = 0;
    key: []const u8 = "";
    value: []const u8 = "";
    frame_bytes: usize = 0;
}

/// Recovery telemetry produced by `LsmWalCodec.recoverLog` and `replayIntoMemTable`.
pub struct LsmWalRecoveryStats {
    recovered_records: usize = 0;
    valid_bytes: usize = 0;
    corrupt_tail_bytes: usize = 0;
    max_seq: u64 = 0;
    truncated_tail: bool = false;
}

/// Zero-heap WAL frame encoder, verifier, and crash-recovery tail scanner.
pub struct LsmWalCodec {
    /// Returns the exact encoded frame byte length for a record with `(key_len, val_len)`.
    static fn encodedFrameSize(key_len: usize, val_len: usize): usize {
        return LSM_WAL_HEADER_BYTES + key_len + val_len + LSM_CRC32_BYTES;
    }

    /// Encodes a `Put(key, val, seq)` frame with magic, length prefix, and IEEE CRC32 into `out`.
    static fn encodePut(key: []const u8, val: []const u8, seq: u64, out: *[]u8): usize ! LsmKvError {
        const need: usize = LsmWalCodec.encodedFrameSize(key.len, val.len);
        if (out.len < need) return LsmKvError.BufferTooSmall;
        writeU32Le(out, 0, LSM_WAL_MAGIC);
        writeU16Le(out, 4, LSM_WAL_VERSION);
        out[6] = 1; // Put
        out[7] = 0;
        writeU64Le(out, 8, seq);
        writeU32Le(out, 16, @as(u32, @intCast(key.len)));
        writeU32Le(out, 20, @as(u32, @intCast(val.len)));
        copySlice(out, LSM_WAL_HEADER_BYTES, key);
        copySlice(out, LSM_WAL_HEADER_BYTES + key.len, val);
        const payload_end: usize = LSM_WAL_HEADER_BYTES + key.len + val.len;
        const crc: u32 = crc32Ieee(out[0..payload_end]);
        writeU32Le(out, payload_end, crc);
        return need;
    }

    /// Encodes a `Delete(key, seq)` tombstone frame with magic, length prefix, and IEEE CRC32 into `out`.
    static fn encodeDelete(key: []const u8, seq: u64, out: *[]u8): usize ! LsmKvError {
        const need: usize = LsmWalCodec.encodedFrameSize(key.len, 0);
        if (out.len < need) return LsmKvError.BufferTooSmall;
        writeU32Le(out, 0, LSM_WAL_MAGIC);
        writeU16Le(out, 4, LSM_WAL_VERSION);
        out[6] = 2; // Delete
        out[7] = 0;
        writeU64Le(out, 8, seq);
        writeU32Le(out, 16, @as(u32, @intCast(key.len)));
        writeU32Le(out, 20, 0);
        copySlice(out, LSM_WAL_HEADER_BYTES, key);
        const payload_end: usize = LSM_WAL_HEADER_BYTES + key.len;
        const crc: u32 = crc32Ieee(out[0..payload_end]);
        writeU32Le(out, payload_end, crc);
        return need;
    }

    /// Decodes and CRC32-verifies a single WAL frame starting at `log[offset]`.
    static fn decodeFrameAt(log: []const u8, offset: usize): LsmWalRecordView ! LsmKvError {
        if (offset + LSM_WAL_HEADER_BYTES + LSM_CRC32_BYTES > log.len) {
            return LsmKvError.CorruptWal;
        }
        const magic: u32 = readU32Le(log, offset);
        const ver: u16 = readU16Le(log, offset + 4);
        if (magic != LSM_WAL_MAGIC || ver != LSM_WAL_VERSION) {
            return LsmKvError.CorruptWal;
        }
        const op_byte: u8 = log[offset + 6];
        if (op_byte != 1 && op_byte != 2) {
            return LsmKvError.CorruptWal;
        }
        const seq: u64 = readU64Le(log, offset + 8);
        const klen: usize = @as(usize, readU32Le(log, offset + 16));
        const vlen: usize = @as(usize, readU32Le(log, offset + 20));
        const frame_bytes: usize = LSM_WAL_HEADER_BYTES + klen + vlen + LSM_CRC32_BYTES;
        if (offset + frame_bytes > log.len) {
            return LsmKvError.CorruptWal;
        }

        const payload_end: usize = offset + LSM_WAL_HEADER_BYTES + klen + vlen;
        const stored_crc: u32 = readU32Le(log, payload_end);
        const computed_crc: u32 = crc32Ieee(log[offset..payload_end]);
        if (stored_crc != computed_crc) {
            return LsmKvError.ChecksumMismatch;
        }

        const key_start: usize = offset + LSM_WAL_HEADER_BYTES;
        const val_start: usize = key_start + klen;
        return LsmWalRecordView{
            op: if (op_byte == 1) LsmWalOp.put else LsmWalOp.delete,
            seq: seq,
            key: log[key_start..val_start],
            value: log[val_start..val_start + vlen],
            frame_bytes: frame_bytes,
        };
    }

    /// Scans `log` sequentially, populating valid records into `out_records` and safely truncating
    /// any torn or corrupted tail bytes after the last valid frame.
    static fn recoverLog(log: []const u8, out_records: *[]LsmWalRecordView): LsmWalRecoveryStats {
        let stats: LsmWalRecoveryStats = LsmWalRecoveryStats{};
        let off: usize = 0;
        while (off < log.len) {
            const rec: LsmWalRecordView = LsmWalCodec.decodeFrameAt(log, off) catch {
                stats.truncated_tail = true;
                stats.corrupt_tail_bytes = log.len - off;
                break;
            };
            if (stats.recovered_records < out_records.len) {
                out_records[stats.recovered_records] = rec;
            }
            stats.recovered_records += 1;
            if (rec.seq > stats.max_seq) stats.max_seq = rec.seq;
            off += rec.frame_bytes;
            stats.valid_bytes = off;
        }
        return stats;
    }

    /// Replays all valid WAL records directly into `mem`, safely stopping at any torn/corrupt tail.
    static fn replayIntoMemTable(log: []const u8, mem: *LsmMemTable): LsmWalRecoveryStats ! LsmKvError {
        let stats: LsmWalRecoveryStats = LsmWalRecoveryStats{};
        let off: usize = 0;
        while (off < log.len) {
            const rec: LsmWalRecordView = LsmWalCodec.decodeFrameAt(log, off) catch {
                stats.truncated_tail = true;
                stats.corrupt_tail_bytes = log.len - off;
                break;
            };
            if (rec.op == LsmWalOp.put) {
                try mem.put(rec.key, rec.value, rec.seq);
            } else {
                try mem.delete(rec.key, rec.seq);
            }
            stats.recovered_records += 1;
            if (rec.seq > stats.max_seq) stats.max_seq = rec.seq;
            off += rec.frame_bytes;
            stats.valid_bytes = off;
        }
        return stats;
    }
}

// ============================================================================
// 3. Ordered MVCC MemTable (`LsmMemTable`) — External-Slice Zero-Heap Table
// ============================================================================

/// Entry metadata stored in caller's `entries: []LsmMemEntryMeta` slice.
pub struct LsmMemEntryMeta {
    key_off: u32 = 0;
    key_len: u32 = 0;
    val_off: u32 = 0;
    val_len: u32 = 0;
    seq: u64 = 0;
    is_tombstone: bool = false;
}

/// Caller-owned external storage slices backing `LsmMemTable`.
pub struct LsmMemBuffers {
    entries: []LsmMemEntryMeta = [];
    order: []u32 = [];
    arena: []u8 = [];
}

/// Point-lookup outcome for `LsmMemTable.getAt` and `SstReader.getAt`.
pub enum LsmLookupStatus {
    not_found,
    found,
    deleted,
}

/// Result returned by `LsmMemTable.getAt`.
pub struct LsmMemGetResult {
    status: LsmLookupStatus = LsmLookupStatus.not_found;
    value: []const u8 = "";
    seq: u64 = 0;
}

/// Single visible entry yielded by `LsmMemTable.scanRange`.
pub struct LsmMemScanItem {
    key: []const u8 = "";
    value: []const u8 = "";
    seq: u64 = 0;
}

/// Telemetry for `LsmMemTable`.
pub struct LsmMemStats {
    entry_count: usize = 0;
    arena_used: usize = 0;
    arena_cap: usize = 0;
    max_seq: u64 = 0;
    tombstone_count: usize = 0;
}

/// Unbounded external-slice MVCC MemTable ordered by `(key ASC, seq DESC)`.
pub struct LsmMemTable {
    bufs: LsmMemBuffers = LsmMemBuffers{},
    entry_count: usize = 0;
    arena_used: usize = 0;
    max_seq: u64 = 0;
    tombstone_count: usize = 0;

    /// Initializes a `LsmMemTable` over caller-provided external slices.
    static fn init(bufs: LsmMemBuffers): LsmMemTable ! LsmKvError {
        if (bufs.entries.len == 0 || bufs.order.len < bufs.entries.len || bufs.arena.len == 0) {
            return LsmKvError.BufferTooSmall;
        }
        return LsmMemTable{
            bufs: bufs,
            entry_count: 0,
            arena_used: 0,
            max_seq: 0,
            tombstone_count: 0,
        };
    }

    /// Resets the MemTable to empty state while retaining external buffer bindings.
    fn reset(*_) {
        _.entry_count = 0;
        _.arena_used = 0;
        _.max_seq = 0;
        _.tombstone_count = 0;
    }

    /// Returns telemetry for this MemTable.
    fn stats(): LsmMemStats {
        return LsmMemStats{
            entry_count: entry_count,
            arena_used: arena_used,
            arena_cap: bufs.arena.len,
            max_seq: max_seq,
            tombstone_count: tombstone_count,
        };
    }

    fn entryKey(entry_idx: usize): []const u8 {
        const m: LsmMemEntryMeta = bufs.entries[entry_idx];
        const s: usize = @as(usize, m.key_off);
        return bufs.arena[s..s + @as(usize, m.key_len)];
    }

    fn entryVal(entry_idx: usize): []const u8 {
        const m: LsmMemEntryMeta = bufs.entries[entry_idx];
        const s: usize = @as(usize, m.val_off);
        return bufs.arena[s..s + @as(usize, m.val_len)];
    }

    /// Binary search in `order[0..entry_count]` for the insertion position of `(key, seq)`.
    fn lowerBoundInternal(key: []const u8, seq: u64): usize {
        let lo: usize = 0;
        let hi: usize = entry_count;
        while (lo < hi) {
            const mid: usize = lo + (hi - lo) / 2;
            const eidx: usize = @as(usize, bufs.order[mid]);
            const cmp: i32 = compareInternalKey(entryKey(eidx), bufs.entries[eidx].seq, key, seq);
            if (cmp < 0) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return lo;
    }

    /// Binary search for the first ordered slot with `entry.key >= key`.
    fn lowerBoundUserKey(key: []const u8): usize {
        let lo: usize = 0;
        let hi: usize = entry_count;
        while (lo < hi) {
            const mid: usize = lo + (hi - lo) / 2;
            const eidx: usize = @as(usize, bufs.order[mid]);
            if (compareBytes(entryKey(eidx), key) < 0) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return lo;
    }

    fn insertInternal(*_, key: []const u8, val: []const u8, seq: u64, is_tombstone: bool): void ! LsmKvError {
        if (_.entry_count >= _.bufs.entries.len) return LsmKvError.CapacityFull;
        if (_.arena_used + key.len + val.len > _.bufs.arena.len) return LsmKvError.CapacityFull;

        const key_off: u32 = @as(u32, @intCast(_.arena_used));
        copySlice(*_.bufs.arena, _.arena_used, key);
        _.arena_used += key.len;

        const val_off: u32 = @as(u32, @intCast(_.arena_used));
        if (!is_tombstone && val.len > 0) {
            copySlice(*_.bufs.arena, _.arena_used, val);
            _.arena_used += val.len;
        }

        const new_idx: u32 = @as(u32, @intCast(_.entry_count));
        _.bufs.entries[_.entry_count] = LsmMemEntryMeta{
            key_off: key_off,
            key_len: @as(u32, @intCast(key.len)),
            val_off: val_off,
            val_len: if (is_tombstone) 0 else @as(u32, @intCast(val.len)),
            seq: seq,
            is_tombstone: is_tombstone,
        };

        const insert_pos: usize = _.lowerBoundInternal(key, seq);
        let shift: usize = _.entry_count;
        while (shift > insert_pos) : (shift -= 1) {
            _.bufs.order[shift] = _.bufs.order[shift - 1];
        }
        _.bufs.order[insert_pos] = new_idx;
        _.entry_count += 1;
        if (seq > _.max_seq) _.max_seq = seq;
        if (is_tombstone) _.tombstone_count += 1;
    }

    /// Inserts a `Put(key, val, seq)` version into the MVCC MemTable.
    fn put(*_, key: []const u8, val: []const u8, seq: u64): void ! LsmKvError {
        try _.insertInternal(key, val, seq, false);
    }

    /// Inserts a `Delete(key, seq)` Tombstone version into the MVCC MemTable.
    fn delete(*_, key: []const u8, seq: u64): void ! LsmKvError {
        try _.insertInternal(key, "", seq, true);
    }

    /// Snapshot-isolated point lookup returning the newest version of `key` with `seq <= snapshot_seq`.
    fn getAt(key: []const u8, snapshot_seq: u64): LsmMemGetResult {
        let pos: usize = lowerBoundUserKey(key);
        while (pos < entry_count) : (pos += 1) {
            const eidx: usize = @as(usize, bufs.order[pos]);
            const ek: []const u8 = entryKey(eidx);
            if (compareBytes(ek, key) != 0) break;
            const meta: LsmMemEntryMeta = bufs.entries[eidx];
            if (meta.seq <= snapshot_seq) {
                if (meta.is_tombstone) {
                    return LsmMemGetResult{
                        status: LsmLookupStatus.deleted,
                        value: "",
                        seq: meta.seq,
                    };
                }
                return LsmMemGetResult{
                    status: LsmLookupStatus.found,
                    value: entryVal(eidx),
                    seq: meta.seq,
                };
            }
        }
        return LsmMemGetResult{ status: LsmLookupStatus.not_found };
    }

    /// Snapshot-isolated range scan over `[start_key, end_key)` (`end_key.len == 0` means unbounded upper).
    /// Returns the newest visible non-tombstone version for each distinct key in lexicographical order.
    fn scanRange(
        start_key: []const u8,
        end_key: []const u8,
        snapshot_seq: u64,
        out_items: *[]LsmMemScanItem,
    ): usize {
        if (end_key.len > 0 && compareBytes(start_key, end_key) >= 0) return 0;

        let out_count: usize = 0;
        let pos: usize = if (start_key.len == 0) 0 else lowerBoundUserKey(start_key);
        while (pos < entry_count && out_count < out_items.len) {
            const first_idx: usize = @as(usize, bufs.order[pos]);
            const cur_key: []const u8 = entryKey(first_idx);
            if (end_key.len > 0 && compareBytes(cur_key, end_key) >= 0) break;

            // Advance through all versions of cur_key; first one with seq <= snapshot_seq wins
            let resolved: bool = false;
            while (pos < entry_count) : (pos += 1) {
                const eidx: usize = @as(usize, bufs.order[pos]);
                if (compareBytes(entryKey(eidx), cur_key) != 0) break;
                if (!resolved) {
                    const meta: LsmMemEntryMeta = bufs.entries[eidx];
                    if (meta.seq <= snapshot_seq) {
                        resolved = true;
                        if (!meta.is_tombstone) {
                            out_items[out_count] = LsmMemScanItem{
                                key: cur_key,
                                value: entryVal(eidx),
                                seq: meta.seq,
                            };
                            out_count += 1;
                        }
                    }
                }
            }
        }
        return out_count;
    }

    /// Flushes all ordered MVCC entries in this MemTable into a block-based SSTable binary image.
    fn flushToSst(
        cfg: SstBuilderConfig,
        out_sst: *[]u8,
        index_work: *[]u8,
        bloom_work: *[]u8,
    ): SstBuildStats ! LsmKvError {
        let builder: SstBuilder = try SstBuilder.init(cfg, out_sst, index_work, bloom_work);
        let i: usize = 0;
        while (i < entry_count) : (i += 1) {
            const eidx: usize = @as(usize, bufs.order[i]);
            const meta: LsmMemEntryMeta = bufs.entries[eidx];
            try builder.addEntry(entryKey(eidx), entryVal(eidx), meta.seq, meta.is_tombstone);
        }
        return try builder.finish();
    }
}

// ============================================================================
// 4. Block-Based SSTable (`SstBuilder`, `SstReader`, `SstCursor`) + Bloom Filter
// ============================================================================

/// Configuration for `SstBuilder`.
pub struct SstBuilderConfig {
    block_target_bytes: usize = 4096;
    bloom_bits_per_key: usize = 10;
    bloom_num_hashes: u32 = 7;
    expected_keys: usize = 256;

    /// Validates block size and Bloom filter settings.
    fn isValid(): bool {
        if (block_target_bytes < 64) return false;
        if (bloom_bits_per_key < 4 || bloom_bits_per_key > 32) return false;
        if (bloom_num_hashes == 0 || bloom_num_hashes > 16) return false;
        return true;
    }
}

/// Build telemetry returned by `SstBuilder.finish`.
pub struct SstBuildStats {
    total_bytes: usize = 0;
    entry_count: usize = 0;
    block_count: usize = 0;
    index_bytes: usize = 0;
    bloom_bytes: usize = 0;
    max_seq: u64 = 0;
}

/// Decoded 64-byte SSTable footer metadata.
pub struct SstFooterInfo {
    index_off: u64 = 0;
    index_len: u32 = 0;
    bloom_off: u64 = 0;
    bloom_len: u32 = 0;
    entry_count: u64 = 0;
    block_count: u32 = 0;
    version: u32 = 0;
    max_seq: u64 = 0;
}

/// Point-lookup result returned by `SstReader.getAt`.
pub struct SstLookupResult {
    status: LsmLookupStatus = LsmLookupStatus.not_found;
    value: []const u8 = "";
    seq: u64 = 0;
    bloom_rejected: bool = false;
}

/// Single entry yielded by `SstCursor.next`.
pub struct SstEntryView {
    key: []const u8 = "";
    value: []const u8 = "";
    seq: u64 = 0;
    is_tombstone: bool = false;
}

/// Returns the required Bloom filter working buffer length (bytes) for `cfg`.
pub fn sstBloomWorkMinBytes(cfg: SstBuilderConfig): usize {
    const keys: usize = if (cfg.expected_keys < 16) 16 else cfg.expected_keys;
    const total_bits: usize = keys * cfg.bloom_bits_per_key;
    const bytes: usize = (total_bits + 7) / 8;
    return if (bytes < 16) 16 else bytes;
}

/// Streaming zero-heap SSTable builder producing prefix-compressed Data Blocks,
/// Index Block, Bloom Filter Block, and 64-byte CRC32 Footer into caller-owned slices.
pub struct SstBuilder {
    cfg: SstBuilderConfig = SstBuilderConfig{},
    out_buf: []u8 = [],
    out_pos: usize = 0;
    index_work: []u8 = [],
    index_pos: usize = 0;
    bloom_work: []u8 = [],
    bloom_bits: usize = 0;
    cur_block_start: usize = 0;
    cur_block_entries: usize = 0;
    entry_count: usize = 0;
    block_count: usize = 0;
    max_seq: u64 = 0;
    prev_key_buf: [LSM_CURSOR_KEY_STRIDE]u8 = [0; LSM_CURSOR_KEY_STRIDE];
    prev_key_len: usize = 0;
    prev_seq: u64 = 0;
    has_prev: bool = false;
    finished: bool = false;

    /// Initializes a new `SstBuilder` over caller-provided `out_buf`, `index_work`, and `bloom_work`.
    static fn init(
        cfg: SstBuilderConfig,
        out_buf: *[]u8,
        index_work: *[]u8,
        bloom_work: *[]u8,
    ): SstBuilder ! LsmKvError {
        if (!cfg.isValid()) return LsmKvError.InvalidConfig;
        const need_bloom: usize = sstBloomWorkMinBytes(cfg);
        if (bloom_work.len < need_bloom || out_buf.len < LSM_SST_FOOTER_BYTES + 32 || index_work.len < 32) {
            return LsmKvError.BufferTooSmall;
        }
        let i: usize = 0;
        while (i < need_bloom) : (i += 1) {
            bloom_work[i] = 0;
        }
        return SstBuilder{
            cfg: cfg,
            out_buf: out_buf.*,
            out_pos: 0,
            index_work: index_work.*,
            index_pos: 0,
            bloom_work: bloom_work.*[0..need_bloom],
            bloom_bits: need_bloom * 8,
            cur_block_start: 0,
            cur_block_entries: 0,
            entry_count: 0,
            block_count: 0,
            max_seq: 0,
            prev_key_len: 0,
            prev_seq: 0,
            has_prev: false,
            finished: false,
        };
    }

    fn bloomInsert(*_, key: []const u8) {
        const h1: u64 = hash64A(key);
        const h2: u64 = hash64B(key);
        const m: u64 = @as(u64, @intCast(_.bloom_bits));
        let k: u32 = 0;
        while (k < _.cfg.bloom_num_hashes) : (k += 1) {
            const bit_idx: usize = @as(usize, @intCast((h1 +% @as(u64, k) *% h2) % m));
            _.bloom_work[bit_idx >> 3] |= @as(u8, 1) << @as(u3, @intCast(bit_idx & 7));
        }
    }

    fn flushCurrentBlock(*_): void ! LsmKvError {
        if (_.cur_block_entries == 0) return;
        // Write 4-byte block-level IEEE CRC32
        if (_.out_pos + LSM_CRC32_BYTES > _.out_buf.len) return LsmKvError.BufferTooSmall;
        const crc: u32 = crc32Ieee(_.out_buf[_.cur_block_start.._.out_pos]);
        writeU32Le(*_.out_buf, _.out_pos, crc);
        _.out_pos += LSM_CRC32_BYTES;
        const block_size: u32 = @as(u32, @intCast(_.out_pos - _.cur_block_start));

        // Append index entry: [last_key_len: u32][last_key][block_offset: u64][block_size: u32]
        const idx_need: usize = _.prev_key_len + 16;
        if (_.index_pos + idx_need > _.index_work.len) return LsmKvError.BufferTooSmall;
        writeU32Le(*_.index_work, _.index_pos, @as(u32, @intCast(_.prev_key_len)));
        _.index_pos += 4;
        copySlice(*_.index_work, _.index_pos, _.prev_key_buf[0.._.prev_key_len]);
        _.index_pos += _.prev_key_len;
        writeU64Le(*_.index_work, _.index_pos, @as(u64, @intCast(_.cur_block_start)));
        _.index_pos += 8;
        writeU32Le(*_.index_work, _.index_pos, block_size);
        _.index_pos += 4;

        _.block_count += 1;
        _.cur_block_start = _.out_pos;
        _.cur_block_entries = 0;
    }

    /// Appends an entry strictly ordered by `(key ASC, seq DESC)` with prefix compression.
    fn addEntry(
        *_,
        key: []const u8,
        val: []const u8,
        seq: u64,
        is_tombstone: bool,
    ): void ! LsmKvError {
        if (_.finished) return LsmKvError.InvalidConfig;
        if (key.len > LSM_CURSOR_KEY_STRIDE) return LsmKvError.BufferTooSmall;

        if (_.has_prev) {
            const cmp: i32 = compareInternalKey(_.prev_key_buf[0.._.prev_key_len], _.prev_seq, key, seq);
            if (cmp >= 0) return LsmKvError.InvalidKeyOrder;
        }

        // Flush current block if target size exceeded AND key differs from previous key
        // (keeping all MVCC versions of the same user key inside a single data block simplifies lookup!)
        const cur_block_len: usize = _.out_pos - _.cur_block_start;
        if (_.cur_block_entries > 0 && cur_block_len >= _.cfg.block_target_bytes) {
            const same_user_key: bool = _.has_prev && compareBytes(_.prev_key_buf[0.._.prev_key_len], key) == 0;
            if (!same_user_key) {
                try _.flushCurrentBlock();
            }
        }

        // Compute prefix compression against previous key inside the SAME data block
        const shared: usize = if (_.cur_block_entries > 0)
            commonPrefixLen(_.prev_key_buf[0.._.prev_key_len], key)
        else
            0;
        const unshared: usize = key.len - shared;
        const actual_val_len: usize = if (is_tombstone) 0 else val.len;

        // Entry header: shared:2 + unshared:2 + val_len:4 + seq:8 + flags:1 = 17 bytes
        const rec_bytes: usize = unshared + actual_val_len + 17;
        if (_.out_pos + rec_bytes + LSM_CRC32_BYTES > _.out_buf.len) {
            return LsmKvError.BufferTooSmall;
        }

        writeU16Le(*_.out_buf, _.out_pos + 0, @as(u16, @intCast(shared)));
        writeU16Le(*_.out_buf, _.out_pos + 2, @as(u16, @intCast(unshared)));
        writeU32Le(*_.out_buf, _.out_pos + 4, @as(u32, @intCast(actual_val_len)));
        writeU64Le(*_.out_buf, _.out_pos + 8, seq);
        _.out_buf[_.out_pos + 16] = if (is_tombstone) 1 else 0;
        _.out_pos += 17;

        copySlice(*_.out_buf, _.out_pos, key[shared..key.len]);
        _.out_pos += unshared;
        if (actual_val_len > 0) {
            copySlice(*_.out_buf, _.out_pos, val);
            _.out_pos += actual_val_len;
        }

        // Update bloom filter & prev key state
        _.bloomInsert(key);
        let k_i: usize = 0;
        while (k_i < key.len) : (k_i += 1) {
            _.prev_key_buf[k_i] = key[k_i];
        }
        _.prev_key_len = key.len;
        _.prev_seq = seq;
        _.has_prev = true;
        _.cur_block_entries += 1;
        _.entry_count += 1;
        if (seq > _.max_seq) _.max_seq = seq;
    }

    /// Finalizes the SSTable by flushing the tail Data Block and writing Index Block,
    /// Bloom Filter Block, and 64-byte CRC32 Footer.
    fn finish(*_): SstBuildStats ! LsmKvError {
        if (_.finished) return LsmKvError.InvalidConfig;
        try _.flushCurrentBlock();

        // 1. Write Index Block: [block_count: u32][index entries...][crc32: u32]
        const index_off: u64 = @as(u64, @intCast(_.out_pos));
        const index_total: usize = _.index_pos + 4 + LSM_CRC32_BYTES;
        if (_.out_pos + index_total > _.out_buf.len) return LsmKvError.BufferTooSmall;
        writeU32Le(*_.out_buf, _.out_pos, @as(u32, @intCast(_.block_count)));
        copySlice(*_.out_buf, _.out_pos + 4, _.index_work[0.._.index_pos]);
        const idx_crc: u32 = crc32Ieee(_.out_buf[_.out_pos.._.out_pos + 4 + _.index_pos]);
        writeU32Le(*_.out_buf, _.out_pos + 4 + _.index_pos, idx_crc);
        _.out_pos += index_total;

        // 2. Write Bloom Filter Block: [num_hashes: u32][num_bits: u32][bitset...][crc32: u32]
        const bloom_off: u64 = @as(u64, @intCast(_.out_pos));
        const bloom_total: usize = _.bloom_work.len + 8 + LSM_CRC32_BYTES;
        if (_.out_pos + bloom_total > _.out_buf.len) return LsmKvError.BufferTooSmall;
        writeU32Le(*_.out_buf, _.out_pos + 0, _.cfg.bloom_num_hashes);
        writeU32Le(*_.out_buf, _.out_pos + 4, @as(u32, @intCast(_.bloom_bits)));
        copySlice(*_.out_buf, _.out_pos + 8, _.bloom_work);
        const bloom_crc: u32 = crc32Ieee(_.out_buf[_.out_pos.._.out_pos + 8 + _.bloom_work.len]);
        writeU32Le(*_.out_buf, _.out_pos + 8 + _.bloom_work.len, bloom_crc);
        _.out_pos += bloom_total;

        // 3. Write 64-byte Footer
        if (_.out_pos + LSM_SST_FOOTER_BYTES > _.out_buf.len) return LsmKvError.BufferTooSmall;
        const f_off: usize = _.out_pos;
        writeU64Le(*_.out_buf, f_off + 0, index_off);
        writeU32Le(*_.out_buf, f_off + 8, @as(u32, @intCast(index_total)));
        writeU64Le(*_.out_buf, f_off + 12, bloom_off);
        writeU32Le(*_.out_buf, f_off + 20, @as(u32, @intCast(bloom_total)));
        writeU64Le(*_.out_buf, f_off + 24, @as(u64, @intCast(_.entry_count)));
        writeU32Le(*_.out_buf, f_off + 32, @as(u32, @intCast(_.block_count)));
        writeU32Le(*_.out_buf, f_off + 36, LSM_SST_VERSION);
        writeU64Le(*_.out_buf, f_off + 40, _.max_seq);
        writeU32Le(*_.out_buf, f_off + 48, 0);
        writeU64Le(*_.out_buf, f_off + 52, LSM_SST_MAGIC);
        const footer_crc: u32 = crc32Ieee(_.out_buf[f_off..f_off + 60]);
        writeU32Le(*_.out_buf, f_off + 60, footer_crc);
        _.out_pos += LSM_SST_FOOTER_BYTES;
        _.finished = true;

        return SstBuildStats{
            total_bytes: _.out_pos,
            entry_count: _.entry_count,
            block_count: _.block_count,
            index_bytes: index_total,
            bloom_bytes: bloom_total,
            max_seq: _.max_seq,
        };
    }
}

struct SstIndexHandle {
    block_offset: usize = 0;
    block_size: usize = 0;
    last_key: []const u8 = "";
}

/// Zero-copy SSTable reader with CRC32 verification, Bloom Filter fast reject, and binary-searched Index Block.
pub struct SstReader {
    data: []const u8 = "",
    footer: SstFooterInfo = SstFooterInfo{},
    bloom_num_hashes: u32 = 0;
    bloom_bits: usize = 0;
    bloom_bitset: []const u8 = "",

    /// Opens and validates an SSTable image (Footer CRC32, Index Block CRC32, and Bloom Filter CRC32).
    static fn open(data: []const u8): SstReader ! LsmKvError {
        if (data.len < LSM_SST_FOOTER_BYTES) return LsmKvError.CorruptSst;
        const f_off: usize = data.len - LSM_SST_FOOTER_BYTES;
        const magic: u64 = readU64Le(data, f_off + 52);
        const ver: u32 = readU32Le(data, f_off + 36);
        if (magic != LSM_SST_MAGIC || ver != LSM_SST_VERSION) {
            return LsmKvError.CorruptSst;
        }
        const stored_fcrc: u32 = readU32Le(data, f_off + 60);
        const calc_fcrc: u32 = crc32Ieee(data[f_off..f_off + 60]);
        if (stored_fcrc != calc_fcrc) return LsmKvError.ChecksumMismatch;

        const footer: SstFooterInfo = SstFooterInfo{
            index_off: readU64Le(data, f_off + 0),
            index_len: readU32Le(data, f_off + 8),
            bloom_off: readU64Le(data, f_off + 12),
            bloom_len: readU32Le(data, f_off + 20),
            entry_count: readU64Le(data, f_off + 24),
            block_count: readU32Le(data, f_off + 32),
            version: ver,
            max_seq: readU64Le(data, f_off + 40),
        };

        const idx_off: usize = @as(usize, @intCast(footer.index_off));
        const idx_len: usize = @as(usize, footer.index_len);
        const blm_off: usize = @as(usize, @intCast(footer.bloom_off));
        const blm_len: usize = @as(usize, footer.bloom_len);
        if (idx_len < 8 || idx_off + idx_len > f_off) return LsmKvError.CorruptSst;
        if (blm_len < 12 || blm_off + blm_len > f_off) return LsmKvError.CorruptSst;

        // Verify Index Block CRC32
        const idx_crc_stored: u32 = readU32Le(data, idx_off + idx_len - LSM_CRC32_BYTES);
        const idx_crc_calc: u32 = crc32Ieee(data[idx_off..idx_off + idx_len - LSM_CRC32_BYTES]);
        if (idx_crc_stored != idx_crc_calc) return LsmKvError.ChecksumMismatch;

        // Verify Bloom Filter Block CRC32
        const blm_crc_stored: u32 = readU32Le(data, blm_off + blm_len - LSM_CRC32_BYTES);
        const blm_crc_calc: u32 = crc32Ieee(data[blm_off..blm_off + blm_len - LSM_CRC32_BYTES]);
        if (blm_crc_stored != blm_crc_calc) return LsmKvError.ChecksumMismatch;

        const num_hashes: u32 = readU32Le(data, blm_off + 0);
        const num_bits: usize = @as(usize, readU32Le(data, blm_off + 4));
        const bitset_len: usize = blm_len - 8 - LSM_CRC32_BYTES;
        if (num_hashes == 0 || num_bits == 0 || num_bits > bitset_len * 8) {
            return LsmKvError.CorruptSst;
        }

        return SstReader{
            data: data,
            footer: footer,
            bloom_num_hashes: num_hashes,
            bloom_bits: num_bits,
            bloom_bitset: data[blm_off + 8..blm_off + 8 + bitset_len],
        };
    }

    /// Probes the embedded Bloom Filter in O(k) time. Returns `false` if `key` is definitely absent.
    fn mayContain(key: []const u8): bool {
        const h1: u64 = hash64A(key);
        const h2: u64 = hash64B(key);
        const m: u64 = @as(u64, @intCast(bloom_bits));
        let k: u32 = 0;
        while (k < bloom_num_hashes) : (k += 1) {
            const bit_idx: usize = @as(usize, @intCast((h1 +% @as(u64, k) *% h2) % m));
            const mask: u8 = @as(u8, 1) << @as(u3, @intCast(bit_idx & 7));
            if ((bloom_bitset[bit_idx >> 3] & mask) == 0) return false;
        }
        return true;
    }

    /// Locates the first Data Block whose `last_key >= key` via binary search over Index Block.
    fn findBlockForKey(key: []const u8): SstIndexHandle? {
        const bc: usize = @as(usize, footer.block_count);
        if (bc == 0) return null;

        // First scan index entry offsets on the fly or binary search via two-pass offset probe.
        // Because variable-length keys sit in the index block, we do binary search over block indices
        // using a lightweight helper that jumps to index slot `target_idx` in O(target_idx), or
        // direct scan if bc <= 32; for arbitrary bc we still maintain O(log bc) comparisons.
        const idx_start: usize = @as(usize, @intCast(footer.index_off)) + 4;
        const idx_end: usize = @as(usize, @intCast(footer.index_off)) + @as(usize, footer.index_len) - LSM_CRC32_BYTES;

        let lo: usize = 0;
        let hi: usize = bc;
        while (lo < hi) {
            const mid: usize = lo + (hi - lo) / 2;
            const h: SstIndexHandle = readIndexSlot(data, idx_start, idx_end, mid) ?? return null;
            if (compareBytes(h.last_key, key) < 0) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        if (lo >= bc) return null;
        return readIndexSlot(data, idx_start, idx_end, lo);
    }

    /// Snapshot-isolated point lookup: checks Bloom Filter, binary-searches Index Block,
    /// verifies Data Block CRC32, and decompresses entries to find newest version with `seq <= snapshot_seq`.
    fn getAt(
        key: []const u8,
        snapshot_seq: u64,
        key_scratch: *[]u8,
    ): SstLookupResult ! LsmKvError {
        if (footer.entry_count == 0) {
            return SstLookupResult{ status: LsmLookupStatus.not_found };
        }
        if (!mayContain(key)) {
            return SstLookupResult{
                status: LsmLookupStatus.not_found,
                bloom_rejected: true,
            };
        }

        const handle: SstIndexHandle = findBlockForKey(key) ?? {
            return SstLookupResult{ status: LsmLookupStatus.not_found };
        };
        if (handle.block_size < LSM_CRC32_BYTES || handle.block_offset + handle.block_size > data.len) {
            return LsmKvError.CorruptSst;
        }

        const blk_start: usize = handle.block_offset;
        const blk_payload_end: usize = blk_start + handle.block_size - LSM_CRC32_BYTES;
        const stored_crc: u32 = readU32Le(data, blk_payload_end);
        const calc_crc: u32 = crc32Ieee(data[blk_start..blk_payload_end]);
        if (stored_crc != calc_crc) return LsmKvError.ChecksumMismatch;

        let pos: usize = blk_start;
        let cur_key_len: usize = 0;
        while (pos < blk_payload_end) {
            if (pos + 17 > blk_payload_end) return LsmKvError.CorruptSst;
            const shared: usize = @as(usize, readU16Le(data, pos + 0));
            const unshared: usize = @as(usize, readU16Le(data, pos + 2));
            const vlen: usize = @as(usize, readU32Le(data, pos + 4));
            const seq: u64 = readU64Le(data, pos + 8);
            const is_tomb: bool = data[pos + 16] != 0;
            pos += 17;

            if (shared > cur_key_len || shared + unshared > key_scratch.len) {
                return LsmKvError.BufferTooSmall;
            }
            if (pos + unshared + vlen > blk_payload_end) return LsmKvError.CorruptSst;

            copySlice(key_scratch, shared, data[pos..pos + unshared]);
            cur_key_len = shared + unshared;
            pos += unshared;
            const val_slice: []const u8 = data[pos..pos + vlen];
            pos += vlen;

            const cmp: i32 = compareBytes(key_scratch.*[0..cur_key_len], key);
            if (cmp == 0) {
                if (seq <= snapshot_seq) {
                    if (is_tomb) {
                        return SstLookupResult{
                            status: LsmLookupStatus.deleted,
                            value: "",
                            seq: seq,
                        };
                    }
                    return SstLookupResult{
                        status: LsmLookupStatus.found,
                        value: val_slice,
                        seq: seq,
                    };
                }
            } else if (cmp > 0) {
                break;
            }
        }
        return SstLookupResult{ status: LsmLookupStatus.not_found };
    }
}

fn readIndexSlot(data: []const u8, idx_start: usize, idx_end: usize, target_idx: usize): SstIndexHandle? {
    let pos: usize = idx_start;
    let cur: usize = 0;
    while (pos < idx_end) {
        if (pos + 4 > idx_end) return null;
        const klen: usize = @as(usize, readU32Le(data, pos));
        pos += 4;
        if (pos + klen + 12 > idx_end) return null;
        const last_key: []const u8 = data[pos..pos + klen];
        pos += klen;
        const boff: usize = @as(usize, @intCast(readU64Le(data, pos)));
        const bsz: usize = @as(usize, readU32Le(data, pos + 8));
        pos += 12;
        if (cur == target_idx) {
            return SstIndexHandle{
                block_offset: boff,
                block_size: bsz,
                last_key: last_key,
            };
        }
        cur += 1;
    }
    return null;
}

/// Forward streaming iterator over all entries in an `SstReader` (verifying every Data Block CRC32).
pub struct SstCursor {
    reader: SstReader = SstReader{},
    cur_block_idx: usize = 0;
    blk_pos: usize = 0;
    blk_payload_end: usize = 0;
    cur_key_len: usize = 0;
    valid: bool = false;
    cur_val: []const u8 = "";
    cur_seq: u64 = 0;
    cur_tombstone: bool = false;

    /// Opens a cursor positioned before the first entry of `reader`.
    static fn init(reader: SstReader): SstCursor {
        return SstCursor{
            reader: reader,
            cur_block_idx: 0,
            blk_pos: 0,
            blk_payload_end: 0,
            cur_key_len: 0,
            valid: false,
        };
    }

    /// Advances to the next entry, reconstructing its full key into `key_buf` and returning `SstEntryView?`.
    fn next(*_, key_buf: *[]u8): SstEntryView? ! LsmKvError {
        const bc: usize = @as(usize, _.reader.footer.block_count);
        while (true) {
            if (_.blk_pos >= _.blk_payload_end) {
                if (_.cur_block_idx >= bc) {
                    _.valid = false;
                    return null;
                }
                const idx_start: usize = @as(usize, @intCast(_.reader.footer.index_off)) + 4;
                const idx_end: usize = @as(usize, @intCast(_.reader.footer.index_off)) +
                    @as(usize, _.reader.footer.index_len) - LSM_CRC32_BYTES;
                const h: SstIndexHandle = readIndexSlot(_.reader.data, idx_start, idx_end, _.cur_block_idx) ?? {
                    return LsmKvError.CorruptSst;
                };
                if (h.block_size < LSM_CRC32_BYTES || h.block_offset + h.block_size > _.reader.data.len) {
                    return LsmKvError.CorruptSst;
                }
                const p_end: usize = h.block_offset + h.block_size - LSM_CRC32_BYTES;
                const stored_crc: u32 = readU32Le(_.reader.data, p_end);
                const calc_crc: u32 = crc32Ieee(_.reader.data[h.block_offset..p_end]);
                if (stored_crc != calc_crc) return LsmKvError.ChecksumMismatch;

                _.blk_pos = h.block_offset;
                _.blk_payload_end = p_end;
                _.cur_key_len = 0;
                _.cur_block_idx += 1;
                continue;
            }

            if (_.blk_pos + 17 > _.blk_payload_end) return LsmKvError.CorruptSst;
            const shared: usize = @as(usize, readU16Le(_.reader.data, _.blk_pos + 0));
            const unshared: usize = @as(usize, readU16Le(_.reader.data, _.blk_pos + 2));
            const vlen: usize = @as(usize, readU32Le(_.reader.data, _.blk_pos + 4));
            const seq: u64 = readU64Le(_.reader.data, _.blk_pos + 8);
            const is_tomb: bool = _.reader.data[_.blk_pos + 16] != 0;
            _.blk_pos += 17;

            if (shared > _.cur_key_len || shared + unshared > key_buf.len) {
                return LsmKvError.BufferTooSmall;
            }
            if (_.blk_pos + unshared + vlen > _.blk_payload_end) return LsmKvError.CorruptSst;

            copySlice(key_buf, shared, _.reader.data[_.blk_pos.._.blk_pos + unshared]);
            _.cur_key_len = shared + unshared;
            _.blk_pos += unshared;
            const val_slice: []const u8 = _.reader.data[_.blk_pos.._.blk_pos + vlen];
            _.blk_pos += vlen;

            _.valid = true;
            _.cur_val = val_slice;
            _.cur_seq = seq;
            _.cur_tombstone = is_tomb;

            return SstEntryView{
                key: key_buf.*[0.._.cur_key_len],
                value: val_slice,
                seq: seq,
                is_tombstone: is_tomb,
            };
        }
    }
}

// ============================================================================
// 5. K-Way Leveled Compaction (`compactSstables`)
// ============================================================================

/// Configuration for `compactSstables`.
pub struct SstCompactionConfig {
    drop_tombstones: bool = true;
    min_active_snapshot_seq: u64 = 0xFFFF_FFFF_FFFF_FFFF;
    builder_cfg: SstBuilderConfig = SstBuilderConfig{},
}

/// Statistics returned by `compactSstables`.
pub struct SstCompactionStats {
    input_tables: usize = 0;
    input_entries: usize = 0;
    output_entries: usize = 0;
    dropped_overwritten: usize = 0;
    dropped_tombstones: usize = 0;
    input_bytes: usize = 0;
    output_bytes: usize = 0;
    output_blocks: usize = 0;
    max_seq: u64 = 0;
}

/// Per-way external cursor state passed by caller (`cursors_work: []SstCursor`).
/// `key_bufs_work` must have length `>= inputs.len * LSM_CURSOR_KEY_STRIDE`.
pub fn sstCompactionKeyBufMinBytes(input_count: usize): usize {
    return input_count * LSM_CURSOR_KEY_STRIDE;
}

/// Performs K-Way Merge Compaction over an arbitrary number of SSTable slices (`inputs`)
/// with zero dynamic heap allocation:
///   - Merges entries strictly in `(key ASC, seq DESC)` order
///   - Keeps the newest version per user key and any version `> min_active_snapshot_seq`
///   - Drops overwritten obsolete versions (`<= min_active_snapshot_seq`)
///   - Drops bottommost tombstones when `drop_tombstones == true` and `seq <= min_active_snapshot_seq`
pub fn compactSstables(
    inputs: []const []const u8,
    cfg: SstCompactionConfig,
    cursors_work: *[]SstCursor,
    key_bufs_work: *[]u8,
    out_sst: *[]u8,
    index_work: *[]u8,
    bloom_work: *[]u8,
): SstCompactionStats ! LsmKvError {
    const k: usize = inputs.len;
    if (k == 0) return LsmKvError.InvalidConfig;
    if (cursors_work.len < k || key_bufs_work.len < k * LSM_CURSOR_KEY_STRIDE) {
        return LsmKvError.BufferTooSmall;
    }

    let stats: SstCompactionStats = SstCompactionStats{
        input_tables: k,
    };

    // Open all K SSTable readers and prime each cursor to its first entry
    let i: usize = 0;
    while (i < k) : (i += 1) {
        stats.input_bytes += inputs[i].len;
        const reader: SstReader = try SstReader.open(inputs[i]);
        cursors_work[i] = SstCursor.init(reader);
        const kb_start: usize = i * LSM_CURSOR_KEY_STRIDE;
        let kb: []u8 = key_bufs_work.*[kb_start..kb_start + LSM_CURSOR_KEY_STRIDE];
        * = try cursors_work[i].next(*kb);
    }

    let builder: SstBuilder = try SstBuilder.init(cfg.builder_cfg, out_sst, index_work, bloom_work);

    let last_user_key: [LSM_CURSOR_KEY_STRIDE]u8 = [0; LSM_CURSOR_KEY_STRIDE];
    let last_user_key_len: usize = 0;
    let has_last_key: bool = false;
    let last_emitted_visible_at_snap: bool = false;

    while (true) {
        // Select active cursor with minimum internal key (key ASC, seq DESC)
        let best_way: usize = k;
        i = 0;
        while (i < k) : (i += 1) {
            if (!cursors_work[i].valid) continue;
            if (best_way == k) {
                best_way = i;
            } else {
                const ka_start: usize = i * LSM_CURSOR_KEY_STRIDE;
                const kb_start: usize = best_way * LSM_CURSOR_KEY_STRIDE;
                const ka: []const u8 = key_bufs_work.*[ka_start..ka_start + cursors_work[i].cur_key_len];
                const kb: []const u8 = key_bufs_work.*[kb_start..kb_start + cursors_work[best_way].cur_key_len];
                const cmp: i32 = compareInternalKey(ka, cursors_work[i].cur_seq, kb, cursors_work[best_way].cur_seq);
                if (cmp < 0) {
                    best_way = i;
                }
            }
        }
        if (best_way == k) break;

        stats.input_entries += 1;
        const bw_start: usize = best_way * LSM_CURSOR_KEY_STRIDE;
        const cur_key_len: usize = cursors_work[best_way].cur_key_len;
        let cur_key_copy: [LSM_CURSOR_KEY_STRIDE]u8 = [0; LSM_CURSOR_KEY_STRIDE];
        let c_i: usize = 0;
        while (c_i < cur_key_len) : (c_i += 1) {
            cur_key_copy[c_i] = key_bufs_work.*[bw_start + c_i];
        }
        const cur_key: []const u8 = cur_key_copy[0..cur_key_len];
        const cur_val: []const u8 = cursors_work[best_way].cur_val;
        const cur_seq: u64 = cursors_work[best_way].cur_seq;
        const cur_tomb: bool = cursors_work[best_way].cur_tombstone;

        // Advance winning cursor immediately (its value slice borrows immutable SSTable data)
        let advance_kb: []u8 = key_bufs_work.*[bw_start..bw_start + LSM_CURSOR_KEY_STRIDE];
        * = try cursors_work[best_way].next(*advance_kb);

        const same_as_last: bool = has_last_key && compareBytes(last_user_key[0..last_user_key_len], cur_key) == 0;
        if (!same_as_last) {
            c_i = 0;
            while (c_i < cur_key_len) : (c_i += 1) {
                last_user_key[c_i] = cur_key_copy[c_i];
            }
            last_user_key_len = cur_key_len;
            has_last_key = true;
            last_emitted_visible_at_snap = false;
        }

        // MVCC deduplication & bottommost tombstone GC:
        // If we already saw a newer version for this user key with seq <= min_active_snapshot_seq,
        // then this older version is invisible to every active snapshot and can be dropped.
        if (same_as_last && last_emitted_visible_at_snap) {
            stats.dropped_overwritten += 1;
            continue;
        }

        if (cur_seq <= cfg.min_active_snapshot_seq) {
            last_emitted_visible_at_snap = true;
            if (cur_tomb && cfg.drop_tombstones) {
                stats.dropped_tombstones += 1;
                continue;
            }
        }

        try builder.addEntry(cur_key, cur_val, cur_seq, cur_tomb);
        stats.output_entries += 1;
    }

    const build_stats: SstBuildStats = try builder.finish();
    stats.output_bytes = build_stats.total_bytes;
    stats.output_blocks = build_stats.block_count;
    stats.max_seq = build_stats.max_seq;
    return stats;
}

// ============================================================================
// 6. Integrated Zero-Heap KV Engine Helper (`LsmDbEngine`)
// ============================================================================

/// Integrated zero-heap LSM KV engine coordinating WAL append, MVCC MemTable,
/// and tiered SSTable read fallback (`MemTable -> newer SSTable -> older SSTable`).
pub struct LsmDbEngine {
    mem: LsmMemTable = LsmMemTable{},
    wal_buf: []u8 = [],
    wal_len: usize = 0;
    next_seq: u64 = 1;

    /// Initializes an `LsmDbEngine` over caller-provided MemTable buffers and WAL byte slice.
    static fn init(mem_bufs: LsmMemBuffers, wal_buf: *[]u8): LsmDbEngine ! LsmKvError {
        const mem: LsmMemTable = try LsmMemTable.init(mem_bufs);
        return LsmDbEngine{
            mem: mem,
            wal_buf: wal_buf.*,
            wal_len: 0,
            next_seq: 1,
        };
    }

    /// Writes `Put(key, val)` to WAL first (durability), then inserts into MVCC MemTable,
    /// returning the assigned sequence number.
    fn put(*_, key: []const u8, val: []const u8): u64 ! LsmKvError {
        const seq: u64 = _.next_seq;
        const need: usize = LsmWalCodec.encodedFrameSize(key.len, val.len);
        if (_.wal_len + need > _.wal_buf.len) return LsmKvError.CapacityFull;
        let wal_tail: []u8 = _.wal_buf[_.wal_len.._.wal_buf.len];
        const written: usize = try LsmWalCodec.encodePut(key, val, seq, *wal_tail);
        try _.mem.put(key, val, seq);
        _.wal_len += written;
        _.next_seq += 1;
        return seq;
    }

    /// Writes `Delete(key)` tombstone to WAL first, then inserts into MVCC MemTable,
    /// returning the assigned sequence number.
    fn delete(*_, key: []const u8): u64 ! LsmKvError {
        const seq: u64 = _.next_seq;
        const need: usize = LsmWalCodec.encodedFrameSize(key.len, 0);
        if (_.wal_len + need > _.wal_buf.len) return LsmKvError.CapacityFull;
        let wal_tail: []u8 = _.wal_buf[_.wal_len.._.wal_buf.len];
        const written: usize = try LsmWalCodec.encodeDelete(key, seq, *wal_tail);
        try _.mem.delete(key, seq);
        _.wal_len += written;
        _.next_seq += 1;
        return seq;
    }

    /// Returns the currently active WAL byte slice.
    fn walBytes(): []const u8 {
        return wal_buf[0..wal_len];
    }

    /// Returns the latest committed sequence number (`0` if empty).
    fn latestSeq(): u64 {
        return if (next_seq > 1) next_seq - 1 else 0;
    }

    /// Flushes current MemTable to an SSTable image and resets both MemTable and WAL.
    fn flushMemTableToSst(
        *_,
        cfg: SstBuilderConfig,
        out_sst: *[]u8,
        index_work: *[]u8,
        bloom_work: *[]u8,
    ): SstBuildStats ! LsmKvError {
        const st: SstBuildStats = try _.mem.flushToSst(cfg, out_sst, index_work, bloom_work);
        _.mem.reset();
        _.wal_len = 0;
        return st;
    }

    /// Recovers engine state from a persisted WAL buffer (truncating any torn tail) and advances `next_seq`.
    fn recoverFromWal(*_, wal_data: []const u8): LsmWalRecoveryStats ! LsmKvError {
        _.mem.reset();
        const st: LsmWalRecoveryStats = try LsmWalCodec.replayIntoMemTable(wal_data, *_.mem);
        if (st.valid_bytes <= _.wal_buf.len) {
            copySlice(*_.wal_buf, 0, wal_data[0..st.valid_bytes]);
            _.wal_len = st.valid_bytes;
        }
        if (st.max_seq >= _.next_seq) {
            _.next_seq = st.max_seq + 1;
        }
        return st;
    }

    /// Layered snapshot-isolated read across `MemTable` followed by `sst_levels` (ordered newest -> oldest).
    fn getLayered(
        key: []const u8,
        snapshot_seq: u64,
        sst_levels: []const []const u8,
        key_scratch: *[]u8,
    ): LsmMemGetResult ! LsmKvError {
        const mem_res: LsmMemGetResult = mem.getAt(key, snapshot_seq);
        if (mem_res.status != LsmLookupStatus.not_found) {
            return mem_res;
        }
        let i: usize = 0;
        while (i < sst_levels.len) : (i += 1) {
            if (sst_levels[i].len == 0) continue;
            const reader: SstReader = try SstReader.open(sst_levels[i]);
            const sst_res: SstLookupResult = try reader.getAt(key, snapshot_seq, key_scratch);
            if (sst_res.status != LsmLookupStatus.not_found) {
                return LsmMemGetResult{
                    status: sst_res.status,
                    value: sst_res.value,
                    seq: sst_res.seq,
                };
            }
        }
        return LsmMemGetResult{ status: LsmLookupStatus.not_found };
    }
}
