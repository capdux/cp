// Source: cp/lib/tlv/types.cp (ASN.1 BER/DER Tag, Length, Error & TlvNode Types)
// Pure CP zero-C-dependency ASN.1 BER/DER Tag-Length-Value core data structures.

const std = @import("std");

/// Default maximum recursion depth when decoding nested constructed TLV nodes.
pub const TLV_MAX_DEPTH: usize = 32;

/// Universal ASN.1 tag numbers.
pub const TAG_NUM_BOOLEAN: u32 = 0x01;
pub const TAG_NUM_INTEGER: u32 = 0x02;
pub const TAG_NUM_BIT_STRING: u32 = 0x03;
pub const TAG_NUM_OCTET_STRING: u32 = 0x04;
pub const TAG_NUM_NULL: u32 = 0x05;
pub const TAG_NUM_OID: u32 = 0x06;
pub const TAG_NUM_UTF8_STRING: u32 = 0x0C;
pub const TAG_NUM_PRINTABLE_STRING: u32 = 0x13;
pub const TAG_NUM_IA5_STRING: u32 = 0x16;
pub const TAG_NUM_UTC_TIME: u32 = 0x17;
pub const TAG_NUM_GENERALIZED_TIME: u32 = 0x18;
pub const TAG_NUM_SEQUENCE: u32 = 0x10;
pub const TAG_NUM_SET: u32 = 0x11;

/// Errors returned by ASN.1 BER/DER TLV decoding, validation, and encoding.
pub err TlvError {
    UnexpectedEof,
    InvalidTag,
    InvalidLength,
    NonCanonicalLength,
    IndefiniteLengthNotAllowed,
    TrailingBytes,
    MaxDepthExceeded,
    InvalidBoolean,
    InvalidInteger,
    TagMismatch,
    InvalidOid,
    OutOfMemory,
}

/// ASN.1 Tag Class (bits 7-6 of the identifier octet).
pub enum TagClass: u8 {
    Universal = 0,
    Application = 1,
    ContextSpecific = 2,
    Private = 3,
}

pub fn tagClassToU8(c: TagClass): u8 {
    return switch (c) {
        TagClass.Universal => 0,
        TagClass.Application => 1,
        TagClass.ContextSpecific => 2,
        TagClass.Private => 3,
    };
}

pub fn tagClassFromU8(v: u8): TagClass ! TlvError {
    return switch (v) {
        0 => TagClass.Universal,
        1 => TagClass.Application,
        2 => TagClass.ContextSpecific,
        3 => TagClass.Private,
        else => TlvError.InvalidTag,
    };
}

/// Result of decoding an ASN.1 BER/DER tag header.
pub struct TlvTagDecodeResult {
    tag: TlvTag = TlvTag{},
    bytes_read: usize = 0,
}

/// Result of decoding an ASN.1 BER/DER length header.
pub struct TlvLengthDecodeResult {
    length: usize = 0,
    bytes_read: usize = 0,
}

/// ASN.1 BER/DER Tag supporting single-byte (0..30) and multi-byte Base-128 high-tag-number form.
pub struct TlvTag {
    tag_class: TagClass = TagClass.Universal,
    constructed: bool = false,
    number: u32 = 0,

    pub static fn init(tag_class: TagClass, constructed: bool, number: u32): TlvTag {
        return TlvTag{
            tag_class: tag_class,
            constructed: constructed,
            number: number,
        };
    }

    pub static fn universal(number: u32, constructed: bool): TlvTag {
        return TlvTag{
            tag_class: TagClass.Universal,
            constructed: constructed,
            number: number,
        };
    }

    pub static fn application(number: u32, constructed: bool): TlvTag {
        return TlvTag{
            tag_class: TagClass.Application,
            constructed: constructed,
            number: number,
        };
    }

    pub static fn contextSpecific(number: u32, constructed: bool): TlvTag {
        return TlvTag{
            tag_class: TagClass.ContextSpecific,
            constructed: constructed,
            number: number,
        };
    }

