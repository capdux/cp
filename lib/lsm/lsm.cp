// Source: cp/lib/lsm/lsm.cp (Pure CP LSM-Tree Storage Engine)
// Zero-C-dependency Log-Structured Merge-Tree with ordered MemTable, immutable SSTables
// (Data Block + Sparse Index Block + CRC32 + "CPS1" magic + Tombstone markers),
// automatic flush, multi-way rangeScan, and full multi-way Compaction.

const std = @import("std");

pub const MAX_KEY_LEN: usize = 65535;
const SST_MAGIC: [4]u8 = ['C', 'P', 'S', '1'];
const SST_FOOTER_SIZE: usize = 20;
const SPARSE_STRIDE: usize = 16;
const MEM_ENTRY_OVERHEAD: usize = 24;

/// Error set for LSM-Tree operations.
pub err LsmError {
    CorruptSst,
    InvalidRange,
    KeyTooLarge,
    OutOfMemory,
}

/// Read-only key-value entry returned by `rangeScan`.
pub struct LsmEntry {
    key: string = "";
    value: string = "";
}

/// Runtime statistics for `LsmEngine`.
pub struct LsmStats {
    memtable_entries: usize = 0;
    memtable_bytes: usize = 0;
    sst_count: usize = 0;
    total_sst_entries: usize = 0;
    compactions_run: usize = 0;
}

/// Input record used to build an `SsTable`.
pub struct SstRecord {
    key: string = "";
    value: string = "";
    seq: u64 = 0;
    is_tombstone: bool = false;
}

/// Result of looking up a key inside an `SsTable`.
pub enum SstLookup {
    Value([]const u8),
    Tombstone,
    NotFound,
}

struct SparseIndexEntry {
    key_offset: usize = 0;
    key_len: usize = 0;
    data_offset: usize = 0;
}

struct TempSparseEntry {
    key: string = "";
    data_offset: usize = 0;
}

struct DecodedRecord {
    key: []const u8 = &.{};
    value: []const u8 = &.{};
    seq: u64 = 0;
    is_tombstone: bool = false;
    next_offset: usize = 0;
}

struct SearchPos {
    found: bool = false;
    idx: usize = 0;
}

struct MemEntry {
    key: []u8 = &.{};
    value: []u8 = &.{};
    seq: u64 = 0;
    is_tombstone: bool = false;
}

struct MergeCursor {
    source_kind: u8 = 0; // 0 = memtable, 1 = l0/l1 sst
    sst_ptr: *const SsTable? = null;
    mem_idx: usize = 0;
    sst_offset: usize = 0;
    sst_end_offset: usize = 0;
    active: bool = false;
    cur_key: []const u8 = &.{};
    cur_val: []const u8 = &.{};
    cur_seq: u64 = 0;
    cur_tombstone: bool = false;
}

fn readU32LE(buf: []const u8, offset: usize): u32 {
    let arr: [4]u8 = undefined;
    @memcpy(arr[0..], buf[offset .. offset + 4]);
    return std.mem.readInt(u32, &arr, .little);
}

fn readU64LE(buf: []const u8, offset: usize): u64 {
    let arr: [8]u8 = undefined;
    @memcpy(arr[0..], buf[offset .. offset + 8]);
    return std.mem.readInt(u64, &arr, .little);
}

fn appendU32LE(list: *std.ArrayList(u8), alloc: std.mem.Allocator, val: u32): void ! LsmError {
    let arr: [4]u8 = undefined;
    std.mem.writeInt(u32, &arr, val, .little);
    list.appendSlice(alloc, arr[0..]) catch return LsmError.OutOfMemory;
}

fn appendU64LE(list: *std.ArrayList(u8), alloc: std.mem.Allocator, val: u64): void ! LsmError {
    let arr: [8]u8 = undefined;
    std.mem.writeInt(u64, &arr, val, .little);
    list.appendSlice(alloc, arr[0..]) catch return LsmError.OutOfMemory;
}

