// Source: cp/lib/http3/http3.cp (RFC 9114 HTTP/3 Framing, Stream Types & RFC 9204 QPACK Codec)

const std = @import("std");

/// Maximum value encodable in an RFC 9000 variable-length integer (2^62 - 1).
pub const MAX_VARINT: u64 = 0x3FFFFFFFFFFFFFFF;

/// Default maximum HTTP/3 frame payload size (16 MiB).
pub const DEFAULT_MAX_FRAME_SIZE: usize = 16 * 1024 * 1024;

/// RFC 9114 §7.2.4.1 HTTP/3 standard setting identifiers.
pub const QPACK_MAX_TABLE_CAPACITY: u64 = 0x01;
pub const MAX_FIELD_SECTION_SIZE: u64 = 0x06;
pub const QPACK_BLOCKED_STREAMS: u64 = 0x07;

/// Errors returned by `cap:http3` framing, settings, stream type, and QPACK codecs.
pub err Http3Error {
    UnexpectedEof,
    InvalidFrame,
    FrameUnexpected,
    DuplicateSetting,
    ReservedSetting,
    InvalidStreamType,
    QpackDecompressionFailed,
    FrameTooLarge,
    OutOfMemory,
}

/// RFC 9114 §6.2 HTTP/3 unidirectional stream types.
pub enum H3StreamType: u64 {
    Control = 0x00,
    Push = 0x01,
    QpackEncoder = 0x02,
    QpackDecoder = 0x03,
}

/// RFC 9114 §7.2 HTTP/3 frame types.
pub enum H3FrameType: u64 {
    Data = 0x00,
    Headers = 0x01,
    CancelPush = 0x03,
    Settings = 0x04,
    PushPromise = 0x05,
    Goaway = 0x07,
    MaxPushId = 0x0d,
}

/// RFC 9114 §7.2.4 HTTP/3 SETTINGS key-value parameter.
pub class H3Setting {
    pub id: u64,
    pub value: u64,
}

/// RFC 9114 §7.2.5 PUSH_PROMISE frame payload.
pub class H3PushPromiseFrame {
    pub push_id: u64,
    pub header_block: []const u8,
}

/// Unknown or GREASE HTTP/3 frame payload preserved for forward compatibility.
pub class H3UnknownFrame {
    pub frame_type: u64,
    pub payload: []const u8,
}

/// Tagged union of RFC 9114 §7.2 HTTP/3 frames.
pub enum H3Frame {
    data: []const u8,
    headers: []const u8,
    cancel_push: u64,
    settings: []const H3Setting,
    push_promise: H3PushPromiseFrame,
    goaway: u64,
    max_push_id: u64,
    unknown: H3UnknownFrame,
}

/// HTTP/3 header name-value pair for RFC 9204 QPACK codec.
pub class H3Header {
    pub name: []const u8,
    pub value: []const u8,
}

class VarintResult {
    value: u64,
    len: usize,
}

fn h3VarintLen(v: u64): usize ! Http3Error {
    if (v <= 63) return 1;
    if (v <= 16383) return 2;
    if (v <= 1073741823) return 4;
    if (v <= MAX_VARINT) return 8;
    return Http3Error.InvalidFrame;
}

fn h3WriteVarint(list: *std.ArrayList(u8), alloc: std.mem.Allocator, v: u64): void ! Http3Error {
    const n = h3VarintLen(v)?;
    if (n == 1) {
        list.append(alloc, @intCast(v)) catch return Http3Error.OutOfMemory;
        return;
    }
    if (n == 2) {
        list.append(alloc, @as(u8, 0x40) | @as(u8, @intCast((v >> 8) & 0x3F))) catch return Http3Error.OutOfMemory;
        list.append(alloc, @intCast(v & 0xFF)) catch return Http3Error.OutOfMemory;
        return;
    }
    if (n == 4) {
        list.append(alloc, @as(u8, 0x80) | @as(u8, @intCast((v >> 24) & 0x3F))) catch return Http3Error.OutOfMemory;
        list.append(alloc, @intCast((v >> 16) & 0xFF)) catch return Http3Error.OutOfMemory;
        list.append(alloc, @intCast((v >> 8) & 0xFF)) catch return Http3Error.OutOfMemory;
        list.append(alloc, @intCast(v & 0xFF)) catch return Http3Error.OutOfMemory;
        return;
    }
    list.append(alloc, @as(u8, 0xC0) | @as(u8, @intCast((v >> 56) & 0x3F))) catch return Http3Error.OutOfMemory;
    list.append(alloc, @intCast((v >> 48) & 0xFF)) catch return Http3Error.OutOfMemory;
    list.append(alloc, @intCast((v >> 40) & 0xFF)) catch return Http3Error.OutOfMemory;
    list.append(alloc, @intCast((v >> 32) & 0xFF)) catch return Http3Error.OutOfMemory;
    list.append(alloc, @intCast((v >> 24) & 0xFF)) catch return Http3Error.OutOfMemory;
    list.append(alloc, @intCast((v >> 16) & 0xFF)) catch return Http3Error.OutOfMemory;
    list.append(alloc, @intCast((v >> 8) & 0xFF)) catch return Http3Error.OutOfMemory;
    list.append(alloc, @intCast(v & 0xFF)) catch return Http3Error.OutOfMemory;
}

