// Source: cp/lib/lsm_sst/block.cp (Prefix-compressed Data Block & Restart Points)
const std = @import("std");

pub const SstError = error{
    UnsortedKeys,
    CorruptedChecksum,
    InvalidFooter,
    InvalidBlock,
    BufferTooSmall,
    OutOfMemory,
};

pub struct BlockHandle {
    pub offset: u64,
    pub size: u64,

    pub fn encode(out: *[16]u8) {
        std.mem.writeInt(u64, out[0..8], _.offset, .little);
        std.mem.writeInt(u64, out[8..16], _.size, .little);
    }

    pub fn decode(data: []const u8): SstError!BlockHandle {
        if (data.len < 16) {
            return SstError.InvalidBlock;
        }
        return BlockHandle{
            .offset = std.mem.readInt(u64, data[0..8], .little),
            .size = std.mem.readInt(u64, data[8..16], .little),
        };
    }
}

fn appendVarint32(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), val: u32): !void {
    let v = val;
    while (v >= 0x80) {
        try buf.append(alloc, @as(u8, @intCast((v & 0x7F) | 0x80)));
        v >>= 7;
    }
    try buf.append(alloc, @as(u8, @intCast(v)));
}

fn readVarint32(data: []const u8, pos: *usize): SstError!u32 {
    let result: u32 = 0;
    let shift: u5 = 0;
    let i: usize = 0;
    while (i < 5) {
        if (pos.* >= data.len) {
            return SstError.InvalidBlock;
        }
        const b = data[pos.*];
        pos.* += 1;
        result |= @as(u32, b & 0x7F) << shift;
        if ((b & 0x80) == 0) {
            return result;
        }
        if (shift >= 28) {
            return SstError.InvalidBlock;
        }
        shift += 7;
        i += 1;
    }
    return SstError.InvalidBlock;
}

pub struct BlockBuilder {
    alloc: std.mem.Allocator,
    restart_interval: u32,
    buf: std.ArrayList(u8),
    restarts: std.ArrayList(u32),
    counter: u32,
    last_key: std.ArrayList(u8),
    finished: bool,

    pub fn init(alloc: std.mem.Allocator, restart_interval: u32): BlockBuilder {
        const interval: u32 = if (restart_interval == 0) 16 else restart_interval;
        return BlockBuilder{
            .alloc = alloc,
            .restart_interval = interval,
            .buf = std.ArrayList(u8).empty,
            .restarts = std.ArrayList(u32).empty,
            .counter = interval,
            .last_key = std.ArrayList(u8).empty,
            .finished = false,
        };
    }

    pub fn deinit(*_) {
        _.buf.deinit(_.alloc);
        _.restarts.deinit(_.alloc);
        _.last_key.deinit(_.alloc);
    }

    pub fn reset(*_) {
        _.buf.clearRetainingCapacity();
        _.restarts.clearRetainingCapacity();
        _.last_key.clearRetainingCapacity();
        _.counter = _.restart_interval;
        _.finished = false;
    }

    pub fn isEmpty(): bool {
        return _.buf.items.len == 0;
    }

    pub fn currentSizeEstimate(): usize {
        return _.buf.items.len + _.restarts.items.len * 4 + 4;
    }

    pub fn add(*_, key: []const u8, value: []const u8): !void {
        let shared: usize = 0;
        if (_.counter < _.restart_interval) {
            const min_len = @min(_.last_key.items.len, key.len);
            while (shared < min_len and _.last_key.items[shared] == key[shared]) {
                shared += 1;
            }
        } else {
            try _.restarts.append(_.alloc, @as(u32, @intCast(_.buf.items.len)));
            _.counter = 0;
        }

        const non_shared = key.len - shared;
        try appendVarint32(_.alloc, &_.buf, @as(u32, @intCast(shared)));
        try appendVarint32(_.alloc, &_.buf, @as(u32, @intCast(non_shared)));
        try appendVarint32(_.alloc, &_.buf, @as(u32, @intCast(value.len)));

        try _.buf.appendSlice(_.alloc, key[shared..]);
        try _.buf.appendSlice(_.alloc, value);

        _.last_key.clearRetainingCapacity();
        try _.last_key.appendSlice(_.alloc, key);
        _.counter += 1;
    }

    pub fn finish(*_): ![]const u8 {
        if (!_.finished) {
            let tmp: [4]u8 = undefined;
            for (r of _.restarts.items) {
                std.mem.writeInt(u32, &tmp, r, .little);
                try _.buf.appendSlice(_.alloc, &tmp);
            }
            std.mem.writeInt(u32, &tmp, @as(u32, @intCast(_.restarts.items.len)), .little);
            try _.buf.appendSlice(_.alloc, &tmp);
            _.finished = true;
        }
        return _.buf.items;
    }
}

