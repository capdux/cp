// Origin: Pure-CP Stdlib (Zero-Alloc LSM-Tree Sorted MemTable with Binary Search, Tombstones & Freeze) | Created: 2026-11-22
// 100% pure CP zero-heap-allocation LSM-Tree Sorted MemTable:
//   - Compact 8 KiB inline byte arena (`MEMTABLE_ARENA_BYTES = 8192`) + 128-slot lexicographically
//     sorted index table (`MEMTABLE_MAX_ENTRIES = 128`) supporting O(log N) binary search.
//   - Full LSM write path: `put(key, value, seq_num)`, `deleteKey(key, seq_num)` (tombstone shadowing
//     for lower SSTable levels), same-key slot/value reuse, and monotonic `seq_num` validation.
//   - Point lookup (`get`: `found_value | found_tombstone | not_found`), O(log N)-seek range scan
//     (`scanRange`) with optional tombstone visibility for SSTable flush, and `scanPrefix`.
//   - Immutable freeze (`freeze()`) prior to SSTable flush + full arena/tombstone telemetry (`stats()`).
//   - Full defensive guards: empty key rejection, frozen write rejection, atomic arena/index full
//     rejection, and monotonic `seq_num` enforcement.

// ============================================================================
// Constants
// ============================================================================

pub const MEMTABLE_MAX_ENTRIES: usize = 128;
pub const MEMTABLE_ARENA_BYTES: usize = 8192;

// ============================================================================
// Internal Byte Comparison Helpers
// ============================================================================

fn compareBytesLex(a: []const u8, b: []const u8): i32 {
    let min_len = a.len;
    if (b.len < min_len) {
        min_len = b.len;
    }
    let i: usize = 0;
    while (i < min_len) : (i += 1) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
    }
    if (a.len < b.len) return -1;
    if (a.len > b.len) return 1;
    return 0;
}

fn hasPrefixBytes(key: []const u8, prefix: []const u8): bool {
    if (key.len < prefix.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (key[i] != prefix[i]) {
            return false;
        }
    }
    return true;
}

// ============================================================================
// 1. Enums & Data Models
// ============================================================================

/// Outcome status of `SortedMemTable.put` and `SortedMemTable.deleteKey`.
pub enum MemTableWriteStatus {
    ok,
    frozen,
    empty_key,
    stale_seq,
    index_full,
    arena_full,
}

/// Point-lookup classification distinguishing live value, tombstone shadow, and absent key.
pub enum MemTableGetKind {
    found_value,
    found_tombstone,
    not_found,
}

/// Compact arena-indexed metadata record for one sorted MemTable entry.
pub struct MemTableEntryRef {
    seq_num: u64 = 0;
    is_tombstone: bool = false;
    key_off: u16 = 0;
    key_len: u16 = 0;
    val_off: u16 = 0;
    val_len: u16 = 0;
}

/// Result returned by `SortedMemTable.get`.
pub struct MemTableGetResult {
    kind: MemTableGetKind = MemTableGetKind.not_found;
    seq_num: u64 = 0;
    entry: MemTableEntryRef = MemTableEntryRef{};
}

/// Memory footprint, sequence bounds, and tombstone statistics of `SortedMemTable`.
pub struct MemTableStats {
    entry_count: usize = 0;
    live_count: usize = 0;
    tombstone_count: usize = 0;
    arena_used_bytes: usize = 0;
    arena_capacity_bytes: usize = MEMTABLE_ARENA_BYTES;
    max_entries: usize = MEMTABLE_MAX_ENTRIES;
    min_seq_num: u64 = 0;
    max_seq_num: u64 = 0;
    frozen: bool = false;
}

// ============================================================================
// 2. Zero-Alloc LSM-Tree Sorted MemTable
// ============================================================================

/// Zero-heap-allocation sorted MemTable with inline 8 KiB byte arena and 128-slot binary-search index.
pub struct SortedMemTable {
    entries: [128]MemTableEntryRef = [MemTableEntryRef{}; 128];
    entry_count: usize = 0;
    live_count: usize = 0;
    tombstone_count: usize = 0;
    arena: [8192]u8 = undefined;
    arena_used: usize = 0;
    min_seq_num: u64 = 0;
    max_seq_num: u64 = 0;
    has_writes: bool = false;
    frozen: bool = false;