    pub static fn privateTag(number: u32, constructed: bool): TlvTag {
        return TlvTag{
            tag_class: TagClass.Private,
            constructed: constructed,
            number: number,
        };
    }

    pub fn eql(other: TlvTag): bool {
        return (tagClassToU8(_.tag_class) == tagClassToU8(other.tag_class)) &&
            (_.constructed == other.constructed) &&
            (_.number == other.number);
    }

    /// Returns the number of bytes required to DER-encode this tag.
    pub fn encodedLen(): usize {
        if (_.number < 31) {
            return 1;
        }
        let n: u32 = _.number;
        let count: usize = 0;
        while (n > 0) {
            count += 1;
            n = n >> 7;
        }
        return 1 + count;
    }

    /// Encodes this tag into `buf`, returning the number of bytes written.
    pub fn encodeTo(buf: []u8): usize ! TlvError {
        const needed = _.encodedLen();
        if (buf.len < needed) {
            return TlvError.UnexpectedEof;
        }
        const class_bits: u8 = (tagClassToU8(_.tag_class) & 0x03) << 6;
        const pc_bit: u8 = if (_.constructed) 0x20 else 0x00;

        if (_.number < 31) {
            buf[0] = class_bits | pc_bit | ((_.number as u8) & 0x1F);
            return 1;
        }

        buf[0] = class_bits | pc_bit | 0x1F;
        let idx: usize = needed - 1;
        let n: u32 = _.number;
        let first: bool = true;
        while (n > 0) {
            let b: u8 = (n & 0x7F) as u8;
            if (!first) {
                b = b | 0x80;
            }
            buf[idx] = b;
            first = false;
            n = n >> 7;
            if (idx > 1) {
                idx -= 1;
            }
        }
        return needed;
    }

    /// Decodes a BER/DER tag from `bytes`, enforcing minimal Base-128 tag encoding.
    pub static fn decode(bytes: []const u8): TlvTagDecodeResult ! TlvError {
        if (bytes.len == 0) {
            return TlvError.UnexpectedEof;
        }
        const b0 = bytes[0];
        const class_val = tagClassFromU8((b0 >> 6) & 0x03)?;
        const constructed = (b0 & 0x20) != 0;
        const low = b0 & 0x1F;

        if (low < 31) {
            return TlvTagDecodeResult{
                tag: TlvTag{
                    tag_class: class_val,
                    constructed: constructed,
                    number: low as u32,
                },
                bytes_read: 1,
            };
        }

        // High-tag-number form (Base-128)
        if (bytes.len < 2) {
            return TlvError.UnexpectedEof;
        }
        // DER forbids leading zero 7-bit group (0x80)
        if (bytes[1] == 0x80) {
            return TlvError.InvalidTag;
        }

        let num: u32 = 0;
        let pos: usize = 1;
        while (pos < bytes.len) : (pos += 1) {
            const b = bytes[pos];
            // Check u32 overflow before shifting 7 bits: max value before shift is 0x01FFFFFF
            if (num > 0x01FFFFFF) {
                return TlvError.InvalidTag;
            }
            num = (num << 7) | ((b & 0x7F) as u32);
            if ((b & 0x80) == 0) {
                // DER forbids using high-tag-number form for numbers < 31
                if (num < 31) {
                    return TlvError.InvalidTag;
                }
                return TlvTagDecodeResult{
                    tag: TlvTag{
                        tag_class: class_val,
                        constructed: constructed,
                        number: num,
                    },
                    bytes_read: pos + 1,
                };
            }
        }

        return TlvError.UnexpectedEof;
    }
}

/// Returns the number of bytes required to DER-encode `length`.
pub fn derLengthSize(length: usize): usize {
    if (length < 128) {
        return 1;
    }
    let tmp: usize = length;
    let count: usize = 0;
    while (tmp > 0) {
        count += 1;
        tmp = tmp >> 8;
    }
    return 1 + count;
}

