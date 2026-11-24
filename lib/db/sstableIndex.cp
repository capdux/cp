// Reference-hardened against fjall-rs/lsm-tree (MIT OR Apache-2.0)
// Origin: Pure-CP Stdlib (Zero-Alloc RocksDB/LevelDB Prefix-Compressed Data Block & Sparse Block Index Engine) | Created: 2026-11-23
// Pure CP zero-heap-allocation RocksDB/LevelDB-style SSTable prefix-compressed data block
// builder/reader & sparse fence-key block index:
//   - 16-lane @Vector(16, u8) SIMD hot-path primitives (`simdPageCommonPrefixLen`, `simdPageCompareLex`,
//     `simdPageCopyBytes`) with 0-diff scalar reference counterparts
//   - PrefixBlockBuilder: encodes strictly ascending keys with `(shared_len, unshared_len, val_len)`
//     LEB128 varints, periodic full Restart Points every `restart_interval` entries, and 4B little-endian
//     restart offset array + restart count trailer; enforces strict key order & atomic space reservation
//   - PrefixBlockReader & PrefixBlockCursor: validates block framing/restarts/monotonic keys on open,
//     provides full-block `validatePrefixBlockStrict`, performs O(log R) binary search over Restart Points
//     + linear scan for `get(key, out_key, out_val)` and `seek(lower_key)` forward iteration with zero heap
//   - SstSparseIndex: fixed 64-block fence-key (`last_key`) index with O(log B) binary `locateBlock(key)`
//     and range pruning `locateRange(start_key, end_key)`

/// Module-local 16-lane byte vector alias for SSTable block & slotted page hot paths.
type PageIdxVec16u8 = @Vector(16, u8);

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Default number of entries between consecutive full-key Restart Points.
pub const PREFIX_BLOCK_DEFAULT_RESTART_INTERVAL: u32 = 16;

/// Maximum inline byte capacity of a single prefix-compressed data block.
pub const PREFIX_BLOCK_MAX_BYTES: usize = 4096;

/// Maximum number of Restart Points recorded in a single data block.
pub const PREFIX_BLOCK_MAX_RESTARTS: usize = 128;

/// Maximum key length (bytes) supported in prefix-compressed blocks & sparse index.
pub const PREFIX_BLOCK_MAX_KEY_LEN: usize = 256;

/// Maximum number of data blocks tracked in `SstSparseIndex`.
pub const SST_INDEX_MAX_BLOCKS: usize = 64;

/// Maximum fence-key length (bytes) stored per entry in `SstSparseIndex`.
pub const SST_INDEX_MAX_KEY_LEN: usize = 256;

/// Status returned by block builder & sparse index write operations.
pub enum PrefixBlockStatus {
    ok,
    empty_key,
    key_too_long,
    out_of_order_key,
    already_finished,
    restarts_full,
    buffer_full,
    index_full,
}

/// Lookup verdict returned by `PrefixBlockReader.get`.
pub enum PrefixBlockLookupStatus {
    found,
    not_found,
    buffer_too_small,
    corrupt_block,
}

/// Result of a point lookup inside a prefix-compressed block.
pub struct PrefixBlockLookup {
    status: PrefixBlockLookupStatus = PrefixBlockLookupStatus.not_found;
    found: bool = false;
    key_len: usize = 0;
    val_len: usize = 0;
    binary_probes: u32 = 0;
    linear_steps: u32 = 0;
}

/// Compression & layout telemetry snapshot for `PrefixBlockBuilder`.
pub struct PrefixBlockStats {
    entry_count: u32 = 0;
    restart_count: u32 = 0;
    restart_interval: u32 = PREFIX_BLOCK_DEFAULT_RESTART_INTERVAL;
    raw_key_bytes: u64 = 0;
    raw_val_bytes: u64 = 0;
    shared_prefix_saved_bytes: u64 = 0;
    encoded_bytes: usize = 0;
    key_compression_permille: u32 = 0;
    overall_ratio_permille: u32 = 0;
    finished: bool = false;
}

/// Single fence-key entry in `SstSparseIndex` describing one SSTable data block.
pub struct SstIndexEntry {
    active: bool = false;
    last_key_buf: [SST_INDEX_MAX_KEY_LEN]u8 = [0; SST_INDEX_MAX_KEY_LEN];
    last_key_len: usize = 0;
    block_offset: u64 = 0;
    block_size: u32 = 0;
    entry_count: u32 = 0;

    pub fn lastKeySlice(): []const u8 {
        return last_key_buf[0..last_key_len];
    }
}

/// Inclusive block index range `[first_block, last_block]` returned by `SstSparseIndex.locateRange`.
pub struct SstIndexRange {
    found: bool = false;
    first_block: usize = 0;
    last_block: usize = 0;
    block_count: usize = 0;
}

/// Summary metrics for `SstSparseIndex`.
pub struct SstSparseIndexStats {
    block_count: usize = 0;
    block_capacity: usize = SST_INDEX_MAX_BLOCKS;
    total_entries: u64 = 0;
    total_block_bytes: u64 = 0;
}

