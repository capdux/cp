// Source: cp/lib/http2_frame/codec.cp (RFC 9113 HTTP/2 Frame Encoder & Decoder)
const std = @import("std");
import {
    H2_PREFACE,
    H2_HEADER_LEN,
    H2_MAX_FRAME_SIZE_LIMIT,
    H2_FLAG_ACK,
    H2_FLAG_END_STREAM,
    H2_FLAG_END_HEADERS,
    H2_FLAG_PADDED,
    H2_FLAG_PRIORITY,
    H2Error,
    H2FrameType,
    H2FrameHeader,
    H2Setting,
    H2PrioritySpec,
    H2DataFrame,
    H2HeadersFrame,
    H2RstStreamFrame,
    H2SettingsFrame,
    H2PingFrame,
    H2GoAwayFrame,
    H2WindowUpdateFrame,
    H2RawFrame,
    H2Frame,
} from "./types.cp";

fn readU16BE(b: []const u8): u16 {
    return (@as(u16, b[0]) << 8) | @as(u16, b[1]);
}

fn readU32BE(b: []const u8): u32 {
    return (@as(u32, b[0]) << 24) |
        (@as(u32, b[1]) << 16) |
        (@as(u32, b[2]) << 8) |
        @as(u32, b[3]);
}

fn writeU16BE(out: []u8, val: u16) {
    out[0] = @intCast((val >> 8) & 0xff);
    out[1] = @intCast(val & 0xff);
}

fn writeU32BE(out: []u8, val: u32) {
    out[0] = @intCast((val >> 24) & 0xff);
    out[1] = @intCast((val >> 16) & 0xff);
    out[2] = @intCast((val >> 8) & 0xff);
    out[3] = @intCast(val & 0xff);
}

pub fn validatePreface(buf: []const u8): H2Error!void {
    if (buf.len < H2_PREFACE.len) {
        return H2Error.InvalidPreface;
    }
    if (!std.mem.eql(u8, buf[0..H2_PREFACE.len], H2_PREFACE)) {
        return H2Error.InvalidPreface;
    }
}

pub fn freeDecodedFrame(alloc: std.mem.Allocator, frame: H2Frame) {
    switch (frame) {
        .settings => |s| {
            if (s.settings.len > 0) {
                alloc.free(s.settings);
            }
        },
        else => {},
    }
}