/// Writes canonical DER length octets into `buf`, returning bytes written.
pub fn encodeDerLengthTo(buf: []u8, length: usize): usize ! TlvError {
    const needed = derLengthSize(length);
    if (buf.len < needed) {
        return TlvError.UnexpectedEof;
    }
    if (length < 128) {
        buf[0] = (length & 0x7F) as u8;
        return 1;
    }
    const num_octets = needed - 1;
    buf[0] = (0x80 as u8) | ((num_octets & 0x7F) as u8);
    let tmp: usize = length;
    let i: usize = needed - 1;
    while (i >= 1) {
        buf[i] = (tmp & 0xFF) as u8;
        tmp = tmp >> 8;
        if (i == 1) {
            break;
        }
        i -= 1;
    }
    return needed;
}

/// Decodes a strict DER length field from `bytes`.
/// Rejects indefinite length (`0x80`), reserved `0xFF`, redundant leading `0x00`, and non-minimal long form (`len < 128`).
pub fn decodeDerLength(bytes: []const u8): TlvLengthDecodeResult ! TlvError {
    if (bytes.len == 0) {
        return TlvError.UnexpectedEof;
    }
    const b0 = bytes[0];
    if (b0 < 0x80) {
        return TlvLengthDecodeResult{
            length: b0 as usize,
            bytes_read: 1,
        };
    }
    if (b0 == 0x80) {
        return TlvError.IndefiniteLengthNotAllowed;
    }
    if (b0 == 0xFF) {
        return TlvError.InvalidLength;
    }

    const num_octets: usize = (b0 & 0x7F) as usize;
    if (num_octets > @sizeOf(usize)) {
        return TlvError.InvalidLength;
    }
    if (1 + num_octets > bytes.len) {
        return TlvError.UnexpectedEof;
    }
    // Redundant leading zero octet in long form is non-canonical in DER
    if (bytes[1] == 0x00) {
        return TlvError.NonCanonicalLength;
    }

    let len: usize = 0;
    let i: usize = 0;
    while (i < num_octets) : (i += 1) {
        len = (len << 8) | (bytes[1 + i] as usize);
    }

    // Length < 128 must use short form in DER
    if (len < 128) {
        return TlvError.NonCanonicalLength;
    }

    return TlvLengthDecodeResult{
        length: len,
        bytes_read: 1 + num_octets,
    };
}

/// Decoded ASN.1 BER/DER TLV tree node with zero-copy `raw_value` slice view.
pub struct TlvNode {
    tag: TlvTag = TlvTag{},
    raw_value: []const u8 = "",
    children: []TlvNode = [],

    /// Recursively frees allocated `children` slices.
    pub fn deinit(*_, alloc: std.mem.Allocator) {
        let i: usize = 0;
        while (i < _.children.len) : (i += 1) {
            _.children[i].deinit(alloc);
        }
        if (_.children.len > 0) {
            alloc.free(_.children);
        }
        _.children = [];
    }

    /// Finds the first direct child matching `tag_number`, or `null` if absent.
    pub fn findChild(tag_number: u32): TlvNode? {
        let i: usize = 0;
        while (i < _.children.len) : (i += 1) {
            if (_.children[i].tag.number == tag_number) {
                return _.children[i];
            }
        }
        return null;
    }

    /// Interprets a Universal primitive BOOLEAN node (`0x00` = false, `0xFF` = true).
    pub fn asBoolean(): bool ! TlvError {
        if (tagClassToU8(_.tag.tag_class) != 0 || _.tag.constructed || _.tag.number != TAG_NUM_BOOLEAN) {
            return TlvError.TagMismatch;
        }
        if (_.raw_value.len != 1) {
            return TlvError.InvalidBoolean;
        }
        if (_.raw_value[0] == 0x00) {
            return false;
        }
        if (_.raw_value[0] == 0xFF) {
            return true;
        }
        return TlvError.InvalidBoolean;
    }

