// Origin: Pure-CP Stdlib (Zero-Alloc RFC 4302 AH / RFC 4303 ESP / RFC 7296 IKEv2 Parser) | Created: 2026-11-22
// Pure CP zero-heap-allocation IPsec & IKEv2 parser:
// RFC 4303 ESP header + decrypted trailer stripping, RFC 4302 AH header & ICV extraction,
// and RFC 7296 IKEv2 28-byte header + chained Generic Payload TLV iterator.

/// IP protocol number for ESP (`50`, RFC 4303).
pub const IPSEC_PROTO_ESP: u8 = 50;

/// IP protocol number for AH (`51`, RFC 4302).
pub const IPSEC_PROTO_AH: u8 = 51;

/// Standard IKE UDP port (`500`).
pub const IKE_UDP_PORT: u16 = 500;

/// NAT-Traversal IKE/ESP-in-UDP port (`4500`).
pub const IKE_NATT_UDP_PORT: u16 = 4500;

/// Minimum header sizes in bytes.
pub const ESP_MIN_HEADER_SIZE: usize = 8;
pub const AH_MIN_HEADER_SIZE: usize = 12;
pub const IKEV2_HEADER_SIZE: usize = 28;

// IKEv2 Exchange Types (RFC 7296 Section 3.1)
pub const IKE_EXCH_SA_INIT: u8 = 34;
pub const IKE_EXCH_AUTH: u8 = 35;
pub const IKE_EXCH_CREATE_CHILD_SA: u8 = 36;
pub const IKE_EXCH_INFORMATIONAL: u8 = 37;

// IKEv2 Header Flags (RFC 7296 Section 3.1)
pub const IKE_FLAG_INITIATOR: u8 = 0x08;
pub const IKE_FLAG_VERSION: u8 = 0x10;
pub const IKE_FLAG_RESPONSE: u8 = 0x20;

// IKEv2 Next Payload Types (RFC 7296 Section 3.2)
pub const PL_NONE: u8 = 0;
pub const PL_SA: u8 = 33;
pub const PL_KE: u8 = 34;
pub const PL_IDI: u8 = 35;
pub const PL_IDR: u8 = 36;
pub const PL_CERT: u8 = 37;
pub const PL_CERTREQ: u8 = 38;
pub const PL_AUTH: u8 = 39;
pub const PL_NONCE: u8 = 40;
pub const PL_NOTIFY: u8 = 41;
pub const PL_DELETE: u8 = 42;
pub const PL_VENDOR_ID: u8 = 43;
pub const PL_TSI: u8 = 44;
pub const PL_TSR: u8 = 45;
pub const PL_SK: u8 = 46;

/// Zero-copy decoded RFC 4303 ESP packet header and opaque payload/trailer slice.
pub struct EspPacket {
    spi: u32 = 0,
    seq_num: u32 = 0,
    payload_and_trailer: []const u8 = "",
}

/// Zero-copy decoded ESP trailer stripped from a decrypted ESP payload.
pub struct EspTrailer {
    inner_payload: []const u8 = "",
    pad_len: u8 = 0,
    next_header: u8 = 0,
    icv: []const u8 = "",
}

/// Zero-copy decoded RFC 4302 Authentication Header (AH) packet.
pub struct AhPacket {
    next_header: u8 = 0,
    payload_len_words: u8 = 0,
    header_bytes: usize = 12,
    spi: u32 = 0,
    seq_num: u32 = 0,
    icv: []const u8 = "",
    payload: []const u8 = "",
}

/// Zero-copy decoded RFC 7296 IKEv2 28-byte header and payload chain slice.
pub struct Ikev2Header {
    initiator_spi: u64 = 0,
    responder_spi: u64 = 0,
    next_payload: u8 = 0,
    major_version: u8 = 2,
    minor_version: u8 = 0,
    exchange_type: u8 = 0,
    flags: u8 = 0,
    message_id: u32 = 0,
    length: u32 = 28,
    payloads_slice: []const u8 = "",

    fn isInitiator(): bool {
        return (_.flags & IKE_FLAG_INITIATOR) != 0;
    }

    fn isResponse(): bool {
        return (_.flags & IKE_FLAG_RESPONSE) != 0;
    }

    /// Returns a zero-allocation chained iterator over the IKEv2 Generic Payloads.
    fn payloads(): Ikev2PayloadIter {
        return Ikev2PayloadIter.init(_.payloads_slice, _.next_payload);
    }
}

/// Zero-copy decoded RFC 7296 IKEv2 Generic Payload element.
pub struct Ikev2Payload {
    payload_type: u8 = 0,
    next_payload: u8 = 0,
    is_critical: bool = false,
    body: []const u8 = "",
}

// ---------------------------------------------------------------------------
// Internal Big-Endian Helpers
// ---------------------------------------------------------------------------

fn readBeU16(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn readBeU32(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24)
        | ((data[off + 1] as u32) << 16)
        | ((data[off + 2] as u32) << 8)
        | (data[off + 3] as u32);
}

fn readBeU64(data: []const u8, off: usize): u64 {
    const hi = readBeU32(data, off) as u64;
    const lo = readBeU32(data, off + 4) as u64;
    return (hi << 32) | lo;
}

// ---------------------------------------------------------------------------
// RFC 4303 ESP Parsing API
// ---------------------------------------------------------------------------

