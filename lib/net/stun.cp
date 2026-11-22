// Source: cp/lib/stun/stun.cp (RFC 5389 & RFC 8489 STUN Wire Message, XOR-MAPPED-ADDRESS & Integrity Codec)

const std = @import("std");
import * as deflate from "cap:deflate";

/// RFC 5389 Section 6 fixed Magic Cookie (0x2112A442).
pub const MAGIC_COOKIE: u32 = 0x2112A442;

/// RFC 5389 Section 15.5 FINGERPRINT XOR mask ("STUN").
pub const FINGERPRINT_XOR: u32 = 0x5354554e;

/// Fixed STUN message header length in bytes.
pub const HEADER_SIZE: usize = 20;

/// Errors returned by `cap:stun` wire protocol encoding, decoding, and verification.
pub err StunError {
    InvalidHeader,
    InvalidMagicCookie,
    InvalidMessageLength,
    TruncatedAttribute,
    InvalidAttributeValue,
    IntegrityMismatch,
    FingerprintMismatch,
    OutOfMemory,
}

/// RFC 5389 Section 6 STUN message classes (2-bit C1/C0).
pub enum StunClass: u8 {
    Request = 0,
    Indication = 1,
    SuccessResponse = 2,
    ErrorResponse = 3,

    pub static fn fromBits(bits: u8): StunClass ! StunError {
        switch (bits) {
            0 => return StunClass.Request,
            1 => return StunClass.Indication,
            2 => return StunClass.SuccessResponse,
            3 => return StunClass.ErrorResponse,
            else => return StunError.InvalidHeader,
        }
    }
}

/// Standard STUN / TURN 12-bit message methods (RFC 5389 / RFC 5766 / RFC 8489).
pub enum StunMethod: u16 {
    Binding = 0x0001,
    Allocate = 0x0003,
    Refresh = 0x0004,
    Send = 0x0006,
    Data = 0x0007,
    CreatePermission = 0x0008,
    ChannelBind = 0x0009,
}

/// Standard STUN attribute types (RFC 5389 / RFC 8489).
pub enum AttrType: u16 {
    MappedAddress = 0x0001,
    Username = 0x0006,
    MessageIntegrity = 0x0008,
    ErrorCode = 0x0009,
    UnknownAttributes = 0x000A,
    Realm = 0x0014,
    Nonce = 0x0015,
    XorMappedAddress = 0x0020,
    Software = 0x8022,
    AlternateServer = 0x8023,
    Fingerprint = 0x8028,
}

/// Tagged union representing an IPv4 or IPv6 network address in STUN attributes.
pub enum StunAddr {
    ipv4: [4]u8,
    ipv6: [16]u8,
}

/// Decoded `XOR-MAPPED-ADDRESS` attribute (RFC 5389 Section 15.2).
pub class XorMappedAddress {
    pub addr: StunAddr,
    pub port: u16,
}

/// Decoded `ERROR-CODE` attribute (RFC 5389 Section 15.6).
pub class StunErrorCode {
    pub code: u16,
    pub reason: []const u8,
}

/// A single STUN TLV attribute (zero-copy slice into the parsed wire buffer).
pub class StunAttribute {
    pub attr_type: u16,
    pub value: []const u8,
}

/// Decoded STUN wire message owning its attribute metadata slice.
pub class StunMessage {
    pub msg_class: StunClass,
    pub method: u16,
    pub transaction_id: [12]u8,
    pub attributes: []StunAttribute,

    /// Releases the owned `attributes` slice.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.attributes.len > 0) {
            allocator.free(_.attributes);
        }
        _.attributes = _.attributes[0..0];
    }

    /// Finds the first attribute matching `attr_type`, or returns `null`.
    pub fn findAttr(attr_type: u16): (*const StunAttribute)? {
        let i: usize = 0;
        while (i < _.attributes.len) : (i += 1) {
            if (_.attributes[i].attr_type == attr_type) {
                return &_.attributes[i];
            }
        }
        return null;
    }
}

fn readU16BE(buf: []const u8, offset: usize): u16 {
    return ((buf[offset] as u16) << 8) | (buf[offset + 1] as u16);
}

fn readU32BE(buf: []const u8, offset: usize): u32 {
    return ((buf[offset] as u32) << 24)
        | ((buf[offset + 1] as u32) << 16)
        | ((buf[offset + 2] as u32) << 8)
        | (buf[offset + 3] as u32);
}

fn writeU16BE(buf: []u8, offset: usize, val: u16) {
    buf[offset] = ((val >> 8) & 0xFF) as u8;
    buf[offset + 1] = (val & 0xFF) as u8;
}