// ============================================================================
// 2. 16-Lane SIMD Hot-Path Primitives (0-Diff with Scalar Reference)
// ============================================================================

/// Scalar reference: returns the length of the longest common prefix of `a` and `b`.
pub fn scalarPageCommonPrefixLen(a: []const u8, b: []const u8): usize {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) : (i += 1) {
        if (a[i] != b[i]) break;
    }
    return i;
}

/// 16-lane SIMD accelerated common prefix length scanner (wired into `PrefixBlockBuilder`).
pub fn simdPageCommonPrefixLen(a: []const u8, b: []const u8): usize {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i + 16 <= min_len) {
        const va: PageIdxVec16u8 = a[i .. i + 16][0..16].*;
        const vb: PageIdxVec16u8 = b[i .. i + 16][0..16].*;
        const diff = va ^ vb;
        if (@reduce(.Or, diff) != 0) {
            while (i < min_len and a[i] == b[i]) : (i += 1) {}
            return i;
        }
        i += 16;
    }
    while (i < min_len and a[i] == b[i]) : (i += 1) {}
    return i;
}

/// Scalar reference: unsigned lexicographical byte comparison returning `-1`, `0`, or `1`.
pub fn scalarPageCompareLex(a: []const u8, b: []const u8): i32 {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) : (i += 1) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
    }
    if (a.len < b.len) return -1;
    if (a.len > b.len) return 1;
    return 0;
}

/// 16-lane SIMD accelerated unsigned lexicographical comparison (wired into restart & sparse index search).
pub fn simdPageCompareLex(a: []const u8, b: []const u8): i32 {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i + 16 <= min_len) {
        const va: PageIdxVec16u8 = a[i .. i + 16][0..16].*;
        const vb: PageIdxVec16u8 = b[i .. i + 16][0..16].*;
        const diff = va ^ vb;
        if (@reduce(.Or, diff) != 0) {
            while (i < min_len) : (i += 1) {
                if (a[i] < b[i]) return -1;
                if (a[i] > b[i]) return 1;
            }
            break;
        }
        i += 16;
    }
    while (i < min_len) : (i += 1) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
    }
    if (a.len < b.len) return -1;
    if (a.len > b.len) return 1;
    return 0;
}

/// Scalar reference: non-overlapping byte copy from `src` into `dst` (clamped to `min(dst.len, src.len)`).
pub fn scalarPageCopyBytes(dst: []u8, src: []const u8) {
    const n = if (dst.len < src.len) dst.len else src.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
}

/// 16-lane SIMD accelerated non-overlapping byte copy for SSTable blocks & slotted pages.
pub fn simdPageCopyBytes(dst: []u8, src: []const u8) {
    const n = if (dst.len < src.len) dst.len else src.len;
    let i: usize = 0;
    while (i + 16 <= n) {
        const v: PageIdxVec16u8 = src[i .. i + 16][0..16].*;
        dst[i .. i + 16][0..16].* = v;
        i += 16;
    }
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
}

/// Unsigned lexicographical byte comparison: returns `< 0`, `0`, or `> 0` (SIMD accelerated).
pub fn compareLexBytes(a: []const u8, b: []const u8): i32 {
    return simdPageCompareLex(a, b);
}

// ============================================================================
// 3. Internal Little-Endian & Varint32 Helpers
// ============================================================================

fn sstIdxVarint32Len(val: u32): usize {
    let v = val;
    let n: usize = 1;
    while (v >= 0x80) : (n += 1) {
        v = v >> 7;
    }
    return n;
}

fn sstIdxWriteVarint32(dst: []u8, offset: usize, val: u32): usize {
    let v = val;
    let pos = offset;
    while (v >= 0x80) {
        dst[pos] = @as(u8, @intCast((v & 0x7F) | 0x80));
        v = v >> 7;
        pos += 1;
    }
    dst[pos] = @as(u8, @intCast(v & 0x7F));
    return pos + 1;
}

struct SstIdxDecodedVarint32 {
    value: u32 = 0;
    next_pos: usize = 0;
}

fn sstIdxReadVarint32(src: []const u8, offset: usize, limit: usize): SstIdxDecodedVarint32? {
    let pos = offset;
    let result: u32 = 0;
    let shift: u5 = 0;
    let count: usize = 0;
    while (pos < limit and count < 5) : (count += 1) {
        const b = src[pos];
        pos += 1;
        if (count == 4 and (b & 0xF0) != 0) {
            return null; // u32 overflow
        }
        result = result | ((@as(u32, b & 0x7F)) << shift);
        if ((b & 0x80) == 0) {
            return SstIdxDecodedVarint32{ value: result, next_pos: pos };
        }
        shift += 7;
    }
    return null;
}

