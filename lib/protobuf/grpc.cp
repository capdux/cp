// Source: cp/lib/protobuf/grpc.cp (gRPC Length-Prefixed Framing, Status Codes & Timeout Codec)
// Pure CP implementation of gRPC over HTTP/2 wire primitives:
// 17 standard GrpcStatus codes, 5-byte length-prefixed framing with streaming
// incremental decoder & 4 MiB default DoS limit, and grpc-timeout codec.

const std = @import("std");

/// Default maximum gRPC message payload size (4 MiB).
pub const DEFAULT_MAX_MESSAGE_BYTES: usize = 4 * 1024 * 1024;
/// Fixed gRPC length-prefixed frame header size in bytes (`1B flag + 4B big-endian length`).
pub const GRPC_FRAME_HEADER_SIZE: usize = 5;

/// Error set returned by gRPC framing and timeout operations.
pub err GrpcError {
    MessageTooLarge,
    InvalidCompressedFlag,
    InvalidTimeout,
    UnexpectedEof,
    OutOfMemory,
}

/// Standard 17 gRPC status codes (`0..16`).
pub enum GrpcStatus: u8 {
    Ok = 0,
    Cancelled = 1,
    Unknown = 2,
    InvalidArgument = 3,
    DeadlineExceeded = 4,
    NotFound = 5,
    AlreadyExists = 6,
    PermissionDenied = 7,
    ResourceExhausted = 8,
    FailedPrecondition = 9,
    Aborted = 10,
    OutOfRange = 11,
    Unimplemented = 12,
    Internal = 13,
    Unavailable = 14,
    DataLoss = 15,
    Unauthenticated = 16,

    /// Converts an integer status code into `GrpcStatus`, returning `null` if out of `0..16`.
    static fn fromCode(raw_code: i32): GrpcStatus? {
        switch (raw_code) {
            0 => return GrpcStatus.Ok,
            1 => return GrpcStatus.Cancelled,
            2 => return GrpcStatus.Unknown,
            3 => return GrpcStatus.InvalidArgument,
            4 => return GrpcStatus.DeadlineExceeded,
            5 => return GrpcStatus.NotFound,
            6 => return GrpcStatus.AlreadyExists,
            7 => return GrpcStatus.PermissionDenied,
            8 => return GrpcStatus.ResourceExhausted,
            9 => return GrpcStatus.FailedPrecondition,
            10 => return GrpcStatus.Aborted,
            11 => return GrpcStatus.OutOfRange,
            12 => return GrpcStatus.Unimplemented,
            13 => return GrpcStatus.Internal,
            14 => return GrpcStatus.Unavailable,
            15 => return GrpcStatus.DataLoss,
            16 => return GrpcStatus.Unauthenticated,
            else => return null,
        }
    }

    /// Returns the numeric gRPC status code (`0..16`).
    fn code(): i32 {
        return (_ as u8) as i32;
    }

    /// Returns the canonical uppercase gRPC status name.
    fn name(): []const u8 {
        switch (_) {
            .Ok => return "OK",
            .Cancelled => return "CANCELLED",
            .Unknown => return "UNKNOWN",
            .InvalidArgument => return "INVALID_ARGUMENT",
            .DeadlineExceeded => return "DEADLINE_EXCEEDED",
            .NotFound => return "NOT_FOUND",
            .AlreadyExists => return "ALREADY_EXISTS",
            .PermissionDenied => return "PERMISSION_DENIED",
            .ResourceExhausted => return "RESOURCE_EXHAUSTED",
            .FailedPrecondition => return "FAILED_PRECONDITION",
            .Aborted => return "ABORTED",
            .OutOfRange => return "OUT_OF_RANGE",
            .Unimplemented => return "UNIMPLEMENTED",
            .Internal => return "INTERNAL",
            .Unavailable => return "UNAVAILABLE",
            .DataLoss => return "DATA_LOSS",
            .Unauthenticated => return "UNAUTHENTICATED",
        }
    }

    /// Returns `true` for `GrpcStatus.Ok`.
    fn isOk(): bool {
        return _ == GrpcStatus.Ok;
    }

    /// Returns `true` for transient/retryable statuses (`Unavailable`, `ResourceExhausted`, `Aborted`).
    fn isRetryable(): bool {
        switch (_) {
            .Unavailable | .ResourceExhausted | .Aborted => return true,
            else => return false,
        }
    }
}

/// Decoded gRPC length-prefixed frame (`[compressed: u8][length: u32 BE][payload]`).
pub struct GrpcFrame {
    compressed: bool = false;
    payload: []const u8 = "";
}

/// Decoded 5-byte gRPC frame header.
pub struct GrpcFrameHeader {
    compressed: bool = false;
    length: u32 = 0;
}

/// Stateless 5-byte gRPC length-prefixed frame encoder and zero-copy single-frame decoder.
pub static class GrpcFrameCodec {
    /// Encodes a gRPC frame (`[flag: u8][len: u32 BE][payload]`) with the default 4 MiB limit.
    fn encode(
        alloc: std.mem.Allocator,
        compressed: bool,
        payload: []const u8,
    ): ([]u8)! {
        return GrpcFrameCodec.encodeWithLimit(alloc, compressed, payload, DEFAULT_MAX_MESSAGE_BYTES);
    }

