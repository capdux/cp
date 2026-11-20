// Source: cp/lib/grpc_wire/frame.cp (gRPC Length-Prefixed Message 5-byte frame codec and streaming framer)

const std = @import("std");

pub const DEFAULT_MAX_MESSAGE_BYTES: usize = 4 * 1024 * 1024;
pub const GRPC_FRAME_HEADER_LEN: usize = 5;

pub struct GrpcFrame {
    pub compressed: bool,
    pub payload: []u8,

    pub fn deinit(alloc: std.mem.Allocator) {
        alloc.free(_.payload);
    }
}

pub fn encodeGrpcFrame(alloc: std.mem.Allocator, compressed: bool, payload: []const u8): ![]u8 {
    if (payload.len > std.math.maxInt(u32)) {
        return error.MessageTooLarge;
    }
    const total_len = GRPC_FRAME_HEADER_LEN + payload.len;
    let out = try alloc.alloc(u8, total_len);
    out[0] = if (compressed) @as(u8, 1) else @as(u8, 0);
    std.mem.writeInt(u32, out[1..5], @intCast(payload.len), .big);
    if (payload.len > 0) {
        @memcpy(out[5..], payload);
    }
    return out;
}

pub fn decodeGrpcFrame(alloc: std.mem.Allocator, data: []const u8, max_message_bytes: usize): !GrpcFrame {
    if (data.len < GRPC_FRAME_HEADER_LEN) {
        return error.TruncatedFrame;
    }
    const flag = data[0];
    if (flag > 1) {
        return error.InvalidCompressedFlag;
    }
    const msg_len: usize = @intCast(std.mem.readInt(u32, data[1..5], .big));
    if (msg_len > max_message_bytes) {
        return error.MessageTooLarge;
    }
    if (data.len < GRPC_FRAME_HEADER_LEN + msg_len) {
        return error.TruncatedFrame;
    }
    const payload = try alloc.alloc(u8, msg_len);
    if (msg_len > 0) {
        @memcpy(payload, data[5 .. 5 + msg_len]);
    }
    return GrpcFrame{
        .compressed = flag == 1,
        .payload = payload,
    };
}

pub struct GrpcStreamFramer {
    buf: std.ArrayList(u8),
    max_message_bytes: usize,

    pub fn init(max_message_bytes: usize): GrpcStreamFramer {
        return GrpcStreamFramer{
            .buf = std.ArrayList(u8).empty,
            .max_message_bytes = max_message_bytes,
        };
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        _.buf.deinit(alloc);
    }

    pub fn bufferedLen(): usize {
        return _.buf.items.len;
    }

    pub fn push(*_, alloc: std.mem.Allocator, chunk: []const u8): !void {
        if (chunk.len == 0) {
            return;
        }
        try _.buf.appendSlice(alloc, chunk);
    }

    pub fn nextFrame(*_, alloc: std.mem.Allocator): !GrpcFrame? {
        if (_.buf.items.len < GRPC_FRAME_HEADER_LEN) {
            return null;
        }
        const flag = _.buf.items[0];
        if (flag > 1) {
            return error.InvalidCompressedFlag;
        }
        const msg_len: usize = @intCast(std.mem.readInt(u32, _.buf.items[1..5], .big));
        if (msg_len > _.max_message_bytes) {
            return error.MessageTooLarge;
        }
        const frame_len = GRPC_FRAME_HEADER_LEN + msg_len;
        if (_.buf.items.len < frame_len) {
            return null;
        }

        const payload = try alloc.alloc(u8, msg_len);
        if (msg_len > 0) {
            @memcpy(payload, _.buf.items[5..frame_len]);
        }

        const remaining = _.buf.items.len - frame_len;
        if (remaining > 0) {
            std.mem.copyForwards(u8, _.buf.items[0..remaining], _.buf.items[frame_len..]);
        }
        _.buf.items.len = remaining;

        return GrpcFrame{
            .compressed = flag == 1,
            .payload = payload,
        };
    }
}
