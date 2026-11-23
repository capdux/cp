// Reference-hardened against RustCrypto/formats (MIT/Apache-2.0)
// Source: cp/lib/asn1/der.cp (ITU-T X.690 ASN.1 DER Encoder, Decoder & OID Codec)
// Pure CP zero-C-dependency ITU-T X.690 Distinguished Encoding Rules (DER)
// zero-copy TLV reader, canonical TLV writer, and Object Identifier (OID) codec.

const std = @import("std");

/// 16-lane SIMD byte vector for DER payload copy/compare and string validation.
type Vec16u8 = @Vector(16, u8);

/// Maximum allowed nested container depth when decoding ASN.1 DER structures (aligned with RustCrypto/formats).
pub const DER_MAX_DEPTH: usize = 32;
pub const ASN1_MAX_DEPTH: usize = DER_MAX_DEPTH;

/// Universal and common context-specific ASN.1 DER tag constants.
pub const TAG_BOOLEAN: u8 = 0x01;
pub const TAG_INTEGER: u8 = 0x02;
pub const TAG_BIT_STRING: u8 = 0x03;
pub const TAG_OCTET_STRING: u8 = 0x04;
pub const TAG_NULL: u8 = 0x05;
pub const TAG_OID: u8 = 0x06;
pub const TAG_UTF8_STRING: u8 = 0x0C;
pub const TAG_PRINTABLE_STRING: u8 = 0x13;
pub const TAG_T61_STRING: u8 = 0x14;
pub const TAG_IA5_STRING: u8 = 0x16;
pub const TAG_UTC_TIME: u8 = 0x17;
pub const TAG_GENERALIZED_TIME: u8 = 0x18;
pub const TAG_BMP_STRING: u8 = 0x1E;
pub const TAG_SEQUENCE: u8 = 0x30;
pub const TAG_SET: u8 = 0x31;

/// Errors returned by ASN.1 DER decoding, OID encoding, and X.509 parsing.
pub err Asn1Error {
    UnexpectedEof,
    InvalidLength,
    NonCanonicalLength,
    TagMismatch,
    InvalidOid,
    InvalidTime,
    MaxDepthExceeded,
    MalformedCert,
    OutOfMemory,
}

/// Zero-copy view of a single decoded ASN.1 DER TLV (Tag-Length-Value) element.
pub struct DerElement {
    tag: u8 = 0,
    content: []const u8 = "",
    raw: []const u8 = "",

    /// Scalar reference: copy bytes (for 0-diff parity with SIMD).
    pub static fn copyBytesScalar(dst: []u8, src: []const u8) {
        let n = if (dst.len < src.len) dst.len else src.len;
        let i: usize = 0;
        while (i < n) : (i += 1) {
            dst[i] = src[i];
        }
    }

    /// 16-lane SIMD accelerated byte copy for DER payloads.
    pub static fn copyBytesSimd(dst: []u8, src: []const u8) {
        let n = if (dst.len < src.len) dst.len else src.len;
        let i: usize = 0;
        while (i + 16 <= n) {
            let v: Vec16u8 = src[i .. i + 16][0..16].*;
            dst[i .. i + 16][0..16].* = v;
            i += 16;
        }
        while (i < n) : (i += 1) {
            dst[i] = src[i];
        }
    }

    /// Scalar reference: constant-length byte slice equality.
    pub static fn eqlBytesScalar(a: []const u8, b: []const u8): bool {
        if (a.len != b.len) return false;
        let i: usize = 0;
        while (i < a.len) : (i += 1) {
            if (a[i] != b[i]) return false;
        }
        return true;
    }

    /// 16-lane SIMD accelerated byte slice equality for DER raw TLV comparisons.
    pub static fn eqlBytesSimd(a: []const u8, b: []const u8): bool {
        if (a.len != b.len) return false;
        let i: usize = 0;
        while (i + 16 <= a.len) {
            const va: Vec16u8 = a[i .. i + 16][0..16].*;
            const vb: Vec16u8 = b[i .. i + 16][0..16].*;
            const diff = va ^ vb;
            if (@reduce(.Or, diff) != 0) return false;
            i += 16;
        }
        while (i < a.len) : (i += 1) {
            if (a[i] != b[i]) return false;
        }
        return true;
    }