fn decodeRecordAt(data: []const u8, offset: usize, limit: usize): DecodedRecord ! LsmError {
    if (offset + 17 > limit) {
        return LsmError.CorruptSst;
    }
    const klen = usize(readU32LE(data, offset));
    const vlen = usize(readU32LE(data, offset + 4));
    const seq = readU64LE(data, offset + 8);
    const tomb_byte = data[offset + 16];
    if (tomb_byte > 1) {
        return LsmError.CorruptSst;
    }
    const header_end = offset + 17;
    if (header_end + klen > limit || header_end + klen + vlen > limit) {
        return LsmError.CorruptSst;
    }
    const key_slice = data[header_end .. header_end + klen];
    const val_slice = data[header_end + klen .. header_end + klen + vlen];
    return DecodedRecord{
        key: key_slice,
        value: val_slice,
        seq: seq,
        is_tombstone: tomb_byte != 0,
        next_offset: header_end + klen + vlen,
    };
}

/// Immutable Sorted String Table (SSTable) with Data Block, Sparse Index Block,
/// `"CPS1"` magic footer, and CRC32 integrity verification.
pub struct SsTable {
    alloc: std.mem.Allocator;
    data: []u8 = &.{};
    index_entries: std.ArrayList(SparseIndexEntry) = .empty;
    min_key_offset: usize = 0;
    min_key_len: usize = 0;
    max_key_offset: usize = 0;
    max_key_len: usize = 0;
    entry_count: u32 = 0;
    index_offset: usize = 0;
    level: u8 = 0;
    id: u64 = 0;

    /// Encode sorted `SstRecord` items into an immutable `SsTable` with sparse index and CRC32 footer.
    pub static fn encodeFromEntries(
        alloc: std.mem.Allocator,
        entries: []const SstRecord,
        level: u8,
        id: u64,
    ): SsTable ! LsmError {
        let buf: std.ArrayList(u8) = .empty;
        errdefer buf.deinit(alloc);

        let sparse: std.ArrayList(TempSparseEntry) = .empty;
        defer sparse.deinit(alloc);

        const count: u32 = u32(entries.len);
        appendU32LE(&buf, alloc, count)?;

        let idx: usize = 0;
        for (rec of entries) {
            if (rec.key.len > MAX_KEY_LEN) {
                return LsmError.KeyTooLarge;
            }
            if (idx % SPARSE_STRIDE == 0) {
                sparse.append(alloc, TempSparseEntry{
                    key: rec.key,
                    data_offset: buf.items.len,
                }) catch return LsmError.OutOfMemory;
            }
            const effective_val_len: u32 = if (rec.is_tombstone) 0 else u32(rec.value.len);
            appendU32LE(&buf, alloc, u32(rec.key.len))?;
            appendU32LE(&buf, alloc, effective_val_len)?;
            appendU64LE(&buf, alloc, rec.seq)?;
            const tomb_u8: u8 = if (rec.is_tombstone) 1 else 0;
            buf.append(alloc, tomb_u8) catch return LsmError.OutOfMemory;
            buf.appendSlice(alloc, rec.key) catch return LsmError.OutOfMemory;
            if (!rec.is_tombstone && rec.value.len > 0) {
                buf.appendSlice(alloc, rec.value) catch return LsmError.OutOfMemory;
            }
            idx += 1;
        }

        const index_offset: u64 = u64(buf.items.len);
        appendU32LE(&buf, alloc, u32(sparse.items.len))?;
        for (sp of sparse.items) {
            appendU32LE(&buf, alloc, u32(sp.key.len))?;
            appendU64LE(&buf, alloc, u64(sp.data_offset))?;
            buf.appendSlice(alloc, sp.key) catch return LsmError.OutOfMemory;
        }

        // Footer: [index_offset: u64][entry_count: u32][crc32: u32][magic: 4 bytes]
        appendU64LE(&buf, alloc, index_offset)?;
        appendU32LE(&buf, alloc, count)?;
        const crc = std.hash.Crc32.hash(buf.items);
        appendU32LE(&buf, alloc, crc)?;
        buf.appendSlice(alloc, SST_MAGIC[0..]) catch return LsmError.OutOfMemory;

        const owned = buf.toOwnedSlice(alloc) catch return LsmError.OutOfMemory;
        errdefer alloc.free(owned);

        let sst = SsTable.decodeOwned(alloc, owned, level, id)?;
        return sst;
    }