    /// Encodes a gRPC frame (`[flag: u8][len: u32 BE][payload]`), enforcing `max_message_bytes`.
    fn encodeWithLimit(
        alloc: std.mem.Allocator,
        compressed: bool,
        payload: []const u8,
        max_message_bytes: usize,
    ): ([]u8)! {
        if (payload.len > max_message_bytes || payload.len > 0xFFFFFFFF) {
            return GrpcError.MessageTooLarge;
        }
        const total_len: usize = GRPC_FRAME_HEADER_SIZE + payload.len;
        let out = alloc.alloc(u8, total_len) catch return GrpcError.OutOfMemory;
        const len_u32: u32 = payload.len as u32;
        out[0] = if (compressed) 1 else 0;
        out[1] = ((len_u32 >> 24) & 0xFF) as u8;
        out[2] = ((len_u32 >> 16) & 0xFF) as u8;
        out[3] = ((len_u32 >> 8) & 0xFF) as u8;
        out[4] = (len_u32 & 0xFF) as u8;
        if (payload.len > 0) {
            @memcpy(out[GRPC_FRAME_HEADER_SIZE..total_len], payload);
        }
        return out;
    }

    /// Encodes a `GrpcFrame` into an owned wire buffer with the default 4 MiB limit.
    fn encodeFrame(
        alloc: std.mem.Allocator,
        frame: GrpcFrame,
    ): ([]u8)! {
        return GrpcFrameCodec.encodeWithLimit(alloc, frame.compressed, frame.payload, DEFAULT_MAX_MESSAGE_BYTES);
    }

    /// Encodes a `GrpcFrame` into an owned wire buffer, enforcing `max_message_bytes`.
    fn encodeFrameWithLimit(
        alloc: std.mem.Allocator,
        frame: GrpcFrame,
        max_message_bytes: usize,
    ): ([]u8)! {
        return GrpcFrameCodec.encodeWithLimit(alloc, frame.compressed, frame.payload, max_message_bytes);
    }

    /// Decodes a 5-byte gRPC frame header with the default 4 MiB limit.
    fn decodeHeader(header: []const u8): GrpcFrameHeader! {
        return GrpcFrameCodec.decodeHeaderWithLimit(header, DEFAULT_MAX_MESSAGE_BYTES);
    }

    /// Decodes a 5-byte gRPC frame header and validates flag and size bounds.
    fn decodeHeaderWithLimit(
        header: []const u8,
        max_message_bytes: usize,
    ): GrpcFrameHeader! {
        if (header.len < GRPC_FRAME_HEADER_SIZE) {
            return GrpcError.UnexpectedEof;
        }
        const flag: u8 = header[0];
        if (flag != 0 && flag != 1) {
            return GrpcError.InvalidCompressedFlag;
        }
        const msg_len: u32 = ((header[1] as u32) << 24)
            | ((header[2] as u32) << 16)
            | ((header[3] as u32) << 8)
            | (header[4] as u32);
        if ((msg_len as usize) > max_message_bytes) {
            return GrpcError.MessageTooLarge;
        }
        return GrpcFrameHeader{
            compressed: flag == 1,
            length: msg_len,
        };
    }

    /// Zero-copy decodes one gRPC frame from the start of `data` using the default 4 MiB limit.
    fn decode(data: []const u8): GrpcFrame! {
        return GrpcFrameCodec.decodeWithLimit(data, DEFAULT_MAX_MESSAGE_BYTES);
    }

    /// Zero-copy decodes one gRPC frame from the start of `data`, enforcing `max_message_bytes`.
    fn decodeWithLimit(
        data: []const u8,
        max_message_bytes: usize,
    ): GrpcFrame! {
        const hdr = GrpcFrameCodec.decodeHeaderWithLimit(data, max_message_bytes)?;
        const end_pos: usize = GRPC_FRAME_HEADER_SIZE + (hdr.length as usize);
        if (data.len < end_pos) {
            return GrpcError.UnexpectedEof;
        }
        return GrpcFrame{
            compressed: hdr.compressed,
            payload: data[GRPC_FRAME_HEADER_SIZE..end_pos],
        };
    }
}

/// Incremental streaming gRPC frame decoder supporting arbitrary chunk boundaries.
pub struct GrpcFrameStream {
    allocator: std.mem.Allocator = undefined;
    buf: std.ArrayList(u8) = .empty;
    max_message_bytes: usize = DEFAULT_MAX_MESSAGE_BYTES;

    /// Initializes a streaming gRPC frame decoder with the default 4 MiB limit.
    static fn init(alloc: std.mem.Allocator): GrpcFrameStream {
        return GrpcFrameStream.initWithLimit(alloc, DEFAULT_MAX_MESSAGE_BYTES);
    }

