// packages/@cap/der-asn1/src/index.cp
// Zero-allocation ITU-T X.690 ASN.1 DER encoder and strict decoder in pure CP.
// Reuses @cap/const-oid for canonical OID TLV (Tag 0x06) encoding and decoding.
// Clean-room MIT implementation (RustCrypto `der` parity).

import { ObjectIdentifier, OidParseResult } from "../../const-oid/src/index.cp";

// Standard ITU-T X.690 DER universal tags
pub const TAG_BOOLEAN: u8 = 0x01;
pub const TAG_INTEGER: u8 = 0x02;
pub const TAG_BIT_STRING: u8 = 0x03;
pub const TAG_OCTET_STRING: u8 = 0x04;
pub const TAG_NULL: u8 = 0x05;
pub const TAG_OID: u8 = 0x06;
pub const TAG_UTF8_STRING: u8 = 0x0C;
pub const TAG_PRINTABLE_STRING: u8 = 0x13;
pub const TAG_IA5_STRING: u8 = 0x16;
pub const TAG_UTC_TIME: u8 = 0x17;
pub const TAG_GENERALIZED_TIME: u8 = 0x18;
pub const TAG_SEQUENCE: u8 = 0x30;
pub const TAG_SET: u8 = 0x31;

fn derEmptySlice(): []const u8 {
    return "";
}

fn derMinBytesUsize(v: usize): usize {
    let n: usize = 0;
    let x = v;
    while (x > 0) {
        n = n + 1;
        x = x >> (8 as u6);
    }
    if (n == 0) {
        return 1;
    }
    return n;
}

pub struct DerHeader {
    pub tag: u8,
    pub len: usize,
    pub header_len: usize,
    pub ok: bool,
}

pub struct DerTlvResult {
    pub tag: u8,
    pub value: []const u8,
    pub ok: bool,
}

pub struct DerSliceResult {
    pub data: []const u8,
    pub ok: bool,
}

pub struct DerBoolResult {
    pub value: bool,
    pub ok: bool,
}

pub struct DerU64Result {
    pub value: u64,
    pub ok: bool,
}

pub struct DerBitStringResult {
    pub unused_bits: u8,
    pub bytes: []const u8,
    pub ok: bool,
}

pub struct DerSequenceResult {
    pub reader: DerReader,
    pub ok: bool,
}

/// Zero-copy cursor reader for strict ITU-T X.690 DER decoding.
pub struct DerReader {
    pub buf: []const u8,
    pub pos: usize,

    pub static fn new(buf: []const u8): DerReader {
        return DerReader {
            buf: buf,
            pos: 0,
        };
    }

    pub fn remaining(): usize {
        if (_.pos >= _.buf.len) {
            return 0;
        }
        return _.buf.len - _.pos;
    }

    pub fn isEmpty(): bool {
        return _.pos >= _.buf.len;
    }

    pub fn peekTag(): u8 {
        if (_.pos >= _.buf.len) {
            return 0 as u8;
        }
        return _.buf[_.pos];
    }

    /// Reads a strict DER tag + definite minimal length header and advances cursor past the header.
    /// Rejects indefinite length (0x80), reserved 0xFF, non-minimal long form, leading 0x00 in length,
    /// length overflow, and truncated payloads.
    pub fn readHeader(*_): DerHeader {
        const fail = DerHeader { tag: 0 as u8, len: 0, header_len: 0, ok: false };
        if (_.pos + 2 > _.buf.len) {
            return fail;
        }
        const tag = _.buf[_.pos];
        const first = _.buf[_.pos + 1];
        let val_len: usize = 0;
        let hdr_len: usize = 2;

        if (first < (0x80 as u8)) {
            val_len = first as usize;
        } else {
            // Indefinite length (0x80) and reserved (0xFF) are forbidden in DER
            if (first == (0x80 as u8) || first == (0xFF as u8)) {
                return fail;
            }
            const num_octets = (first & (0x7F as u8)) as usize;
            if (num_octets > (8 as usize) || _.pos + 2 + num_octets > _.buf.len) {
                return fail;
            }
            // X.690 §10.1: long-form length must not have leading 0x00 byte
            if (_.buf[_.pos + 2] == (0x00 as u8)) {
                return fail;
            }
            let acc: u64 = 0 as u64;
            let i: usize = 0;
            while (i < num_octets) {
                acc = (acc << (8 as u6)) | (_.buf[_.pos + 2 + i] as u64);
                i = i + 1;
            }
            // Long form is only valid for lengths >= 128
            if (acc < (128 as u64)) {
                return fail;
            }
            val_len = acc as usize;
            hdr_len = (2 as usize) + num_octets;
        }

        // Verify payload fits inside remaining buffer (overflow-safe subtraction)
        if (val_len > _.buf.len - (_.pos + hdr_len)) {
            return fail;
        }

        _.pos = _.pos + hdr_len;
        return DerHeader {
            tag: tag,
            len: val_len,
            header_len: hdr_len,
            ok: true,
        };
    }