    /// Scalar reference: validates X.680 IA5String (7-bit ASCII < 0x80).
    pub static fn isValidIa5StringScalar(s: []const u8): bool {
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            if (s[i] >= 0x80) return false;
        }
        return true;
    }

    /// 16-lane SIMD accelerated X.680 IA5String validation (all bytes < 0x80).
    pub static fn isValidIa5StringSimd(s: []const u8): bool {
        const hi_mask: Vec16u8 = @splat(0x80);
        let i: usize = 0;
        while (i + 16 <= s.len) {
            const v: Vec16u8 = s[i .. i + 16][0..16].*;
            if (@reduce(.Or, v & hi_mask) != 0) return false;
            i += 16;
        }
        while (i < s.len) : (i += 1) {
            if (s[i] >= 0x80) return false;
        }
        return true;
    }

    /// Checks a single byte against the X.680 PrintableString alphabet.
    static fn isPrintableByte(c: u8): bool {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            return true;
        }
        return c == ' ' || c == '\'' || c == '(' || c == ')' || c == '+' || c == ',' || c == '-' || c == '.' || c == '/' || c == ':' || c == '=' || c == '?';
    }

    /// Scalar reference: validates X.680 PrintableString alphabet.
    pub static fn isValidPrintableStringScalar(s: []const u8): bool {
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            if (!DerElement.isPrintableByte(s[i])) return false;
        }
        return true;
    }

    /// 16-lane SIMD accelerated X.680 PrintableString validation.
    pub static fn isValidPrintableStringSimd(s: []const u8): bool {
        const hi_mask: Vec16u8 = @splat(0x80);
        const lo_a: Vec16u8 = @splat('a');
        const hi_z: Vec16u8 = @splat('z');
        const lo_A: Vec16u8 = @splat('A');
        const hi_Z: Vec16u8 = @splat('Z');
        const lo_0: Vec16u8 = @splat('0');
        const hi_9: Vec16u8 = @splat('9');

        let i: usize = 0;
        while (i + 16 <= s.len) {
            const v: Vec16u8 = s[i .. i + 16][0..16].*;
            if (@reduce(.Or, v & hi_mask) != 0) return false;
            const is_lower = (v >= lo_a) & (v <= hi_z);
            const is_upper = (v >= lo_A) & (v <= hi_Z);
            const is_digit = (v >= lo_0) & (v <= hi_9);
            const mask: u16 = @bitCast(is_lower | is_upper | is_digit);
            if (mask != 0xFFFF) {
                let lane: usize = 0;
                while (lane < 16) : (lane += 1) {
                    if (((mask >> (lane as u4)) & 1) == 0) {
                        if (!DerElement.isPrintableByte(s[i + lane])) return false;
                    }
                }
            }
            i += 16;
        }
        while (i < s.len) : (i += 1) {
            if (!DerElement.isPrintableByte(s[i])) return false;
        }
        return true;
    }

    /// Returns a child `DerReader` scoped to this element's content octets.
    pub fn reader(): DerReader {
        return DerReader.init(_.content);
    }

    /// Decodes a `BOOLEAN` element per X.690 DER (`0x00` = false, `0xFF` = true).
    pub fn asBoolean(): bool ! Asn1Error {
        if (_.tag != TAG_BOOLEAN) {
            return Asn1Error.TagMismatch;
        }
        if (_.content.len != 1) {
            return Asn1Error.InvalidLength;
        }
        if (_.content[0] == 0x00) {
            return false;
        }
        if (_.content[0] == 0xFF) {
            return true;
        }
        return Asn1Error.NonCanonicalLength;
    }

    /// Validates canonical DER `INTEGER` content and returns its raw two's-complement octets.
    pub fn asIntegerBytes(): ([]const u8) ! Asn1Error {
        if (_.tag != TAG_INTEGER) {
            return Asn1Error.TagMismatch;
        }
        if (_.content.len == 0) {
            return Asn1Error.InvalidLength;
        }
        if (_.content.len >= 2) {
            const b0 = _.content[0];
            const b1 = _.content[1];
            if ((b0 == 0x00 && (b1 & 0x80) == 0) || (b0 == 0xFF && (b1 & 0x80) != 0)) {
                return Asn1Error.NonCanonicalLength;
            }
        }
        return _.content;
    }

    /// Decodes a non-negative `INTEGER` element into `u64`.
    pub fn asUint64(): u64 ! Asn1Error {
        const bytes = _.asIntegerBytes()?;
        if ((bytes[0] & 0x80) != 0) {
            return Asn1Error.InvalidLength;
        }
        let slice = bytes;
        if (slice.len > 1 && slice[0] == 0x00) {
            slice = slice[1..];
        }
        if (slice.len > 8) {
            return Asn1Error.InvalidLength;
        }
        let acc: u64 = 0;
        for (b of slice) {
            acc = (acc << 8) | (b as u64);
        }
        return acc;
    }

    /// Decodes an `OCTET STRING` element as a zero-copy byte slice.
    pub fn asOctetString(): ([]const u8) ! Asn1Error {
        if (_.tag != TAG_OCTET_STRING) {
            return Asn1Error.TagMismatch;
        }
        return _.content;
    }

    /// Decodes a `BIT STRING` element, validating unused-bit count and zero-padded trailing bits per X.690 DER.
    pub fn asBitString(): ([]const u8) ! Asn1Error {
        if (_.tag != TAG_BIT_STRING) {
            return Asn1Error.TagMismatch;
        }
        if (_.content.len == 0) {
            return Asn1Error.InvalidLength;
        }
        const unused = _.content[0];
        if (unused > 7) {
            return Asn1Error.InvalidLength;
        }
        if (_.content.len == 1 && unused != 0) {
            return Asn1Error.InvalidLength;
        }
        if (unused > 0 && _.content.len > 1) {
            const mask: u8 = ((1 as u8) << (unused as u3)) - 1;
            if ((_.content[_.content.len - 1] & mask) != 0) {
                return Asn1Error.NonCanonicalLength;
            }
        }
        return _.content[1..];
    }

    /// Returns the number of unused trailing bits in a `BIT STRING` element.
    pub fn asBitStringUnusedBits(): u8 ! Asn1Error {
        * = _.asBitString()?;
        return _.content[0];
    }

    /// Decodes `UTF8String`, `PrintableString`, `IA5String`, or `T61String` as a zero-copy slice.
    pub fn asString(): ([]const u8) ! Asn1Error {
        if (_.tag == TAG_UTF8_STRING) {
            if (!std.unicode.utf8ValidateSlice(_.content)) {
                return Asn1Error.MalformedCert;
            }
            return _.content;
        }
        if (_.tag == TAG_PRINTABLE_STRING) {
            if (!DerElement.isValidPrintableStringSimd(_.content)) {
                return Asn1Error.MalformedCert;
            }
            return _.content;
        }
        if (_.tag == TAG_IA5_STRING) {
            if (!DerElement.isValidIa5StringSimd(_.content)) {
                return Asn1Error.MalformedCert;
            }
            return _.content;
        }
        if (_.tag == TAG_T61_STRING) {
            return _.content;
        }
        return Asn1Error.TagMismatch;
    }
}