    /// Validates and returns minimal two's-complement DER INTEGER octets.
    pub fn asIntegerBytes(): ([]const u8) ! TlvError {
        if (tagClassToU8(_.tag.tag_class) != 0 || _.tag.constructed || _.tag.number != TAG_NUM_INTEGER) {
            return TlvError.TagMismatch;
        }
        if (_.raw_value.len == 0) {
            return TlvError.InvalidInteger;
        }
        if (_.raw_value.len >= 2) {
            const b0 = _.raw_value[0];
            const b1 = _.raw_value[1];
            if ((b0 == 0x00 && (b1 & 0x80) == 0) || (b0 == 0xFF && (b1 & 0x80) != 0)) {
                return TlvError.InvalidInteger;
            }
        }
        return _.raw_value;
    }

    /// Decodes a non-negative DER INTEGER node into `u64`.
    pub fn asU64(): u64 ! TlvError {
        const bytes = _.asIntegerBytes()?;
        if ((bytes[0] & 0x80) != 0) {
            return TlvError.InvalidInteger;
        }
        let slice = bytes;
        if (slice.len > 1 && slice[0] == 0x00) {
            slice = slice[1..];
        }
        if (slice.len > 8) {
            return TlvError.InvalidInteger;
        }
        let acc: u64 = 0;
        let i: usize = 0;
        while (i < slice.len) : (i += 1) {
            acc = (acc << 8) | (slice[i] as u64);
        }
        return acc;
    }

    /// Returns the raw payload of a Universal primitive OCTET STRING node.
    pub fn asOctetString(): ([]const u8) ! TlvError {
        if (tagClassToU8(_.tag.tag_class) != 0 || _.tag.constructed || _.tag.number != TAG_NUM_OCTET_STRING) {
            return TlvError.TagMismatch;
        }
        return _.raw_value;
    }

    /// Returns and validates a Universal primitive UTF8String node.
    pub fn asUtf8String(): ([]const u8) ! TlvError {
        if (tagClassToU8(_.tag.tag_class) != 0 || _.tag.constructed || _.tag.number != TAG_NUM_UTF8_STRING) {
            return TlvError.TagMismatch;
        }
        if (!std.unicode.utf8ValidateSlice(_.raw_value)) {
            return TlvError.InvalidLength;
        }
        return _.raw_value;
    }
}

/// Internal growable byte buffer used by OID, encoder, and writer.
pub struct ByteBuf {
    alloc: std.mem.Allocator = undefined,
    data: []u8 = [],
    len: usize = 0,

    pub static fn init(alloc: std.mem.Allocator): ByteBuf {
        return ByteBuf{
            alloc: alloc,
            data: [],
            len: 0,
        };
    }

    pub fn deinit(*_) {
        if (_.data.len > 0) {
            _.alloc.free(_.data);
        }
        _.data = [];
        _.len = 0;
    }

    pub fn ensureUnused(*_, extra: usize): ! TlvError {
        const needed = _.len + extra;
        if (needed <= _.data.len) {
            return;
        }
        let new_cap: usize = if (_.data.len == 0) 64 else _.data.len;
        while (new_cap < needed) {
            new_cap = new_cap * 2;
        }
        const next_buf = _.alloc.realloc(_.data, new_cap) catch return TlvError.OutOfMemory;
        _.data = next_buf;
    }

    pub fn appendByte(*_, b: u8): ! TlvError {
        _.ensureUnused(1)?;
        _.data[_.len] = b;
        _.len += 1;
    }

    pub fn appendSlice(*_, s: []const u8): ! TlvError {
        if (s.len == 0) {
            return;
        }
        _.ensureUnused(s.len)?;
        @memcpy(_.data[_.len .. _.len + s.len], s);
        _.len += s.len;
    }

    pub fn toOwnedSlice(*_): ([]u8) ! TlvError {
        if (_.len == 0) {
            _.deinit();
            return [];
        }
        const out = _.alloc.alloc(u8, _.len) catch return TlvError.OutOfMemory;
        @memcpy(out, _.data[0 .. _.len]);
        _.deinit();
        return out;
    }
}