    /// Initializes an empty mutable `SortedMemTable`.
    static fn init(): SortedMemTable {
        return SortedMemTable{};
    }

    /// Freezes the MemTable into an immutable read-only state prior to SSTable flush.
    fn freeze(*_) {
        _.frozen = true;
    }

    /// Returns `true` if the MemTable has been frozen.
    fn isFrozen(): bool {
        return _.frozen;
    }

    /// Inserts or updates `(key, value)` with strictly increasing `seq_num`.
    /// - Reuses existing key bytes and in-place value buffer when the updated value fits.
    /// - Atomically rejects writes when frozen, empty key, stale `seq_num`, or index/arena full.
    fn put(*_, key: []const u8, value: []const u8, seq_num: u64): MemTableWriteStatus {
        if (_.frozen) {
            return MemTableWriteStatus.frozen;
        }
        if (key.len == 0) {
            return MemTableWriteStatus.empty_key;
        }
        if (_.has_writes && seq_num <= _.max_seq_num) {
            return MemTableWriteStatus.stale_seq;
        }
        if (key.len > MEMTABLE_ARENA_BYTES || value.len > MEMTABLE_ARENA_BYTES) {
            return MemTableWriteStatus.arena_full;
        }

        const pos = _.lowerBound(key);
        if (pos < _.entry_count && _.keyMatchesAt(pos, key)) {
            // Update existing key entry in-place (reusing key_off/key_len!)
            const existing = _.entries[pos];
            let new_val_off: u16 = existing.val_off;
            if (!existing.is_tombstone && value.len <= (existing.val_len as usize)) {
                // Reuse existing value slice in-place
                const off = new_val_off as usize;
                if (value.len > 0) {
                    @memcpy(_.arena[off .. off + value.len], value);
                }
            } else {
                // Append new value bytes to arena
                if (_.arena_used + value.len > MEMTABLE_ARENA_BYTES) {
                    return MemTableWriteStatus.arena_full;
                }
                new_val_off = _.arena_used as u16;
                if (value.len > 0) {
                    @memcpy(_.arena[_.arena_used .. _.arena_used + value.len], value);
                }
                _.arena_used += value.len;
            }

            if (existing.is_tombstone) {
                _.tombstone_count -= 1;
                _.live_count += 1;
            }
            _.entries[pos].val_off = new_val_off;
            _.entries[pos].val_len = value.len as u16;
            _.entries[pos].seq_num = seq_num;
            _.entries[pos].is_tombstone = false;
            _.recordSeq(seq_num);
            return MemTableWriteStatus.ok;
        }

        // Insert brand-new key
        if (_.entry_count >= MEMTABLE_MAX_ENTRIES) {
            return MemTableWriteStatus.index_full;
        }
        if (_.arena_used + key.len + value.len > MEMTABLE_ARENA_BYTES) {
            return MemTableWriteStatus.arena_full;
        }

        const k_off = _.arena_used;
        @memcpy(_.arena[k_off .. k_off + key.len], key);
        _.arena_used += key.len;

        const v_off = _.arena_used;
        if (value.len > 0) {
            @memcpy(_.arena[v_off .. v_off + value.len], value);
        }
        _.arena_used += value.len;

        // Shift index entries right to preserve sorted order
        let shift_i = _.entry_count;
        while (shift_i > pos) : (shift_i -= 1) {
            _.entries[shift_i] = _.entries[shift_i - 1];
        }
        _.entries[pos] = MemTableEntryRef{
            seq_num: seq_num,
            is_tombstone: false,
            key_off: k_off as u16,
            key_len: key.len as u16,
            val_off: v_off as u16,
            val_len: value.len as u16,
        };
        _.entry_count += 1;
        _.live_count += 1;
        _.recordSeq(seq_num);
        return MemTableWriteStatus.ok;
    }