    /// Decode and verify an `SsTable` from raw serialized bytes (validates `"CPS1"` magic, CRC32, and block bounds).
    pub static fn decodeAndVerify(alloc: std.mem.Allocator, raw_bytes: []const u8): SsTable ! LsmError {
        const owned = alloc.dupe(u8, raw_bytes) catch return LsmError.OutOfMemory;
        errdefer alloc.free(owned);
        return SsTable.decodeOwned(alloc, owned, 0, 0);
    }

    static fn decodeOwned(
        alloc: std.mem.Allocator,
        owned: []u8,
        level: u8,
        id: u64,
    ): SsTable ! LsmError {
        if (owned.len < 4 + SST_FOOTER_SIZE) {
            return LsmError.CorruptSst;
        }

        const footer_pos = owned.len - SST_FOOTER_SIZE;
        if (!std.mem.eql(u8, owned[owned.len - 4 .. owned.len], SST_MAGIC[0..])) {
            return LsmError.CorruptSst;
        }

        const expected_crc = readU32LE(owned, owned.len - 8);
        const actual_crc = std.hash.Crc32.hash(owned[0 .. owned.len - 8]);
        if (actual_crc != expected_crc) {
            return LsmError.CorruptSst;
        }

        const idx_off_u64 = readU64LE(owned, footer_pos);
        const footer_count = readU32LE(owned, footer_pos + 8);
        if (idx_off_u64 < 4 || idx_off_u64 > u64(footer_pos)) {
            return LsmError.CorruptSst;
        }
        const index_offset = usize(idx_off_u64);

        const data_count = readU32LE(owned, 0);
        if (data_count != footer_count) {
            return LsmError.CorruptSst;
        }

        let min_off: usize = 0;
        let min_len: usize = 0;
        let max_off: usize = 0;
        let max_len: usize = 0;

        let pos: usize = 4;
        let prev_key: []const u8 = "";
        let i: u32 = 0;
        while (i < data_count) : (i += 1) {
            const rec = decodeRecordAt(owned, pos, index_offset)?;
            const k_off = pos + 17;
            if (i == 0) {
                min_off = k_off;
                min_len = rec.key.len;
            } else {
                if (std.mem.order(u8, prev_key, rec.key) != .lt) {
                    return LsmError.CorruptSst;
                }
            }
            max_off = k_off;
            max_len = rec.key.len;
            prev_key = rec.key;
            pos = rec.next_offset;
        }
        if (pos != index_offset) {
            return LsmError.CorruptSst;
        }

        if (index_offset + 4 > footer_pos) {
            return LsmError.CorruptSst;
        }
        const num_index = readU32LE(owned, index_offset);
        let index_entries: std.ArrayList(SparseIndexEntry) = .empty;
        errdefer index_entries.deinit(alloc);

        let ipos: usize = index_offset + 4;
        let j: u32 = 0;
        while (j < num_index) : (j += 1) {
            if (ipos + 12 > footer_pos) {
                return LsmError.CorruptSst;
            }
            const klen = usize(readU32LE(owned, ipos));
            const data_off = usize(readU64LE(owned, ipos + 4));
            const k_off = ipos + 12;
            if (k_off + klen > footer_pos) {
                return LsmError.CorruptSst;
            }
            if (data_off < 4 || data_off >= index_offset) {
                return LsmError.CorruptSst;
            }
            index_entries.append(alloc, SparseIndexEntry{
                key_offset: k_off,
                key_len: klen,
                data_offset: data_off,
            }) catch return LsmError.OutOfMemory;
            ipos = k_off + klen;
        }
        if (ipos != footer_pos) {
            return LsmError.CorruptSst;
        }

        return SsTable{
            alloc: alloc,
            data: owned,
            index_entries: index_entries,
            min_key_offset: min_off,
            min_key_len: min_len,
            max_key_offset: max_off,
            max_key_len: max_len,
            entry_count: data_count,
            index_offset: index_offset,
            level: level,
            id: id,
        };
    }

