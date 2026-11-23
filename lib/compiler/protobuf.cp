// Reference-hardened against tokio-rs/prost (Apache-2.0)
// Source: cp/lib/protobuf/protobuf.cp (Protocol Buffers Wire Format Zero-Copy Codec)
// Pure CP zero-copy Protocol Buffers wire format encoder & decoder.
// Supports LEB128 Varint (strict 10-byte 64-bit bound), ZigZag sint32/sint64,
// Fixed32/Fixed64/Float/Double, length-delimited bytes/strings/submessages,
// StartGroup/EndGroup pairing, packed repeated fields, zero-heap skipField /
// validateProtobufWire, recursion depth protection (PROTOBUF_MAX_RECURSION_DEPTH = 100),
// strict UTF-8 validation with cap:simd Simd.isAscii fast-path, and @Vector(16, u8)
// SIMD payload copy.

const std = @import("std");
import { Simd } from "cap:simd";

/// Maximum valid Protobuf field number (`2^29 - 1`).
pub const MAX_FIELD_NUMBER: u32 = 536870911;
/// Legacy default maximum nested submessage recursion depth.
pub const DEFAULT_MAX_DEPTH: usize = 64;
/// Default maximum nested submessage/group recursion depth (aligned with `prost::RECURSION_LIMIT`).
pub const PROTOBUF_MAX_RECURSION_DEPTH: usize = 100;

/// Error set returned by Protocol Buffers wire format encoding/decoding operations.
pub err ProtoError {
    UnexpectedEof,
    VarintOverflow,
    InvalidWireType,
    InvalidFieldNumber,
    TruncatedLengthDelimited,
    MaxDepthExceeded,
    UnexpectedEndGroupTag,
    InvalidUtf8,
    OutOfMemory,
}

/// Protocol Buffers wire format types (3-bit tag discriminator).
pub enum WireType: u8 {
    Varint = 0,
    Fixed64 = 1,
    LengthDelimited = 2,
    StartGroup = 3,
    EndGroup = 4,
    Fixed32 = 5,

    /// Converts a raw 3-bit wire type integer into `WireType`.
    static fn fromInt(v: u8): WireType! {
        switch (v) {
            0 => return WireType.Varint,
            1 => return WireType.Fixed64,
            2 => return WireType.LengthDelimited,
            3 => return WireType.StartGroup,
            4 => return WireType.EndGroup,
            5 => return WireType.Fixed32,
            else => return ProtoError.InvalidWireType,
        }
    }

    /// Returns the numeric wire type code.
    fn toInt(): u8 {
        return _ as u8;
    }
}

/// Encodes a 32-bit signed integer using ZigZag encoding.
pub fn zigzagEncode32(v: i32): u32 {
    const u: u32 = @bitCast(v);
    const sign_mask: u32 = if (v < 0) std.math.maxInt(u32) else 0;
    return (u << 1) ^ sign_mask;
}

/// Decodes a ZigZag-encoded 32-bit unsigned integer back into `i32`.
pub fn zigzagDecode32(v: u32): i32 {
    const shifted: u32 = v >> 1;
    const sign_mask: u32 = if ((v & 1) != 0) std.math.maxInt(u32) else 0;
    return @bitCast(shifted ^ sign_mask);
}

/// Encodes a 64-bit signed integer using ZigZag encoding.
pub fn zigzagEncode64(v: i64): u64 {
    const u: u64 = @bitCast(v);
    const sign_mask: u64 = if (v < 0) std.math.maxInt(u64) else 0;
    return (u << 1) ^ sign_mask;
}

/// Decodes a ZigZag-encoded 64-bit unsigned integer back into `i64`.
pub fn zigzagDecode64(v: u64): i64 {
    const shifted: u64 = v >> 1;
    const sign_mask: u64 = if ((v & 1) != 0) std.math.maxInt(u64) else 0;
    return @bitCast(shifted ^ sign_mask);
}

/// Decoded zero-copy Protobuf field yielded by `ProtoReader.next()`.
pub struct ProtoField {
    field_number: u32;
    wire_type: WireType;
    raw_varint: u64 = 0;
    raw_fixed64: u64 = 0;
    raw_fixed32: u32 = 0;
    raw_bytes: []const u8 = "";

    /// Interprets `raw_varint` as an unsigned 64-bit integer (`uint64`).
    fn asUint64(): u64 {
        return _.raw_varint;
    }