fn writeU32BE(buf: []u8, offset: usize, val: u32) {
    buf[offset] = ((val >> 24) & 0xFF) as u8;
    buf[offset + 1] = ((val >> 16) & 0xFF) as u8;
    buf[offset + 2] = ((val >> 8) & 0xFF) as u8;
    buf[offset + 3] = (val & 0xFF) as u8;
}

fn encodeMessageType(msg_class: StunClass, method: u16): u16 ! StunError {
    if (method > 0x0FFF) {
        return StunError.InvalidHeader;
    }
    const c_val = (msg_class as u8) as u16;
    const c0 = (c_val & 0x01) << 4;
    const c1 = (c_val & 0x02) << 7;
    const m_low = method & 0x000F;
    const m_mid = (method & 0x0070) << 1;
    const m_high = (method & 0x0F80) << 2;
    return m_low | m_mid | m_high | c0 | c1;
}

fn paddedLength(len: usize): usize {
    return (len + 3) & ~@as(usize, 3);
}

fn validateHeader(bytes: []const u8): u16 ! StunError {
    if (bytes.len < HEADER_SIZE) {
        return StunError.InvalidHeader;
    }
    const msg_type = readU16BE(bytes, 0);
    if ((msg_type & 0xC000) != 0) {
        return StunError.InvalidHeader;
    }
    const msg_len = readU16BE(bytes, 2);
    if ((msg_len % 4) != 0) {
        return StunError.InvalidMessageLength;
    }
    const cookie = readU32BE(bytes, 4);
    if (cookie != MAGIC_COOKIE) {
        return StunError.InvalidMagicCookie;
    }
    const expected_total = HEADER_SIZE + (msg_len as usize);
    if (bytes.len != expected_total) {
        return StunError.InvalidMessageLength;
    }
    return msg_type;
}

fn constantTimeEqual20(a: []const u8, b: []const u8): bool {
    if (a.len != 20 || b.len != 20) {
        return false;
    }
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < 20) : (i += 1) {
        diff |= a[i] ^ b[i];
    }
    return diff == 0;
}

/// Builder for constructing RFC 5389 / RFC 8489 STUN wire messages.
pub class StunBuilder {
    allocator: std.mem.Allocator,
    msg_class: StunClass,
    method: u16,
    transaction_id: [12]u8,
    attrs_buf: std.ArrayList(u8),

    /// Initializes a STUN message builder with the given class, 12-bit method, and 96-bit transaction ID.
    pub fn init(
        allocator: std.mem.Allocator,
        msg_class: StunClass,
        method: u16,
        transaction_id: [12]u8,
    ): StunBuilder {
        return StunBuilder{
            allocator: allocator,
            msg_class: msg_class,
            method: method,
            transaction_id: transaction_id,
            attrs_buf: .empty,
        };
    }

    /// Releases internal attribute buffer memory.
    pub fn deinit(*_) {
        _.attrs_buf.deinit(_.allocator);
    }

    /// Appends an arbitrary STUN attribute with automatic 4-byte zero-padding.
    pub fn addAttribute(*_, attr_type: u16, value: []const u8): void ! StunError {
        if (value.len > 0xFFFF) {
            return StunError.InvalidAttributeValue;
        }
        const pad_len = (4 - (value.len % 4)) % 4;
        if (_.attrs_buf.items.len + 4 + value.len + pad_len > 0xFFFF) {
            return StunError.InvalidMessageLength;
        }

        let hdr: [4]u8 = [0; 4];
        writeU16BE(&hdr, 0, attr_type);
        writeU16BE(&hdr, 2, value.len as u16);
        _.attrs_buf.appendSlice(_.allocator, &hdr) catch return StunError.OutOfMemory;
        if (value.len > 0) {
            _.attrs_buf.appendSlice(_.allocator, value) catch return StunError.OutOfMemory;
        }
        let p: usize = 0;
        while (p < pad_len) : (p += 1) {
            _.attrs_buf.append(_.allocator, 0) catch return StunError.OutOfMemory;
        }
    }