pub struct BlockReader {
    data: []const u8,
    restart_offset: usize,
    pub num_restarts: u32,

    pub fn init(data: []const u8): SstError!BlockReader {
        if (data.len < 4) {
            return SstError.InvalidBlock;
        }
        const num_restarts = std.mem.readInt(u32, data[data.len - 4 .. data.len][0..4], .little);
        if (num_restarts == 0) {
            return SstError.InvalidBlock;
        }
        const restarts_bytes: usize = @as(usize, num_restarts) * 4;
        if (restarts_bytes + 4 > data.len) {
            return SstError.InvalidBlock;
        }
        return BlockReader{
            .data = data,
            .restart_offset = data.len - 4 - restarts_bytes,
            .num_restarts = num_restarts,
        };
    }

    pub fn getRestartPoint(idx: u32): u32 {
        const off = _.restart_offset + @as(usize, idx) * 4;
        return std.mem.readInt(u32, _.data[off .. off + 4][0..4], .little);
    }

    pub fn iter(alloc: std.mem.Allocator): BlockIter {
        return BlockIter{
            .alloc = alloc,
            .reader = BlockReader{
                .data = _.data,
                .restart_offset = _.restart_offset,
                .num_restarts = _.num_restarts,
            },
            .current_offset = _.restart_offset,
            .key_buf = std.ArrayList(u8).empty,
            .val_slice = "",
            .valid = false,
        };
    }
}

pub struct BlockIter {
    alloc: std.mem.Allocator,
    reader: BlockReader,
    current_offset: usize,
    key_buf: std.ArrayList(u8),
    val_slice: []const u8,
    valid: bool,

    pub fn deinit(*_) {
        _.key_buf.deinit(_.alloc);
    }

    pub fn isValid(): bool {
        return _.valid;
    }

    pub fn key(): []const u8 {
        return _.key_buf.items;
    }

    pub fn value(): []const u8 {
        return _.val_slice;
    }

    fn parseEntryAt(*_, offset: usize): !bool {
        if (offset >= _.reader.restart_offset) {
            _.valid = false;
            _.current_offset = _.reader.restart_offset;
            return false;
        }
        let pos = offset;
        const shared = try readVarint32(_.reader.data, &pos);
        const non_shared = try readVarint32(_.reader.data, &pos);
        const val_len = try readVarint32(_.reader.data, &pos);

        if (shared > _.key_buf.items.len) {
            return SstError.InvalidBlock;
        }
        const total_needed = @as(usize, non_shared) + @as(usize, val_len);
        if (pos + total_needed > _.reader.restart_offset) {
            return SstError.InvalidBlock;
        }

        _.key_buf.shrinkRetainingCapacity(shared);
        try _.key_buf.appendSlice(_.alloc, _.reader.data[pos .. pos + non_shared]);
        pos += non_shared;
        _.val_slice = _.reader.data[pos .. pos + val_len];
        pos += val_len;
        _.current_offset = pos;
        _.valid = true;
        return true;
    }

    pub fn seekToFirst(*_): !void {
        _.key_buf.clearRetainingCapacity();
        const start_off: usize = @as(usize, _.reader.getRestartPoint(0));
        * = try _.parseEntryAt(start_off);
    }

    pub fn next(*_): !bool {
        if (!_.valid) {
            return false;
        }
        return try _.parseEntryAt(_.current_offset);
    }

    pub fn seek(*_, target: []const u8): !bool {
        let left: u32 = 0;
        let right: u32 = _.reader.num_restarts - 1;

        while (left < right) {
            const mid = left + (right - left + 1) / 2;
            const region_off: usize = @as(usize, _.reader.getRestartPoint(mid));
            let pos = region_off;
            const shared = try readVarint32(_.reader.data, &pos);
            if (shared != 0) {
                return SstError.InvalidBlock;
            }
            const non_shared = try readVarint32(_.reader.data, &pos);
            * = try readVarint32(_.reader.data, &pos);
            if (pos + non_shared > _.reader.restart_offset) {
                return SstError.InvalidBlock;
            }
            const mid_key = _.reader.data[pos .. pos + non_shared];
            if (std.mem.order(u8, mid_key, target) == .lt) {
                left = mid;
            } else {
                right = mid - 1;
            }
        }

        _.key_buf.clearRetainingCapacity();
        const start_off: usize = @as(usize, _.reader.getRestartPoint(left));
        if (!try _.parseEntryAt(start_off)) {
            return false;
        }

        while (_.valid) {
            if (std.mem.order(u8, _.key_buf.items, target) != .lt) {
                return true;
            }
            if (!try _.next()) {
                return false;
            }
        }
        return false;
    }
}