    /// Interprets `raw_varint` as an unsigned 32-bit integer (`uint32`).
    fn asUint32(): u32 {
        return (_.raw_varint & 0xFFFFFFFF) as u32;
    }

    /// Interprets `raw_varint` as a two's-complement 64-bit signed integer (`int64`).
    fn asInt64(): i64 {
        return @bitCast(_.raw_varint);
    }

    /// Interprets `raw_varint` as a two's-complement 32-bit signed integer (`int32`).
    fn asInt32(): i32 {
        const signed64: i64 = @bitCast(_.raw_varint);
        return signed64 as i32;
    }

    /// Interprets `raw_varint` as a ZigZag-encoded 32-bit signed integer (`sint32`).
    fn asSint32(): i32 {
        return zigzagDecode32((_.raw_varint & 0xFFFFFFFF) as u32);
    }

    /// Interprets `raw_varint` as a ZigZag-encoded 64-bit signed integer (`sint64`).
    fn asSint64(): i64 {
        return zigzagDecode64(_.raw_varint);
    }

    /// Interprets `raw_varint` as a boolean (`0` -> `false`, non-zero -> `true`).
    fn asBool(): bool {
        return _.raw_varint != 0;
    }

    /// Interprets `raw_fixed32` as a 32-bit unsigned integer (`fixed32`).
    fn asFixed32(): u32 {
        return _.raw_fixed32;
    }

    /// Interprets `raw_fixed64` as a 64-bit unsigned integer (`fixed64`).
    fn asFixed64(): u64 {
        return _.raw_fixed64;
    }

    /// Interprets `raw_fixed32` as a 32-bit signed integer (`sfixed32`).
    fn asSfixed32(): i32 {
        return @bitCast(_.raw_fixed32);
    }

    /// Interprets `raw_fixed64` as a 64-bit signed integer (`sfixed64`).
    fn asSfixed64(): i64 {
        return @bitCast(_.raw_fixed64);
    }

    /// Interprets `raw_fixed32` as an IEEE-754 32-bit float (`float`).
    fn asFloat(): f32 {
        return @bitCast(_.raw_fixed32);
    }

    /// Interprets `raw_fixed64` as an IEEE-754 64-bit double (`double`).
    fn asDouble(): f64 {
        return @bitCast(_.raw_fixed64);
    }

    /// Returns the zero-copy byte slice payload (`bytes` / `submessage`).
    fn asBytes(): []const u8 {
        return _.raw_bytes;
    }

    /// Returns the zero-copy string slice payload (`string`).
    fn asString(): []const u8 {
        return _.raw_bytes;
    }

    /// Returns the zero-copy string slice payload after strict UTF-8 validation (`ProtoError.InvalidUtf8`).
    fn asStringChecked(): ([]const u8)! {
        if (!ProtoReader.isValidUtf8(_.raw_bytes)) {
            return ProtoError.InvalidUtf8;
        }
        return _.raw_bytes;
    }
}

/// 16-lane SIMD `@Vector(16, u8)` payload copy for hot-path `bytes`/`string` serialization.
pub fn copyPayloadSimd(dst: []u8, src: []const u8) {
    let i: usize = 0;
    while (i + 16 <= src.len) : (i += 16) {
        const chunk: @Vector(16, u8) = src[i..][0..16].*;
        dst[i..][0..16].* = chunk;
    }
    while (i < src.len) : (i += 1) {
        dst[i] = src[i];
    }
}

/// Scalar reference payload copy for 0-diff parity verification against `copyPayloadSimd`.
pub fn copyPayloadScalar(dst: []u8, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[i] = src[i];
    }
}

/// Protocol Buffers binary wire encoder backed by an owned `std.ArrayList(u8)`.
pub struct ProtoWriter {
    allocator: std.mem.Allocator = undefined;
    buf: std.ArrayList(u8) = .empty;

    /// Creates an empty `ProtoWriter` using `allocator`.
    static fn init(allocator: std.mem.Allocator): ProtoWriter {
        return ProtoWriter{
            allocator: allocator,
            buf: .empty,
        };
    }

    /// Releases the internal buffer.
    fn deinit(*_) {
        _.buf.deinit(_.allocator);
    }

    /// Clears all written bytes while retaining allocated capacity.
    fn clear(*_) {
        _.buf.clearRetainingCapacity();
    }

    /// Returns the number of bytes currently written.
    fn len(): usize {
        return _.buf.items.len;
    }

