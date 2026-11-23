// Reference-hardened against hyperium/tonic (MIT)
// Source: cp/lib/grpc_wire/frame.cp (gRPC Length-Prefixed Message 5-byte frame codec, streaming framer & 16-lane SIMD payload copy)

const std = @import("std");

type GrpcVec16u8 = @Vector(16, u8);

pub const DEFAULT_MAX_MESSAGE_BYTES: usize = 4 * 1024 * 1024;
pub const GRPC_DEFAULT_MAX_DECODING_MESSAGE_SIZE: usize = DEFAULT_MAX_MESSAGE_BYTES;
pub const GRPC_FRAME_HEADER_LEN: usize = 5;

fn grpcLoadVec16(data: []const u8, off: usize): GrpcVec16u8 {
    const v: GrpcVec16u8 = [
        data[off], data[off + 1], data[off + 2], data[off + 3],
        data[off + 4], data[off + 5], data[off + 6], data[off + 7],
        data[off + 8], data[off + 9], data[off + 10], data[off + 11],
        data[off + 12], data[off + 13], data[off + 14], data[off + 15],
    ];
    return v;
}

/// 16-lane `@Vector(16, u8)` SIMD payload copy for gRPC LPM frames.
pub fn simdGrpcCopyPayload(dst: []u8, src: []const u8) {
    const n = if (dst.len <= src.len) dst.len else src.len;
    let i: usize = 0;
    while (i + 16 <= n) {
        const v = grpcLoadVec16(src, i);
        const arr: [16]u8 = v;
        @memcpy(dst[i .. i + 16], arr[0..16]);
        i += 16;
    }
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
}

/// Scalar reference for `simdGrpcCopyPayload` (differential verification).
pub fn simdGrpcCopyPayloadScalar(dst: []u8, src: []const u8) {
    const n = if (dst.len <= src.len) dst.len else src.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
}

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
        simdGrpcCopyPayload(out[5..], payload);
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
    if (data.len - GRPC_FRAME_HEADER_LEN < msg_len) {
        return error.TruncatedFrame;
    }
    const payload = try alloc.alloc(u8, msg_len);
    if (msg_len > 0) {
        simdGrpcCopyPayload(payload, data[5 .. 5 + msg_len]);
    }
    return GrpcFrame{
        .compressed = flag == 1,
        .payload = payload,
    };
}

pub struct GrpcStreamFramer {
    buf: std.ArrayList(u8),
    max_message_bytes: usize,

    pub static fn init(max_message_bytes: usize): GrpcStreamFramer {
        return GrpcStreamFramer{
            .buf = std.ArrayList(u8).empty,
            .max_message_bytes = max_message_bytes,
        };
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        _.buf.deinit(alloc);
    }

    pub fn reset(*_) {
        _.buf.clearRetainingCapacity();
    }

    pub fn bufferedLen(): usize {
        return _.buf.items.len;
    }

    pub fn push(*_, alloc: std.mem.Allocator, chunk: []const u8): !void {
        if (chunk.len == 0) {
            return;
        }
        const max_buf = if (_.max_message_bytes <= (0xFFFFFFFF - 65536)) (_.max_message_bytes + 65536) else _.max_message_bytes;
        if (_.buf.items.len > _.max_message_bytes + GRPC_FRAME_HEADER_LEN && _.buf.items.len + chunk.len > max_buf) {
            return error.MessageTooLarge;
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
        if (_.buf.items.len - GRPC_FRAME_HEADER_LEN < msg_len) {
            return null;
        }
        const frame_len = GRPC_FRAME_HEADER_LEN + msg_len;

        const payload = try alloc.alloc(u8, msg_len);
        if (msg_len > 0) {
            simdGrpcCopyPayload(payload, _.buf.items[5..frame_len]);
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
