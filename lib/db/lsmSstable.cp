// Source: cp/lib/lsm_sst/sstable.cp (SSTable Builder, Reader, CRC32C Trailer & Multi-way Merge Iterator)
const std = @import("std");
import { SstError, BlockHandle, BlockBuilder, BlockReader, BlockIter } from "./lsmBlock.cp";
import { BloomFilter } from "./lsmBloom.cp";

pub const SST_MAGIC: []const u8 = "CPSST001";
pub const SST_FOOTER_SIZE: usize = 48;
pub const BLOCK_TRAILER_SIZE: usize = 5;

const VALUE_TAG_DATA: u8 = 0x00;
const VALUE_TAG_TOMBSTONE: u8 = 0x01;

pub struct SstOptions {
    pub block_size: usize = 4096,
    pub restart_interval: u32 = 16,
    pub bloom_bits_per_key: u32 = 10,
}

pub struct SstEntry {
    pub key: []const u8,
    pub value: []const u8,
    pub is_tombstone: bool,
}

pub struct SstOwnedEntry {
    pub key: []u8,
    pub value: []u8,
    pub is_tombstone: bool,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.key);
        alloc.free(_.value);
    }
}

pub struct SstGetResult {
    pub value: []const u8,
    pub is_tombstone: bool,
}

fn computeBlockCrc(payload: []const u8, block_type: u8): u32 {
    let hasher = std.hash.crc.Crc32Iscsi.init();
    hasher.update(payload);
    const t: [1]u8 = .{block_type};
    hasher.update(&t);
    return hasher.final();
}

pub struct SstBuilder {
    alloc: std.mem.Allocator,
    opts: SstOptions,
    out: std.ArrayList(u8),
    data_block: BlockBuilder,
    index_block: BlockBuilder,
    keys: std.ArrayList([]u8),
    last_key: std.ArrayList(u8),
    has_last_key: bool,
    pending_index_entry: bool,
    pending_handle: BlockHandle,
    entry_count: u64,

    pub static fn init(alloc: std.mem.Allocator, opts: SstOptions): SstBuilder {
        return SstBuilder{
            .alloc = alloc,
            .opts = opts,
            .out = std.ArrayList(u8).empty,
            .data_block = BlockBuilder.init(alloc, opts.restart_interval),
            .index_block = BlockBuilder.init(alloc, 1),
            .keys = std.ArrayList([]u8).empty,
            .last_key = std.ArrayList(u8).empty,
            .has_last_key = false,
            .pending_index_entry = false,
            .pending_handle = BlockHandle{ .offset = 0, .size = 0 },
            .entry_count = 0,
        };
    }

    pub fn deinit(*_) {
        _.out.deinit(_.alloc);
        _.data_block.deinit();
        _.index_block.deinit();
        for (k of _.keys.items) {
            _.alloc.free(k);
        }
        _.keys.deinit(_.alloc);
        _.last_key.deinit(_.alloc);
    }

    fn writeRawBlock(*_, payload: []const u8): !BlockHandle {
        const offset: u64 = @as(u64, @intCast(_.out.items.len));
        const size: u64 = @as(u64, @intCast(payload.len));
        try _.out.appendSlice(_.alloc, payload);
        try _.out.append(_.alloc, 0);
        const crc = computeBlockCrc(payload, 0);
        let crc_buf: [4]u8 = undefined;
        std.mem.writeInt(u32, &crc_buf, crc, .little);
        try _.out.appendSlice(_.alloc, &crc_buf);
        return BlockHandle{ .offset = offset, .size = size };
    }

    fn flushDataBlock(*_): !void {
        if (_.data_block.isEmpty()) {
            return;
        }
        const raw = try _.data_block.finish();
        _.pending_handle = try _.writeRawBlock(raw);
        _.pending_index_entry = true;
        _.data_block.reset();
    }

    fn addInternal(*_, key: []const u8, value: []const u8, is_tombstone: bool): !void {
        if (_.has_last_key) {
            if (std.mem.order(u8, _.last_key.items, key) != .lt) {
                return SstError.UnsortedKeys;
            }
        }
        if (_.pending_index_entry) {
            let handle_buf: [16]u8 = undefined;
            _.pending_handle.encode(&handle_buf);
            try _.index_block.add(_.last_key.items, &handle_buf);
            _.pending_index_entry = false;
        }

        const key_copy = try _.alloc.dupe(u8, key);
        errdefer _.alloc.free(key_copy);
        try _.keys.append(_.alloc, key_copy);

        let encoded = try _.alloc.alloc(u8, 1 + value.len);
        defer _.alloc.free(encoded);
        encoded[0] = if (is_tombstone) VALUE_TAG_TOMBSTONE else VALUE_TAG_DATA;
        if (value.len > 0) {
            @memcpy(encoded[1..], value);
        }

        try _.data_block.add(key, encoded);
        _.last_key.clearRetainingCapacity();
        try _.last_key.appendSlice(_.alloc, key);
        _.has_last_key = true;
        _.entry_count += 1;

        if (_.data_block.currentSizeEstimate() >= _.opts.block_size) {
            try _.flushDataBlock();
        }
    }