    /// Returns a read-only view of the currently encoded bytes.
    fn slice(): []const u8 {
        return _.buf.items;
    }

    /// Transfers ownership of the encoded byte buffer to the caller.
    fn toOwnedSlice(*_): ([]u8)! {
        return _.buf.toOwnedSlice(_.allocator) catch return ProtoError.OutOfMemory;
    }

    /// Computes the encoded byte length of an unsigned varint without allocating.
    static fn varintSize(v: u64): usize {
        let val: u64 = v;
        let size: usize = 1;
        while (val >= 0x80) {
            val >>= 7;
            size += 1;
        }
        return size;
    }

    /// Writes a validated `(field_num, wire_type)` tag as a varint.
    fn writeTag(*_, field_num: u32, wire_type: WireType): ! {
        if (field_num == 0 || field_num > MAX_FIELD_NUMBER) {
            return ProtoError.InvalidFieldNumber;
        }
        const tag_val: u64 = ((field_num as u64) << 3) | ((wire_type as u8) as u64);
        _.writeVarint(tag_val)?;
    }

    /// Encodes and appends a raw base-128 unsigned varint (1 to 10 bytes).
    fn writeVarint(*_, v: u64): ! {
        let val: u64 = v;
        while (val >= 0x80) {
            const byte: u8 = ((val & 0x7F) as u8) | 0x80;
            _.buf.append(_.allocator, byte) catch return ProtoError.OutOfMemory;
            val >>= 7;
        }
        _.buf.append(_.allocator, (val & 0x7F) as u8) catch return ProtoError.OutOfMemory;
    }

    /// Writes a raw 32-bit little-endian value without a field tag.
    fn writeRawFixed32(*_, v: u32): ! {
        let out: [4]u8 = [
            (v & 0xFF) as u8,
            ((v >> 8) & 0xFF) as u8,
            ((v >> 16) & 0xFF) as u8,
            ((v >> 24) & 0xFF) as u8,
        ];
        _.buf.appendSlice(_.allocator, &out) catch return ProtoError.OutOfMemory;
    }

    /// Writes a raw 64-bit little-endian value without a field tag.
    fn writeRawFixed64(*_, v: u64): ! {
        let out: [8]u8 = [
            (v & 0xFF) as u8,
            ((v >> 8) & 0xFF) as u8,
            ((v >> 16) & 0xFF) as u8,
            ((v >> 24) & 0xFF) as u8,
            ((v >> 32) & 0xFF) as u8,
            ((v >> 40) & 0xFF) as u8,
            ((v >> 48) & 0xFF) as u8,
            ((v >> 56) & 0xFF) as u8,
        ];
        _.buf.appendSlice(_.allocator, &out) catch return ProtoError.OutOfMemory;
    }

    /// Writes an unsigned `uint32` varint field.
    fn writeUint32(*_, field_num: u32, v: u32): ! {
        _.writeTag(field_num, WireType.Varint)?;
        _.writeVarint(v as u64)?;
    }

    /// Writes an unsigned `uint64` varint field.
    fn writeUint64(*_, field_num: u32, v: u64): ! {
        _.writeTag(field_num, WireType.Varint)?;
        _.writeVarint(v)?;
    }

    /// Writes a two's-complement `int32` field (sign-extended to 64 bits if negative).
    fn writeInt32(*_, field_num: u32, v: i32): ! {
        const ext: i64 = v as i64;
        const raw: u64 = @bitCast(ext);
        _.writeTag(field_num, WireType.Varint)?;
        _.writeVarint(raw)?;
    }

    /// Writes a two's-complement `int64` field.
    fn writeInt64(*_, field_num: u32, v: i64): ! {
        const raw: u64 = @bitCast(v);
        _.writeTag(field_num, WireType.Varint)?;
        _.writeVarint(raw)?;
    }

    /// Writes a ZigZag-encoded `sint32` field.
    fn writeSint32(*_, field_num: u32, v: i32): ! {
        _.writeTag(field_num, WireType.Varint)?;
        _.writeVarint(zigzagEncode32(v) as u64)?;
    }

    /// Writes a ZigZag-encoded `sint64` field.
    fn writeSint64(*_, field_num: u32, v: i64): ! {
        _.writeTag(field_num, WireType.Varint)?;
        _.writeVarint(zigzagEncode64(v))?;
    }