    /// Reads the next complete DER TLV element (zero-copy value slice).
    pub fn readTlv(*_): DerTlvResult {
        const saved = _.pos;
        const hdr = _.readHeader();
        if (!hdr.ok) {
            _.pos = saved;
            return DerTlvResult { tag: 0 as u8, value: derEmptySlice(), ok: false };
        }
        const v_start = _.pos;
        const v_end = v_start + hdr.len;
        _.pos = v_end;
        return DerTlvResult {
            tag: hdr.tag,
            value: _.buf[v_start..v_end],
            ok: true,
        };
    }

    /// Reads a strict DER BOOLEAN (Tag 0x01, length 1, value strictly 0x00 or 0xFF).
    pub fn readBool(*_): DerBoolResult {
        const saved = _.pos;
        const tlv = _.readTlv();
        if (!tlv.ok || tlv.tag != TAG_BOOLEAN || tlv.value.len != 1) {
            _.pos = saved;
            return DerBoolResult { value: false, ok: false };
        }
        const b = tlv.value[0];
        if (b == (0x00 as u8)) {
            return DerBoolResult { value: false, ok: true };
        }
        if (b == (0xFF as u8)) {
            return DerBoolResult { value: true, ok: true };
        }
        _.pos = saved;
        return DerBoolResult { value: false, ok: false };
    }

    /// Reads a strict DER non-negative INTEGER into `u64`.
    /// Rejects empty integer, redundant leading 0x00 / 0xFF, negative integers, and >64-bit values.
    pub fn readU64(*_): DerU64Result {
        const saved = _.pos;
        const tlv = _.readTlv();
        if (!tlv.ok || tlv.tag != TAG_INTEGER || tlv.value.len == 0) {
            _.pos = saved;
            return DerU64Result { value: 0 as u64, ok: false };
        }
        const v = tlv.value;
        // X.690 §8.3.2 minimal encoding check
        if (v.len > 1) {
            if (v[0] == (0x00 as u8) && (v[1] & (0x80 as u8)) == 0) {
                _.pos = saved;
                return DerU64Result { value: 0 as u64, ok: false };
            }
            if (v[0] == (0xFF as u8) && (v[1] & (0x80 as u8)) != 0) {
                _.pos = saved;
                return DerU64Result { value: 0 as u64, ok: false };
            }
        }
        // Reject negative integers for unsigned u64 reader
        if ((v[0] & (0x80 as u8)) != 0) {
            _.pos = saved;
            return DerU64Result { value: 0 as u64, ok: false };
        }

        let start: usize = 0;
        if (v.len > 1 && v[0] == (0x00 as u8)) {
            start = 1;
        }
        const mag_len = v.len - start;
        if (mag_len > (8 as usize)) {
            _.pos = saved;
            return DerU64Result { value: 0 as u64, ok: false };
        }

        let acc: u64 = 0 as u64;
        let i = start;
        while (i < v.len) {
            acc = (acc << (8 as u6)) | (v[i] as u64);
            i = i + 1;
        }
        return DerU64Result { value: acc, ok: true };
    }

    /// Reads a DER OCTET STRING (Tag 0x04) as a zero-copy byte slice.
    pub fn readOctetString(*_): DerSliceResult {
        const saved = _.pos;
        const tlv = _.readTlv();
        if (!tlv.ok || tlv.tag != TAG_OCTET_STRING) {
            _.pos = saved;
            return DerSliceResult { data: derEmptySlice(), ok: false };
        }
        return DerSliceResult { data: tlv.value, ok: true };
    }