fn h3DecodeVarint(bytes: []const u8): VarintResult ! Http3Error {
    if (bytes.len == 0) return Http3Error.UnexpectedEof;
    const first = bytes[0];
    const prefix = first >> 6;
    const byte_len: usize = @as(usize, 1) << @intCast(prefix);
    if (bytes.len < byte_len) return Http3Error.UnexpectedEof;

    let val: u64 = @as(u64, first & 0x3F);
    let i: usize = 1;
    while (i < byte_len) {
        val = (val << 8) | @as(u64, bytes[i]);
        i += 1;
    }
    return VarintResult {
        value: val,
        len: byte_len,
    };
}

fn isReservedHttp2Setting(id: u64): bool {
    return id == 0x02 or id == 0x03 or id == 0x04 or id == 0x05;
}

fn isReservedHttp2Frame(frame_type: u64): bool {
    return frame_type == 0x02 or frame_type == 0x06 or frame_type == 0x08 or frame_type == 0x09;
}

/// Validates and maps a decoded stream type varint to `H3StreamType`.
pub fn parseStreamType(stream_type_id: u64): H3StreamType ! Http3Error {
    if (stream_type_id == 0x00) return H3StreamType.Control;
    if (stream_type_id == 0x01) return H3StreamType.Push;
    if (stream_type_id == 0x02) return H3StreamType.QpackEncoder;
    if (stream_type_id == 0x03) return H3StreamType.QpackDecoder;
    return Http3Error.InvalidStreamType;
}

/// Encodes a single `H3Frame` into a newly allocated wire buffer.
pub fn encodeFrame(allocator: std.mem.Allocator, frame: H3Frame): ([]u8) ! Http3Error {
    let list: std.ArrayList(u8) = .empty;
    errdefer list.deinit(allocator);

    switch (frame) {
        .data => |d| {
            h3WriteVarint(&list, allocator, 0x00)?;
            h3WriteVarint(&list, allocator, @intCast(d.len))?;
            list.appendSlice(allocator, d) catch return Http3Error.OutOfMemory;
        },
        .headers => |h| {
            h3WriteVarint(&list, allocator, 0x01)?;
            h3WriteVarint(&list, allocator, @intCast(h.len))?;
            list.appendSlice(allocator, h) catch return Http3Error.OutOfMemory;
        },
        .cancel_push => |push_id| {
            const payload_len = h3VarintLen(push_id)?;
            h3WriteVarint(&list, allocator, 0x03)?;
            h3WriteVarint(&list, allocator, @intCast(payload_len))?;
            h3WriteVarint(&list, allocator, push_id)?;
        },
        .settings => |settings| {
            let payload_len: usize = 0;
            let i: usize = 0;
            while (i < settings.len) {
                const s = settings[i];
                if (isReservedHttp2Setting(s.id)) return Http3Error.ReservedSetting;
                let j: usize = 0;
                while (j < i) {
                    if (settings[j].id == s.id) return Http3Error.DuplicateSetting;
                    j += 1;
                }
                payload_len += h3VarintLen(s.id)?;
                payload_len += h3VarintLen(s.value)?;
                i += 1;
            }
            h3WriteVarint(&list, allocator, 0x04)?;
            h3WriteVarint(&list, allocator, @intCast(payload_len))?;
            for (s in settings) {
                h3WriteVarint(&list, allocator, s.id)?;
                h3WriteVarint(&list, allocator, s.value)?;
            }
        },
        .push_promise => |pp| {
            const id_len = h3VarintLen(pp.push_id)?;
            h3WriteVarint(&list, allocator, 0x05)?;
            h3WriteVarint(&list, allocator, @intCast(id_len + pp.header_block.len))?;
            h3WriteVarint(&list, allocator, pp.push_id)?;
            list.appendSlice(allocator, pp.header_block) catch return Http3Error.OutOfMemory;
        },
        .goaway => |stream_id| {
            const payload_len = h3VarintLen(stream_id)?;
            h3WriteVarint(&list, allocator, 0x07)?;
            h3WriteVarint(&list, allocator, @intCast(payload_len))?;
            h3WriteVarint(&list, allocator, stream_id)?;
        },
        .max_push_id => |push_id| {
            const payload_len = h3VarintLen(push_id)?;
            h3WriteVarint(&list, allocator, 0x0d)?;
            h3WriteVarint(&list, allocator, @intCast(payload_len))?;
            h3WriteVarint(&list, allocator, push_id)?;
        },
        .unknown => |unk| {
            if (isReservedHttp2Frame(unk.frame_type)) return Http3Error.FrameUnexpected;
            h3WriteVarint(&list, allocator, unk.frame_type)?;
            h3WriteVarint(&list, allocator, @intCast(unk.payload.len))?;
            list.appendSlice(allocator, unk.payload) catch return Http3Error.OutOfMemory;
        },
    }

    return list.toOwnedSlice(allocator) catch return Http3Error.OutOfMemory;
}

