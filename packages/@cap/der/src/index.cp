// @cap/der — Zero-allocation ITU-T X.690 ASN.1 DER TLV codec for CP
// Clean-room MIT implementation inspired by RustCrypto `der` (crates.io #274, 613M+ downloads).
//
// Features:
// - Standard ASN.1 universal tag constants (`TAG_BOOLEAN` .. `TAG_SET`).
// - Strict DER TLV decoder (`parseDerTlv`) supporting short (<128) and long (0x81..0x84) lengths
//   while rejecting indefinite lengths (0x80), non-minimal length encodings, leading-zero lengths, and truncation.
// - Strict DER `BOOLEAN` (only 0x00 / 0xFF) and unsigned `INTEGER` (`u64`) decoders rejecting non-minimal forms.
// - Zero-allocation DER encoders (`encodeDerTlv`, `encodeDerBool`, `encodeDerNull`, `encodeDerU64`,
//   `encodeDerOctetString`, `encodeDerSequence`) writing directly into caller-provided `out: []u8`.

pub const TAG_BOOLEAN: u8 = 0x01;
pub const TAG_INTEGER: u8 = 0x02;
pub const TAG_BIT_STRING: u8 = 0x03;
pub const TAG_OCTET_STRING: u8 = 0x04;
pub const TAG_NULL: u8 = 0x05;
pub const TAG_OID: u8 = 0x06;
pub const TAG_UTF8_STRING: u8 = 0x0C;
pub const TAG_PRINTABLE_STRING: u8 = 0x13;
pub const TAG_SEQUENCE: u8 = 0x30;
pub const TAG_SET: u8 = 0x31;

/// Zero-copy ASN.1 DER TLV view whose `value` slices directly into the input buffer.
pub struct DerTlv {
    pub valid: bool,
    pub tag: u8,
    pub value: string,
    pub total_len: usize,

    pub static fn invalid() -> DerTlv {
        return DerTlv{
            valid: false,
            tag: 0,
            value: "",
            total_len: 0,
        };
    }
}

/// Strictly parses one ASN.1 DER TLV element from the start of `input`.
/// Rejects indefinite length (`0x80`), non-minimal long-form length (`< 128`),
/// leading zero length octets, length > 4 octets, and truncated inputs.
pub fn parseDerTlv(input: string) -> DerTlv {
    if (input.len < 2) {
        return DerTlv.invalid();
    }

    const tag = input[0];
    const len_byte = input[1];
    let val_len: usize = 0;
    let header_len: usize = 2;

    if (len_byte < 0x80) {
        val_len = len_byte as usize;
    } else {
        const num_octets = (len_byte & 0x7F) as usize;
        // Reject indefinite length (0x80) and > 4-byte length
        if (num_octets == 0 || num_octets > 4) {
            return DerTlv.invalid();
        }
        if (2 + num_octets > input.len) {
            return DerTlv.invalid();
        }
        // Reject non-minimal leading zero octet in length
        if (input[2] == 0x00) {
            return DerTlv.invalid();
        }
        let i: usize = 0;
        while (i < num_octets) {
            val_len = (val_len << 8) | (input[2 + i] as usize);
            i += 1;
        }
        // Reject long-form length for values that fit in short form (< 128)
        if (val_len < 128) {
            return DerTlv.invalid();
        }
        header_len = (2 as usize) + num_octets;
    }

    if (val_len > input.len - header_len) {
        return DerTlv.invalid();
    }

    return DerTlv{
        valid: true,
        tag: tag,
        value: input[header_len..(header_len + val_len)],
        total_len: header_len + val_len,
    };
}

/// Decodes a DER `BOOLEAN` TLV. Strictly allows only `0x00` (false) or `0xFF` (true).
pub fn decodeDerBool(tlv: DerTlv) -> bool? {
    if (!tlv.valid || tlv.tag != TAG_BOOLEAN || tlv.value.len != 1) {
        return null;
    }
    const b = tlv.value[0];
    if (b == 0x00) {
        return false;
    }
    if (b == 0xFF) {
        return true;
    }
    return null;
}

/// Decodes a non-negative DER `INTEGER` TLV into `u64`.
/// Strictly rejects negative integers, empty payloads, non-minimal leading `0x00`, and values > `u64::MAX`.
pub fn decodeDerU64(tlv: DerTlv) -> u64? {
    if (!tlv.valid || tlv.tag != TAG_INTEGER || tlv.value.len == 0) {
        return null;
    }
    const v = tlv.value;
    // Reject negative two's-complement integers
    if ((v[0] & 0x80) != 0) {
        return null;
    }

    let start: usize = 0;
    if (v.len > 1 && v[0] == 0x00) {
        // Leading 0x00 is only valid in DER when the next byte has its high bit set
        if ((v[1] & 0x80) == 0) {
            return null;
        }
        start = 1;
    }

    const sig_len = v.len - start;
    if (sig_len > 8) {
        return null;
    }

    let acc: u64 = 0;
    let i = start;
    while (i < v.len) {
        acc = (acc << 8) | (v[i] as u64);
        i += 1;
    }
    return acc;
}