/// Zero-copy ITU-T X.690 DER sequential TLV reader with strict canonical length enforcement.
pub struct DerReader {
    src: []const u8 = "",
    pos: usize = 0,
    depth: usize = 0,

    /// Creates a new `DerReader` positioned at the start of `data`.
    pub static fn init(data: []const u8): DerReader {
        return DerReader{ src: data, pos: 0, depth: 0 };
    }

    /// Creates a nested `DerReader` while enforcing `ASN1_MAX_DEPTH`.
    pub static fn initWithDepth(data: []const u8, depth: usize): DerReader ! Asn1Error {
        if (depth > ASN1_MAX_DEPTH) {
            return Asn1Error.MaxDepthExceeded;
        }
        return DerReader{ src: data, pos: 0, depth: depth };
    }

    /// Returns the number of unread bytes remaining.
    pub fn remaining(): usize {
        if (_.pos >= _.src.len) {
            return 0;
        }
        return _.src.len - _.pos;
    }

    /// Returns `true` if at least one unread byte remains.
    pub fn hasNext(): bool {
        return _.pos < _.src.len;
    }

    /// Peeks the next tag byte without advancing the cursor, or `null` at EOF.
    pub fn peekTag(): u8? {
        if (_.pos >= _.src.len) {
            return null;
        }
        return _.src[_.pos];
    }

    /// Returns a slice of all unread bytes remaining in this reader.
    pub fn sliceRemaining(): []const u8 {
        if (_.pos >= _.src.len) {
            return "";
        }
        return _.src[_.pos..];
    }

    /// Reads the next TLV element, strictly enforcing X.690 DER shortest-form definite lengths.
    /// Rejects indefinite length (`0x80`), reserved length (`0xFF`), non-canonical multi-byte lengths,
    /// and lengths exceeding the available buffer.
    pub fn readElement(*_): DerElement ! Asn1Error {
        if (_.pos >= _.src.len) {
            return Asn1Error.UnexpectedEof;
        }
        const start_pos = _.pos;
        const tag = _.src[_.pos];
        _.pos += 1;

        // High-tag-number form (bits 5..1 all 1) is not represented in single-octet `tag: u8`.
        if ((tag & 0x1F) == 0x1F) {
            return Asn1Error.TagMismatch;
        }

        if (_.pos >= _.src.len) {
            return Asn1Error.UnexpectedEof;
        }
        const len_b = _.src[_.pos];
        _.pos += 1;

        let content_len: usize = 0;
        if (len_b < 0x80) {
            content_len = len_b as usize;
        } else if (len_b == 0x80 || len_b == 0xFF) {
            // 0x80 = BER indefinite-length (forbidden in DER); 0xFF = reserved by X.690.
            return Asn1Error.InvalidLength;
        } else {
            const num_octets = (len_b & 0x7F) as usize;
            if (num_octets > @sizeOf(usize)) {
                return Asn1Error.InvalidLength;
            }
            if (_.remaining() < num_octets) {
                return Asn1Error.UnexpectedEof;
            }
            // Shortest-form DER forbids leading zero octets in multi-byte length.
            if (_.src[_.pos] == 0x00) {
                return Asn1Error.NonCanonicalLength;
            }
            let acc: usize = 0;
            let i: usize = 0;
            while (i < num_octets) : (i += 1) {
                acc = (acc << 8) | (_.src[_.pos] as usize);
                _.pos += 1;
            }
            // Lengths < 128 must use the single-byte short form.
            if (acc < 128) {
                return Asn1Error.NonCanonicalLength;
            }
            content_len = acc;
        }

        if (content_len > _.remaining()) {
            return Asn1Error.UnexpectedEof;
        }

        const content = _.src[_.pos .. _.pos + content_len];
        _.pos += content_len;
        const raw = _.src[start_pos .. _.pos];
        return DerElement{
            tag: tag,
            content: content,
            raw: raw,
        };
    }

    /// Reads the next element and verifies its tag equals `expected_tag`.
    pub fn expectTag(*_, expected_tag: u8): DerElement ! Asn1Error {
        const elem = _.readElement()?;
        if (elem.tag != expected_tag) {
            return Asn1Error.TagMismatch;
        }
        return elem;
    }