fn sstIdxWriteU32Le(dst: []u8, offset: usize, val: u32) {
    dst[offset] = @as(u8, @intCast(val & 0xFF));
    dst[offset + 1] = @as(u8, @intCast((val >> 8) & 0xFF));
    dst[offset + 2] = @as(u8, @intCast((val >> 16) & 0xFF));
    dst[offset + 3] = @as(u8, @intCast((val >> 24) & 0xFF));
}

fn sstIdxReadU32Le(src: []const u8, offset: usize): u32 {
    const b0 = @as(u32, src[offset]);
    const b1 = @as(u32, src[offset + 1]) << 8;
    const b2 = @as(u32, src[offset + 2]) << 16;
    const b3 = @as(u32, src[offset + 3]) << 24;
    return b0 | b1 | b2 | b3;
}

struct SstIdxDecodedEntryMeta {
    shared_len: usize = 0;
    unshared_len: usize = 0;
    val_len: usize = 0;
    delta_offset: usize = 0;
    val_offset: usize = 0;
    next_offset: usize = 0;
}

fn sstIdxDecodeEntryAt(data: []const u8, offset: usize, data_end: usize): SstIdxDecodedEntryMeta? {
    if (offset >= data_end) return null;
    const s_dec = sstIdxReadVarint32(data, offset, data_end) ?? return null;
    const u_dec = sstIdxReadVarint32(data, s_dec.next_pos, data_end) ?? return null;
    const v_dec = sstIdxReadVarint32(data, u_dec.next_pos, data_end) ?? return null;

    const shared = @as(usize, s_dec.value);
    const unshared = @as(usize, u_dec.value);
    const vlen = @as(usize, v_dec.value);

    if (shared > PREFIX_BLOCK_MAX_KEY_LEN or unshared > PREFIX_BLOCK_MAX_KEY_LEN) return null;
    if (shared + unshared > PREFIX_BLOCK_MAX_KEY_LEN) return null;
    if (shared + unshared == 0) return null;

    const delta_off = v_dec.next_pos;
    if (delta_off > data_end or unshared > data_end - delta_off) return null;
    const val_off = delta_off + unshared;
    if (vlen > data_end - val_off) return null;
    const next_off = val_off + vlen;

    return SstIdxDecodedEntryMeta{
        shared_len: shared,
        unshared_len: unshared,
        val_len: vlen,
        delta_offset: delta_off,
        val_offset: val_off,
        next_offset: next_off,
    };
}

// ============================================================================
// 4. PrefixBlockBuilder — Zero-Heap RocksDB/LevelDB Data Block Encoder
// ============================================================================

/// Zero-heap-allocation prefix-compressed SSTable data block builder.
pub struct PrefixBlockBuilder {
    buf: [PREFIX_BLOCK_MAX_BYTES]u8 = [0; PREFIX_BLOCK_MAX_BYTES];
    pos: usize = 0;
    restarts: [PREFIX_BLOCK_MAX_RESTARTS]u32 = [0; PREFIX_BLOCK_MAX_RESTARTS];
    restart_count: usize = 0;
    restart_interval: u32 = PREFIX_BLOCK_DEFAULT_RESTART_INTERVAL;
    since_restart: u32 = 0;
    entry_count: u32 = 0;
    last_key_buf: [PREFIX_BLOCK_MAX_KEY_LEN]u8 = [0; PREFIX_BLOCK_MAX_KEY_LEN];
    last_key_len: usize = 0;
    raw_key_bytes: u64 = 0;
    raw_val_bytes: u64 = 0;
    shared_prefix_saved_bytes: u64 = 0;
    finished: bool = false;

    /// Initialize a new block builder with the specified `restart_interval` (`0` is clamped to `1`).
    static fn init(restart_interval: u32): PrefixBlockBuilder {
        let b = PrefixBlockBuilder{};
        b.restart_interval = if (restart_interval == 0) 1 else restart_interval;
        return b;
    }

    /// Reset builder state for reuse without heap allocation.
    pub fn reset(*_) {
        pos = 0;
        restart_count = 0;
        since_restart = 0;
        entry_count = 0;
        last_key_len = 0;
        raw_key_bytes = 0;
        raw_val_bytes = 0;
        shared_prefix_saved_bytes = 0;
        finished = false;
    }