fn freeFramesList(allocator: std.mem.Allocator, frames: []H3Frame) {
    for (f in frames) {
        switch (f) {
            .settings => |s| {
                if (s.len > 0) {
                    allocator.free(s);
                }
            },
            else => {},
        }
    }
}

/// Frees a slice of `H3Frame`s returned by `decodeFrames` or `decodeFramesWithMaxSize`.
pub fn freeFrames(allocator: std.mem.Allocator, frames: []H3Frame) {
    freeFramesList(allocator, frames);
    allocator.free(frames);
}

fn decodeSingleVarintPayload(payload: []const u8): u64 ! Http3Error {
    if (payload.len == 0) return Http3Error.InvalidFrame;
    const decoded_val = h3DecodeVarint(payload) catch return Http3Error.InvalidFrame;
    if (decoded_val.len != payload.len) return Http3Error.InvalidFrame;
    return decoded_val.value;
}

fn decodeSettingsPayload(allocator: std.mem.Allocator, payload: []const u8): ([]const H3Setting) ! Http3Error {
    let settings: std.ArrayList(H3Setting) = .empty;
    errdefer settings.deinit(allocator);

    let pos: usize = 0;
    while (pos < payload.len) {
        const id_dec = h3DecodeVarint(payload[pos..]) catch return Http3Error.InvalidFrame;
        pos += id_dec.len;
        const val_dec = h3DecodeVarint(payload[pos..]) catch return Http3Error.InvalidFrame;
        pos += val_dec.len;

        if (isReservedHttp2Setting(id_dec.value)) return Http3Error.ReservedSetting;
        for (existing in settings.items) {
            if (existing.id == id_dec.value) return Http3Error.DuplicateSetting;
        }
        settings.append(allocator, H3Setting {
            id: id_dec.value,
            value: val_dec.value,
        }) catch return Http3Error.OutOfMemory;
    }

    return settings.toOwnedSlice(allocator) catch return Http3Error.OutOfMemory;
}