    /// Reads a `SEQUENCE` (`0x30`) and returns a nested `DerReader` with incremented depth.
    pub fn readSequence(*_): DerReader ! Asn1Error {
        if (_.depth + 1 > ASN1_MAX_DEPTH) {
            return Asn1Error.MaxDepthExceeded;
        }
        const elem = _.expectTag(TAG_SEQUENCE)?;
        return DerReader{ src: elem.content, pos: 0, depth: _.depth + 1 };
    }

    /// Reads a `SET` (`0x31`) and returns a nested `DerReader` with incremented depth.
    pub fn readSet(*_): DerReader ! Asn1Error {
        if (_.depth + 1 > ASN1_MAX_DEPTH) {
            return Asn1Error.MaxDepthExceeded;
        }
        const elem = _.expectTag(TAG_SET)?;
        return DerReader{ src: elem.content, pos: 0, depth: _.depth + 1 };
    }

    /// Reads an explicit constructed context-specific tag `[ctx_num]` (`0xA0 | ctx_num`).
    pub fn readContextExplicit(*_, ctx_num: u8): DerReader ! Asn1Error {
        if (_.depth + 1 > ASN1_MAX_DEPTH) {
            return Asn1Error.MaxDepthExceeded;
        }
        const expected: u8 = (0xA0 as u8) | (ctx_num & (0x1F as u8));
        const elem = _.expectTag(expected)?;
        return DerReader{ src: elem.content, pos: 0, depth: _.depth + 1 };
    }

    /// Returns `true` if the next TLV tag is `[ctx_num]` (`0xA0 | ctx_num`).
    pub fn hasContextExplicit(ctx_num: u8): bool {
        const expected: u8 = (0xA0 as u8) | (ctx_num & (0x1F as u8));
        if (next_tag := _.peekTag()) {
            return next_tag == expected;
        }
        return false;
    }

    /// Conditionally reads `[ctx_num]` (`0xA0 | ctx_num`) if present next, returning `null` otherwise.
    pub fn tryReadContextExplicit(*_, ctx_num: u8): DerReader?! {
        const next_tag_opt = _.peekTag();
        if (next_tag_opt == null) {
            return null;
        }
        const expected: u8 = (0xA0 as u8) | (ctx_num & (0x1F as u8));
        if (next_tag_opt.? != expected) {
            return null;
        }
        if (_.depth + 1 > ASN1_MAX_DEPTH) {
            return Asn1Error.MaxDepthExceeded;
        }
        const elem = _.expectTag(expected)?;
        return DerReader{ src: elem.content, pos: 0, depth: _.depth + 1 };
    }

    /// Reads and validates a `BOOLEAN` element.
    pub fn readBoolean(*_): bool ! Asn1Error {
        const elem = _.readElement()?;
        return elem.asBoolean();
    }

    /// Reads and validates a canonical `INTEGER` element, returning its raw content bytes.
    pub fn readIntegerBytes(*_): ([]const u8) ! Asn1Error {
        const elem = _.readElement()?;
        return elem.asIntegerBytes();
    }

    /// Reads a non-negative `INTEGER` element as `u64`.
    pub fn readUint64(*_): u64 ! Asn1Error {
        const elem = _.readElement()?;
        return elem.asUint64();
    }

    /// Reads an `OCTET STRING` element as a zero-copy byte slice.
    pub fn readOctetString(*_): ([]const u8) ! Asn1Error {
        const elem = _.readElement()?;
        return elem.asOctetString();
    }

    /// Reads a `BIT STRING` element, returning payload octets after validating unused bits.
    pub fn readBitString(*_): ([]const u8) ! Asn1Error {
        const elem = _.readElement()?;
        return elem.asBitString();
    }

    /// Reads and validates a `NULL` (`0x05 0x00`) element.
    pub fn readNull(*_): void ! Asn1Error {
        const elem = _.expectTag(TAG_NULL)?;
        if (elem.content.len != 0) {
            return Asn1Error.InvalidLength;
        }
    }

    /// Reads an `OBJECT IDENTIFIER` element, validates canonical VLQ encoding, and returns raw DER content octets.
    pub fn readOidBytes(*_): ([]const u8) ! Asn1Error {
        const elem = _.expectTag(TAG_OID)?;
        OidCodec.validate(elem.content)?;
        return elem.content;
    }

    /// Reads an `OBJECT IDENTIFIER` element and decodes it to an owned dotted string.
    pub fn readOid(*_, alloc: std.mem.Allocator): ([]u8) ! Asn1Error {
        const raw_oid = _.readOidBytes()?;
        return OidCodec.decode(alloc, raw_oid);
    }

    /// Reads `UTF8String`, `PrintableString`, `IA5String`, or `T61String` as a zero-copy slice.
    pub fn readString(*_): ([]const u8) ! Asn1Error {
        const elem = _.readElement()?;
        return elem.asString();
    }

