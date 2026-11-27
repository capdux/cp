// Source: cp/lib/http2_frame/decoder.cp (RFC 9113 HTTP/2 Incremental Stream Frame Decoder)
const std = @import("std");
import {
    H2_HEADER_LEN,
    H2_DEFAULT_MAX_FRAME_SIZE,
    H2_MAX_FRAME_SIZE_LIMIT,
    H2Error,
    H2FrameHeader,
    H2Frame,
} from "./frameTypes.cp";
import {
    decodeFrame,
    freeDecodedFrame,
} from "./frameCodec.cp";

pub struct H2DecodedFrame {
    pub header: H2FrameHeader,
    pub frame: H2Frame,
    pub raw_payload: []u8,

    pub fn deinit(alloc: std.mem.Allocator) {
        freeDecodedFrame(alloc, _.frame);
        if (_.raw_payload.len > 0) {
            alloc.free(_.raw_payload);
        }
    }
}

pub struct H2StreamDecoder {
    alloc: std.mem.Allocator,
    max_frame_size: u32,
    buf: std.ArrayList(u8),

    pub static fn init(alloc: std.mem.Allocator, max_frame_size: u32): H2Error!H2StreamDecoder {
        if (max_frame_size < H2_DEFAULT_MAX_FRAME_SIZE or max_frame_size > H2_MAX_FRAME_SIZE_LIMIT) {
            return H2Error.InvalidSettings;
        }
        return H2StreamDecoder{
            .alloc = alloc,
            .max_frame_size = max_frame_size,
            .buf = std.ArrayList(u8).empty,
        };
    }

    pub fn deinit(*_) {
        _.buf.deinit(_.alloc);
    }

    pub fn setMaxFrameSize(*_, max_size: u32): H2Error!void {
        if (max_size < H2_DEFAULT_MAX_FRAME_SIZE or max_size > H2_MAX_FRAME_SIZE_LIMIT) {
            return H2Error.InvalidSettings;
        }
        _.max_frame_size = max_size;
    }

    pub fn bufferedLen(): usize {
        return _.buf.items.len;
    }

    pub fn feed(*_, chunk: []const u8): H2Error!void {
        if (chunk.len == 0) {
            return;
        }
        _.buf.appendSlice(_.alloc, chunk) catch return H2Error.OutOfMemory;
    }

    pub fn nextFrame(*_): H2Error!H2DecodedFrame? {
        if (_.buf.items.len < H2_HEADER_LEN) {
            return null;
        }
        const header = try H2FrameHeader.parse(_.buf.items[0..H2_HEADER_LEN]);
        if (header.length > _.max_frame_size) {
            return H2Error.FrameTooLarge;
        }
        const total_len: usize = H2_HEADER_LEN + @as(usize, header.length);
        if (_.buf.items.len < total_len) {
            return null;
        }

        const raw_payload = _.alloc.alloc(u8, @as(usize, header.length)) catch return H2Error.OutOfMemory;
        errdefer _.alloc.free(raw_payload);
        if (header.length > 0) {
            std.mem.copyForwards(u8, raw_payload, _.buf.items[H2_HEADER_LEN..total_len]);
        }

        const frame = try decodeFrame(_.alloc, header, raw_payload);

        const remaining = _.buf.items.len - total_len;
        if (remaining > 0) {
            std.mem.copyForwards(u8, _.buf.items[0..remaining], _.buf.items[total_len.._.buf.items.len]);
        }
        _.buf.shrinkRetainingCapacity(remaining);

        return H2DecodedFrame{
            .header = header,
            .frame = frame,
            .raw_payload = raw_payload,
        };
    }
}