    /// Writes a boolean field as a 1-byte varint (`0` or `1`).
    fn writeBool(*_, field_num: u32, v: bool): ! {
        _.writeTag(field_num, WireType.Varint)?;
        _.writeVarint(if (v) 1 else 0)?;
    }

    /// Writes a `fixed32` field in little-endian byte order.
    fn writeFixed32(*_, field_num: u32, v: u32): ! {
        _.writeTag(field_num, WireType.Fixed32)?;
        _.writeRawFixed32(v)?;
    }

    /// Writes a `fixed64` field in little-endian byte order.
    fn writeFixed64(*_, field_num: u32, v: u64): ! {
        _.writeTag(field_num, WireType.Fixed64)?;
        _.writeRawFixed64(v)?;
    }

    /// Writes an IEEE-754 `float` (`f32`) field as 32-bit little-endian bits.
    fn writeFloat(*_, field_num: u32, v: f32): ! {
        const bits: u32 = @bitCast(v);
        _.writeFixed32(field_num, bits)?;
    }

    /// Writes an IEEE-754 `double` (`f64`) field as 64-bit little-endian bits.
    fn writeDouble(*_, field_num: u32, v: f64): ! {
        const bits: u64 = @bitCast(v);
        _.writeFixed64(field_num, bits)?;
    }

    /// Writes a length-delimited `bytes` field (uses 16-lane SIMD payload copy for `>= 16` bytes).
    fn writeBytes(*_, field_num: u32, data: []const u8): ! {
        _.writeTag(field_num, WireType.LengthDelimited)?;
        _.writeVarint(data.len as u64)?;
        if (data.len > 0) {
            if (data.len >= 16) {
                _.buf.ensureUnusedCapacity(_.allocator, data.len) catch return ProtoError.OutOfMemory;
                const old_len = _.buf.items.len;
                const dst = _.buf.items.ptr[old_len .. old_len + data.len];
                copyPayloadSimd(dst, data);
                _.buf.items.len = old_len + data.len;
            } else {
                _.buf.appendSlice(_.allocator, data) catch return ProtoError.OutOfMemory;
            }
        }
    }

    /// Writes a length-delimited UTF-8 `string` field.
    fn writeString(*_, field_num: u32, s: []const u8): ! {
        _.writeBytes(field_num, s)?;
    }

    /// Writes an embedded submessage encoded as a length-delimited byte payload.
    fn writeSubmessage(*_, field_num: u32, sub_bytes: []const u8): ! {
        _.writeBytes(field_num, sub_bytes)?;
    }

    /// Writes a `StartGroup` (`WireType.StartGroup = 3`) field opener.
    fn writeStartGroup(*_, field_num: u32): ! {
        _.writeTag(field_num, WireType.StartGroup)?;
    }

    /// Writes an `EndGroup` (`WireType.EndGroup = 4`) field closer.
    fn writeEndGroup(*_, field_num: u32): ! {
        _.writeTag(field_num, WireType.EndGroup)?;
    }

    /// Writes a proto3 packed repeated varint field (`WireType.LengthDelimited`).
    /// Emits nothing when `vals` is empty, matching proto3 canonical serialization.
    fn writePackedVarints(*_, field_num: u32, vals: []const u64): ! {
        if (vals.len == 0) {
            return;
        }
        let total_bytes: usize = 0;
        for (v of vals) {
            total_bytes += ProtoWriter.varintSize(v);
        }
        _.writeTag(field_num, WireType.LengthDelimited)?;
        _.writeVarint(total_bytes as u64)?;
        for (v of vals) {
            _.writeVarint(v)?;
        }
    }

    /// Writes a proto3 packed repeated ZigZag `sint32` field (`WireType.LengthDelimited`).
    fn writePackedSint32(*_, field_num: u32, vals: []const i32): ! {
        if (vals.len == 0) return;
        let total_bytes: usize = 0;
        for (v of vals) {
            total_bytes += ProtoWriter.varintSize(zigzagEncode32(v) as u64);
        }
        _.writeTag(field_num, WireType.LengthDelimited)?;
        _.writeVarint(total_bytes as u64)?;
        for (v of vals) {
            _.writeVarint(zigzagEncode32(v) as u64)?;
        }
    }