    /// Initializes a streaming gRPC frame decoder with configurable `max_message_bytes`.
    static fn initWithLimit(
        alloc: std.mem.Allocator,
        max_message_bytes: usize,
    ): GrpcFrameStream {
        return GrpcFrameStream{
            allocator: alloc,
            buf: .empty,
            max_message_bytes: max_message_bytes,
        };
    }

    /// Releases internal stream buffering.
    fn deinit(*_) {
        _.buf.deinit(_.allocator);
    }

    /// Clears buffered partial bytes while retaining capacity.
    fn reset(*_) {
        _.buf.clearRetainingCapacity();
    }

    /// Returns the number of currently buffered incomplete bytes.
    fn bufferedBytes(): usize {
        return _.buf.items.len;
    }

    /// Frees an array of frames returned by `feed`.
    static fn freeFrames(alloc: std.mem.Allocator, frames: []GrpcFrame) {
        for (f of frames) {
            if (f.payload.len > 0) {
                alloc.free(f.payload);
            }
        }
        if (frames.len > 0) {
            alloc.free(frames);
        }
    }

    /// Feeds an incoming byte chunk and returns all complete `GrpcFrame`s parsed so far.
    fn feed(*_, chunk: []const u8): ([]GrpcFrame)! {
        if (chunk.len > 0) {
            _.buf.appendSlice(_.allocator, chunk) catch return GrpcError.OutOfMemory;
        }

        let frames: std.ArrayList(GrpcFrame) = .empty;
        errdefer {
            for (f of frames.items) {
                if (f.payload.len > 0) {
                    _.allocator.free(f.payload);
                }
            }
            frames.deinit(_.allocator);
        }

        let read_pos: usize = 0;
        while (_.buf.items.len - read_pos >= GRPC_FRAME_HEADER_SIZE) {
            const rem = _.buf.items[read_pos..];
            const hdr = GrpcFrameCodec.decodeHeaderWithLimit(rem, _.max_message_bytes) catch e => {
                _.reset();
                return e;
            };
            const frame_len: usize = GRPC_FRAME_HEADER_SIZE + (hdr.length as usize);
            if (rem.len < frame_len) {
                break;
            }

            const payload_src = rem[GRPC_FRAME_HEADER_SIZE..frame_len];
            let payload_owned: []const u8 = "";
            if (payload_src.len > 0) {
                payload_owned = _.allocator.dupe(u8, payload_src) catch return GrpcError.OutOfMemory;
            }
            errdefer if (payload_owned.len > 0) _.allocator.free(payload_owned);

            frames.append(_.allocator, GrpcFrame{
                compressed: hdr.compressed,
                payload: payload_owned,
            }) catch return GrpcError.OutOfMemory;

            read_pos += frame_len;
        }

        if (read_pos > 0) {
            const remaining = _.buf.items.len - read_pos;
            if (remaining > 0) {
                std.mem.copyForwards(u8, _.buf.items[0..remaining], _.buf.items[read_pos..]);
            }
            _.buf.shrinkRetainingCapacity(remaining);
        }

        return frames.toOwnedSlice(_.allocator) catch return GrpcError.OutOfMemory;
    }
}

/// Codec for `grpc-timeout` HTTP/2 header strings (`H`, `M`, `S`, `m`, `u`, `n`).
pub static class GrpcTimeout {
    /// Parses a gRPC timeout string (e.g. `"100m"`, `"5S"`, `"2H"`) into nanoseconds (`u64`).
    fn parse(raw: []const u8): u64! {
        if (raw.len < 2 || raw.len > 9) {
            return GrpcError.InvalidTimeout;
        }
        const unit = raw[raw.len - 1];
        const digits = raw[0 .. raw.len - 1];
        let val: u64 = 0;
        for (c of digits) {
            if (c < '0' || c > '9') {
                return GrpcError.InvalidTimeout;
            }
            val = val * 10 + ((c - '0') as u64);
        }

        let mult: u64 = 0;
        switch (unit) {
            'H' => { mult = 3600000000000; },
            'M' => { mult = 60000000000; },
            'S' => { mult = 1000000000; },
            'm' => { mult = 1000000; },
            'u' => { mult = 1000; },
            'n' => { mult = 1; },
            else => return GrpcError.InvalidTimeout,
        }
        return val * mult;
    }

    /// Formats a duration in milliseconds as an ASCII `grpc-timeout` value (e.g. `"100m"`),
    /// automatically escalating to `S`/`M`/`H` when `ms` exceeds 8 digits (`99999999`).
    fn formatMs(alloc: std.mem.Allocator, ms: u64): ([]u8)! {
        let val: u64 = ms;
        let unit: u8 = 'm';
        if (val > 99999999) {
            val = val / 1000;
            unit = 'S';
            if (val > 99999999) {
                val = val / 60;
                unit = 'M';
                if (val > 99999999) {
                    val = val / 60;
                    unit = 'H';
                    if (val > 99999999) {
                        return GrpcError.InvalidTimeout;
                    }
                }
            }
        }
        return std.fmt.allocPrint(alloc, "{d}{c}", .{ val, unit }) catch return GrpcError.OutOfMemory;
    }
}