    /// Append a `(key, value)` pair in strictly ascending lexicographical key order.
    /// Atomically checks buffer capacity (including restart trailer) and overflow bounds before writing.
    pub fn add(*_, key: []const u8, value: []const u8): PrefixBlockStatus {
        if (finished) return PrefixBlockStatus.already_finished;
        if (key.len == 0) return PrefixBlockStatus.empty_key;
        if (key.len > PREFIX_BLOCK_MAX_KEY_LEN) return PrefixBlockStatus.key_too_long;
        if (value.len > PREFIX_BLOCK_MAX_BYTES) return PrefixBlockStatus.buffer_full;

        if (entry_count > 0) {
            if (simdPageCompareLex(key, last_key_buf[0..last_key_len]) <= 0) {
                return PrefixBlockStatus.out_of_order_key;
            }
        }

        const is_restart = entry_count == 0 or since_restart >= restart_interval;
        if (is_restart and restart_count >= PREFIX_BLOCK_MAX_RESTARTS) {
            return PrefixBlockStatus.restarts_full;
        }

        const shared: usize = if (is_restart) 0 else simdPageCommonPrefixLen(last_key_buf[0..last_key_len], key);
        const unshared: usize = key.len - shared;

        const header_bytes = sstIdxVarint32Len(@as(u32, @intCast(shared))) +
            sstIdxVarint32Len(@as(u32, @intCast(unshared))) +
            sstIdxVarint32Len(@as(u32, @intCast(value.len)));
        const record_bytes = header_bytes + unshared + value.len;
        const future_restarts = if (is_restart) restart_count + 1 else restart_count;
        const trailer_bytes = (future_restarts + 1) * 4;

        if (record_bytes > PREFIX_BLOCK_MAX_BYTES or trailer_bytes > PREFIX_BLOCK_MAX_BYTES) {
            return PrefixBlockStatus.buffer_full;
        }
        if (pos > PREFIX_BLOCK_MAX_BYTES - record_bytes) {
            return PrefixBlockStatus.buffer_full;
        }
        if (pos + record_bytes > PREFIX_BLOCK_MAX_BYTES - trailer_bytes) {
            return PrefixBlockStatus.buffer_full;
        }

        if (is_restart) {
            restarts[restart_count] = @as(u32, @intCast(pos));
            restart_count += 1;
            since_restart = 0;
        }

        let p = pos;
        p = sstIdxWriteVarint32(buf[0..], p, @as(u32, @intCast(shared)));
        p = sstIdxWriteVarint32(buf[0..], p, @as(u32, @intCast(unshared)));
        p = sstIdxWriteVarint32(buf[0..], p, @as(u32, @intCast(value.len)));
        simdPageCopyBytes(buf[p .. p + unshared], key[shared..]);
        p += unshared;
        simdPageCopyBytes(buf[p .. p + value.len], value);
        p += value.len;
        pos = p;

        simdPageCopyBytes(last_key_buf[0..key.len], key);
        last_key_len = key.len;

        entry_count += 1;
        since_restart += 1;
        raw_key_bytes += @as(u64, @intCast(key.len));
        raw_val_bytes += @as(u64, @intCast(value.len));
        shared_prefix_saved_bytes += @as(u64, @intCast(shared));
        return PrefixBlockStatus.ok;
    }

    /// Seal the block by appending the Restart Point offset array and 4-byte count trailer.
    /// Idempotent: calling `finish()` again returns the finalized block slice.
    pub fn finish(*_): []const u8 {
        if (!finished) {
            let i: usize = 0;
            while (i < restart_count) : (i += 1) {
                sstIdxWriteU32Le(buf[0..], pos, restarts[i]);
                pos += 4;
            }
            sstIdxWriteU32Le(buf[0..], pos, @as(u32, @intCast(restart_count)));
            pos += 4;
            finished = true;
        }
        return buf[0..pos];
    }

    /// Return the encoded block bytes slice (call after `finish()`).
    pub fn blockBytes(): []const u8 {
        return buf[0..pos];
    }

    /// Return the last (largest) key added to the block (useful as fence key for `SstSparseIndex`).
    pub fn lastKeySlice(): []const u8 {
        return last_key_buf[0..last_key_len];
    }

    /// Return current estimated encoded size including the restart trailer.
    pub fn estimatedSizeBytes(): usize {
        if (finished) return pos;
        return pos + (restart_count + 1) * 4;
    }

    /// Return compression & layout telemetry snapshot.
    pub fn stats(): PrefixBlockStats {
        const enc = estimatedSizeBytes();
        const key_comp: u32 = if (raw_key_bytes > 0)
            @as(u32, @intCast((shared_prefix_saved_bytes * 1000) / raw_key_bytes))
        else
            0;
        const raw_total = raw_key_bytes + raw_val_bytes;
        const overall: u32 = if (raw_total > 0)
            @as(u32, @intCast((@as(u64, @intCast(enc)) * 1000) / raw_total))
        else
            0;
        return PrefixBlockStats{
            entry_count: entry_count,
            restart_count: @as(u32, @intCast(restart_count)),
            restart_interval: restart_interval,
            raw_key_bytes: raw_key_bytes,
            raw_val_bytes: raw_val_bytes,
            shared_prefix_saved_bytes: shared_prefix_saved_bytes,
            encoded_bytes: enc,
            key_compression_permille: key_comp,
            overall_ratio_permille: overall,
            finished: finished,
        };
    }
}

// ============================================================================
// 5. PrefixBlockCursor & PrefixBlockReader — Zero-Heap Binary Search & Scan
// ============================================================================