    /// Writes a proto3 packed repeated `fixed32` field (`WireType.LengthDelimited`).
    fn writePackedFixed32(*_, field_num: u32, vals: []const u32): ! {
        if (vals.len == 0) return;
        _.writeTag(field_num, WireType.LengthDelimited)?;
        _.writeVarint((vals.len * 4) as u64)?;
        for (v of vals) {
            _.writeRawFixed32(v)?;
        }
    }

    /// Writes a proto3 packed repeated `fixed64` field (`WireType.LengthDelimited`).
    fn writePackedFixed64(*_, field_num: u32, vals: []const u64): ! {
        if (vals.len == 0) return;
        _.writeTag(field_num, WireType.LengthDelimited)?;
        _.writeVarint((vals.len * 8) as u64)?;
        for (v of vals) {
            _.writeRawFixed64(v)?;
        }
    }
}

/// Zero-copy slice cursor for streaming Protocol Buffers wire payloads.
pub struct ProtoReader {
    data: []const u8;
    pos: usize = 0;
    depth: usize = 0;
    max_depth: usize = PROTOBUF_MAX_RECURSION_DEPTH;

    /// Creates a `ProtoReader` over `data` with default maximum recursion depth (`100`).
    static fn init(data: []const u8): ProtoReader {
        return ProtoReader{
            data: data,
            pos: 0,
            depth: 0,
            max_depth: PROTOBUF_MAX_RECURSION_DEPTH,
        };
    }

    /// Strict UTF-8 validation with `cap:simd` `Simd.isAscii` fast-path.
    pub static fn isValidUtf8(s: []const u8): bool {
        if (Simd.isAscii(s)) return true;
        return std.unicode.utf8ValidateSlice(s);
    }

    /// Scalar reference UTF-8 validation for 0-diff parity testing against `isValidUtf8`.
    pub static fn isValidUtf8Scalar(s: []const u8): bool {
        return std.unicode.utf8ValidateSlice(s);
    }

    /// Creates a `ProtoReader` over `data` with an explicit maximum recursion depth.
    static fn initWithMaxDepth(data: []const u8, max_depth: usize): ProtoReader {
        return ProtoReader{
            data: data,
            pos: 0,
            depth: 0,
            max_depth: max_depth,
        };
    }

    /// Returns `true` when unread bytes remain.
    fn hasMore(): bool {
        return _.pos < _.data.len;
    }

    /// Returns the number of unread bytes remaining.
    fn remaining(): usize {
        return _.data.len - _.pos;
    }

    /// Returns the current byte cursor offset.
    fn position(): usize {
        return _.pos;
    }

    /// Resets the cursor back to byte offset `0`.
    fn reset(*_) {
        _.pos = 0;
    }

    /// Decodes a base-128 unsigned varint (1..10 bytes), guarding against overflow and EOF.
    fn readVarint(*_): u64! {
        let result: u64 = 0;
        let shift: u6 = 0;
        let i: usize = 0;
        while (i < 10) : (i += 1) {
            if (_.pos >= _.data.len) {
                return ProtoError.UnexpectedEof;
            }
            const b: u8 = _.data[_.pos];
            _.pos += 1;

            if (i == 9) {
                if ((b & 0x80) != 0 || b > 1) {
                    return ProtoError.VarintOverflow;
                }
            }

            result |= ((b & 0x7F) as u64) << shift;

            if ((b & 0x80) == 0) {
                return result;
            }
            shift += 7;
        }
        return ProtoError.VarintOverflow;
    }