    pub fn put(*_, key: []const u8, value: []const u8): !void {
        try _.addInternal(key, value, false);
    }

    pub fn delete(*_, key: []const u8): !void {
        try _.addInternal(key, "", true);
    }

    pub fn finish(*_): ![]u8 {
        if (!_.data_block.isEmpty()) {
            try _.flushDataBlock();
        }
        if (_.pending_index_entry) {
            let handle_buf: [16]u8 = undefined;
            _.pending_handle.encode(&handle_buf);
            try _.index_block.add(_.last_key.items, &handle_buf);
            _.pending_index_entry = false;
        }

        // Write Bloom Filter Block
        const bloom_bytes = try BloomFilter.buildFromMutKeys(_.alloc, _.keys.items, _.opts.bloom_bits_per_key);
        defer _.alloc.free(bloom_bytes);
        const bloom_handle = try _.writeRawBlock(bloom_bytes);

        const index_bytes = try _.index_block.finish();
        const index_handle = try _.writeRawBlock(index_bytes);

        let footer: [SST_FOOTER_SIZE]u8 = undefined;
        @memset(&footer, 0);
        bloom_handle.encode(footer[0..16]);
        index_handle.encode(footer[16..32]);
        std.mem.writeInt(u64, footer[32..40], _.entry_count, .little);
        @memcpy(footer[40..48], SST_MAGIC);
        try _.out.appendSlice(_.alloc, &footer);

        return try _.out.toOwnedSlice(_.alloc);
    }
}

pub struct SstReader {
    alloc: std.mem.Allocator,
    data: []const u8,
    bloom_handle: BlockHandle,
    index_handle: BlockHandle,
    pub entry_count: u64,
    bloom_slice: []const u8,
    index_slice: []const u8,

    static fn verifyBlockSlice(data: []const u8, handle: BlockHandle): SstError![]const u8 {
        const off: usize = @as(usize, @intCast(handle.offset));
        const sz: usize = @as(usize, @intCast(handle.size));
        if (off > data.len or sz + BLOCK_TRAILER_SIZE > data.len - off) {
            return SstError.InvalidBlock;
        }
        const payload = data[off .. off + sz];
        const block_type = data[off + sz];
        const expected_crc = std.mem.readInt(u32, data[off + sz + 1 .. off + sz + 5][0..4], .little);
        const actual_crc = computeBlockCrc(payload, block_type);
        if (actual_crc != expected_crc) {
            return SstError.CorruptedChecksum;
        }
        return payload;
    }

    pub static fn init(alloc: std.mem.Allocator, data: []const u8): SstError!SstReader {
        if (data.len < SST_FOOTER_SIZE) {
            return SstError.InvalidFooter;
        }
        const footer = data[data.len - SST_FOOTER_SIZE .. data.len];
        if (!std.mem.eql(u8, footer[40..48], SST_MAGIC)) {
            return SstError.InvalidFooter;
        }
        const bloom_handle = try BlockHandle.decode(footer[0..16]);
        const index_handle = try BlockHandle.decode(footer[16..32]);
        const entry_count = std.mem.readInt(u64, footer[32..40][0..8], .little);

        const bloom_slice = try verifyBlockSlice(data, bloom_handle);
        const index_slice = try verifyBlockSlice(data, index_handle);
        * = try BlockReader.init(index_slice);

        return SstReader{
            .alloc = alloc,
            .data = data,
            .bloom_handle = bloom_handle,
            .index_handle = index_handle,
            .entry_count = entry_count,
            .bloom_slice = bloom_slice,
            .index_slice = index_slice,
        };
    }

    pub fn deinit(*_) {
    }