/// Zero-heap forward iterator over a prefix-compressed SSTable data block.
pub struct PrefixBlockCursor {
    data: []const u8 = "",
    data_end: usize = 0;
    next_offset: usize = 0;
    valid: bool = false;
    key_buf: [PREFIX_BLOCK_MAX_KEY_LEN]u8 = [0; PREFIX_BLOCK_MAX_KEY_LEN];
    key_len: usize = 0;
    val_offset: usize = 0;
    val_len: usize = 0;

    /// Returns true if the cursor currently points to a valid entry.
    pub fn isValid(): bool {
        return valid;
    }

    /// Current entry's reconstructed full key slice (empty if invalid).
    pub fn keySlice(): []const u8 {
        if (!valid) return "";
        return key_buf[0..key_len];
    }

    /// Current entry's zero-copy value slice (empty if invalid).
    pub fn valueSlice(): []const u8 {
        if (!valid) return "";
        return data[val_offset .. val_offset + val_len];
    }

    /// Advance cursor to the next entry in the block; returns false when exhausted or corrupt.
    pub fn next(*_): bool {
        if (!valid) return false;
        if (next_offset >= data_end) {
            valid = false;
            return false;
        }
        if (entry := sstIdxDecodeEntryAt(data, next_offset, data_end)) {
            if (entry.shared_len > key_len) {
                valid = false;
                return false;
            }
            simdPageCopyBytes(key_buf[entry.shared_len .. entry.shared_len + entry.unshared_len], data[entry.delta_offset .. entry.delta_offset + entry.unshared_len]);
            key_len = entry.shared_len + entry.unshared_len;
            val_offset = entry.val_offset;
            val_len = entry.val_len;
            next_offset = entry.next_offset;
            return true;
        }
        valid = false;
        return false;
    }
}

/// Zero-heap reader for prefix-compressed SSTable data blocks with O(log R) Restart Point lookup.
pub struct PrefixBlockReader {
    data: []const u8 = "",
    data_end: usize = 0;
    restarts_offset: usize = 0;
    num_restarts: usize = 0;

    /// Open and structurally validate an encoded prefix-compressed block.
    /// Rejects truncated blocks, excessive/invalid restart counts, out-of-bounds or non-monotonic
    /// restart offsets, overlapping restart entries, non-monotonic restart keys, and restart entries
    /// with non-zero `shared_len`.
    static fn open(data: []const u8): PrefixBlockReader? {
        if (data.len < 4) return null;
        const count_u32 = sstIdxReadU32Le(data, data.len - 4);
        const count = @as(usize, count_u32);

        if (count == 0) {
            if (data.len != 4) return null;
            return PrefixBlockReader{
                data: data,
                data_end: 0,
                restarts_offset: 0,
                num_restarts: 0,
            };
        }

        if (count > PREFIX_BLOCK_MAX_RESTARTS) return null;
        // Guard against integer overflow when computing trailer size.
        if (count > (data.len - 4) / 4) return null;
        const trailer_bytes = (count + 1) * 4;
        const restarts_off = data.len - trailer_bytes;
        if (restarts_off == 0) return null;

        // Validate every restart offset is strictly increasing, in-bounds, starts with shared_len == 0,
        // does not overlap the next restart entry, and has strictly ascending restart keys.
        let prev_off: usize = 0;
        let prev_next_off: usize = 0;
        let prev_key: []const u8 = "";
        let i: usize = 0;
        while (i < count) : (i += 1) {
            const off = @as(usize, sstIdxReadU32Le(data, restarts_off + i * 4));
            if (off >= restarts_off) return null;
            if (i == 0) {
                if (off != 0) return null;
            } else {
                if (off <= prev_off or off < prev_next_off) return null;
            }
            const meta = sstIdxDecodeEntryAt(data, off, restarts_off) ?? return null;
            if (meta.shared_len != 0) return null;
            const cur_key = data[meta.delta_offset .. meta.delta_offset + meta.unshared_len];
            if (i > 0) {
                if (simdPageCompareLex(prev_key, cur_key) >= 0) return null;
            }
            prev_off = off;
            prev_next_off = meta.next_offset;
            prev_key = cur_key;
        }

        return PrefixBlockReader{
            data: data,
            data_end: restarts_off,
            restarts_offset: restarts_off,
            num_restarts: count,
        };
    }

    /// Number of Restart Points in this block.
    pub fn restartCount(): usize {
        return num_restarts;
    }

    /// Return byte offset of Restart Point `idx`.
    pub fn restartOffset(idx: usize): usize? {
        if (idx >= num_restarts) return null;
        return @as(usize, sstIdxReadU32Le(data, restarts_offset + idx * 4));
    }

    /// Read the full key slice stored at Restart Point `idx` (zero-copy directly from `data`).
    fn restartKeySlice(idx: usize): ([]const u8)? {
        const off = restartOffset(idx) ?? return null;
        const meta = sstIdxDecodeEntryAt(data, off, data_end) ?? return null;
        if (meta.shared_len != 0) return null;
        return data[meta.delta_offset .. meta.delta_offset + meta.unshared_len];
    }