    /// Writes a tombstone marker for `key` at `seq_num` to shadow lower-level SSTables.
    fn deleteKey(*_, key: []const u8, seq_num: u64): MemTableWriteStatus {
        if (_.frozen) {
            return MemTableWriteStatus.frozen;
        }
        if (key.len == 0) {
            return MemTableWriteStatus.empty_key;
        }
        if (_.has_writes && seq_num <= _.max_seq_num) {
            return MemTableWriteStatus.stale_seq;
        }
        if (key.len > MEMTABLE_ARENA_BYTES) {
            return MemTableWriteStatus.arena_full;
        }

        const pos = _.lowerBound(key);
        if (pos < _.entry_count && _.keyMatchesAt(pos, key)) {
            if (!_.entries[pos].is_tombstone) {
                _.live_count -= 1;
                _.tombstone_count += 1;
            }
            _.entries[pos].is_tombstone = true;
            _.entries[pos].val_len = 0;
            _.entries[pos].seq_num = seq_num;
            _.recordSeq(seq_num);
            return MemTableWriteStatus.ok;
        }

        // Insert new tombstone entry so lower SSTable levels are shadowed
        if (_.entry_count >= MEMTABLE_MAX_ENTRIES) {
            return MemTableWriteStatus.index_full;
        }
        if (_.arena_used + key.len > MEMTABLE_ARENA_BYTES) {
            return MemTableWriteStatus.arena_full;
        }

        const k_off = _.arena_used;
        @memcpy(_.arena[k_off .. k_off + key.len], key);
        _.arena_used += key.len;

        let shift_i = _.entry_count;
        while (shift_i > pos) : (shift_i -= 1) {
            _.entries[shift_i] = _.entries[shift_i - 1];
        }
        _.entries[pos] = MemTableEntryRef{
            seq_num: seq_num,
            is_tombstone: true,
            key_off: k_off as u16,
            key_len: key.len as u16,
            val_off: 0,
            val_len: 0,
        };
        _.entry_count += 1;
        _.tombstone_count += 1;
        _.recordSeq(seq_num);
        return MemTableWriteStatus.ok;
    }

    /// Performs O(log N) binary-search point lookup for `key`.
    fn get(key: []const u8): MemTableGetResult {
        if (key.len == 0 || _.entry_count == 0) {
            return MemTableGetResult{ kind: MemTableGetKind.not_found };
        }
        const pos = _.lowerBound(key);
        if (pos < _.entry_count && _.keyMatchesAt(pos, key)) {
            const e = _.entries[pos];
            return MemTableGetResult{
                kind: if (e.is_tombstone) MemTableGetKind.found_tombstone else MemTableGetKind.found_value,
                seq_num: e.seq_num,
                entry: e,
            };
        }
        return MemTableGetResult{ kind: MemTableGetKind.not_found };
    }

    /// Scans entries in half-open lexicographical range `[start_key, end_key)` into `out_entries`.
    /// - Empty `start_key` starts at the smallest key; empty `end_key` scans through the largest key.
    /// - Set `include_tombstones = true` when flushing to SSTable; `false` for user reads.
    fn scanRange(
        start_key: []const u8,
        end_key: []const u8,
        include_tombstones: bool,
        out_entries: *[]MemTableEntryRef,
    ): usize {
        if (_.entry_count == 0 || out_entries.len == 0) {
            return 0;
        }
        if (start_key.len > 0 && end_key.len > 0 && compareBytesLex(start_key, end_key) >= 0) {
            return 0;
        }

        let pos: usize = if (start_key.len > 0) _.lowerBound(start_key) else 0;
        let written: usize = 0;
        while (pos < _.entry_count && written < out_entries.len) : (pos += 1) {
            const e = _.entries[pos];
            if (end_key.len > 0) {
                const k_slice = _.arena[(e.key_off as usize) .. (e.key_off as usize) + (e.key_len as usize)];
                if (compareBytesLex(k_slice, end_key) >= 0) {
                    break;
                }
            }
            if (!include_tombstones && e.is_tombstone) {
                continue;
            }
            out_entries[written] = e;
            written += 1;
        }
        return written;
    }