pub fn decodeFrame(alloc: std.mem.Allocator, header: H2FrameHeader, payload: []const u8): H2Error!H2Frame {
    if (payload.len != header.length) {
        return H2Error.InvalidFrameLength;
    }

    const kind_opt = header.kind();
    if (kind_opt == null) {
        return H2Frame{ .unknown = H2RawFrame{ .header = header, .payload = payload } };
    }

    const kind = kind_opt.?;
    switch (kind) {
        .data => {
            if (header.stream_id == 0) {
                return H2Error.InvalidStreamId;
            }
            let offset: usize = 0;
            let pad_len: u8 = 0;
            if (header.hasFlag(H2_FLAG_PADDED)) {
                if (payload.len < 1) {
                    return H2Error.InvalidPadding;
                }
                pad_len = payload[0];
                offset = 1;
                if (@as(usize, pad_len) >= payload.len) {
                    return H2Error.InvalidPadding;
                }
            }
            const end_idx = payload.len - @as(usize, pad_len);
            if (end_idx < offset) {
                return H2Error.InvalidPadding;
            }
            return H2Frame{
                .data = H2DataFrame{
                    .stream_id = header.stream_id,
                    .end_stream = header.hasFlag(H2_FLAG_END_STREAM),
                    .pad_len = pad_len,
                    .payload = payload[offset..end_idx],
                },
            };
        },
        .headers => {
            if (header.stream_id == 0) {
                return H2Error.InvalidStreamId;
            }
            let offset: usize = 0;
            let pad_len: u8 = 0;
            if (header.hasFlag(H2_FLAG_PADDED)) {
                if (payload.len < 1) {
                    return H2Error.InvalidPadding;
                }
                pad_len = payload[0];
                offset = 1;
                if (@as(usize, pad_len) >= payload.len) {
                    return H2Error.InvalidPadding;
                }
            }
            let prio: H2PrioritySpec? = null;
            if (header.hasFlag(H2_FLAG_PRIORITY)) {
                if (payload.len - offset - @as(usize, pad_len) < 5) {
                    return H2Error.InvalidFrameLength;
                }
                const raw_dep = readU32BE(payload[offset .. offset + 4]);
                const exclusive = (raw_dep & 0x80000000) != 0;
                const dep = raw_dep & 0x7fffffff;
                const weight = payload[offset + 4];
                offset += 5;
                prio = H2PrioritySpec{
                    .exclusive = exclusive,
                    .stream_dependency = dep,
                    .weight = weight,
                };
            }
            const end_idx = payload.len - @as(usize, pad_len);
            if (end_idx < offset) {
                return H2Error.InvalidPadding;
            }
            return H2Frame{
                .headers = H2HeadersFrame{
                    .stream_id = header.stream_id,
                    .end_stream = header.hasFlag(H2_FLAG_END_STREAM),
                    .end_headers = header.hasFlag(H2_FLAG_END_HEADERS),
                    .pad_len = pad_len,
                    .priority = prio,
                    .header_block = payload[offset..end_idx],
                },
            };
        },
        .priority => {
            if (header.stream_id == 0) {
                return H2Error.InvalidStreamId;
            }
            if (payload.len != 5) {
                return H2Error.InvalidFrameLength;
            }
            return H2Frame{ .priority = H2RawFrame{ .header = header, .payload = payload } };
        },
        .rst_stream => {
            if (header.stream_id == 0) {
                return H2Error.InvalidStreamId;
            }
            if (payload.len != 4) {
                return H2Error.InvalidFrameLength;
            }
            return H2Frame{
                .rst_stream = H2RstStreamFrame{
                    .stream_id = header.stream_id,
                    .error_code = readU32BE(payload[0..4]),
                },
            };
        },
        .settings => {
            if (header.stream_id != 0) {
                return H2Error.InvalidStreamId;
            }
            const is_ack = header.hasFlag(H2_FLAG_ACK);
            if (is_ack) {
                if (payload.len != 0) {
                    return H2Error.InvalidFrameLength;
                }
                const empty_slice = alloc.alloc(H2Setting, 0) catch return H2Error.OutOfMemory;
                return H2Frame{
                    .settings = H2SettingsFrame{
                        .ack = true,
                        .settings = empty_slice,
                    },
                };
            }
            if (payload.len % 6 != 0) {
                return H2Error.InvalidFrameLength;
            }
            const count = payload.len / 6;
            let list = alloc.alloc(H2Setting, count) catch return H2Error.OutOfMemory;
            let i: usize = 0;
            while (i < count) {
                const base = i * 6;
                list[i] = H2Setting{
                    .id = readU16BE(payload[base .. base + 2]),
                    .value = readU32BE(payload[base + 2 .. base + 6]),
                };
                i += 1;
            }
            return H2Frame{
                .settings = H2SettingsFrame{
                    .ack = false,
                    .settings = list,
                },
            };
        },
        .push_promise => {
            if (header.stream_id == 0) {
                return H2Error.InvalidStreamId;
            }
            return H2Frame{ .push_promise = H2RawFrame{ .header = header, .payload = payload } };
        },
        .ping => {
            if (header.stream_id != 0) {
                return H2Error.InvalidStreamId;
            }
            if (payload.len != 8) {
                return H2Error.InvalidFrameLength;
            }
            let opaque_data: [8]u8 = undefined;
            std.mem.copyForwards(u8, opaque_data[0..8], payload[0..8]);
            return H2Frame{
                .ping = H2PingFrame{
                    .ack = header.hasFlag(H2_FLAG_ACK),
                    .opaque_data = opaque_data,
                },
            };
        },
        .goaway => {
            if (header.stream_id != 0) {
                return H2Error.InvalidStreamId;
            }
            if (payload.len < 8) {
                return H2Error.InvalidFrameLength;
            }
            const last_stream_id = readU32BE(payload[0..4]) & 0x7fffffff;
            const error_code = readU32BE(payload[4..8]);
            return H2Frame{
                .goaway = H2GoAwayFrame{
                    .last_stream_id = last_stream_id,
                    .error_code = error_code,
                    .debug_data = payload[8..],
                },
            };
        },
        .window_update => {
            if (payload.len != 4) {
                return H2Error.InvalidFrameLength;
            }
            const increment = readU32BE(payload[0..4]) & 0x7fffffff;
            if (increment == 0) {
                return H2Error.InvalidIncrement;
            }
            return H2Frame{
                .window_update = H2WindowUpdateFrame{
                    .stream_id = header.stream_id,
                    .increment = increment,
                },
            };
        },
        .continuation => {
            if (header.stream_id == 0) {
                return H2Error.InvalidStreamId;
            }
            return H2Frame{ .continuation = H2RawFrame{ .header = header, .payload = payload } };
        },
    }
}

