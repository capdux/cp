// Source: cp/lib/protobuf/wire.cp (Protocol Buffers v3 Wire Format Zero-Copy Codec)
// Pure CP zero-copy Protocol Buffers v3 wire format encoder & decoder.
// Supports LEB128 Varint, ZigZag sint32/sint64, Fixed32/Fixed64/Float/Double,
// length-delimited bytes/strings/submessages, packed repeated varints, and skipField.

const std = @import("std");

/// Error set returned by Protocol Buffers v3 wire encoding/decoding operations.
pub err ProtoError {
    UnexpectedEof,
    VarintOverflow,
    InvalidWireType,
    InvalidFieldNumber,
    InvalidLength,
    OutOfMemory,
}

/// Protocol Buffers wire format types (3-bit tag discriminator).
pub enum WireType: u8 {
    Varint = 0,
    Fixed64 = 1,
    Len = 2,
    Fixed32 = 5,

    /// Converts a raw 3-bit wire type integer into `WireType`.
    static fn fromInt(v: u8): WireType! {
        switch (v) {
            0 => return WireType.Varint,
            1 => return WireType.Fixed64,
            2 => return WireType.Len,
            5 => return WireType.Fixed32,
            else => return ProtoError.InvalidWireType,
        }
    }

    /// Returns the numeric wire type code.
    fn toInt(): u8 {
        return _ as u8;
    }
}

/// ZigZag signed integer codec mapping signed integers to unsigned varints.
pub static class ZigZag {
    /// Encodes a 32-bit signed integer using ZigZag encoding.
    fn encode32(n: i32): u32 {
        const u: u32 = @bitCast(n);
        const sign_mask: u32 = if (n < 0) std.math.maxInt(u32) else 0;
        return (u << 1) ^ sign_mask;
    }

    /// Decodes a ZigZag-encoded 32-bit unsigned value back into `i32`.
    fn decode32(n: u32): i32 {
        const shifted: u32 = n >> 1;
        const sign_mask: u32 = if ((n & 1) != 0) std.math.maxInt(u32) else 0;
        return @bitCast(shifted ^ sign_mask);
    }

    /// Encodes a 64-bit signed integer using ZigZag encoding.
    fn encode64(n: i64): u64 {
        const u: u64 = @bitCast(n);
        const sign_mask: u64 = if (n < 0) std.math.maxInt(u64) else 0;
        return (u << 1) ^ sign_mask;
    }

    /// Decodes a ZigZag-encoded 64-bit unsigned value back into `i64`.
    fn decode64(n: u64): i64 {
        const shifted: u64 = n >> 1;
        const sign_mask: u64 = if ((n & 1) != 0) std.math.maxInt(u64) else 0;
        return @bitCast(shifted ^ sign_mask);
    }
}

/// Maximum valid Protobuf field number (`2^29 - 1`).
pub const MAX_FIELD_NUMBER: u32 = 536870911;
/// Start of Protobuf reserved implementation field number range (inclusive).
pub const RESERVED_FIELD_START: u32 = 19000;
/// End of Protobuf reserved implementation field number range (inclusive).
pub const RESERVED_FIELD_END: u32 = 19999;

/// Decoded Protobuf field tag combining a validated field number and wire type.
pub struct ProtoTag {
    field_number: u32;
    wire_type: WireType;

    /// Returns `true` if `field_number` is within `1..536870911` and outside `19000..19999`.
    static fn isValidFieldNumber(field_number: u32): bool {
        if (field_number == 0 || field_number > MAX_FIELD_NUMBER) {
            return false;
        }
        if (field_number >= RESERVED_FIELD_START && field_number <= RESERVED_FIELD_END) {
            return false;
        }
        return true;
    }

    /// Constructs a validated `ProtoTag`.
    static fn init(field_number: u32, wire_type: WireType): ProtoTag! {
        if (!ProtoTag.isValidFieldNumber(field_number)) {
            return ProtoError.InvalidFieldNumber;
        }
        return ProtoTag{
            field_number: field_number,
            wire_type: wire_type,
        };
    }