    /// Scans all live (non-tombstone) entries whose key begins with `prefix` in lexicographical order.
    fn scanPrefix(prefix: []const u8, out_entries: *[]MemTableEntryRef): usize {
        if (_.entry_count == 0 || out_entries.len == 0) {
            return 0;
        }
        let pos: usize = if (prefix.len > 0) _.lowerBound(prefix) else 0;
        let written: usize = 0;
        while (pos < _.entry_count && written < out_entries.len) : (pos += 1) {
            const e = _.entries[pos];
            if (prefix.len > 0) {
                const k_slice = _.arena[(e.key_off as usize) .. (e.key_off as usize) + (e.key_len as usize)];
                if (!hasPrefixBytes(k_slice, prefix)) {
                    break;
                }
            }
            if (e.is_tombstone) {
                continue;
            }
            out_entries[written] = e;
            written += 1;
        }
        return written;
    }

    /// Copies the key bytes of `entry` into `out_buf`. Returns written length or `null` if too small.
    fn copyKey(entry: MemTableEntryRef, out_buf: *[]u8): usize? {
        const klen = entry.key_len as usize;
        const koff = entry.key_off as usize;
        if (out_buf.len < klen || koff + klen > _.arena_used) {
            return null;
        }
        let i: usize = 0;
        while (i < klen) : (i += 1) {
            out_buf[i] = _.arena[koff + i];
        }
        return klen;
    }

    /// Copies the value bytes of `entry` into `out_buf`. Returns written length or `null` if too small.
    fn copyValue(entry: MemTableEntryRef, out_buf: *[]u8): usize? {
        if (entry.is_tombstone) {
            return 0;
        }
        const vlen = entry.val_len as usize;
        const voff = entry.val_off as usize;
        if (out_buf.len < vlen || voff + vlen > _.arena_used) {
            return null;
        }
        let i: usize = 0;
        while (i < vlen) : (i += 1) {
            out_buf[i] = _.arena[voff + i];
        }
        return vlen;
    }

    /// Compares `entry`'s key bytes against `expected` without copying.
    fn keyEquals(entry: MemTableEntryRef, expected: []const u8): bool {
        const klen = entry.key_len as usize;
        const koff = entry.key_off as usize;
        if (klen != expected.len || koff + klen > _.arena_used) {
            return false;
        }
        return compareBytesLex(_.arena[koff .. koff + klen], expected) == 0;
    }

    /// Compares `entry`'s value bytes against `expected` without copying.
    fn valueEquals(entry: MemTableEntryRef, expected: []const u8): bool {
        if (entry.is_tombstone) {
            return false;
        }
        const vlen = entry.val_len as usize;
        const voff = entry.val_off as usize;
        if (vlen != expected.len || voff + vlen > _.arena_used) {
            return false;
        }
        return compareBytesLex(_.arena[voff .. voff + vlen], expected) == 0;
    }

    /// Returns memory usage, entry/tombstone counts, sequence range, and freeze state.
    fn stats(): MemTableStats {
        return MemTableStats{
            entry_count: _.entry_count,
            live_count: _.live_count,
            tombstone_count: _.tombstone_count,
            arena_used_bytes: _.arena_used,
            arena_capacity_bytes: MEMTABLE_ARENA_BYTES,
            max_entries: MEMTABLE_MAX_ENTRIES,
            min_seq_num: _.min_seq_num,
            max_seq_num: _.max_seq_num,
            frozen: _.frozen,
        };
    }

    // ========================================================================
    // Internal Binary Search & Sequence Helpers
    // ========================================================================

    fn recordSeq(*_, seq_num: u64) {
        if (!_.has_writes) {
            _.min_seq_num = seq_num;
            _.max_seq_num = seq_num;
            _.has_writes = true;
        } else {
            _.max_seq_num = seq_num;
        }
    }

    fn lowerBound(key: []const u8): usize {
        let lo: usize = 0;
        let hi: usize = _.entry_count;
        while (lo < hi) {
            const mid = lo + (hi - lo) / 2;
            const e = _.entries[mid];
            const k_slice = _.arena[(e.key_off as usize) .. (e.key_off as usize) + (e.key_len as usize)];
            if (compareBytesLex(k_slice, key) < 0) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return lo;
    }

    fn keyMatchesAt(pos: usize, key: []const u8): bool {
        const e = _.entries[pos];
        const k_slice = _.arena[(e.key_off as usize) .. (e.key_off as usize) + (e.key_len as usize)];
        return compareBytesLex(k_slice, key) == 0;
    }
}