    /// Full-block structural & monotonicity verification across every entry and restart boundary.
    pub fn validateAllEntries(): bool {
        if (num_restarts == 0) {
            return data_end == 0 and data.len == 4;
        }
        let prev_key_buf: [PREFIX_BLOCK_MAX_KEY_LEN]u8 = [0; PREFIX_BLOCK_MAX_KEY_LEN];
        let prev_key_len: usize = 0;
        let cur_key_buf: [PREFIX_BLOCK_MAX_KEY_LEN]u8 = [0; PREFIX_BLOCK_MAX_KEY_LEN];
        let has_prev = false;

        let r_idx: usize = 0;
        while (r_idx < num_restarts) : (r_idx += 1) {
            const start_off = restartOffset(r_idx) ?? return false;
            const end_off = if (r_idx + 1 < num_restarts)
                (restartOffset(r_idx + 1) ?? return false)
            else
                data_end;
            if (start_off >= end_off or end_off > data_end) return false;

            let off = start_off;
            let first_in_restart = true;
            while (off < end_off) {
                const meta = sstIdxDecodeEntryAt(data, off, end_off) ?? return false;
                if (first_in_restart and meta.shared_len != 0) return false;
                if (meta.shared_len > prev_key_len) return false;

                if (meta.shared_len > 0) {
                    simdPageCopyBytes(cur_key_buf[0..meta.shared_len], prev_key_buf[0..meta.shared_len]);
                }
                simdPageCopyBytes(
                    cur_key_buf[meta.shared_len .. meta.shared_len + meta.unshared_len],
                    data[meta.delta_offset .. meta.delta_offset + meta.unshared_len],
                );
                const cur_len = meta.shared_len + meta.unshared_len;
                if (has_prev) {
                    if (simdPageCompareLex(prev_key_buf[0..prev_key_len], cur_key_buf[0..cur_len]) >= 0) {
                        return false;
                    }
                }
                simdPageCopyBytes(prev_key_buf[0..cur_len], cur_key_buf[0..cur_len]);
                prev_key_len = cur_len;
                has_prev = true;
                first_in_restart = false;
                off = meta.next_offset;
            }
            if (off != end_off) return false;
        }
        return true;
    }

    /// Return a cursor positioned at the first entry in the block (invalid if block is empty).
    pub fn seekToFirst(): PrefixBlockCursor {
        let cur = PrefixBlockCursor{
            data: data,
            data_end: data_end,
            next_offset: 0,
            valid: false,
        };
        if (num_restarts == 0 or data_end == 0) return cur;
        const meta = sstIdxDecodeEntryAt(data, 0, data_end) ?? return cur;
        if (meta.shared_len != 0) return cur;
        simdPageCopyBytes(cur.key_buf[0..meta.unshared_len], data[meta.delta_offset .. meta.delta_offset + meta.unshared_len]);
        cur.key_len = meta.unshared_len;
        cur.val_offset = meta.val_offset;
        cur.val_len = meta.val_len;
        cur.next_offset = meta.next_offset;
        cur.valid = true;
        return cur;
    }

    /// O(log R) Restart Point binary search + linear scan to position a forward cursor at the
    /// first entry whose key is `>= lower_key`.
    pub fn seek(lower_key: []const u8): PrefixBlockCursor {
        if (num_restarts == 0 or data_end == 0) {
            return PrefixBlockCursor{ data: data, data_end: data_end, valid: false };
        }
        if (lower_key.len == 0) {
            return seekToFirst();
        }

        // Binary search for the largest restart index `lo` with `restart_key[lo] <= lower_key`.
        let lo: usize = 0;
        let hi: usize = num_restarts - 1;
        while (lo < hi) {
            const mid = lo + (hi - lo + 1) / 2;
            const rk_opt = restartKeySlice(mid);
            if (rk_opt == null) {
                return PrefixBlockCursor{ data: data, data_end: data_end, valid: false };
            }
            const rk = rk_opt ?? "";
            if (simdPageCompareLex(rk, lower_key) <= 0) {
                lo = mid;
            } else {
                hi = mid - 1;
            }
        }

        // Position cursor at restart point `lo`, then advance linearly until `key >= lower_key`.
        const start_off_opt = restartOffset(lo);
        if (start_off_opt == null) {
            return PrefixBlockCursor{ data: data, data_end: data_end, valid: false };
        }
        const start_off = start_off_opt ?? 0;
        if (meta0 := sstIdxDecodeEntryAt(data, start_off, data_end)) {
            if (meta0.shared_len != 0) {
                return PrefixBlockCursor{ data: data, data_end: data_end, valid: false };
            }

            let cur = PrefixBlockCursor{
                data: data,
                data_end: data_end,
                next_offset: meta0.next_offset,
                valid: true,
                key_len: meta0.unshared_len,
                val_offset: meta0.val_offset,
                val_len: meta0.val_len,
            };
            simdPageCopyBytes(cur.key_buf[0..meta0.unshared_len], data[meta0.delta_offset .. meta0.delta_offset + meta0.unshared_len]);

            while (cur.isValid()) {
                if (simdPageCompareLex(cur.keySlice(), lower_key) >= 0) {
                    return cur;
                }
                if (!cur.next()) break;
            }
            return cur;
        }
        return PrefixBlockCursor{ data: data, data_end: data_end, valid: false };
    }