    /// Encodes and appends an `XOR-MAPPED-ADDRESS` (0x0020) attribute per RFC 5389 Section 15.2.
    pub fn addXorMappedAddress(*_, addr: StunAddr, port: u16): void ! StunError {
        const cookie_hi: u16 = ((MAGIC_COOKIE >> 16) & 0xFFFF) as u16;
        const xport: u16 = port ^ cookie_hi;

        let cookie_bytes: [4]u8 = [0; 4];
        writeU32BE(&cookie_bytes, 0, MAGIC_COOKIE);

        switch (addr) {
            StunAddr.ipv4 => |ip4| {
                let buf: [8]u8 = [0; 8];
                buf[0] = 0x00;
                buf[1] = 0x01;
                writeU16BE(&buf, 2, xport);
                let i: usize = 0;
                while (i < 4) : (i += 1) {
                    buf[4 + i] = ip4[i] ^ cookie_bytes[i];
                }
                _.addAttribute(AttrType.XorMappedAddress as u16, &buf)?;
            },
            StunAddr.ipv6 => |ip6| {
                let buf: [20]u8 = [0; 20];
                buf[0] = 0x00;
                buf[1] = 0x02;
                writeU16BE(&buf, 2, xport);
                let i: usize = 0;
                while (i < 4) : (i += 1) {
                    buf[4 + i] = ip6[i] ^ cookie_bytes[i];
                }
                while (i < 16) : (i += 1) {
                    buf[4 + i] = ip6[i] ^ _.transaction_id[i - 4];
                }
                _.addAttribute(AttrType.XorMappedAddress as u16, &buf)?;
            },
        }
    }

    /// Encodes and appends an `ERROR-CODE` (0x0009) attribute (code 300..699) per RFC 5389 Section 15.6.
    pub fn addErrorCode(*_, code: u16, reason: []const u8): void ! StunError {
        if (code < 300 || code > 699) {
            return StunError.InvalidAttributeValue;
        }
        const err_class = code / 100;
        const err_num = code % 100;
        if (4 + reason.len > 0xFFFF) {
            return StunError.InvalidAttributeValue;
        }

        let buf = _.allocator.alloc(u8, 4 + reason.len) catch return StunError.OutOfMemory;
        defer _.allocator.free(buf);

        buf[0] = 0x00;
        buf[1] = 0x00;
        buf[2] = (err_class & 0x07) as u8;
        buf[3] = err_num as u8;
        if (reason.len > 0) {
            @memcpy(buf[4..(4 + reason.len)], reason);
        }
        _.addAttribute(AttrType.ErrorCode as u16, buf)?;
    }

    /// Serializes the STUN message without integrity or fingerprint attributes.
    pub fn finish(*_, allocator: std.mem.Allocator): []u8 ! StunError {
        const attrs_len = _.attrs_buf.items.len;
        if (attrs_len > 0xFFFF || (attrs_len % 4) != 0) {
            return StunError.InvalidMessageLength;
        }
        const msg_type = encodeMessageType(_.msg_class, _.method)?;
        const out = allocator.alloc(u8, HEADER_SIZE + attrs_len) catch return StunError.OutOfMemory;
        writeU16BE(out, 0, msg_type);
        writeU16BE(out, 2, attrs_len as u16);
        writeU32BE(out, 4, MAGIC_COOKIE);
        @memcpy(out[8..20], &_.transaction_id);
        if (attrs_len > 0) {
            @memcpy(out[20..(20 + attrs_len)], _.attrs_buf.items);
        }
        return out;
    }

    /// Serializes the STUN message, appending `MESSAGE-INTEGRITY` (HMAC-SHA1) and optionally `FINGERPRINT` (CRC-32 ^ 0x5354554e).
    pub fn finishWithIntegrity(
        *_,
        allocator: std.mem.Allocator,
        key: []const u8,
        add_fingerprint: bool,
    ): []u8 ! StunError {
        const base_len = _.attrs_buf.items.len;
        const mi_attr_len: usize = 24;
        const fp_attr_len: usize = if (add_fingerprint) 8 else 0;
        const total_attrs_len = base_len + mi_attr_len + fp_attr_len;

        if (total_attrs_len > 0xFFFF || (base_len % 4) != 0) {
            return StunError.InvalidMessageLength;
        }

        const msg_type = encodeMessageType(_.msg_class, _.method)?;
        const out = allocator.alloc(u8, HEADER_SIZE + total_attrs_len) catch return StunError.OutOfMemory;
        errdefer allocator.free(out);

        // Step 1: Write header with length covering up to end of MESSAGE-INTEGRITY (excluding FINGERPRINT).
        const mi_msg_len = (base_len + mi_attr_len) as u16;
        writeU16BE(out, 0, msg_type);
        writeU16BE(out, 2, mi_msg_len);
        writeU32BE(out, 4, MAGIC_COOKIE);
        @memcpy(out[8..20], &_.transaction_id);
        if (base_len > 0) {
            @memcpy(out[20..(20 + base_len)], _.attrs_buf.items);
        }

        // Step 2: Compute HMAC-SHA1 over [0 .. HEADER_SIZE + base_len] and append MESSAGE-INTEGRITY.
        const mi_offset = HEADER_SIZE + base_len;
        let mac: [20]u8 = [0; 20];
        std.crypto.auth.hmac.HmacSha1.create(&mac, out[0..mi_offset], key);
        writeU16BE(out, mi_offset, AttrType.MessageIntegrity as u16);
        writeU16BE(out, mi_offset + 2, 20);
        @memcpy(out[(mi_offset + 4)..(mi_offset + 24)], &mac);

        // Step 3: Optionally update header length and append FINGERPRINT (CRC-32 ^ 0x5354554e).
        if (add_fingerprint) {
            const fp_offset = mi_offset + mi_attr_len;
            writeU16BE(out, 2, total_attrs_len as u16);
            const crc = deflate.crc32(out[0..fp_offset]) ^ FINGERPRINT_XOR;
            writeU16BE(out, fp_offset, AttrType.Fingerprint as u16);
            writeU16BE(out, fp_offset + 2, 4);
            writeU32BE(out, fp_offset + 4, crc);
        }

        return out;
    }
}