    pub fn get(key: []const u8): !SstGetResult? {
        if (!BloomFilter.mayContain(_.bloom_slice, key)) {
            return null;
        }
        const idx_reader = try BlockReader.init(_.index_slice);
        let idx_iter = idx_reader.iter(_.alloc);
        defer idx_iter.deinit();

        if (!try idx_iter.seek(key)) {
            return null;
        }
        const data_handle = try BlockHandle.decode(idx_iter.value());
        const data_payload = try verifyBlockSlice(_.data, data_handle);
        const data_reader = try BlockReader.init(data_payload);
        let data_iter = data_reader.iter(_.alloc);
        defer data_iter.deinit();

        if (!try data_iter.seek(key)) {
            return null;
        }
        if (!std.mem.eql(u8, data_iter.key(), key)) {
            return null;
        }
        const raw_val = data_iter.value();
        if (raw_val.len == 0) {
            return SstError.InvalidBlock;
        }
        return SstGetResult{
            .value = raw_val[1..],
            .is_tombstone = raw_val[0] == VALUE_TAG_TOMBSTONE,
        };
    }

    pub fn rangeScan(
        alloc: std.mem.Allocator,
        start: ([]const u8)?,
        end: ([]const u8)?,
    ): ![]SstOwnedEntry {
        let results = std.ArrayList(SstOwnedEntry).empty;
        errdefer {
            for (item of results.items) {
                alloc.free(item.key);
                alloc.free(item.value);
            }
            results.deinit(alloc);
        }

        const idx_reader = try BlockReader.init(_.index_slice);
        let idx_iter = idx_reader.iter(alloc);
        defer idx_iter.deinit();

        if (start != null) {
            if (!try idx_iter.seek(start.?)) {
                return try results.toOwnedSlice(alloc);
            }
        } else {
            try idx_iter.seekToFirst();
        }

        let first_block = true;
        while (idx_iter.isValid()) {
            const data_handle = try BlockHandle.decode(idx_iter.value());
            const data_payload = try verifyBlockSlice(_.data, data_handle);
            const data_reader = try BlockReader.init(data_payload);
            let data_iter = data_reader.iter(alloc);
            defer data_iter.deinit();

            if (first_block and start != null) {
                * = try data_iter.seek(start.?);
            } else {
                try data_iter.seekToFirst();
            }
            first_block = false;

            while (data_iter.isValid()) {
                const k = data_iter.key();
                if (end != null) {
                    if (std.mem.order(u8, k, end.?) != .lt) {
                        return try results.toOwnedSlice(alloc);
                    }
                }
                const raw_val = data_iter.value();
                if (raw_val.len == 0) {
                    return SstError.InvalidBlock;
                }
                if (raw_val[0] != VALUE_TAG_TOMBSTONE) {
                    const k_dup = try alloc.dupe(u8, k);
                    errdefer alloc.free(k_dup);
                    const v_dup = try alloc.dupe(u8, raw_val[1..]);
                    errdefer alloc.free(v_dup);
                    try results.append(alloc, SstOwnedEntry{
                        .key = k_dup,
                        .value = v_dup,
                        .is_tombstone = false,
                    });
                }
                * = try data_iter.next();
            }
            * = try idx_iter.next();
        }
        return try results.toOwnedSlice(alloc);
    }
}

struct ReaderCursor {
    alloc: std.mem.Allocator,
    reader: *const SstReader,
    idx_iter: BlockIter,
    data_iter: BlockIter?,
    valid: bool,

    pub static fn init(alloc: std.mem.Allocator, reader: *const SstReader): !ReaderCursor {
        const idx_reader = try BlockReader.init(reader.index_slice);
        return ReaderCursor{
            .alloc = alloc,
            .reader = reader,
            .idx_iter = idx_reader.iter(alloc),
            .data_iter = null,
            .valid = false,
        };
    }

    pub fn deinit(*_) {
        if (_.data_iter != null) {
            _.data_iter.?.deinit();
        }
        _.idx_iter.deinit();
    }

    fn loadDataBlock(*_): !bool {
        if (_.data_iter != null) {
            _.data_iter.?.deinit();
            _.data_iter = null;
        }
        if (!_.idx_iter.isValid()) {
            _.valid = false;
            return false;
        }
        const handle = try BlockHandle.decode(_.idx_iter.value());
        const payload = try SstReader.verifyBlockSlice(_.reader.data, handle);
        const dr = try BlockReader.init(payload);
        _.data_iter = dr.iter(_.alloc);
        return true;
    }

    pub fn seekToFirst(*_): !void {
        try _.idx_iter.seekToFirst();
        if (!try _.loadDataBlock()) {
            return;
        }
        try _.data_iter.?.seekToFirst();
        _.valid = _.data_iter.?.isValid();
    }