    /// O(log R) Restart Point binary search + linear scan for exact key lookup.
    /// Copies the matched key and value into `out_key` and `out_val` with zero heap allocation.
    pub fn get(key: []const u8, out_key: *[]u8, out_val: *[]u8): PrefixBlockLookup {
        if (key.len == 0 or num_restarts == 0 or data_end == 0) {
            return PrefixBlockLookup{ status: PrefixBlockLookupStatus.not_found, found: false };
        }

        let probes: u32 = 0;

        // Check if key is smaller than the very first key in the block.
        const rk0_opt = restartKeySlice(0);
        if (rk0_opt == null) {
            return PrefixBlockLookup{ status: PrefixBlockLookupStatus.corrupt_block, found: false };
        }
        const rk0 = rk0_opt ?? "";
        probes += 1;
        if (simdPageCompareLex(key, rk0) < 0) {
            return PrefixBlockLookup{
                status: PrefixBlockLookupStatus.not_found,
                found: false,
                binary_probes: probes,
            };
        }

        // Binary search for largest restart index `lo` where `restart_key[lo] <= key`.
        let lo: usize = 0;
        let hi: usize = num_restarts - 1;
        while (lo < hi) {
            const mid = lo + (hi - lo + 1) / 2;
            const rk_opt = restartKeySlice(mid);
            if (rk_opt == null) {
                return PrefixBlockLookup{
                    status: PrefixBlockLookupStatus.corrupt_block,
                    found: false,
                    binary_probes: probes,
                };
            }
            const rk = rk_opt ?? "";
            probes += 1;
            if (simdPageCompareLex(rk, key) <= 0) {
                lo = mid;
            } else {
                hi = mid - 1;
            }
        }

        // Linear scan starting from restart point `lo`.
        const start_off_opt = restartOffset(lo);
        if (start_off_opt == null) {
            return PrefixBlockLookup{
                status: PrefixBlockLookupStatus.corrupt_block,
                found: false,
                binary_probes: probes,
            };
        }
        const start_off = start_off_opt ?? 0;

        let key_buf: [PREFIX_BLOCK_MAX_KEY_LEN]u8 = [0; PREFIX_BLOCK_MAX_KEY_LEN];
        let cur_key_len: usize = 0;
        let off = start_off;
        let steps: u32 = 0;

        while (off < data_end) {
            const meta_opt = sstIdxDecodeEntryAt(data, off, data_end);
            if (meta_opt == null) {
                return PrefixBlockLookup{
                    status: PrefixBlockLookupStatus.corrupt_block,
                    found: false,
                    binary_probes: probes,
                    linear_steps: steps,
                };
            }
            const meta = meta_opt ?? SstIdxDecodedEntryMeta{};
            if (meta.shared_len > cur_key_len) {
                return PrefixBlockLookup{
                    status: PrefixBlockLookupStatus.corrupt_block,
                    found: false,
                    binary_probes: probes,
                    linear_steps: steps,
                };
            }
            simdPageCopyBytes(
                key_buf[meta.shared_len .. meta.shared_len + meta.unshared_len],
                data[meta.delta_offset .. meta.delta_offset + meta.unshared_len],
            );
            cur_key_len = meta.shared_len + meta.unshared_len;
            steps += 1;

            const cmp = simdPageCompareLex(key_buf[0..cur_key_len], key);
            if (cmp == 0) {
                if (cur_key_len > out_key.len or meta.val_len > out_val.len) {
                    return PrefixBlockLookup{
                        status: PrefixBlockLookupStatus.buffer_too_small,
                        found: false,
                        key_len: cur_key_len,
                        val_len: meta.val_len,
                        binary_probes: probes,
                        linear_steps: steps,
                    };
                }
                simdPageCopyBytes(out_key[0..cur_key_len], key_buf[0..cur_key_len]);
                simdPageCopyBytes(out_val[0..meta.val_len], data[meta.val_offset .. meta.val_offset + meta.val_len]);
                return PrefixBlockLookup{
                    status: PrefixBlockLookupStatus.found,
                    found: true,
                    key_len: cur_key_len,
                    val_len: meta.val_len,
                    binary_probes: probes,
                    linear_steps: steps,
                };
            }
            if (cmp > 0) {
                return PrefixBlockLookup{
                    status: PrefixBlockLookupStatus.not_found,
                    found: false,
                    binary_probes: probes,
                    linear_steps: steps,
                };
            }

            off = meta.next_offset;
        }

        return PrefixBlockLookup{
            status: PrefixBlockLookupStatus.not_found,
            found: false,
            binary_probes: probes,
            linear_steps: steps,
        };
    }
}