/// Parses and strictly validates an RFC 5389 / RFC 8489 STUN wire message.
pub fn parseMessage(allocator: std.mem.Allocator, bytes: []const u8): StunMessage ! StunError {
    const msg_type = validateHeader(bytes)?;

    const c_bits = (((msg_type & 0x0010) >> 4) | ((msg_type & 0x0100) >> 7)) as u8;
    const msg_class = StunClass.fromBits(c_bits)?;
    const method = (msg_type & 0x000F) | ((msg_type & 0x00E0) >> 1) | ((msg_type & 0x3E00) >> 2);

    let tid: [12]u8 = [0; 12];
    @memcpy(&tid, bytes[8..20]);

    const body_end = bytes.len;
    let count: usize = 0;
    let offset: usize = HEADER_SIZE;
    while (offset < body_end) {
        if (offset + 4 > body_end) {
            return StunError.TruncatedAttribute;
        }
        const attr_len = readU16BE(bytes, offset + 2) as usize;
        const pad_len = paddedLength(attr_len);
        if (offset + 4 + attr_len > body_end || offset + 4 + pad_len > body_end) {
            return StunError.TruncatedAttribute;
        }
        count += 1;
        offset += 4 + pad_len;
    }

    let attrs: []StunAttribute = undefined;
    if (count == 0) {
        attrs = allocator.alloc(StunAttribute, 0) catch return StunError.OutOfMemory;
    } else {
        attrs = allocator.alloc(StunAttribute, count) catch return StunError.OutOfMemory;
        let idx: usize = 0;
        offset = HEADER_SIZE;
        while (offset < body_end) {
            const attr_type = readU16BE(bytes, offset);
            const attr_len = readU16BE(bytes, offset + 2) as usize;
            const pad_len = paddedLength(attr_len);
            const val_start = offset + 4;
            attrs[idx] = StunAttribute{
                attr_type: attr_type,
                value: bytes[val_start..(val_start + attr_len)],
            };
            idx += 1;
            offset += 4 + pad_len;
        }
    }

    return StunMessage{
        msg_class: msg_class,
        method: method,
        transaction_id: tid,
        attributes: attrs,
    };
}

/// Verifies the `MESSAGE-INTEGRITY` (HMAC-SHA1) attribute against `key`.
pub fn verifyMessageIntegrity(bytes: []const u8, key: []const u8): bool ! StunError {
    * = validateHeader(bytes)?;

    const body_end = bytes.len;
    let offset: usize = HEADER_SIZE;
    let mi_offset: usize = 0;
    let found = false;

    while (offset < body_end) {
        if (offset + 4 > body_end) {
            return StunError.TruncatedAttribute;
        }
        const attr_type = readU16BE(bytes, offset);
        const attr_len = readU16BE(bytes, offset + 2) as usize;
        const pad_len = paddedLength(attr_len);
        if (offset + 4 + attr_len > body_end || offset + 4 + pad_len > body_end) {
            return StunError.TruncatedAttribute;
        }
        if (attr_type == (AttrType.MessageIntegrity as u16)) {
            if (attr_len != 20) {
                return StunError.InvalidAttributeValue;
            }
            mi_offset = offset;
            found = true;
            break;
        }
        offset += 4 + pad_len;
    }

    if (!found) {
        return StunError.IntegrityMismatch;
    }

    // RFC 5389 Section 15.4: length in header is adjusted to end of MESSAGE-INTEGRITY.
    const adjusted_len = (mi_offset + 24 - HEADER_SIZE) as u16;
    let len_bytes: [2]u8 = [0; 2];
    writeU16BE(&len_bytes, 0, adjusted_len);

    let ctx = std.crypto.auth.hmac.HmacSha1.init(key);
    ctx.update(bytes[0..2]);
    ctx.update(&len_bytes);
    ctx.update(bytes[4..mi_offset]);
    let expected: [20]u8 = [0; 20];
    ctx.final(&expected);

    const actual = bytes[(mi_offset + 4)..(mi_offset + 24)];
    if (!constantTimeEqual20(&expected, actual)) {
        return StunError.IntegrityMismatch;
    }
    return true;
}