pub fn encodeFrame(alloc: std.mem.Allocator, frame: H2Frame): H2Error![]u8 {
    switch (frame) {
        .data => |f| {
            if (f.stream_id == 0) {
                return H2Error.InvalidStreamId;
            }
            let flags: u8 = 0;
            if (f.end_stream) {
                flags |= H2_FLAG_END_STREAM;
            }
            let extra: usize = 0;
            if (f.pad_len > 0) {
                flags |= H2_FLAG_PADDED;
                extra = @as(usize, 1) + @as(usize, f.pad_len);
            }
            const payload_len = f.payload.len + extra;
            if (payload_len > H2_MAX_FRAME_SIZE_LIMIT) {
                return H2Error.FrameTooLarge;
            }
            let out = alloc.alloc(u8, H2_HEADER_LEN + payload_len) catch return H2Error.OutOfMemory;
            const hdr = H2FrameHeader{
                .length = @intCast(payload_len),
                .frame_type = @intFromEnum(H2FrameType.data),
                .flags = flags,
                .stream_id = f.stream_id,
            };
            try hdr.encode(out[0..H2_HEADER_LEN]);
            let pos: usize = H2_HEADER_LEN;
            if (f.pad_len > 0) {
                out[pos] = f.pad_len;
                pos += 1;
            }
            if (f.payload.len > 0) {
                std.mem.copyForwards(u8, out[pos .. pos + f.payload.len], f.payload);
                pos += f.payload.len;
            }
            if (f.pad_len > 0) {
                @memset(out[pos .. pos + @as(usize, f.pad_len)], 0);
            }
            return out;
        },
        .headers => |f| {
            if (f.stream_id == 0) {
                return H2Error.InvalidStreamId;
            }
            let flags: u8 = 0;
            if (f.end_stream) {
                flags |= H2_FLAG_END_STREAM;
            }
            if (f.end_headers) {
                flags |= H2_FLAG_END_HEADERS;
            }
            let extra: usize = 0;
            if (f.pad_len > 0) {
                flags |= H2_FLAG_PADDED;
                extra += @as(usize, 1) + @as(usize, f.pad_len);
            }
            if (f.priority != null) {
                flags |= H2_FLAG_PRIORITY;
                extra += 5;
            }
            const payload_len = f.header_block.len + extra;
            if (payload_len > H2_MAX_FRAME_SIZE_LIMIT) {
                return H2Error.FrameTooLarge;
            }
            let out = alloc.alloc(u8, H2_HEADER_LEN + payload_len) catch return H2Error.OutOfMemory;
            const hdr = H2FrameHeader{
                .length = @intCast(payload_len),
                .frame_type = @intFromEnum(H2FrameType.headers),
                .flags = flags,
                .stream_id = f.stream_id,
            };
            try hdr.encode(out[0..H2_HEADER_LEN]);
            let pos: usize = H2_HEADER_LEN;
            if (f.pad_len > 0) {
                out[pos] = f.pad_len;
                pos += 1;
            }
            if (prio := f.priority) {
                let dep_word: u32 = prio.stream_dependency & 0x7fffffff;
                if (prio.exclusive) {
                    dep_word |= 0x80000000;
                }
                writeU32BE(out[pos .. pos + 4], dep_word);
                out[pos + 4] = prio.weight;
                pos += 5;
            }
            if (f.header_block.len > 0) {
                std.mem.copyForwards(u8, out[pos .. pos + f.header_block.len], f.header_block);
                pos += f.header_block.len;
            }
            if (f.pad_len > 0) {
                @memset(out[pos .. pos + @as(usize, f.pad_len)], 0);
            }
            return out;
        },
        .rst_stream => |f| {
            if (f.stream_id == 0) {
                return H2Error.InvalidStreamId;
            }
            const out = alloc.alloc(u8, H2_HEADER_LEN + 4) catch return H2Error.OutOfMemory;
            const hdr = H2FrameHeader{
                .length = 4,
                .frame_type = @intFromEnum(H2FrameType.rst_stream),
                .flags = 0,
                .stream_id = f.stream_id,
            };
            try hdr.encode(out[0..H2_HEADER_LEN]);
            writeU32BE(out[H2_HEADER_LEN .. H2_HEADER_LEN + 4], f.error_code);
            return out;
        },
        .settings => |f| {
            if (f.ack and f.settings.len > 0) {
                return H2Error.InvalidSettings;
            }
            const payload_len: usize = if (f.ack) @as(usize, 0) else f.settings.len * 6;
            if (payload_len > H2_MAX_FRAME_SIZE_LIMIT) {
                return H2Error.FrameTooLarge;
            }
            const out = alloc.alloc(u8, H2_HEADER_LEN + payload_len) catch return H2Error.OutOfMemory;
            const hdr = H2FrameHeader{
                .length = @intCast(payload_len),
                .frame_type = @intFromEnum(H2FrameType.settings),
                .flags = if (f.ack) H2_FLAG_ACK else 0,
                .stream_id = 0,
            };
            try hdr.encode(out[0..H2_HEADER_LEN]);
            if (!f.ack) {
                let i: usize = 0;
                while (i < f.settings.len) {
                    const pos = H2_HEADER_LEN + i * 6;
                    writeU16BE(out[pos .. pos + 2], f.settings[i].id);
                    writeU32BE(out[pos + 2 .. pos + 6], f.settings[i].value);
                    i += 1;
                }
            }
            return out;
        },
        .ping => |f| {
            const out = alloc.alloc(u8, H2_HEADER_LEN + 8) catch return H2Error.OutOfMemory;
            const hdr = H2FrameHeader{
                .length = 8,
                .frame_type = @intFromEnum(H2FrameType.ping),
                .flags = if (f.ack) H2_FLAG_ACK else 0,
                .stream_id = 0,
            };
            try hdr.encode(out[0..H2_HEADER_LEN]);
            std.mem.copyForwards(u8, out[H2_HEADER_LEN .. H2_HEADER_LEN + 8], f.opaque_data[0..8]);
            return out;
        },
        .goaway => |f| {
            const payload_len = 8 + f.debug_data.len;
            if (payload_len > H2_MAX_FRAME_SIZE_LIMIT) {
                return H2Error.FrameTooLarge;
            }
            const out = alloc.alloc(u8, H2_HEADER_LEN + payload_len) catch return H2Error.OutOfMemory;
            const hdr = H2FrameHeader{
                .length = @intCast(payload_len),
                .frame_type = @intFromEnum(H2FrameType.goaway),
                .flags = 0,
                .stream_id = 0,
            };
            try hdr.encode(out[0..H2_HEADER_LEN]);
            writeU32BE(out[H2_HEADER_LEN .. H2_HEADER_LEN + 4], f.last_stream_id & 0x7fffffff);
            writeU32BE(out[H2_HEADER_LEN + 4 .. H2_HEADER_LEN + 8], f.error_code);
            if (f.debug_data.len > 0) {
                std.mem.copyForwards(u8, out[H2_HEADER_LEN + 8 .. H2_HEADER_LEN + payload_len], f.debug_data);
            }
            return out;
        },
        .window_update => |f| {
            if (f.increment == 0 or f.increment > 0x7fffffff) {
                return H2Error.InvalidIncrement;
            }
            const out = alloc.alloc(u8, H2_HEADER_LEN + 4) catch return H2Error.OutOfMemory;
            const hdr = H2FrameHeader{
                .length = 4,
                .frame_type = @intFromEnum(H2FrameType.window_update),
                .flags = 0,
                .stream_id = f.stream_id,
            };
            try hdr.encode(out[0..H2_HEADER_LEN]);
            writeU32BE(out[H2_HEADER_LEN .. H2_HEADER_LEN + 4], f.increment & 0x7fffffff);
            return out;
        },
        .priority, .push_promise, .continuation, .unknown => |raw| {
            if (raw.payload.len > H2_MAX_FRAME_SIZE_LIMIT) {
                return H2Error.FrameTooLarge;
            }
            const out = alloc.alloc(u8, H2_HEADER_LEN + raw.payload.len) catch return H2Error.OutOfMemory;
            let hdr = raw.header;
            hdr.length = @intCast(raw.payload.len);
            try hdr.encode(out[0..H2_HEADER_LEN]);
            if (raw.payload.len > 0) {
                std.mem.copyForwards(u8, out[H2_HEADER_LEN .. H2_HEADER_LEN + raw.payload.len], raw.payload);
            }
            return out;
        },
    }
}