fn derLengthSize(val_len: usize) -> usize {
    if (val_len < 128) {
        return 1;
    }
    if (val_len <= 0xFF) {
        return 2;
    }
    if (val_len <= 0xFFFF) {
        return 3;
    }
    if (val_len <= 0xFFFFFF) {
        return 4;
    }
    return 5;
}

fn writeDerLength(out: []u8, pos: usize, val_len: usize) -> usize {
    if (val_len < 128) {
        out[pos] = val_len as u8;
        return pos + 1;
    }
    if (val_len <= 0xFF) {
        out[pos] = 0x81;
        out[pos + 1] = val_len as u8;
        return pos + 2;
    }
    if (val_len <= 0xFFFF) {
        out[pos] = 0x82;
        out[pos + 1] = ((val_len >> 8) & 0xFF) as u8;
        out[pos + 2] = (val_len & 0xFF) as u8;
        return pos + 3;
    }
    if (val_len <= 0xFFFFFF) {
        out[pos] = 0x83;
        out[pos + 1] = ((val_len >> 16) & 0xFF) as u8;
        out[pos + 2] = ((val_len >> 8) & 0xFF) as u8;
        out[pos + 3] = (val_len & 0xFF) as u8;
        return pos + 4;
    }
    out[pos] = 0x84;
    out[pos + 1] = ((val_len >> 24) & 0xFF) as u8;
    out[pos + 2] = ((val_len >> 16) & 0xFF) as u8;
    out[pos + 3] = ((val_len >> 8) & 0xFF) as u8;
    out[pos + 4] = (val_len & 0xFF) as u8;
    return pos + 5;
}

/// Encodes an arbitrary DER TLV (`tag`, canonical length, `value`) into `out`. Returns `null` if `out` is too small.
pub fn encodeDerTlv(tag: u8, value: string, out: []u8) -> string? {
    const lsize = derLengthSize(value.len);
    const total = 1 + lsize + value.len;
    if (total > out.len) {
        return null;
    }
    out[0] = tag;
    const vpos = writeDerLength(out, 1, value.len);
    let i: usize = 0;
    while (i < value.len) {
        out[vpos + i] = value[i];
        i += 1;
    }
    return out[0..total];
}

/// Encodes a canonical DER `BOOLEAN` (`01 01 00` or `01 01 FF`) into `out`.
pub fn encodeDerBool(v: bool, out: []u8) -> string? {
    if (out.len < 3) {
        return null;
    }
    out[0] = TAG_BOOLEAN;
    out[1] = 0x01;
    out[2] = if (v) { 0xFF as u8 } else { 0x00 as u8 };
    return out[0..3];
}

/// Encodes a DER `NULL` (`05 00`) into `out`.
pub fn encodeDerNull(out: []u8) -> string? {
    if (out.len < 2) {
        return null;
    }
    out[0] = TAG_NULL;
    out[1] = 0x00;
    return out[0..2];
}

/// Encodes an unsigned `u64` as a canonical non-negative DER `INTEGER` into `out`.
/// Automatically prepends a leading `0x00` when the most significant byte has bit 7 set (`>= 0x80`).
pub fn encodeDerU64(v: u64, out: []u8) -> string? {
    let payload: [9]u8 = undefined;
    let plen: usize = 0;

    if (v == 0) {
        payload[0] = 0x00;
        plen = 1;
    } else {
        let rev: [8]u8 = undefined;
        let rlen: usize = 0;
        let x = v;
        while (x > 0) {
            rev[rlen] = (x & 0xFF) as u8;
            rlen += 1;
            x = x >> 8;
        }
        // Prepend 0x00 if MSB has bit 7 set to keep integer non-negative
        if ((rev[rlen - 1] & 0x80) != 0) {
            payload[plen] = 0x00;
            plen += 1;
        }
        let k = rlen;
        while (k > 0) {
            k -= 1;
            payload[plen] = rev[k];
            plen += 1;
        }
    }

    return encodeDerTlv(TAG_INTEGER, payload[0..plen], out);
}

/// Encodes `data` as a DER `OCTET STRING` into `out`.
pub fn encodeDerOctetString(data: string, out: []u8) -> string? {
    return encodeDerTlv(TAG_OCTET_STRING, data, out);
}

/// Wraps pre-encoded DER `inner_bytes` inside a DER `SEQUENCE` (`0x30`) into `out`.
pub fn encodeDerSequence(inner_bytes: string, out: []u8) -> string? {
    return encodeDerTlv(TAG_SEQUENCE, inner_bytes, out);
}