    /// Reads any supported X.509 directory string (including `BMPString`) as an owned UTF-8 slice.
    pub fn readAnyStringAlloc(*_, alloc: std.mem.Allocator): ([]u8) ! Asn1Error {
        const elem = _.readElement()?;
        if (elem.tag == TAG_BMP_STRING) {
            if ((elem.content.len & 1) != 0) {
                return Asn1Error.MalformedCert;
            }
            let out: std.ArrayList(u8) = .empty;
            errdefer out.deinit(alloc);
            let i: usize = 0;
            while (i + 1 < elem.content.len) : (i += 2) {
                const cp: u21 = (((elem.content[i] as u21) << 8) | (elem.content[i + 1] as u21));
                let buf: [4]u8 = [0; 4];
                const n = std.unicode.utf8Encode(cp, &buf) catch return Asn1Error.MalformedCert;
                out.appendSlice(alloc, buf[0..n]) catch return Asn1Error.OutOfMemory;
            }
            return out.toOwnedSlice(alloc) catch return Asn1Error.OutOfMemory;
        }
        const s = elem.asString()?;
        return alloc.dupe(u8, s) catch return Asn1Error.OutOfMemory;
    }

    /// Skips the next TLV element after validating its framing.
    pub fn skip(*_): void ! Asn1Error {
        * = _.readElement()?;
    }
}

/// ITU-T X.690 ASN.1 DER TLV builder writing shortest-form canonical encodings.
pub struct DerWriter {
    buf: std.ArrayList(u8) = .empty,
    alloc: std.mem.Allocator = undefined,

    /// Initializes an empty `DerWriter` backed by `alloc`.
    pub static fn init(alloc: std.mem.Allocator): DerWriter {
        return DerWriter{
            buf: .empty,
            alloc: alloc,
        };
    }

    /// Releases the internal byte buffer.
    pub fn deinit(*_) {
        _.buf.deinit(_.alloc);
    }

    /// Clears the accumulated bytes while keeping capacity.
    pub fn reset(*_) {
        _.buf.clearRetainingCapacity();
    }

    /// Returns a read-only view of the currently encoded DER bytes.
    pub fn asSlice(): []const u8 {
        return _.buf.items;
    }

    /// Transfers ownership of the encoded DER byte slice to the caller.
    pub fn toOwnedSlice(*_): ([]u8) ! Asn1Error {
        return _.buf.toOwnedSlice(_.alloc) catch return Asn1Error.OutOfMemory;
    }

    /// Encodes `len` using canonical X.690 DER shortest definite length form into `out`.
    pub static fn writeLength(out: *std.ArrayList(u8), alloc: std.mem.Allocator, len: usize): void ! Asn1Error {
        if (len < 128) {
            out.append(alloc, len as u8) catch return Asn1Error.OutOfMemory;
        } else {
            let tmp: [8]u8 = [0; 8];
            let count: usize = 0;
            let v: usize = len;
            while (v > 0) {
                count += 1;
                tmp[8 - count] = (v & 0xFF) as u8;
                v >>= 8;
            }
            out.append(alloc, (0x80 | count) as u8) catch return Asn1Error.OutOfMemory;
            out.appendSlice(alloc, tmp[(8 - count)..8]) catch return Asn1Error.OutOfMemory;
        }
    }

    /// Appends a raw TLV `(tag, len(content), content)`.
    pub fn writeTlv(*_, tag: u8, content: []const u8): void ! Asn1Error {
        _.buf.append(_.alloc, tag) catch return Asn1Error.OutOfMemory;
        DerWriter.writeLength(&_.buf, _.alloc, content.len)?;
        _.buf.appendSlice(_.alloc, content) catch return Asn1Error.OutOfMemory;
    }

    /// Appends a canonical DER `BOOLEAN` (`0x00` or `0xFF`).
    pub fn writeBoolean(*_, value: bool): void ! Asn1Error {
        let b: [1]u8 = [0; 1];
        b[0] = if (value) 0xFF else 0x00;
        _.writeTlv(TAG_BOOLEAN, &b)?;
    }

    /// Appends an unsigned big-endian byte magnitude as a canonical non-negative DER `INTEGER`.
    /// Strips redundant leading zero bytes and inserts a single `0x00` sign-guard if bit 7 is set.
    pub fn writeIntegerBytes(*_, bytes: []const u8): void ! Asn1Error {
        let start: usize = 0;
        while (start < bytes.len && bytes[start] == 0x00) {
            start += 1;
        }
        if (start == bytes.len) {
            const zero: [1]u8 = [0; 1];
            _.writeTlv(TAG_INTEGER, &zero)?;
        } else {
            const trimmed = bytes[start..];
            const needs_sign_zero = (trimmed[0] & 0x80) != 0;
            const total_len = trimmed.len + (if (needs_sign_zero) (1 as usize) else (0 as usize));

            _.buf.append(_.alloc, TAG_INTEGER) catch return Asn1Error.OutOfMemory;
            DerWriter.writeLength(&_.buf, _.alloc, total_len)?;
            if (needs_sign_zero) {
                _.buf.append(_.alloc, 0x00) catch return Asn1Error.OutOfMemory;
            }
            _.buf.appendSlice(_.alloc, trimmed) catch return Asn1Error.OutOfMemory;
        }
    }

    /// Appends an unsigned 64-bit integer as a canonical DER `INTEGER`.
    pub fn writeUint64(*_, value: u64): void ! Asn1Error {
        if (value == 0) {
            const zero: [1]u8 = [0; 1];
            _.writeTlv(TAG_INTEGER, &zero)?;
        } else {
            let raw: [8]u8 = [0; 8];
            let v: u64 = value;
            let idx: usize = 8;
            while (v > 0) {
                idx -= 1;
                raw[idx] = (v & 0xFF) as u8;
                v >>= 8;
            }
            _.writeIntegerBytes(raw[idx..8])?;
        }
    }