/// Parses an RFC 4303 ESP packet header (`SPI`, `SeqNum`, `payload_and_trailer`).
pub fn parseEsp(data: []const u8): EspPacket? {
    if (data.len < ESP_MIN_HEADER_SIZE) {
        return null;
    }
    return EspPacket {
        spi: readBeU32(data, 0),
        seq_num: readBeU32(data, 4),
        payload_and_trailer: data[ESP_MIN_HEADER_SIZE..data.len],
    };
}

/// Strips the trailing `icv_len` bytes, `next_header`, `pad_len`, and padding from a decrypted
/// ESP payload, returning the inner plaintext payload slice and trailer metadata.
pub fn parseEspTrailer(decrypted_payload: []const u8, icv_len: usize): EspTrailer? {
    if (decrypted_payload.len < icv_len || decrypted_payload.len - icv_len < 2) {
        return null;
    }
    const body_end = decrypted_payload.len - icv_len;
    const trailer_start = body_end - 2;
    const pad_len = decrypted_payload[trailer_start];
    const next_header = decrypted_payload[trailer_start + 1];
    const pad_usize = pad_len as usize;
    if (pad_usize > trailer_start) {
        return null;
    }
    const inner_end = trailer_start - pad_usize;
    return EspTrailer {
        inner_payload: decrypted_payload[0..inner_end],
        pad_len: pad_len,
        next_header: next_header,
        icv: decrypted_payload[body_end..decrypted_payload.len],
    };
}

// ---------------------------------------------------------------------------
// RFC 4302 AH Parsing API
// ---------------------------------------------------------------------------

/// Strictly parses an RFC 4302 Authentication Header (AH) packet.
/// Enforces `reserved == 0` and `12 <= (payload_len_words + 2) * 4 <= data.len`.
pub fn parseAh(data: []const u8): AhPacket? {
    if (data.len < AH_MIN_HEADER_SIZE) {
        return null;
    }
    const next_header = data[0];
    const payload_len_words = data[1];
    const reserved = readBeU16(data, 2);
    if (reserved != 0) {
        return null;
    }
    const header_bytes = ((payload_len_words as usize) + 2) * 4;
    if (header_bytes < AH_MIN_HEADER_SIZE || header_bytes > data.len) {
        return null;
    }
    return AhPacket {
        next_header: next_header,
        payload_len_words: payload_len_words,
        header_bytes: header_bytes,
        spi: readBeU32(data, 4),
        seq_num: readBeU32(data, 8),
        icv: data[AH_MIN_HEADER_SIZE..header_bytes],
        payload: data[header_bytes..data.len],
    };
}

// ---------------------------------------------------------------------------
// RFC 7296 IKEv2 Header & Chained Generic Payload Iterator
// ---------------------------------------------------------------------------

/// Strictly parses an RFC 7296 IKEv2 28-byte header (`major_version == 2`, `28 <= length <= data.len`).
pub fn parseIkev2(data: []const u8): Ikev2Header? {
    if (data.len < IKEV2_HEADER_SIZE) {
        return null;
    }
    const ver_byte = data[17];
    const major_version: u8 = ver_byte >> 4;
    const minor_version: u8 = ver_byte & 0x0F;
    if (major_version != 2) {
        return null;
    }
    const msg_len = readBeU32(data, 24);
    const msg_len_usize = msg_len as usize;
    if (msg_len_usize < IKEV2_HEADER_SIZE || msg_len_usize > data.len) {
        return null;
    }
    return Ikev2Header {
        initiator_spi: readBeU64(data, 0),
        responder_spi: readBeU64(data, 8),
        next_payload: data[16],
        major_version: major_version,
        minor_version: minor_version,
        exchange_type: data[18],
        flags: data[19],
        message_id: readBeU32(data, 20),
        length: msg_len,
        payloads_slice: data[IKEV2_HEADER_SIZE..msg_len_usize],
    };
}

/// Returns `true` if `data` starts with a structurally valid RFC 7296 IKEv2 header.
pub fn isIkev2(data: []const u8): bool {
    if (hdr := parseIkev2(data)) {
        * = hdr;
        return true;
    }
    return false;
}

/// Zero-allocation chained iterator over RFC 7296 IKEv2 Generic Payloads.
pub struct Ikev2PayloadIter {
    data: []const u8 = "",
    pos: usize = 0,
    current_type: u8 = PL_NONE,
    failed: bool = false,

    pub fn init(payloads_slice: []const u8, first_payload: u8): Ikev2PayloadIter {
        return Ikev2PayloadIter {
            data: payloads_slice,
            pos: 0,
            current_type: first_payload,
            failed: false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): Ikev2Payload? {
        if (_.failed || _.current_type == PL_NONE) {
            return null;
        }
        if (_.pos >= _.data.len || _.data.len - _.pos < 4) {
            _.failed = true;
            return null;
        }
        const next_pl = _.data[_.pos];
        const pl_flags = _.data[_.pos + 1];
        const is_critical = (pl_flags & 0x80) != 0;
        const pl_len = readBeU16(_.data, _.pos + 2) as usize;
        if (pl_len < 4 || pl_len > _.data.len - _.pos) {
            _.failed = true;
            return null;
        }
        const body = _.data[(_.pos + 4)..(_.pos + pl_len)];
        const this_type = _.current_type;
        _.current_type = next_pl;
        _.pos += pl_len;

        return Ikev2Payload {
            payload_type: this_type,
            next_payload: next_pl,
            is_critical: is_critical,
            body: body,
        };
    }
}
