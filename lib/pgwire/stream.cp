// Source: cp/lib/pgwire/stream.cp (PostgreSQL v3.0 Incremental Streaming Packet Decoders)

const std = @import("std");
const types = @import("./types.cp");
const codec = @import("./codec.cp");

pub const PgError = types.PgError;
pub const PgBackendMsg = types.PgBackendMsg;
pub const PgFrontendMsg = types.PgFrontendMsg;

fn consumePrefix(buf: *std.ArrayList(u8), total: usize) {
    const rem = buf.items.len - total;
    if (rem > 0) {
        std.mem.copyForwards(u8, buf.items[0..rem], buf.items[total..]);
    }
    buf.items.len = rem;
}

pub struct PgBackendStreamDecoder {
    alloc: std.mem.Allocator,
    buf: std.ArrayList(u8),
    max_packet_bytes: usize,

    pub static fn init(alloc: std.mem.Allocator, max_packet_bytes: usize): PgBackendStreamDecoder {
        return PgBackendStreamDecoder{
            .alloc = alloc,
            .buf = std.ArrayList(u8).empty,
            .max_packet_bytes = max_packet_bytes,
        };
    }

    pub fn deinit(*_) {
        _.buf.deinit(_.alloc);
    }

    pub fn feed(*_, chunk: []const u8): !void {
        if (chunk.len > 0) {
            try _.buf.appendSlice(_.alloc, chunk);
        }
    }

    pub fn next(*_): !PgBackendMsg? {
        if (_.buf.items.len < 5) {
            return null;
        }
        const tag = _.buf.items[0];
        const len_u32 = std.mem.readInt(u32, _.buf.items[1..5], .big);
        const len_usize: usize = @intCast(len_u32);
        if (len_usize < 4) {
            return PgError.InvalidPacket;
        }
        if (len_usize > _.max_packet_bytes) {
            return PgError.PacketTooLarge;
        }
        const total = 1 + len_usize;
        if (_.buf.items.len < total) {
            return null;
        }
        const payload = _.buf.items[5..total];
        const msg = try codec.decodeBackendPayload(_.alloc, tag, payload);
        consumePrefix(&_.buf, total);
        return msg;
    }
}

pub struct PgFrontendStreamDecoder {
    alloc: std.mem.Allocator,
    buf: std.ArrayList(u8),
    max_packet_bytes: usize,
    expect_startup: bool,

    pub static fn init(alloc: std.mem.Allocator, max_packet_bytes: usize, expect_startup: bool): PgFrontendStreamDecoder {
        return PgFrontendStreamDecoder{
            .alloc = alloc,
            .buf = std.ArrayList(u8).empty,
            .max_packet_bytes = max_packet_bytes,
            .expect_startup = expect_startup,
        };
    }

    pub fn deinit(*_) {
        _.buf.deinit(_.alloc);
    }

    pub fn feed(*_, chunk: []const u8): !void {
        if (chunk.len > 0) {
            try _.buf.appendSlice(_.alloc, chunk);
        }
    }

    pub fn next(*_): !PgFrontendMsg? {
        if (_.expect_startup) {
            if (_.buf.items.len < 4) {
                return null;
            }
            const len_u32 = std.mem.readInt(u32, _.buf.items[0..4], .big);
            const len_usize: usize = @intCast(len_u32);
            if (len_usize < 8) {
                return PgError.InvalidPacket;
            }
            if (len_usize > _.max_packet_bytes) {
                return PgError.PacketTooLarge;
            }
            if (_.buf.items.len < len_usize) {
                return null;
            }
            const payload = _.buf.items[4..len_usize];
            const msg = try codec.decodeFrontendPayload(_.alloc, null, payload);
            consumePrefix(&_.buf, len_usize);
            switch (msg) {
                .startup => {
                    _.expect_startup = false;
                },
                else => {},
            }
            return msg;
        }

        if (_.buf.items.len < 5) {
            return null;
        }
        const tag = _.buf.items[0];
        const len_u32 = std.mem.readInt(u32, _.buf.items[1..5], .big);
        const len_usize: usize = @intCast(len_u32);
        if (len_usize < 4) {
            return PgError.InvalidPacket;
        }
        if (len_usize > _.max_packet_bytes) {
            return PgError.PacketTooLarge;
        }
        const total = 1 + len_usize;
        if (_.buf.items.len < total) {
            return null;
        }
        const payload = _.buf.items[5..total];
        const msg = try codec.decodeFrontendPayload(_.alloc, tag, payload);
        consumePrefix(&_.buf, total);
        return msg;
    }
}