    /// Decodes a raw varint tag into a validated `ProtoTag`.
    static fn decode(raw: u64): ProtoTag! {
        const wt_raw: u8 = (raw & 0x07) as u8;
        const wt = WireType.fromInt(wt_raw)?;
        const fn_u64: u64 = raw >> 3;
        if (fn_u64 == 0 || fn_u64 > (MAX_FIELD_NUMBER as u64)) {
            return ProtoError.InvalidFieldNumber;
        }
        const field_num: u32 = fn_u64 as u32;
        if (field_num >= RESERVED_FIELD_START && field_num <= RESERVED_FIELD_END) {
            return ProtoError.InvalidFieldNumber;
        }
        return ProtoTag{
            field_number: field_num,
            wire_type: wt,
        };
    }

    /// Encodes the tag into its raw unsigned 32-bit key value.
    fn encode(): u32 {
        return (_.field_number << 3) | ((_.wire_type as u8) as u32);
    }
}

/// Protocol Buffers v3 binary wire encoder backed by an owned `std.ArrayList(u8)`.
pub struct ProtoWriter {
    allocator: std.mem.Allocator = undefined;
    buf: std.ArrayList(u8) = .empty;

    /// Creates an empty `ProtoWriter` using `alloc`.
    static fn init(alloc: std.mem.Allocator): ProtoWriter {
        return ProtoWriter{
            allocator: alloc,
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

    /// Writes a validated `(field_number, wire_type)` tag as a varint.
    fn writeTag(*_, field_number: u32, wire_type: WireType): ! {
        const tag = ProtoTag.init(field_number, wire_type)?;
        _.writeRawVarint(tag.encode() as u64)?;
    }

    /// Encodes and appends a raw base-128 unsigned varint (1 to 10 bytes).
    fn writeRawVarint(*_, val: u64): ! {
        let v: u64 = val;
        while (v >= 0x80) {
            const byte: u8 = ((v & 0x7F) as u8) | 0x80;
            _.buf.append(_.allocator, byte) catch return ProtoError.OutOfMemory;
            v >>= 7;
        }
        _.buf.append(_.allocator, (v & 0x7F) as u8) catch return ProtoError.OutOfMemory;
    }

    /// Writes a raw 32-bit little-endian value without a field tag.
    fn writeRawFixed32(*_, val: u32): ! {
        let out: [4]u8 = [
            (val & 0xFF) as u8,
            ((val >> 8) & 0xFF) as u8,
            ((val >> 16) & 0xFF) as u8,
            ((val >> 24) & 0xFF) as u8,
        ];
        _.buf.appendSlice(_.allocator, &out) catch return ProtoError.OutOfMemory;
    }

    /// Writes a raw 64-bit little-endian value without a field tag.
    fn writeRawFixed64(*_, val: u64): ! {
        let out: [8]u8 = [
            (val & 0xFF) as u8,
            ((val >> 8) & 0xFF) as u8,
            ((val >> 16) & 0xFF) as u8,
            ((val >> 24) & 0xFF) as u8,
            ((val >> 32) & 0xFF) as u8,
            ((val >> 40) & 0xFF) as u8,
            ((val >> 48) & 0xFF) as u8,
            ((val >> 56) & 0xFF) as u8,
        ];
        _.buf.appendSlice(_.allocator, &out) catch return ProtoError.OutOfMemory;
    }

    /// Writes an unsigned varint field (`uint32` / `uint64` / `enum`).
    fn writeVarint(*_, field_number: u32, val: u64): ! {
        _.writeTag(field_number, WireType.Varint)?;
        _.writeRawVarint(val)?;
    }

    /// Writes a standard two's-complement `int32` field (sign-extended to 64 bits if negative).
    fn writeInt32(*_, field_number: u32, val: i32): ! {
        const ext: i64 = val as i64;
        const raw: u64 = @bitCast(ext);
        _.writeVarint(field_number, raw)?;
    }

    /// Writes a standard two's-complement `int64` field.
    fn writeInt64(*_, field_number: u32, val: i64): ! {
        const raw: u64 = @bitCast(val);
        _.writeVarint(field_number, raw)?;
    }

    /// Writes a ZigZag-encoded `sint32` field.
    fn writeSint32(*_, field_number: u32, val: i32): ! {
        _.writeTag(field_number, WireType.Varint)?;
        _.writeRawVarint(ZigZag.encode32(val) as u64)?;
    }

    /// Writes a ZigZag-encoded `sint64` field.
    fn writeSint64(*_, field_number: u32, val: i64): ! {
        _.writeTag(field_number, WireType.Varint)?;
        _.writeRawVarint(ZigZag.encode64(val))?;
    }

    /// Writes a boolean field as a 1-byte varint (`0` or `1`).
    fn writeBool(*_, field_number: u32, val: bool): ! {
        _.writeTag(field_number, WireType.Varint)?;
        _.writeRawVarint(if (val) 1 else 0)?;
    }

    /// Writes a `fixed32` / `sfixed32` field in little-endian byte order.
    fn writeFixed32(*_, field_number: u32, val: u32): ! {
        _.writeTag(field_number, WireType.Fixed32)?;
        _.writeRawFixed32(val)?;
    }

    /// Writes a `fixed64` / `sfixed64` field in little-endian byte order.
    fn writeFixed64(*_, field_number: u32, val: u64): ! {
        _.writeTag(field_number, WireType.Fixed64)?;
        _.writeRawFixed64(val)?;
    }

    /// Writes an IEEE-754 `float` (`f32`) field as 32-bit little-endian bits.
    fn writeFloat(*_, field_number: u32, val: f32): ! {
        const bits: u32 = @bitCast(val);
        _.writeFixed32(field_number, bits)?;
    }

    /// Writes an IEEE-754 `double` (`f64`) field as 64-bit little-endian bits.
    fn writeDouble(*_, field_number: u32, val: f64): ! {
        const bits: u64 = @bitCast(val);
        _.writeFixed64(field_number, bits)?;
    }

    /// Writes a length-delimited `bytes` field.
    fn writeBytes(*_, field_number: u32, data: []const u8): ! {
        if (data.len > 0x7FFFFFFF) {
            return ProtoError.InvalidLength;
        }
        _.writeTag(field_number, WireType.Len)?;
        _.writeRawVarint(data.len as u64)?;
        if (data.len > 0) {
            _.buf.appendSlice(_.allocator, data) catch return ProtoError.OutOfMemory;
        }
    }

    /// Writes a length-delimited UTF-8 `string` field.
    fn writeString(*_, field_number: u32, text: []const u8): ! {
        _.writeBytes(field_number, text)?;
    }

    /// Writes an embedded submessage encoded as a length-delimited byte payload.
    fn writeSubmessage(*_, field_number: u32, encoded: []const u8): ! {
        _.writeBytes(field_number, encoded)?;
    }

    /// Writes a proto3 packed repeated varint field (`WireType.Len`).
    /// Emits nothing when `vals` is empty, matching proto3 canonical serialization.
    fn writePackedVarints(*_, field_number: u32, vals: []const u64): ! {
        if (vals.len == 0) {
            return;
        }
        let total_bytes: usize = 0;
        for (v of vals) {
            total_bytes += ProtoWriter.varintSize(v);
        }
        if (total_bytes > 0x7FFFFFFF) {
            return ProtoError.InvalidLength;
        }
        _.writeTag(field_number, WireType.Len)?;
        _.writeRawVarint(total_bytes as u64)?;
        for (v of vals) {
            _.writeRawVarint(v)?;
        }
    }

    /// Computes the encoded byte length of an unsigned varint without allocating.
    static fn varintSize(val: u64): usize {
        let v: u64 = val;
        let size: usize = 1;
        while (v >= 0x80) {
            v >>= 7;
            size += 1;
        }
        return size;
    }
}

/// Zero-copy slice cursor for decoding Protocol Buffers v3 wire payloads.
pub struct ProtoReader {
    data: []const u8;
    pos: usize = 0;

    /// Creates a `ProtoReader` over `data` without copying or allocating.
    static fn init(data: []const u8): ProtoReader {
        return ProtoReader{
            data: data,
            pos: 0,
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

    /// Reads the next field tag, returning `null` cleanly at end-of-stream.
    fn nextTag(*_): ProtoTag?! {
        if (_.pos >= _.data.len) {
            return null;
        }
        const raw = _.readVarint()?;
        return ProtoTag.decode(raw)?;
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
                // 10th byte must terminate (MSB=0) and carry at most bit 63 (value 0 or 1).
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

    /// Reads a two's-complement `int32` varint field.
    fn readInt32(*_): i32! {
        const raw = _.readVarint()?;
        const signed64: i64 = @bitCast(raw);
        return signed64 as i32;
    }

    /// Reads a two's-complement `int64` varint field.
    fn readInt64(*_): i64! {
        const raw = _.readVarint()?;
        return @bitCast(raw);
    }

    /// Reads and ZigZag-decodes an `sint32` varint field.
    fn readSint32(*_): i32! {
        const raw = _.readVarint()?;
        if (raw > 0xFFFFFFFF) {
            return ProtoError.VarintOverflow;
        }
        return ZigZag.decode32(raw as u32);
    }

    /// Reads and ZigZag-decodes an `sint64` varint field.
    fn readSint64(*_): i64! {
        const raw = _.readVarint()?;
        return ZigZag.decode64(raw);
    }

    /// Reads a boolean varint field (`0` -> `false`, non-zero -> `true`).
    fn readBool(*_): bool! {
        const raw = _.readVarint()?;
        return raw != 0;
    }

    /// Reads a 32-bit little-endian `fixed32` value.
    fn readFixed32(*_): u32! {
        if (_.remaining() < 4) {
            return ProtoError.UnexpectedEof;
        }
        const p = _.pos;
        const val: u32 = (_.data[p] as u32)
            | ((_.data[p + 1] as u32) << 8)
            | ((_.data[p + 2] as u32) << 16)
            | ((_.data[p + 3] as u32) << 24);
        _.pos += 4;
        return val;
    }

    /// Reads a 64-bit little-endian `fixed64` value.
    fn readFixed64(*_): u64! {
        if (_.remaining() < 8) {
            return ProtoError.UnexpectedEof;
        }
        const p = _.pos;
        const val: u64 = (_.data[p] as u64)
            | ((_.data[p + 1] as u64) << 8)
            | ((_.data[p + 2] as u64) << 16)
            | ((_.data[p + 3] as u64) << 24)
            | ((_.data[p + 4] as u64) << 32)
            | ((_.data[p + 5] as u64) << 40)
            | ((_.data[p + 6] as u64) << 48)
            | ((_.data[p + 7] as u64) << 56);
        _.pos += 8;
        return val;
    }

    /// Reads a 32-bit IEEE-754 `float`.
    fn readFloat(*_): f32! {
        const bits = _.readFixed32()?;
        return @bitCast(bits);
    }

    /// Reads a 64-bit IEEE-754 `double`.
    fn readDouble(*_): f64! {
        const bits = _.readFixed64()?;
        return @bitCast(bits);
    }

    /// Reads a length-delimited byte slice as a zero-copy view into the underlying buffer.
    fn readBytes(*_): ([]const u8)! {
        const len_u64 = _.readVarint()?;
        if (len_u64 > 0x7FFFFFFF) {
            return ProtoError.InvalidLength;
        }
        const byte_len: usize = len_u64 as usize;
        if (byte_len > _.remaining()) {
            return ProtoError.UnexpectedEof;
        }
        const slice = _.data[_.pos .. _.pos + byte_len];
        _.pos += byte_len;
        return slice;
    }

    /// Reads a length-delimited UTF-8 string as a zero-copy view into the underlying buffer.
    fn readString(*_): ([]const u8)! {
        return _.readBytes();
    }

    /// Reads a length-delimited submessage and wraps it in a nested zero-copy `ProtoReader`.
    fn readSubmessage(*_): ProtoReader! {
        const payload = _.readBytes()?;
        return ProtoReader.init(payload);
    }

    /// Decodes a length-delimited packed repeated varint field into an owned `[]u64` slice.
    fn readPackedVarints(*_, alloc: std.mem.Allocator): ([]u64)! {
        const packed_slice = _.readBytes()?;
        let sub = ProtoReader.init(packed_slice);
        let out: std.ArrayList(u64) = .empty;
        errdefer out.deinit(alloc);
        while (sub.hasMore()) {
            const v = sub.readVarint()?;
            out.append(alloc, v) catch return ProtoError.OutOfMemory;
        }
        return out.toOwnedSlice(alloc) catch return ProtoError.OutOfMemory;
    }

    /// Skips an unknown field's payload based on its `WireType` for forward compatibility.
    fn skipField(*_, wire_type: WireType): ! {
        switch (wire_type) {
            .Varint => {
                * = _.readVarint()?;
            },
            .Fixed64 => {
                * = _.readFixed64()?;
            },
            .Len => {
                * = _.readBytes()?;
            },
            .Fixed32 => {
                * = _.readFixed32()?;
            },
        }
    }
}