/// Zero-heap full structural and key-order validator for an encoded prefix-compressed block.
pub fn validatePrefixBlockStrict(data: []const u8): bool {
    if (reader := PrefixBlockReader.open(data)) {
        return reader.validateAllEntries();
    }
    return false;
}

// ============================================================================
// 6. SstSparseIndex — Zero-Heap 64-Block Fence-Key Sparse Index
// ============================================================================

/// Zero-heap sparse SSTable block index storing up to 64 block fence keys (`last_key`).
pub struct SstSparseIndex {
    entries: [SST_INDEX_MAX_BLOCKS]SstIndexEntry = [SstIndexEntry{}; SST_INDEX_MAX_BLOCKS];
    block_count: usize = 0;
    total_entries: u64 = 0;
    total_block_bytes: u64 = 0;

    /// Initialize an empty sparse block index.
    static fn init(): SstSparseIndex {
        return SstSparseIndex{};
    }

    /// Clear all indexed blocks.
    pub fn clear(*_) {
        let i: usize = 0;
        while (i < SST_INDEX_MAX_BLOCKS) : (i += 1) {
            entries[i] = SstIndexEntry{};
        }
        block_count = 0;
        total_entries = 0;
        total_block_bytes = 0;
    }

    /// Append a data block descriptor with strictly ascending fence key `last_key`.
    pub fn addBlock(
        *_,
        last_key: []const u8,
        block_offset: u64,
        block_size: u32,
        entry_count: u32,
    ): PrefixBlockStatus {
        if (last_key.len == 0) return PrefixBlockStatus.empty_key;
        if (last_key.len > SST_INDEX_MAX_KEY_LEN) return PrefixBlockStatus.key_too_long;
        if (block_count >= SST_INDEX_MAX_BLOCKS) return PrefixBlockStatus.index_full;

        if (block_count > 0) {
            if (simdPageCompareLex(last_key, entries[block_count - 1].lastKeySlice()) <= 0) {
                return PrefixBlockStatus.out_of_order_key;
            }
        }

        let e = SstIndexEntry{};
        e.active = true;
        simdPageCopyBytes(e.last_key_buf[0..last_key.len], last_key);
        e.last_key_len = last_key.len;
        e.block_offset = block_offset;
        e.block_size = block_size;
        e.entry_count = entry_count;

        entries[block_count] = e;
        block_count += 1;
        total_entries += @as(u64, entry_count);
        total_block_bytes += @as(u64, block_size);
        return PrefixBlockStatus.ok;
    }

    /// Number of indexed data blocks.
    pub fn count(): usize {
        return block_count;
    }

    /// Return a read-only copy of block metadata at `index`, or `null` if out of bounds.
    pub fn getBlock(index: usize): SstIndexEntry? {
        if (index >= block_count) return null;
        return entries[index];
    }

    /// O(log B) binary search for the first block whose fence key `last_key >= key`.
    /// Returns `null` if the index is empty or `key` exceeds all indexed fence keys.
    pub fn locateBlock(key: []const u8): usize? {
        if (block_count == 0) return null;
        if (simdPageCompareLex(entries[block_count - 1].lastKeySlice(), key) < 0) {
            return null;
        }

        let lo: usize = 0;
        let hi: usize = block_count - 1;
        while (lo < hi) {
            const mid = lo + (hi - lo) / 2;
            if (simdPageCompareLex(entries[mid].lastKeySlice(), key) >= 0) {
                hi = mid;
            } else {
                lo = mid + 1;
            }
        }
        return lo;
    }

    /// O(log B) range pruning for `[start_key, end_key]` (empty `start_key` = unbounded lower,
    /// empty `end_key` = unbounded upper).
    pub fn locateRange(start_key: []const u8, end_key: []const u8): SstIndexRange {
        if (block_count == 0) {
            return SstIndexRange{ found: false };
        }
        if (start_key.len > 0 and end_key.len > 0 and simdPageCompareLex(start_key, end_key) > 0) {
            return SstIndexRange{ found: false };
        }

        let first: usize = 0;
        if (start_key.len > 0) {
            const first_opt = locateBlock(start_key);
            if (first_opt == null) {
                return SstIndexRange{ found: false };
            }
            first = first_opt ?? 0;
        }

        const last = if (end_key.len == 0)
            block_count - 1
        else
            locateBlock(end_key) ?? (block_count - 1);

        if (first > last) {
            return SstIndexRange{ found: false };
        }

        return SstIndexRange{
            found: true,
            first_block: first,
            last_block: last,
            block_count: last - first + 1,
        };
    }

    /// Return summary telemetry for the sparse index.
    pub fn stats(): SstSparseIndexStats {
        return SstSparseIndexStats{
            block_count: block_count,
            block_capacity: SST_INDEX_MAX_BLOCKS,
            total_entries: total_entries,
            total_block_bytes: total_block_bytes,
        };
    }
}