    pub fn seek(*_, target: []const u8): !void {
        if (!try _.idx_iter.seek(target)) {
            _.valid = false;
            return;
        }
        if (!try _.loadDataBlock()) {
            return;
        }
        * = try _.data_iter.?.seek(target);
        while (!_.data_iter.?.isValid()) {
            if (!try _.idx_iter.next()) {
                _.valid = false;
                return;
            }
            if (!try _.loadDataBlock()) {
                return;
            }
            try _.data_iter.?.seekToFirst();
        }
        _.valid = true;
    }

    pub fn next(*_): !void {
        if (!_.valid) {
            return;
        }
        if (try _.data_iter.?.next()) {
            return;
        }
        while (true) {
            if (!try _.idx_iter.next()) {
                _.valid = false;
                return;
            }
            if (!try _.loadDataBlock()) {
                return;
            }
            try _.data_iter.?.seekToFirst();
            if (_.data_iter.?.isValid()) {
                _.valid = true;
                return;
            }
        }
    }

    pub fn key(): []const u8 {
        return _.data_iter.?.key();
    }

    pub fn rawValue(): []const u8 {
        return _.data_iter.?.value();
    }
}

pub struct SstMergeIter {
    alloc: std.mem.Allocator,
    cursors: []ReaderCursor,
    cur_key: std.ArrayList(u8),
    cur_val: std.ArrayList(u8),
    valid: bool,

    pub static fn init(alloc: std.mem.Allocator, readers: []const *const SstReader): !SstMergeIter {
        let cursors = try alloc.alloc(ReaderCursor, readers.len);
        let initialized: usize = 0;
        errdefer {
            let j: usize = 0;
            while (j < initialized) {
                cursors[j].deinit();
                j += 1;
            }
            alloc.free(cursors);
        }
        while (initialized < readers.len) {
            cursors[initialized] = try ReaderCursor.init(alloc, readers[initialized]);
            initialized += 1;
        }
        return SstMergeIter{
            .alloc = alloc,
            .cursors = cursors,
            .cur_key = std.ArrayList(u8).empty,
            .cur_val = std.ArrayList(u8).empty,
            .valid = false,
        };
    }

    pub fn deinit(*_) {
        for (c of _.cursors) {
            c.deinit();
        }
        _.alloc.free(_.cursors);
        _.cur_key.deinit(_.alloc);
        _.cur_val.deinit(_.alloc);
    }

    pub fn isValid(): bool {
        return _.valid;
    }

    pub fn key(): []const u8 {
        return _.cur_key.items;
    }

    pub fn value(): []const u8 {
        return _.cur_val.items;
    }

    fn advanceToNextVisible(*_): !bool {
        while (true) {
            let min_idx: usize? = null;
            let i: usize = 0;
            while (i < _.cursors.len) {
                if (!_.cursors[i].valid) {
                    i += 1;
                    continue;
                }
                if (min_idx == null) {
                    min_idx = i;
                } else {
                    const ord = std.mem.order(u8, _.cursors[i].key(), _.cursors[min_idx.?].key());
                    if (ord == .lt) {
                        min_idx = i;
                    }
                }
                i += 1;
            }

            if (min_idx == null) {
                _.valid = false;
                return false;
            }

            const winner = min_idx.?;
            _.cur_key.clearRetainingCapacity();
            try _.cur_key.appendSlice(_.alloc, _.cursors[winner].key());

            const raw = _.cursors[winner].rawValue();
            if (raw.len == 0) {
                return SstError.InvalidBlock;
            }
            const is_tombstone = raw[0] == VALUE_TAG_TOMBSTONE;
            if (!is_tombstone) {
                _.cur_val.clearRetainingCapacity();
                try _.cur_val.appendSlice(_.alloc, raw[1..]);
            }

            let j: usize = 0;
            while (j < _.cursors.len) {
                if (_.cursors[j].valid and std.mem.eql(u8, _.cursors[j].key(), _.cur_key.items)) {
                    try _.cursors[j].next();
                }
                j += 1;
            }

            if (!is_tombstone) {
                _.valid = true;
                return true;
            }
        }
    }

    pub fn seekToFirst(*_): !bool {
        for (c of _.cursors) {
            try c.seekToFirst();
        }
        return try _.advanceToNextVisible();
    }

    pub fn seek(*_, target: []const u8): !bool {
        for (c of _.cursors) {
            try c.seek(target);
        }
        return try _.advanceToNextVisible();
    }

    pub fn next(*_): !bool {
        if (!_.valid) {
            return false;
        }
        return try _.advanceToNextVisible();
    }
}