    /// Release the serialized byte image and sparse index storage.
    pub fn deinit(*_) {
        _.index_entries.deinit(_.alloc);
        if (_.data.len > 0) {
            _.alloc.free(_.data);
        }
        _.data = &.{};
        _.entry_count = 0;
    }

    /// Return the smallest key in this SSTable (empty string if empty).
    pub fn minKey(): string {
        if (_.entry_count == 0) return "";
        return _.data[_.min_key_offset .. _.min_key_offset + _.min_key_len];
    }

    /// Return the largest key in this SSTable (empty string if empty).
    pub fn maxKey(): string {
        if (_.entry_count == 0) return "";
        return _.data[_.max_key_offset .. _.max_key_offset + _.max_key_len];
    }

    /// Return the number of entries stored in this SSTable.
    pub fn len(): usize {
        return usize(_.entry_count);
    }

    /// Return the underlying serialized byte slice.
    pub fn rawBytes(): []const u8 {
        return _.data;
    }

    fn sparseKeyAt(idx: usize): []const u8 {
        const e = _.index_entries.items[idx];
        return _.data[e.key_offset .. e.key_offset + e.key_len];
    }

    /// Binary search the sparse index, then scan the target data block for `key`.
    pub fn get(key: string): SstLookup {
        if (_.entry_count == 0 || _.index_entries.items.len == 0) {
            return .NotFound;
        }
        if (std.mem.order(u8, key, _.minKey()) == .lt) {
            return .NotFound;
        }
        if (std.mem.order(u8, key, _.maxKey()) == .gt) {
            return .NotFound;
        }

        const block_idx = _.findBlockForKey(key);
        let pos = _.index_entries.items[block_idx].data_offset;
        const limit = if (block_idx + 1 < _.index_entries.items.len)
            _.index_entries.items[block_idx + 1].data_offset
        else
            _.index_offset;

        while (pos < limit) {
            const rec = decodeRecordAt(_.data, pos, limit) catch return .NotFound;
            const ord = std.mem.order(u8, rec.key, key);
            if (ord == .eq) {
                if (rec.is_tombstone) {
                    return .Tombstone;
                }
                return SstLookup{ .Value = rec.value };
            }
            if (ord == .gt) {
                return .NotFound;
            }
            pos = rec.next_offset;
        }
        return .NotFound;
    }