    /// Reads a strict DER BIT STRING (Tag 0x03).
    /// Enforces `unused_bits <= 7`, `unused_bits == 0` when empty, and zeroed trailing unused bits.
    pub fn readBitString(*_): DerBitStringResult {
        const saved = _.pos;
        const fail = DerBitStringResult { unused_bits: 0 as u8, bytes: derEmptySlice(), ok: false };
        const tlv = _.readTlv();
        if (!tlv.ok || tlv.tag != TAG_BIT_STRING || tlv.value.len == 0) {
            _.pos = saved;
            return fail;
        }
        const unused = tlv.value[0];
        if (unused > (7 as u8)) {
            _.pos = saved;
            return fail;
        }
        if (tlv.value.len == 1) {
            if (unused != (0 as u8)) {
                _.pos = saved;
                return fail;
            }
            return DerBitStringResult { unused_bits: 0 as u8, bytes: derEmptySlice(), ok: true };
        }
        if (unused > (0 as u8)) {
            const mask: u8 = (((1 as u32) << (unused as u5)) - (1 as u32)) as u8;
            const last = tlv.value[tlv.value.len - 1];
            if ((last & mask) != (0 as u8)) {
                _.pos = saved;
                return fail;
            }
        }
        return DerBitStringResult {
            unused_bits: unused,
            bytes: tlv.value[1..tlv.value.len],
            ok: true,
        };
    }

    /// Reads a DER NULL (Tag 0x05, length 0).
    pub fn readNull(*_): bool {
        const saved = _.pos;
        const tlv = _.readTlv();
        if (!tlv.ok || tlv.tag != TAG_NULL || tlv.value.len != 0) {
            _.pos = saved;
            return false;
        }
        return true;
    }

    /// Reads a DER OBJECT IDENTIFIER (Tag 0x06), delegating parsing to `@cap/const-oid`.
    pub fn readOid(*_): OidParseResult {
        const saved = _.pos;
        const tlv = _.readTlv();
        if (!tlv.ok || tlv.tag != TAG_OID) {
            _.pos = saved;
            return OidParseResult { ok: false, oid: ObjectIdentifier.empty() };
        }
        const res = ObjectIdentifier.fromDer(tlv.value);
        if (!res.ok) {
            _.pos = saved;
        }
        return res;
    }

    /// Reads a DER SEQUENCE (Tag 0x30) and returns a nested zero-copy `DerReader` over its elements.
    pub fn readSequence(*_): DerSequenceResult {
        const saved = _.pos;
        const tlv = _.readTlv();
        if (!tlv.ok || tlv.tag != TAG_SEQUENCE) {
            _.pos = saved;
            return DerSequenceResult { reader: DerReader.new(derEmptySlice()), ok: false };
        }
        return DerSequenceResult {
            reader: DerReader.new(tlv.value),
            ok: true,
        };
    }
}

/// Fixed-slice zero-allocation writer for canonical ITU-T X.690 DER encoding.
pub struct DerWriter {
    pub buf: []u8,
    pub pos: usize,

    pub static fn new(buf: []u8): DerWriter {
        return DerWriter {
            buf: buf,
            pos: 0,
        };
    }

    pub fn len(): usize {
        return _.pos;
    }

    pub fn remaining(): usize {
        if (_.pos >= _.buf.len) {
            return 0;
        }
        return _.buf.len - _.pos;
    }

    pub fn written(): []const u8 {
        return _.buf[0.._.pos];
    }

    /// Returns the number of bytes required to encode `length` in canonical DER definite form.
    pub static fn encodedLengthSize(length: usize): usize {
        if (length < (128 as usize)) {
            return 1;
        }
        return (1 as usize) + derMinBytesUsize(length);
    }

    /// Encodes `length` in canonical DER definite length form into `out`.
    /// Returns number of bytes written, or `0` if `out` is too small.
    pub static fn encodeLength(length: usize, out: []u8): usize {
        if (length < (128 as usize)) {
            if (out.len < 1) {
                return 0;
            }
            out[0] = length as u8;
            return 1;
        }
        const n = derMinBytesUsize(length);
        const total = (1 as usize) + n;
        if (out.len < total) {
            return 0;
        }
        out[0] = ((0x80 as usize) | n) as u8;
        let i: usize = 0;
        while (i < n) {
            const shift = ((n - 1 - i) * 8) as u6;
            out[1 + i] = ((length >> shift) & (0xFF as usize)) as u8;
            i = i + 1;
        }
        return total;
    }