/// Decodes all HTTP/3 frames from `bytes` enforcing `max_frame_size`.
/// Payload slices borrow from `bytes`. Call `freeFrames(allocator, frames)` to release.
pub fn decodeFramesWithMaxSize(allocator: std.mem.Allocator, bytes: []const u8, max_frame_size: usize): ([]H3Frame) ! Http3Error {
    let frames: std.ArrayList(H3Frame) = .empty;
    errdefer {
        freeFramesList(allocator, frames.items);
        frames.deinit(allocator);
    }

    let pos: usize = 0;
    while (pos < bytes.len) {
        const type_dec = h3DecodeVarint(bytes[pos..])?;
        pos += type_dec.len;

        const len_dec = h3DecodeVarint(bytes[pos..])?;
        pos += len_dec.len;

        if (len_dec.value > @as(u64, @intCast(max_frame_size))) {
            return Http3Error.FrameTooLarge;
        }
        const frame_len: usize = @intCast(len_dec.value);
        if (bytes.len - pos < frame_len) {
            return Http3Error.UnexpectedEof;
        }

        const payload = bytes[pos .. pos + frame_len];
        pos += frame_len;

        const ftype = type_dec.value;
        if (isReservedHttp2Frame(ftype)) {
            return Http3Error.FrameUnexpected;
        }

        if (ftype == 0x00) {
            frames.append(allocator, H3Frame { data: payload }) catch return Http3Error.OutOfMemory;
        } else if (ftype == 0x01) {
            frames.append(allocator, H3Frame { headers: payload }) catch return Http3Error.OutOfMemory;
        } else if (ftype == 0x03) {
            const push_id = decodeSingleVarintPayload(payload)?;
            frames.append(allocator, H3Frame { cancel_push: push_id }) catch return Http3Error.OutOfMemory;
        } else if (ftype == 0x04) {
            const settings = decodeSettingsPayload(allocator, payload)?;
            frames.append(allocator, H3Frame { settings: settings }) catch {
                if (settings.len > 0) allocator.free(settings);
                return Http3Error.OutOfMemory;
            };
        } else if (ftype == 0x05) {
            if (payload.len == 0) return Http3Error.InvalidFrame;
            const id_dec = h3DecodeVarint(payload) catch return Http3Error.InvalidFrame;
            const header_block = payload[id_dec.len..];
            frames.append(allocator, H3Frame {
                push_promise: H3PushPromiseFrame {
                    push_id: id_dec.value,
                    header_block: header_block,
                },
            }) catch return Http3Error.OutOfMemory;
        } else if (ftype == 0x07) {
            const stream_id = decodeSingleVarintPayload(payload)?;
            frames.append(allocator, H3Frame { goaway: stream_id }) catch return Http3Error.OutOfMemory;
        } else if (ftype == 0x0d) {
            const push_id = decodeSingleVarintPayload(payload)?;
            frames.append(allocator, H3Frame { max_push_id: push_id }) catch return Http3Error.OutOfMemory;
        } else {
            frames.append(allocator, H3Frame {
                unknown: H3UnknownFrame {
                    frame_type: ftype,
                    payload: payload,
                },
            }) catch return Http3Error.OutOfMemory;
        }
    }

    return frames.toOwnedSlice(allocator) catch return Http3Error.OutOfMemory;
}

/// Decodes all HTTP/3 frames from `bytes` using `DEFAULT_MAX_FRAME_SIZE`.
pub fn decodeFrames(allocator: std.mem.Allocator, bytes: []const u8): ([]H3Frame) ! Http3Error {
    return decodeFramesWithMaxSize(allocator, bytes, DEFAULT_MAX_FRAME_SIZE);
}

// ============================================================================
// RFC 9204 QPACK Static Table & Field Section Codec (Static-Only Mode)
// ============================================================================