    /// Reads the next streaming field in zero-copy fashion, returning `null` at EOF.
    fn next(*_): ProtoField?! {
        if (_.pos >= _.data.len) {
            return null;
        }
        const tag_raw = _.readVarint()?;
        const wt_raw: u8 = (tag_raw & 0x07) as u8;
        const wt = WireType.fromInt(wt_raw)?;

        const field_num_u64: u64 = tag_raw >> 3;
        if (field_num_u64 == 0 || field_num_u64 > (MAX_FIELD_NUMBER as u64)) {
            return ProtoError.InvalidFieldNumber;
        }
        const field_num: u32 = field_num_u64 as u32;

        let field = ProtoField{
            field_number: field_num,
            wire_type: wt,
        };

        switch (wt) {
            .Varint => {
                field.raw_varint = _.readVarint()?;
            },
            .Fixed64 => {
                if (_.remaining() < 8) {
                    return ProtoError.UnexpectedEof;
                }
                const p = _.pos;
                field.raw_fixed64 = (_.data[p] as u64)
                    | ((_.data[p + 1] as u64) << 8)
                    | ((_.data[p + 2] as u64) << 16)
                    | ((_.data[p + 3] as u64) << 24)
                    | ((_.data[p + 4] as u64) << 32)
                    | ((_.data[p + 5] as u64) << 40)
                    | ((_.data[p + 6] as u64) << 48)
                    | ((_.data[p + 7] as u64) << 56);
                _.pos += 8;
            },
            .LengthDelimited => {
                const len_u64 = _.readVarint()?;
                if (len_u64 > (_.remaining() as u64)) {
                    return ProtoError.TruncatedLengthDelimited;
                }
                const byte_len: usize = len_u64 as usize;
                field.raw_bytes = _.data[_.pos .. _.pos + byte_len];
                _.pos += byte_len;
            },
            .Fixed32 => {
                if (_.remaining() < 4) {
                    return ProtoError.UnexpectedEof;
                }
                const p = _.pos;
                field.raw_fixed32 = (_.data[p] as u32)
                    | ((_.data[p + 1] as u32) << 8)
                    | ((_.data[p + 2] as u32) << 16)
                    | ((_.data[p + 3] as u32) << 24);
                _.pos += 4;
            },
            .StartGroup, .EndGroup => {},
        }

        return field;
    }

    /// Skips an unknown field for forward compatibility with strict group-tag pairing
    /// and recursion depth enforcement (`PROTOBUF_MAX_RECURSION_DEPTH = 100`).
    fn skipField(*_, field: ProtoField): ! {
        if (field.wire_type == WireType.EndGroup) {
            return ProtoError.UnexpectedEndGroupTag;
        }
        if (field.wire_type == WireType.StartGroup) {
            let group_stack: [128]u32 = undefined;
            if (_.depth >= _.max_depth) {
                return ProtoError.MaxDepthExceeded;
            }
            group_stack[0] = field.field_number;
            let group_depth: usize = 1;
            while (group_depth > 0) {
                const inner = _.next()? ?? return ProtoError.UnexpectedEof;
                if (inner.wire_type == WireType.StartGroup) {
                    if (_.depth + group_depth >= _.max_depth || group_depth >= 128) {
                        return ProtoError.MaxDepthExceeded;
                    }
                    group_stack[group_depth] = inner.field_number;
                    group_depth += 1;
                } else if (inner.wire_type == WireType.EndGroup) {
                    if (inner.field_number != group_stack[group_depth - 1]) {
                        return ProtoError.UnexpectedEndGroupTag;
                    }
                    group_depth -= 1;
                }
            }
        }
    }

    /// Zero-heap full-stream wire format validator (verifies tags, varints, bounds, and group pairing).
    pub static fn validateProtobufWire(data: []const u8): ! {
        try ProtoReader.validateProtobufWireWithMaxDepth(data, PROTOBUF_MAX_RECURSION_DEPTH);
    }

    /// Zero-heap full-stream wire format validator with explicit recursion depth limit.
    pub static fn validateProtobufWireWithMaxDepth(data: []const u8, max_depth: usize): ! {
        let reader = ProtoReader.initWithMaxDepth(data, max_depth);
        while (field := reader.next()?) {
            try reader.skipField(field);
        }
    }

    /// Creates a nested zero-copy `ProtoReader` for a `LengthDelimited` submessage field,
    /// enforcing the maximum recursion depth limit (`MaxDepthExceeded`).
    fn readSubmessage(field: ProtoField): ProtoReader! {
        if (field.wire_type != WireType.LengthDelimited) {
            return ProtoError.InvalidWireType;
        }
        if (_.depth >= _.max_depth) {
            return ProtoError.MaxDepthExceeded;
        }
        return ProtoReader{
            data: field.raw_bytes,
            pos: 0,
            depth: _.depth + 1,
            max_depth: _.max_depth,
        };
    }

    /// Decodes a packed repeated varint byte slice into an owned `[]u64` slice.
    static fn decodePackedVarints(allocator: std.mem.Allocator, raw_bytes: []const u8): ([]u64)! {
        let sub = ProtoReader.init(raw_bytes);
        let out: std.ArrayList(u64) = .empty;
        errdefer out.deinit(allocator);
        while (sub.hasMore()) {
            const v = sub.readVarint()?;
            out.append(allocator, v) catch return ProtoError.OutOfMemory;
        }
        return out.toOwnedSlice(allocator) catch return ProtoError.OutOfMemory;
    }
}