    /// Appends an `OCTET STRING` (`0x04`).
    pub fn writeOctetString(*_, data: []const u8): void ! Asn1Error {
        _.writeTlv(TAG_OCTET_STRING, data)?;
    }

    /// Appends a `BIT STRING` (`0x03`) with explicit `unused_bits` (`0..=7`).
    pub fn writeBitString(*_, unused_bits: u8, data: []const u8): void ! Asn1Error {
        if (unused_bits > 7) {
            return Asn1Error.InvalidLength;
        }
        if (data.len == 0 && unused_bits != 0) {
            return Asn1Error.InvalidLength;
        }
        _.buf.append(_.alloc, TAG_BIT_STRING) catch return Asn1Error.OutOfMemory;
        DerWriter.writeLength(&_.buf, _.alloc, 1 + data.len)?;
        _.buf.append(_.alloc, unused_bits) catch return Asn1Error.OutOfMemory;
        _.buf.appendSlice(_.alloc, data) catch return Asn1Error.OutOfMemory;
    }

    /// Appends a `UTF8String` (`0x0C`).
    pub fn writeUtf8String(*_, s: []const u8): void ! Asn1Error {
        _.writeTlv(TAG_UTF8_STRING, s)?;
    }

    /// Appends a `PrintableString` (`0x13`).
    pub fn writePrintableString(*_, s: []const u8): void ! Asn1Error {
        _.writeTlv(TAG_PRINTABLE_STRING, s)?;
    }

    /// Appends an `IA5String` (`0x16`).
    pub fn writeIa5String(*_, s: []const u8): void ! Asn1Error {
        _.writeTlv(TAG_IA5_STRING, s)?;
    }

    /// Appends a `UTCTime` (`0x17`).
    pub fn writeUtcTime(*_, s: []const u8): void ! Asn1Error {
        _.writeTlv(TAG_UTC_TIME, s)?;
    }

    /// Appends a `GeneralizedTime` (`0x18`).
    pub fn writeGeneralizedTime(*_, s: []const u8): void ! Asn1Error {
        _.writeTlv(TAG_GENERALIZED_TIME, s)?;
    }

    /// Appends a canonical DER `NULL` (`0x05 0x00`).
    pub fn writeNull(*_): void ! Asn1Error {
        _.writeTlv(TAG_NULL, "")?;
    }

    /// Encodes a dotted OID string (e.g. `"1.2.840.113549.1.1.11"`) and appends it as `TAG_OID`.
    pub fn writeOid(*_, dotted: []const u8): void ! Asn1Error {
        const encoded = OidCodec.encode(_.alloc, dotted)?;
        defer _.alloc.free(encoded);
        _.writeTlv(TAG_OID, encoded)?;
    }

    /// Appends a `SEQUENCE` (`0x30`) wrapping `content`.
    pub fn writeSequence(*_, content: []const u8): void ! Asn1Error {
        _.writeTlv(TAG_SEQUENCE, content)?;
    }

    /// Appends a `SET` (`0x31`) wrapping `content`.
    pub fn writeSet(*_, content: []const u8): void ! Asn1Error {
        _.writeTlv(TAG_SET, content)?;
    }

    /// Appends an explicit constructed context-specific tag `[ctx_num]` (`0xA0 | ctx_num`).
    pub fn writeContextExplicit(*_, ctx_num: u8, content: []const u8): void ! Asn1Error {
        const tag: u8 = (0xA0 as u8) | (ctx_num & (0x1F as u8));
        _.writeTlv(tag, content)?;
    }

    /// Appends an implicit context-specific tag (`[ctx_num]`) with primitive (`0x80`) or constructed (`0xA0`) class bit.
    pub fn writeContextImplicit(*_, ctx_num: u8, constructed: bool, content: []const u8): void ! Asn1Error {
        const base: u8 = if (constructed) 0xA0 else 0x80;
        const tag: u8 = base | (ctx_num & (0x1F as u8));
        _.writeTlv(tag, content)?;
    }
}

/// Parses a single decimal OID arc component with strict validation (no leading zero unless `"0"`).
fn parseOidArc(seg: []const u8): u64 ! Asn1Error {
    if (seg.len == 0) {
        return Asn1Error.InvalidOid;
    }
    if (seg.len > 1 && seg[0] == '0') {
        return Asn1Error.InvalidOid;
    }
    const max_u64 = std.math.maxInt(u64);
    let acc: u64 = 0;
    for (ch of seg) {
        if (ch < '0' || ch > '9') {
            return Asn1Error.InvalidOid;
        }
        const digit = (ch - '0') as u64;
        if (acc > (max_u64 - digit) / 10) {
            return Asn1Error.InvalidOid;
        }
        acc = acc * 10 + digit;
    }
    return acc;
}