/// Verifies the trailing `FINGERPRINT` (CRC-32 ^ 0x5354554e) attribute per RFC 5389 Section 15.5.
pub fn verifyFingerprint(bytes: []const u8): bool ! StunError {
    * = validateHeader(bytes)?;

    const body_end = bytes.len;
    if (body_end < HEADER_SIZE + 8) {
        return StunError.FingerprintMismatch;
    }

    let offset: usize = HEADER_SIZE;
    let last_offset: usize = 0;
    let last_type: u16 = 0;
    let last_len: usize = 0;
    let has_attr = false;

    while (offset < body_end) {
        if (offset + 4 > body_end) {
            return StunError.TruncatedAttribute;
        }
        const attr_type = readU16BE(bytes, offset);
        const attr_len = readU16BE(bytes, offset + 2) as usize;
        const pad_len = paddedLength(attr_len);
        if (offset + 4 + attr_len > body_end || offset + 4 + pad_len > body_end) {
            return StunError.TruncatedAttribute;
        }
        last_offset = offset;
        last_type = attr_type;
        last_len = attr_len;
        has_attr = true;
        offset += 4 + pad_len;
    }

    if (!has_attr || last_type != (AttrType.Fingerprint as u16) || last_len != 4) {
        return StunError.FingerprintMismatch;
    }

    const expected = deflate.crc32(bytes[0..last_offset]) ^ FINGERPRINT_XOR;
    const actual = readU32BE(bytes, last_offset + 4);
    if (expected != actual) {
        return StunError.FingerprintMismatch;
    }
    return true;
}

/// Decodes an `XOR-MAPPED-ADDRESS` attribute value using `MAGIC_COOKIE` and `transaction_id`.
pub fn decodeXorMappedAddress(attr_val: []const u8, transaction_id: [12]u8): XorMappedAddress ! StunError {
    if (attr_val.len < 4) {
        return StunError.InvalidAttributeValue;
    }
    const family = attr_val[1];
    const cookie_hi: u16 = ((MAGIC_COOKIE >> 16) & 0xFFFF) as u16;
    const port: u16 = readU16BE(attr_val, 2) ^ cookie_hi;

    let cookie_bytes: [4]u8 = [0; 4];
    writeU32BE(&cookie_bytes, 0, MAGIC_COOKIE);

    if (family == 0x01) {
        if (attr_val.len != 8) {
            return StunError.InvalidAttributeValue;
        }
        let ip4: [4]u8 = [0; 4];
        let i: usize = 0;
        while (i < 4) : (i += 1) {
            ip4[i] = attr_val[4 + i] ^ cookie_bytes[i];
        }
        return XorMappedAddress{
            addr: StunAddr{ ipv4: ip4 },
            port: port,
        };
    } else if (family == 0x02) {
        if (attr_val.len != 20) {
            return StunError.InvalidAttributeValue;
        }
        let ip6: [16]u8 = [0; 16];
        let i: usize = 0;
        while (i < 4) : (i += 1) {
            ip6[i] = attr_val[4 + i] ^ cookie_bytes[i];
        }
        while (i < 16) : (i += 1) {
            ip6[i] = attr_val[4 + i] ^ transaction_id[i - 4];
        }
        return XorMappedAddress{
            addr: StunAddr{ ipv6: ip6 },
            port: port,
        };
    } else {
        return StunError.InvalidAttributeValue;
    }
}

/// Decodes an `ERROR-CODE` attribute value into a status code (300..699) and UTF-8 reason slice.
pub fn decodeErrorCode(attr_val: []const u8): StunErrorCode ! StunError {
    if (attr_val.len < 4) {
        return StunError.InvalidAttributeValue;
    }
    const err_class = (attr_val[2] & 0x07) as u16;
    const err_num = attr_val[3] as u16;
    if (err_class < 3 || err_class > 6 || err_num >= 100) {
        return StunError.InvalidAttributeValue;
    }
    return StunErrorCode{
        code: err_class * 100 + err_num,
        reason: attr_val[4..],
    };
}