/// Complete 99-entry RFC 9204 Appendix A QPACK static table.
pub const QPACK_STATIC_TABLE: [99]H3Header = .{
    H3Header { name: ":authority", value: "" },
    H3Header { name: ":path", value: "/" },
    H3Header { name: "age", value: "0" },
    H3Header { name: "content-disposition", value: "" },
    H3Header { name: "content-length", value: "0" },
    H3Header { name: "cookie", value: "" },
    H3Header { name: "date", value: "" },
    H3Header { name: "etag", value: "" },
    H3Header { name: "if-modified-since", value: "" },
    H3Header { name: "if-none-match", value: "" },
    H3Header { name: "last-modified", value: "" },
    H3Header { name: "link", value: "" },
    H3Header { name: "location", value: "" },
    H3Header { name: "referer", value: "" },
    H3Header { name: "set-cookie", value: "" },
    H3Header { name: ":method", value: "CONNECT" },
    H3Header { name: ":method", value: "DELETE" },
    H3Header { name: ":method", value: "GET" },
    H3Header { name: ":method", value: "HEAD" },
    H3Header { name: ":method", value: "OPTIONS" },
    H3Header { name: ":method", value: "POST" },
    H3Header { name: ":method", value: "PUT" },
    H3Header { name: ":scheme", value: "http" },
    H3Header { name: ":scheme", value: "https" },
    H3Header { name: ":status", value: "103" },
    H3Header { name: ":status", value: "200" },
    H3Header { name: ":status", value: "304" },
    H3Header { name: ":status", value: "404" },
    H3Header { name: ":status", value: "503" },
    H3Header { name: "accept", value: "*/*" },
    H3Header { name: "accept", value: "application/dns-message" },
    H3Header { name: "accept-encoding", value: "gzip, deflate, br" },
    H3Header { name: "accept-ranges", value: "bytes" },
    H3Header { name: "access-control-allow-headers", value: "cache-control" },
    H3Header { name: "access-control-allow-headers", value: "content-type" },
    H3Header { name: "access-control-allow-origin", value: "*" },
    H3Header { name: "cache-control", value: "max-age=0" },
    H3Header { name: "cache-control", value: "max-age=2592000" },
    H3Header { name: "cache-control", value: "max-age=604800" },
    H3Header { name: "cache-control", value: "no-cache" },
    H3Header { name: "cache-control", value: "no-store" },
    H3Header { name: "cache-control", value: "public, max-age=31536000" },
    H3Header { name: "content-encoding", value: "br" },
    H3Header { name: "content-encoding", value: "gzip" },
    H3Header { name: "content-type", value: "application/dns-message" },
    H3Header { name: "content-type", value: "application/javascript" },
    H3Header { name: "content-type", value: "application/json" },
    H3Header { name: "content-type", value: "application/x-www-form-urlencoded" },
    H3Header { name: "content-type", value: "image/gif" },
    H3Header { name: "content-type", value: "image/jpeg" },
    H3Header { name: "content-type", value: "image/png" },
    H3Header { name: "content-type", value: "text/css" },
    H3Header { name: "content-type", value: "text/html; charset=utf-8" },
    H3Header { name: "content-type", value: "text/plain" },
    H3Header { name: "content-type", value: "text/plain;charset=utf-8" },
    H3Header { name: "range", value: "bytes=0-" },
    H3Header { name: "strict-transport-security", value: "max-age=31536000" },
    H3Header { name: "strict-transport-security", value: "max-age=31536000; includesubdomains" },
    H3Header { name: "strict-transport-security", value: "max-age=31536000; includesubdomains; preload" },
    H3Header { name: "vary", value: "accept-encoding" },
    H3Header { name: "vary", value: "origin" },
    H3Header { name: "x-content-type-options", value: "nosniff" },
    H3Header { name: "x-xss-protection", value: "1; mode=block" },
    H3Header { name: ":status", value: "100" },
    H3Header { name: ":status", value: "204" },
    H3Header { name: ":status", value: "206" },
    H3Header { name: ":status", value: "302" },
    H3Header { name: ":status", value: "400" },
    H3Header { name: ":status", value: "403" },
    H3Header { name: ":status", value: "421" },
    H3Header { name: ":status", value: "425" },
    H3Header { name: ":status", value: "500" },
    H3Header { name: "accept-language", value: "" },
    H3Header { name: "access-control-allow-credentials", value: "FALSE" },
    H3Header { name: "access-control-allow-credentials", value: "TRUE" },
    H3Header { name: "access-control-allow-headers", value: "*" },
    H3Header { name: "access-control-allow-methods", value: "get" },
    H3Header { name: "access-control-allow-methods", value: "get, post, options" },
    H3Header { name: "access-control-allow-methods", value: "options" },
    H3Header { name: "access-control-expose-headers", value: "content-length" },
    H3Header { name: "access-control-request-headers", value: "content-type" },
    H3Header { name: "access-control-request-method", value: "get" },
    H3Header { name: "access-control-request-method", value: "post" },
    H3Header { name: "alt-svc", value: "clear" },
    H3Header { name: "authorization", value: "" },
    H3Header { name: "content-security-policy", value: "script-src 'none'; object-src 'none'; base-uri 'none'" },
    H3Header { name: "early-data", value: "1" },
    H3Header { name: "expect-ct", value: "" },
    H3Header { name: "forwarded", value: "" },
    H3Header { name: "if-range", value: "" },
    H3Header { name: "origin", value: "" },
    H3Header { name: "purpose", value: "prefetch" },
    H3Header { name: "server", value: "" },
    H3Header { name: "timing-allow-origin", value: "*" },
    H3Header { name: "upgrade-insecure-requests", value: "1" },
    H3Header { name: "user-agent", value: "" },
    H3Header { name: "x-forwarded-for", value: "" },
    H3Header { name: "x-frame-options", value: "deny" },
    H3Header { name: "x-frame-options", value: "sameorigin" },
};

class PrefixIntResult {
    value: u64,
    len: usize,
}