/// Appends one OID subidentifier in canonical big-endian base-128 VLQ encoding.
fn writeOidSubid(out: *std.ArrayList(u8), alloc: std.mem.Allocator, value: u64): void ! Asn1Error {
    if (value < 128) {
        out.append(alloc, value as u8) catch return Asn1Error.OutOfMemory;
    } else {
        let tmp: [10]u8 = [0; 10];
        let count: usize = 0;
        let v: u64 = value;
        while (v > 0) {
            count += 1;
            tmp[10 - count] = (v & 0x7F) as u8;
            v >>= 7;
        }
        let i: usize = (10 as usize) - count;
        while (i < 9) : (i += 1) {
            out.append(alloc, tmp[i] | (0x80 as u8)) catch return Asn1Error.OutOfMemory;
        }
        out.append(alloc, tmp[9]) catch return Asn1Error.OutOfMemory;
    }
}

/// Appends an unsigned 64-bit integer formatted in ASCII decimal into `out`.
fn writeAsciiU64(out: *std.ArrayList(u8), alloc: std.mem.Allocator, value: u64): void ! Asn1Error {
    if (value == 0) {
        out.append(alloc, '0') catch return Asn1Error.OutOfMemory;
    } else {
        let tmp: [20]u8 = [0; 20];
        let count: usize = 0;
        let v: u64 = value;
        while (v > 0) {
            count += 1;
            tmp[20 - count] = ('0' as u64 + (v % 10)) as u8;
            v /= 10;
        }
        out.appendSlice(alloc, tmp[(20 - count)..20]) catch return Asn1Error.OutOfMemory;
    }
}

/// ITU-T X.690 ASN.1 Object Identifier (OID) encoder, decoder, and well-known registry.
pub static class OidCodec {
    /// Encodes a dotted OID string (e.g. `"1.2.840.113549.1.1.11"`) into canonical DER OID value bytes.
    /// Caller owns the returned slice and must free it with `alloc.free(...)`.
    pub fn encode(alloc: std.mem.Allocator, dotted: []const u8): ([]u8) ! Asn1Error {
        if (dotted.len == 0) {
            return Asn1Error.InvalidOid;
        }
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        let arc_idx: usize = 0;
        let first_arc: u64 = 0;
        let pos: usize = 0;
        const max_u64 = std.math.maxInt(u64);

        while (true) {
            let end = pos;
            while (end < dotted.len && dotted[end] != '.') {
                end += 1;
            }
            const seg = dotted[pos..end];
            const val = parseOidArc(seg)?;

            if (arc_idx == 0) {
                if (val > 2) {
                    return Asn1Error.InvalidOid;
                }
                first_arc = val;
            } else if (arc_idx == 1) {
                if (first_arc < 2 && val > 39) {
                    return Asn1Error.InvalidOid;
                }
                if (val > max_u64 - (first_arc * 40)) {
                    return Asn1Error.InvalidOid;
                }
                writeOidSubid(&out, alloc, first_arc * 40 + val)?;
            } else {
                writeOidSubid(&out, alloc, val)?;
            }

            arc_idx += 1;
            if (end >= dotted.len) {
                break;
            }
            pos = end + 1;
            if (pos == dotted.len) {
                // Trailing '.' is invalid.
                return Asn1Error.InvalidOid;
            }
        }

        if (arc_idx < 2) {
            return Asn1Error.InvalidOid;
        }

        return out.toOwnedSlice(alloc) catch return Asn1Error.OutOfMemory;
    }

    /// Validates raw DER OID content octets (non-empty, no leading 0x80, no u64 overflow, no truncated VLQ).
    pub fn validate(bytes: []const u8): void ! Asn1Error {
        if (bytes.len == 0) {
            return Asn1Error.InvalidOid;
        }
        const max_subid = std.math.maxInt(u64) >> 7;
        let pos: usize = 0;
        while (pos < bytes.len) {
            if (bytes[pos] == 0x80) {
                return Asn1Error.InvalidOid;
            }
            let subid: u64 = 0;
            let finished = false;
            while (pos < bytes.len) {
                const b = bytes[pos];
                pos += 1;
                if (subid > max_subid) {
                    return Asn1Error.InvalidOid;
                }
                subid = (subid << 7) | ((b & 0x7F) as u64);
                if ((b & 0x80) == 0) {
                    finished = true;
                    break;
                }
            }
            if (!finished) {
                return Asn1Error.InvalidOid;
            }
        }
    }

    /// Decodes DER OID content octets into an owned dotted decimal string.
    /// Rejects empty inputs, truncated VLQ sequences, and non-canonical leading `0x80` bytes.
    pub fn decode(alloc: std.mem.Allocator, bytes: []const u8): ([]u8) ! Asn1Error {
        if (bytes.len == 0) {
            return Asn1Error.InvalidOid;
        }
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        const max_subid = std.math.maxInt(u64) >> 7;
        let pos: usize = 0;
        let is_first = true;

        while (pos < bytes.len) {
            if (bytes[pos] == 0x80) {
                // Non-canonical base-128 VLQ leading zero octet.
                return Asn1Error.InvalidOid;
            }
            let subid: u64 = 0;
            let finished = false;
            while (pos < bytes.len) {
                const b = bytes[pos];
                pos += 1;
                if (subid > max_subid) {
                    return Asn1Error.InvalidOid;
                }
                subid = (subid << 7) | ((b & 0x7F) as u64);
                if ((b & 0x80) == 0) {
                    finished = true;
                    break;
                }
            }
            if (!finished) {
                return Asn1Error.InvalidOid;
            }

            if (is_first) {
                is_first = false;
                let a: u64 = 0;
                let b: u64 = 0;
                if (subid < 40) {
                    a = 0;
                    b = subid;
                } else if (subid < 80) {
                    a = 1;
                    b = subid - 40;
                } else {
                    a = 2;
                    b = subid - 80;
                }
                writeAsciiU64(&out, alloc, a)?;
                out.append(alloc, '.') catch return Asn1Error.OutOfMemory;
                writeAsciiU64(&out, alloc, b)?;
            } else {
                out.append(alloc, '.') catch return Asn1Error.OutOfMemory;
                writeAsciiU64(&out, alloc, subid)?;
            }
        }

        return out.toOwnedSlice(alloc) catch return Asn1Error.OutOfMemory;
    }