    fn findBlockForKey(key: string): usize {
        let lo: usize = 0;
        let hi: usize = _.index_entries.items.len;
        while (lo < hi) {
            const mid = lo + (hi - lo) / 2;
            const ord = std.mem.order(u8, _.sparseKeyAt(mid), key);
            if (ord != .gt) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        if (lo == 0) return 0;
        return lo - 1;
    }

    fn findStartOffsetForRange(start_key: string): usize {
        if (_.entry_count == 0 || _.index_entries.items.len == 0) {
            return _.index_offset;
        }
        if (std.mem.order(u8, start_key, _.maxKey()) == .gt) {
            return _.index_offset;
        }
        if (std.mem.order(u8, start_key, _.minKey()) != .gt) {
            return 4;
        }
        const block_idx = _.findBlockForKey(start_key);
        let pos = _.index_entries.items[block_idx].data_offset;
        while (pos < _.index_offset) {
            const rec = decodeRecordAt(_.data, pos, _.index_offset) catch return _.index_offset;
            if (std.mem.order(u8, rec.key, start_key) != .lt) {
                return pos;
            }
            pos = rec.next_offset;
        }
        return _.index_offset;
    }
}

fn searchMemTable(items: []const MemEntry, key: string): SearchPos {
    let lo: usize = 0;
    let hi: usize = items.len;
    while (lo < hi) {
        const mid = lo + (hi - lo) / 2;
        const ord = std.mem.order(u8, items[mid].key, key);
        if (ord == .lt) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    if (lo < items.len && std.mem.eql(u8, items[lo].key, key)) {
        return SearchPos{ found: true, idx: lo };
    }
    return SearchPos{ found: false, idx: lo };
}

fn advanceCursor(cursor: *MergeCursor, mem_items: []const MemEntry, end_key_opt: string?) {
    if (cursor.source_kind == 0) {
        if (cursor.mem_idx >= mem_items.len) {
            cursor.active = false;
            return;
        }
        const e = &mem_items[cursor.mem_idx];
        if (ek := end_key_opt) {
            if (std.mem.order(u8, e.key, ek) == .gt) {
                cursor.active = false;
                return;
            }
        }
        cursor.cur_key = e.key;
        cursor.cur_val = e.value;
        cursor.cur_seq = e.seq;
        cursor.cur_tombstone = e.is_tombstone;
        cursor.mem_idx += 1;
        cursor.active = true;
        return;
    }

    const sst = cursor.sst_ptr.?;
    if (cursor.sst_offset >= cursor.sst_end_offset) {
        cursor.active = false;
        return;
    }
    const rec = decodeRecordAt(sst.data, cursor.sst_offset, cursor.sst_end_offset) catch {
        cursor.active = false;
        return;
    };
    if (ek := end_key_opt) {
        if (std.mem.order(u8, rec.key, ek) == .gt) {
            cursor.active = false;
            return;
        }
    }
    cursor.cur_key = rec.key;
    cursor.cur_val = rec.value;
    cursor.cur_seq = rec.seq;
    cursor.cur_tombstone = rec.is_tombstone;
    cursor.sst_offset = rec.next_offset;
    cursor.active = true;
}

/// High-write-throughput LSM-Tree storage engine with ordered MemTable,
/// automatic flush to L0 SSTables, Tombstone deletes, rangeScan, and Compaction.
pub struct LsmEngine {
    alloc: std.mem.Allocator;
    memtable_max_bytes: usize;
    next_seq: u64 = 1;
    next_sst_id: u64 = 1;
    memtable: std.ArrayList(MemEntry) = .empty;
    memtable_bytes: usize = 0;
    l0_ssts: std.ArrayList(SsTable) = .empty;
    l1_ssts: std.ArrayList(SsTable) = .empty;
    compactions_run: usize = 0;

    /// Initialize an empty `LsmEngine` with the given MemTable byte threshold for auto-flush.
    pub static fn init(alloc: std.mem.Allocator, memtable_max_bytes: usize): LsmEngine {
        return LsmEngine{
            alloc: alloc,
            memtable_max_bytes: memtable_max_bytes,
            next_seq: 1,
            next_sst_id: 1,
            memtable: .empty,
            memtable_bytes: 0,
            l0_ssts: .empty,
            l1_ssts: .empty,
            compactions_run: 0,
        };
    }

    /// Release all MemTable entries and all L0/L1 SSTables.
    pub fn deinit(*_) {
        _.clearMemTable();
        _.memtable.deinit(_.alloc);

        for (*sst of _.l0_ssts.items) {
            sst.deinit();
        }
        _.l0_ssts.deinit(_.alloc);

        for (*sst of _.l1_ssts.items) {
            sst.deinit();
        }
        _.l1_ssts.deinit(_.alloc);
    }

    fn clearMemTable(*_) {
        for (e of _.memtable.items) {
            _.alloc.free(e.key);
            if (e.value.len > 0) {
                _.alloc.free(e.value);
            }
        }
        _.memtable.clearRetainingCapacity();
        _.memtable_bytes = 0;
    }

    /// Insert or overwrite a key-value pair. Automatically flushes MemTable when threshold is reached.
    pub fn put(*_, key: string, value: string): void ! LsmError {
        if (key.len > MAX_KEY_LEN) {
            return LsmError.KeyTooLarge;
        }
        const seq = _.next_seq;
        _.next_seq += 1;

        const pos = searchMemTable(_.memtable.items, key);
        if (pos.found) {
            let entry = *_.memtable.items[pos.idx];
            const new_val = if (value.len > 0)
                _.alloc.dupe(u8, value) catch return LsmError.OutOfMemory
            else
                &.{};
            const old_len = entry.value.len;
            if (old_len > 0) {
                _.alloc.free(entry.value);
            }
            entry.value = new_val;
            entry.seq = seq;
            entry.is_tombstone = false;
            _.memtable_bytes = _.memtable_bytes - old_len + new_val.len;
        } else {
            const new_key = _.alloc.dupe(u8, key) catch return LsmError.OutOfMemory;
            errdefer _.alloc.free(new_key);
            const new_val = if (value.len > 0)
                _.alloc.dupe(u8, value) catch return LsmError.OutOfMemory
            else
                &.{};
            errdefer if (new_val.len > 0) _.alloc.free(new_val);

            _.memtable.insert(_.alloc, pos.idx, MemEntry{
                key: new_key,
                value: new_val,
                seq: seq,
                is_tombstone: false,
            }) catch return LsmError.OutOfMemory;
            _.memtable_bytes += key.len + value.len + MEM_ENTRY_OVERHEAD;
        }

        if (_.memtable_bytes >= _.memtable_max_bytes) {
            _.flush()?;
        }
    }

    /// Write a Tombstone marker for `key`. Automatically flushes MemTable when threshold is reached.
    pub fn delete(*_, key: string): void ! LsmError {
        if (key.len > MAX_KEY_LEN) {
            return LsmError.KeyTooLarge;
        }
        const seq = _.next_seq;
        _.next_seq += 1;

        const pos = searchMemTable(_.memtable.items, key);
        if (pos.found) {
            let entry = *_.memtable.items[pos.idx];
            const old_len = entry.value.len;
            if (old_len > 0) {
                _.alloc.free(entry.value);
            }
            entry.value = &.{};
            entry.seq = seq;
            entry.is_tombstone = true;
            _.memtable_bytes -= old_len;
        } else {
            const new_key = _.alloc.dupe(u8, key) catch return LsmError.OutOfMemory;
            errdefer _.alloc.free(new_key);

            _.memtable.insert(_.alloc, pos.idx, MemEntry{
                key: new_key,
                value: &.{},
                seq: seq,
                is_tombstone: true,
            }) catch return LsmError.OutOfMemory;
            _.memtable_bytes += key.len + MEM_ENTRY_OVERHEAD;
        }

        if (_.memtable_bytes >= _.memtable_max_bytes) {
            _.flush()?;
        }
    }

    /// Point lookup across MemTable -> newest-to-oldest L0 SSTables -> L1 SSTables.
    /// Stops immediately on Tombstone so deleted keys never read stale older values.
    pub fn get(key: string): ([]const u8)? {
        const pos = searchMemTable(_.memtable.items, key);
        if (pos.found) {
            const e = &_.memtable.items[pos.idx];
            if (e.is_tombstone) return null;
            return e.value;
        }

        let i: usize = _.l0_ssts.items.len;
        while (i > 0) {
            i -= 1;
            const res = _.l0_ssts.items[i].get(key);
            switch (res) {
                .Value v => return v,
                .Tombstone => return null,
                .NotFound => {},
            }
        }

        for (sst of _.l1_ssts.items) {
            const res = sst.get(key);
            switch (res) {
                .Value v => return v,
                .Tombstone => return null,
                .NotFound => {},
            }
        }

        return null;
    }

    /// Flush the active MemTable into a new immutable L0 SSTable.
    pub fn flush(*_): void ! LsmError {
        if (_.memtable.items.len == 0) {
            return;
        }

        let recs: std.ArrayList(SstRecord) = .empty;
        defer recs.deinit(_.alloc);

        for (e of _.memtable.items) {
            recs.append(_.alloc, SstRecord{
                key: e.key,
                value: e.value,
                seq: e.seq,
                is_tombstone: e.is_tombstone,
            }) catch return LsmError.OutOfMemory;
        }

        const sst_id = _.next_sst_id;
        _.next_sst_id += 1;
        let sst = SsTable.encodeFromEntries(_.alloc, recs.items, 0, sst_id)?;
        errdefer sst.deinit();

        _.l0_ssts.append(_.alloc, sst) catch return LsmError.OutOfMemory;
        _.clearMemTable();
    }

    /// Multi-way sequence-ordered range scan over `[start_key, end_key]` inclusive.
    /// Caller owns the returned slice and must free it with `alloc.free(entries)`.
    pub fn rangeScan(
        alloc: std.mem.Allocator,
        start_key: string,
        end_key: string,
    ): []LsmEntry ! LsmError {
        if (std.mem.order(u8, start_key, end_key) == .gt) {
            return LsmError.InvalidRange;
        }

        let cursors: std.ArrayList(MergeCursor) = .empty;
        defer cursors.deinit(alloc);

        // 1. MemTable cursor
        const mem_pos = searchMemTable(_.memtable.items, start_key);
        let mem_c = MergeCursor{
            source_kind: 0,
            mem_idx: mem_pos.idx,
        };
        advanceCursor(&mem_c, _.memtable.items, end_key);
        if (mem_c.active) {
            cursors.append(alloc, mem_c) catch return LsmError.OutOfMemory;
        }

        // 2. L0 SSTables
        for (sst of _.l0_ssts.items) {
            if (sst.len() == 0) continue;
            if (std.mem.order(u8, sst.maxKey(), start_key) == .lt) continue;
            if (std.mem.order(u8, sst.minKey(), end_key) == .gt) continue;
            let c = MergeCursor{
                source_kind: 1,
                sst_ptr: &sst,
                sst_offset: sst.findStartOffsetForRange(start_key),
                sst_end_offset: sst.index_offset,
            };
            advanceCursor(&c, _.memtable.items, end_key);
            if (c.active) {
                cursors.append(alloc, c) catch return LsmError.OutOfMemory;
            }
        }

        // 3. L1 SSTables
        for (sst of _.l1_ssts.items) {
            if (sst.len() == 0) continue;
            if (std.mem.order(u8, sst.maxKey(), start_key) == .lt) continue;
            if (std.mem.order(u8, sst.minKey(), end_key) == .gt) continue;
            let c = MergeCursor{
                source_kind: 1,
                sst_ptr: &sst,
                sst_offset: sst.findStartOffsetForRange(start_key),
                sst_end_offset: sst.index_offset,
            };
            advanceCursor(&c, _.memtable.items, end_key);
            if (c.active) {
                cursors.append(alloc, c) catch return LsmError.OutOfMemory;
            }
        }

        let out: std.ArrayList(LsmEntry) = .empty;
        errdefer out.deinit(alloc);

        while (true) {
            let min_key_opt: ([]const u8)? = null;
            for (c of cursors.items) {
                if (!c.active) continue;
                if (mk := min_key_opt) {
                    if (std.mem.order(u8, c.cur_key, mk) == .lt) {
                        min_key_opt = c.cur_key;
                    }
                } else {
                    min_key_opt = c.cur_key;
                }
            }

            if (min_key_opt == null) {
                break;
            }
            const min_key = min_key_opt.?;

            let best_seq: u64 = 0;
            let best_val: []const u8 = "";
            let best_tombstone: bool = false;
            let has_best: bool = false;

            for (*c of cursors.items) {
                if (!c.active) continue;
                if (std.mem.eql(u8, c.cur_key, min_key)) {
                    if (!has_best || c.cur_seq > best_seq) {
                        best_seq = c.cur_seq;
                        best_val = c.cur_val;
                        best_tombstone = c.cur_tombstone;
                        has_best = true;
                    }
                    advanceCursor(c, _.memtable.items, end_key);
                }
            }

            if (has_best && !best_tombstone) {
                out.append(alloc, LsmEntry{
                    key: min_key,
                    value: best_val,
                }) catch return LsmError.OutOfMemory;
            }
        }

        return out.toOwnedSlice(alloc) catch return LsmError.OutOfMemory;
    }

    /// Full multi-way compaction across all L0 and L1 SSTables.
    /// Keeps only the highest-seq version of each key and drops bottom-level Tombstones.
    pub fn compact(*_): void ! LsmError {
        if (_.l0_ssts.items.len == 0 && _.l1_ssts.items.len == 0) {
            return;
        }

        let cursors: std.ArrayList(MergeCursor) = .empty;
        defer cursors.deinit(_.alloc);

        for (*sst of _.l0_ssts.items) {
            if (sst.len() == 0) continue;
            let c = MergeCursor{
                source_kind: 1,
                sst_ptr: sst,
                sst_offset: 4,
                sst_end_offset: sst.index_offset,
            };
            advanceCursor(&c, _.memtable.items, null);
            if (c.active) {
                cursors.append(_.alloc, c) catch return LsmError.OutOfMemory;
            }
        }

        for (*sst of _.l1_ssts.items) {
            if (sst.len() == 0) continue;
            let c = MergeCursor{
                source_kind: 1,
                sst_ptr: sst,
                sst_offset: 4,
                sst_end_offset: sst.index_offset,
            };
            advanceCursor(&c, _.memtable.items, null);
            if (c.active) {
                cursors.append(_.alloc, c) catch return LsmError.OutOfMemory;
            }
        }

        let merged: std.ArrayList(SstRecord) = .empty;
        defer merged.deinit(_.alloc);

        while (true) {
            let min_key_opt: ([]const u8)? = null;
            for (c of cursors.items) {
                if (!c.active) continue;
                if (mk := min_key_opt) {
                    if (std.mem.order(u8, c.cur_key, mk) == .lt) {
                        min_key_opt = c.cur_key;
                    }
                } else {
                    min_key_opt = c.cur_key;
                }
            }

            if (min_key_opt == null) {
                break;
            }
            const min_key = min_key_opt.?;

            let best_seq: u64 = 0;
            let best_val: []const u8 = "";
            let best_tombstone: bool = false;
            let has_best: bool = false;

            for (*c of cursors.items) {
                if (!c.active) continue;
                if (std.mem.eql(u8, c.cur_key, min_key)) {
                    if (!has_best || c.cur_seq > best_seq) {
                        best_seq = c.cur_seq;
                        best_val = c.cur_val;
                        best_tombstone = c.cur_tombstone;
                        has_best = true;
                    }
                    advanceCursor(c, _.memtable.items, null);
                }
            }

            if (has_best && !best_tombstone) {
                merged.append(_.alloc, SstRecord{
                    key: min_key,
                    value: best_val,
                    seq: best_seq,
                    is_tombstone: false,
                }) catch return LsmError.OutOfMemory;
            }
        }

        let new_l1: std.ArrayList(SsTable) = .empty;
        errdefer {
            for (*s of new_l1.items) s.deinit();
            new_l1.deinit(_.alloc);
        }

        if (merged.items.len > 0) {
            const sst_id = _.next_sst_id;
            _.next_sst_id += 1;
            let compacted = SsTable.encodeFromEntries(_.alloc, merged.items, 1, sst_id)?;
            errdefer compacted.deinit();
            new_l1.append(_.alloc, compacted) catch return LsmError.OutOfMemory;
        }

        for (*sst of _.l0_ssts.items) {
            sst.deinit();
        }
        _.l0_ssts.clearRetainingCapacity();

        for (*sst of _.l1_ssts.items) {
            sst.deinit();
        }
        _.l1_ssts.deinit(_.alloc);
        _.l1_ssts = new_l1;
        _.compactions_run += 1;
    }

    /// Return current engine statistics.
    pub fn stats(): LsmStats {
        let total_sst_entries: usize = 0;
        for (sst of _.l0_ssts.items) {
            total_sst_entries += sst.len();
        }
        for (sst of _.l1_ssts.items) {
            total_sst_entries += sst.len();
        }
        return LsmStats{
            memtable_entries: _.memtable.items.len,
            memtable_bytes: _.memtable_bytes,
            sst_count: _.l0_ssts.items.len + _.l1_ssts.items.len,
            total_sst_entries: total_sst_entries,
            compactions_run: _.compactions_run,
        };
    }
}