fn writePrefixInt(list: *std.ArrayList(u8), alloc: std.mem.Allocator, prefix_bits: u3, first_mask: u8, value: u64): void ! Http3Error {
    const max_prefix: u64 = (@as(u64, 1) << prefix_bits) - 1;
    if (value < max_prefix) {
        list.append(alloc, first_mask | @as(u8, @intCast(value))) catch return Http3Error.OutOfMemory;
        return;
    }
    list.append(alloc, first_mask | @as(u8, @intCast(max_prefix))) catch return Http3Error.OutOfMemory;
    let rem = value - max_prefix;
    while (rem >= 128) {
        list.append(alloc, @as(u8, @intCast((rem & 0x7F) | 0x80))) catch return Http3Error.OutOfMemory;
        rem >>= 7;
    }
    list.append(alloc, @as(u8, @intCast(rem))) catch return Http3Error.OutOfMemory;
}

fn decodePrefixInt(bytes: []const u8, prefix_bits: u3): PrefixIntResult ! Http3Error {
    if (bytes.len == 0) return Http3Error.QpackDecompressionFailed;
    const max_prefix: u64 = (@as(u64, 1) << prefix_bits) - 1;
    let val: u64 = @as(u64, bytes[0]) & max_prefix;
    if (val < max_prefix) {
        return PrefixIntResult { value: val, len: 1 };
    }
    let m: u6 = 0;
    let pos: usize = 1;
    while (pos < bytes.len) {
        const b = bytes[pos];
        pos += 1;
        if (m >= 56 and (b & 0x7F) > 1) return Http3Error.QpackDecompressionFailed;
        val += @as(u64, b & 0x7F) << m;
        if ((b & 0x80) == 0) {
            return PrefixIntResult { value: val, len: pos };
        }
        m += 7;
        if (m > 62) return Http3Error.QpackDecompressionFailed;
    }
    return Http3Error.QpackDecompressionFailed;
}