    /// Maps well-known RSA/ECDSA/Ed25519/X.500/X.509 dotted OIDs to their canonical short names.
    pub fn knownName(dotted: []const u8): ([]const u8)? {
        // Signature & Public Key Algorithms
        if (std.mem.eql(u8, dotted, "1.2.840.113549.1.1.1")) return "rsaEncryption";
        if (std.mem.eql(u8, dotted, "1.2.840.113549.1.1.5")) return "sha1WithRSAEncryption";
        if (std.mem.eql(u8, dotted, "1.2.840.113549.1.1.10")) return "rsassaPss";
        if (std.mem.eql(u8, dotted, "1.2.840.113549.1.1.11")) return "sha256WithRSAEncryption";
        if (std.mem.eql(u8, dotted, "1.2.840.113549.1.1.12")) return "sha384WithRSAEncryption";
        if (std.mem.eql(u8, dotted, "1.2.840.113549.1.1.13")) return "sha512WithRSAEncryption";
        if (std.mem.eql(u8, dotted, "1.2.840.10045.2.1")) return "ecPublicKey";
        if (std.mem.eql(u8, dotted, "1.2.840.10045.4.3.2")) return "ecdsa-with-SHA256";
        if (std.mem.eql(u8, dotted, "1.2.840.10045.4.3.3")) return "ecdsa-with-SHA384";
        if (std.mem.eql(u8, dotted, "1.2.840.10045.4.3.4")) return "ecdsa-with-SHA512";
        if (std.mem.eql(u8, dotted, "1.2.840.10045.3.1.7")) return "prime256v1";
        if (std.mem.eql(u8, dotted, "1.3.132.0.34")) return "secp384r1";
        if (std.mem.eql(u8, dotted, "1.3.101.110")) return "X25519";
        if (std.mem.eql(u8, dotted, "1.3.101.112")) return "Ed25519";
        if (std.mem.eql(u8, dotted, "1.3.101.113")) return "Ed448";
        if (std.mem.eql(u8, dotted, "2.16.840.1.101.3.4.2.1")) return "sha256";
        if (std.mem.eql(u8, dotted, "2.16.840.1.101.3.4.2.2")) return "sha384";
        if (std.mem.eql(u8, dotted, "2.16.840.1.101.3.4.2.3")) return "sha512";

        // X.500 Distinguished Name Attributes
        if (std.mem.eql(u8, dotted, "2.5.4.3")) return "CN";
        if (std.mem.eql(u8, dotted, "2.5.4.5")) return "serialNumber";
        if (std.mem.eql(u8, dotted, "2.5.4.6")) return "C";
        if (std.mem.eql(u8, dotted, "2.5.4.7")) return "L";
        if (std.mem.eql(u8, dotted, "2.5.4.8")) return "ST";
        if (std.mem.eql(u8, dotted, "2.5.4.10")) return "O";
        if (std.mem.eql(u8, dotted, "2.5.4.11")) return "OU";

        // X.509 v3 Extensions & Extended Key Usage
        if (std.mem.eql(u8, dotted, "2.5.29.14")) return "subjectKeyIdentifier";
        if (std.mem.eql(u8, dotted, "2.5.29.15")) return "keyUsage";
        if (std.mem.eql(u8, dotted, "2.5.29.17")) return "subjectAltName";
        if (std.mem.eql(u8, dotted, "2.5.29.19")) return "basicConstraints";
        if (std.mem.eql(u8, dotted, "2.5.29.31")) return "cRLDistributionPoints";
        if (std.mem.eql(u8, dotted, "2.5.29.32")) return "certificatePolicies";
        if (std.mem.eql(u8, dotted, "2.5.29.35")) return "authorityKeyIdentifier";
        if (std.mem.eql(u8, dotted, "2.5.29.37")) return "extKeyUsage";
        if (std.mem.eql(u8, dotted, "1.3.6.1.5.5.7.3.1")) return "serverAuth";
        if (std.mem.eql(u8, dotted, "1.3.6.1.5.5.7.3.2")) return "clientAuth";
        if (std.mem.eql(u8, dotted, "1.3.6.1.5.5.7.3.3")) return "codeSigning";
        if (std.mem.eql(u8, dotted, "1.3.6.1.5.5.7.3.4")) return "emailProtection";

        return null;
    }
}