    /// Writes a canonical DER TLV element (`tag` + minimal definite length + `value`).
    pub fn writeTlv(*_, tag: u8, value: []const u8): bool {
        const lsize = DerWriter.encodedLengthSize(value.len);
        const total = (1 as usize) + lsize + value.len;
        if (_.pos + total > _.buf.len) {
            return false;
        }
        _.buf[_.pos] = tag;
        _.pos = _.pos + 1;
        const lw = DerWriter.encodeLength(value.len, _.buf[_.pos.._.buf.len]);
        if (lw == 0) {
            return false;
        }
        _.pos = _.pos + lw;
        let i: usize = 0;
        while (i < value.len) {
            _.buf[_.pos + i] = value[i];
            i = i + 1;
        }
        _.pos = _.pos + value.len;
        return true;
    }

    /// Writes a canonical DER BOOLEAN (`0x00` for false, `0xFF` for true).
    pub fn writeBool(*_, val: bool): bool {
        let b: [1]u8 = [0 as u8];
        if (val) {
            b[0] = 0xFF as u8;
        }
        return _.writeTlv(TAG_BOOLEAN, b[0..1]);
    }

    /// Writes a canonical minimal unsigned DER INTEGER from `u64`.
    /// Automatically prepends a single `0x00` byte when the high bit (0x80) is set.
    pub fn writeU64(*_, val: u64): bool {
        if (val == (0 as u64)) {
            let z: [1]u8 = [0 as u8];
            return _.writeTlv(TAG_INTEGER, z[0..1]);
        }
        let tmp: [9]u8 = [0; 9];
        let n: usize = 0;
        let x = val;
        while (x > (0 as u64)) {
            tmp[n] = (x & (0xFF as u64)) as u8;
            x = x >> (8 as u6);
            n = n + 1;
        }
        // Reverse into big-endian, prepending 0x00 if MSB has bit 7 set
        let out: [9]u8 = [0; 9];
        let start: usize = 0;
        if ((tmp[n - 1] & (0x80 as u8)) != 0) {
            out[0] = 0 as u8;
            start = 1;
        }
        let i: usize = 0;
        while (i < n) {
            out[start + i] = tmp[n - 1 - i];
            i = i + 1;
        }
        return _.writeTlv(TAG_INTEGER, out[0..(start + n)]);
    }

    /// Writes a DER OCTET STRING (Tag 0x04).
    pub fn writeOctetString(*_, data: []const u8): bool {
        return _.writeTlv(TAG_OCTET_STRING, data);
    }

    /// Writes a strict DER BIT STRING (Tag 0x03).
    /// Rejects `unused_bits > 7`, non-zero `unused_bits` on empty data, or non-zero trailing unused bits.
    pub fn writeBitString(*_, unused_bits: u8, data: []const u8): bool {
        if (unused_bits > (7 as u8)) {
            return false;
        }
        if (data.len == 0 && unused_bits != (0 as u8)) {
            return false;
        }
        if (data.len > 0 && unused_bits > (0 as u8)) {
            const mask: u8 = (((1 as u32) << (unused_bits as u5)) - (1 as u32)) as u8;
            if ((data[data.len - 1] & mask) != (0 as u8)) {
                return false;
            }
        }
        const payload_len = (1 as usize) + data.len;
        const lsize = DerWriter.encodedLengthSize(payload_len);
        const total = (1 as usize) + lsize + payload_len;
        if (_.pos + total > _.buf.len) {
            return false;
        }
        _.buf[_.pos] = TAG_BIT_STRING;
        _.pos = _.pos + 1;
        const lw = DerWriter.encodeLength(payload_len, _.buf[_.pos.._.buf.len]);
        if (lw == 0) {
            return false;
        }
        _.pos = _.pos + lw;
        _.buf[_.pos] = unused_bits;
        _.pos = _.pos + 1;
        let i: usize = 0;
        while (i < data.len) {
            _.buf[_.pos + i] = data[i];
            i = i + 1;
        }
        _.pos = _.pos + data.len;
        return true;
    }

    /// Writes a DER NULL (Tag 0x05, length 0).
    pub fn writeNull(*_): bool {
        return _.writeTlv(TAG_NULL, derEmptySlice());
    }

    /// Writes a DER OBJECT IDENTIFIER (Tag 0x06), delegating TLV encoding to `@cap/const-oid`.
    pub fn writeOid(*_, oid: ObjectIdentifier): bool {
        const n_written = oid.toDerTlv(_.buf[_.pos.._.buf.len]);
        if (n_written == 0) {
            return false;
        }
        _.pos = _.pos + n_written;
        return true;
    }

    /// Writes a DER SEQUENCE (Tag 0x30) wrapping pre-encoded `content` bytes.
    pub fn writeSequence(*_, content: []const u8): bool {
        return _.writeTlv(TAG_SEQUENCE, content);
    }
}