fn findStaticExact(name: []const u8, value: []const u8): usize? {
    let i: usize = 0;
    while (i < QPACK_STATIC_TABLE.len) {
        const entry = QPACK_STATIC_TABLE[i];
        if (std.mem.eql(u8, entry.name, name) and std.mem.eql(u8, entry.value, value)) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn findStaticName(name: []const u8): usize? {
    let i: usize = 0;
    while (i < QPACK_STATIC_TABLE.len) {
        if (std.mem.eql(u8, QPACK_STATIC_TABLE[i].name, name)) {
            return i;
        }
        i += 1;
    }
    return null;
}

/// Encodes HTTP/3 header pairs into an RFC 9204 QPACK header block (Required Insert Count=0, Delta Base=0).
pub fn encodeQpackHeaders(allocator: std.mem.Allocator, headers: []const H3Header): ([]u8) ! Http3Error {
    let list: std.ArrayList(u8) = .empty;
    errdefer list.deinit(allocator);

    // 2-byte Header Block Prefix: Required Insert Count = 0, Delta Base = 0
    list.append(allocator, 0x00) catch return Http3Error.OutOfMemory;
    list.append(allocator, 0x00) catch return Http3Error.OutOfMemory;

    for (h in headers) {
        if (idx := findStaticExact(h.name, h.value)) {
            // Indexed Field Line (Static): 1 1 Index(6+)
            writePrefixInt(&list, allocator, 6, 0xC0, @intCast(idx))?;
        } else if (idx := findStaticName(h.name)) {
            // Literal Field Line With Name Reference (Static): 0 1 0 1 NameIndex(4+)
            writePrefixInt(&list, allocator, 4, 0x50, @intCast(idx))?;
            // Value with 7-bit prefix length (H=0)
            writePrefixInt(&list, allocator, 7, 0x00, @intCast(h.value.len))?;
            list.appendSlice(allocator, h.value) catch return Http3Error.OutOfMemory;
        } else {
            // Literal Field Line With Literal Name: 0 0 1 N=0 H=0 NameLen(3+)
            writePrefixInt(&list, allocator, 3, 0x20, @intCast(h.name.len))?;
            list.appendSlice(allocator, h.name) catch return Http3Error.OutOfMemory;
            // Value with 7-bit prefix length (H=0)
            writePrefixInt(&list, allocator, 7, 0x00, @intCast(h.value.len))?;
            list.appendSlice(allocator, h.value) catch return Http3Error.OutOfMemory;
        }
    }

    return list.toOwnedSlice(allocator) catch return Http3Error.OutOfMemory;
}

/// Decodes an RFC 9204 QPACK header block (static table mode) into `[]H3Header`.
/// Name/value slices borrow from `QPACK_STATIC_TABLE` or `block`. Caller frees returned slice via `allocator.free(headers)`.
pub fn decodeQpackHeaders(allocator: std.mem.Allocator, block: []const u8): ([]H3Header) ! Http3Error {
    if (block.len < 2) return Http3Error.QpackDecompressionFailed;

    let pos: usize = 0;
    const ric_dec = decodePrefixInt(block[pos..], 8)?;
    pos += ric_dec.len;
    if (ric_dec.value != 0) return Http3Error.QpackDecompressionFailed;

    const base_dec = decodePrefixInt(block[pos..], 7)?;
    pos += base_dec.len;
    if (base_dec.value != 0) return Http3Error.QpackDecompressionFailed;

    let headers: std.ArrayList(H3Header) = .empty;
    errdefer headers.deinit(allocator);

    while (pos < block.len) {
        const first = block[pos];
        if ((first & 0x80) != 0) {
            // Indexed Field Line: 1 T Index(6+)
            const is_static = (first & 0x40) != 0;
            if (!is_static) return Http3Error.QpackDecompressionFailed;
            const idx_dec = decodePrefixInt(block[pos..], 6)?;
            pos += idx_dec.len;
            if (idx_dec.value >= QPACK_STATIC_TABLE.len) return Http3Error.QpackDecompressionFailed;
            const idx: usize = @intCast(idx_dec.value);
            const entry = QPACK_STATIC_TABLE[idx];
            headers.append(allocator, entry) catch return Http3Error.OutOfMemory;
        } else if ((first & 0xC0) == 0x40) {
            // Literal Field Line With Name Reference: 0 1 N T NameIndex(4+)
            const is_static = (first & 0x10) != 0;
            if (!is_static) return Http3Error.QpackDecompressionFailed;
            const idx_dec = decodePrefixInt(block[pos..], 4)?;
            pos += idx_dec.len;
            if (idx_dec.value >= QPACK_STATIC_TABLE.len) return Http3Error.QpackDecompressionFailed;

            if (pos >= block.len) return Http3Error.QpackDecompressionFailed;
            if ((block[pos] & 0x80) != 0) return Http3Error.QpackDecompressionFailed;
            const val_len_dec = decodePrefixInt(block[pos..], 7)?;
            pos += val_len_dec.len;
            const val_len: usize = @intCast(val_len_dec.value);
            if (block.len - pos < val_len) return Http3Error.QpackDecompressionFailed;
            const val_slice = block[pos .. pos + val_len];
            pos += val_len;

            const idx: usize = @intCast(idx_dec.value);
            headers.append(allocator, H3Header {
                name: QPACK_STATIC_TABLE[idx].name,
                value: val_slice,
            }) catch return Http3Error.OutOfMemory;
        } else if ((first & 0xE0) == 0x20) {
            // Literal Field Line With Literal Name: 0 0 1 N H NameLen(3+)
            if ((first & 0x08) != 0) return Http3Error.QpackDecompressionFailed;
            const name_len_dec = decodePrefixInt(block[pos..], 3)?;
            pos += name_len_dec.len;
            const name_len: usize = @intCast(name_len_dec.value);
            if (block.len - pos < name_len) return Http3Error.QpackDecompressionFailed;
            const name_slice = block[pos .. pos + name_len];
            pos += name_len;

            if (pos >= block.len) return Http3Error.QpackDecompressionFailed;
            if ((block[pos] & 0x80) != 0) return Http3Error.QpackDecompressionFailed;
            const val_len_dec = decodePrefixInt(block[pos..], 7)?;
            pos += val_len_dec.len;
            const val_len: usize = @intCast(val_len_dec.value);
            if (block.len - pos < val_len) return Http3Error.QpackDecompressionFailed;
            const val_slice = block[pos .. pos + val_len];
            pos += val_len;

            headers.append(allocator, H3Header {
                name: name_slice,
                value: val_slice,
            }) catch return Http3Error.OutOfMemory;
        } else {
            // Post-base dynamic table representations require a dynamic table (unsupported in static mode)
            return Http3Error.QpackDecompressionFailed;
        }
    }

    return headers.toOwnedSlice(allocator) catch return Http3Error.OutOfMemory;
}
