// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RFC 3748 EAP + IEEE 802.1X EAPOL Parser)
// Pure CP zero-heap-allocation IEEE 802.1X EAPOL and RFC 3748 EAP wire protocol parser:
// EAPOL frame unwrapping, EAP Request/Response/Success/Failure decoding, and
// RFC 3748 Section 5.7 Expanded Type (254) Vendor-ID / Vendor-Type extraction.

/// IEEE 802.1X EAPOL EtherType (`0x888E`).
pub const EAPOL_ETHERTYPE: u16 = 0x888E;

/// Fixed IEEE 802.1X EAPOL header size in bytes (`4`).
pub const EAPOL_HEADER_SIZE: usize = 4;

/// Fixed RFC 3748 EAP header size in bytes (`4`).
pub const EAP_HEADER_SIZE: usize = 4;

/// RFC 3748 Expanded Type header size in bytes (`8`: 1B type=254 + 3B Vendor-ID + 4B Vendor-Type).
pub const EAP_EXPANDED_HEADER_SIZE: usize = 8;

// Common RFC 3748 / IANA EAP Method Types
pub const EAP_TYPE_IDENTITY: u8 = 1;
pub const EAP_TYPE_NOTIFICATION: u8 = 2;
pub const EAP_TYPE_NAK: u8 = 3;
pub const EAP_TYPE_MD5_CHALLENGE: u8 = 4;
pub const EAP_TYPE_OTP: u8 = 5;
pub const EAP_TYPE_GTC: u8 = 6;
pub const EAP_TYPE_TLS: u8 = 13;
pub const EAP_TYPE_TTLS: u8 = 21;
pub const EAP_TYPE_AKA: u8 = 23;
pub const EAP_TYPE_PEAP: u8 = 25;
pub const EAP_TYPE_MSCHAPV2: u8 = 26;
pub const EAP_TYPE_EXPANDED: u8 = 254;

/// IEEE 802.1X EAPOL Packet Types.
pub enum EapolType: u8 {
    eap_packet = 0,
    start = 1,
    logoff = 2,
    key = 3,
    encap_asf_alert = 4,
    unknown = 255,
}

/// RFC 3748 EAP Packet Codes.
pub enum EapCode: u8 {
    request = 1,
    response = 2,
    success = 3,
    failure = 4,
}

/// Zero-copy decoded IEEE 802.1X EAPOL frame.
pub struct EapolFrame {
    version: u8 = 1,
    pkt_type: EapolType = EapolType.eap_packet,
    raw_type: u8 = 0,
    body_len: u16 = 0,
    body: []const u8 = "",

    /// Convenience helper: decodes the inner EAP packet when `pkt_type == EapolType.eap_packet`.
    fn asEap(): EapPacket? {
        if (_.pkt_type != EapolType.eap_packet) {
            return null;
        }
        return parseEap(_.body);
    }
}

/// Zero-copy decoded RFC 3748 EAP packet.
pub struct EapPacket {
    code: EapCode = EapCode.request,
    identifier: u8 = 0,
    length: u16 = 4,
    eap_type: u8? = null,
    type_data: []const u8 = "",
}

/// Zero-copy decoded RFC 3748 Section 5.7 Expanded EAP Type payload (`eap_type == 254`).
pub struct EapExpanded {
    vendor_id: u32 = 0,
    vendor_type: u32 = 0,
    vendor_data: []const u8 = "",
}

// ---------------------------------------------------------------------------
// Internal Helpers
// ---------------------------------------------------------------------------

fn readBeU16(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn readBeU24(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 16)
        | ((data[off + 1] as u32) << 8)
        | (data[off + 2] as u32);
}

fn readBeU32(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24)
        | ((data[off + 1] as u32) << 16)
        | ((data[off + 2] as u32) << 8)
        | (data[off + 3] as u32);
}

fn classifyEapolType(raw: u8): EapolType {
    if (raw == 0) { return EapolType.eap_packet; }
    if (raw == 1) { return EapolType.start; }
    if (raw == 2) { return EapolType.logoff; }
    if (raw == 3) { return EapolType.key; }
    if (raw == 4) { return EapolType.encap_asf_alert; }
    return EapolType.unknown;
}

// ---------------------------------------------------------------------------
// Public EAPOL & EAP Parsing API
// ---------------------------------------------------------------------------

/// Strictly parses an IEEE 802.1X EAPOL frame (`version in 1..3`, `4 + body_len <= data.len`).
pub fn parseEapol(data: []const u8): EapolFrame? {
    if (data.len < EAPOL_HEADER_SIZE) {
        return null;
    }
    const version = data[0];
    if (version < 1 || version > 3) {
        return null;
    }
    const raw_type = data[1];
    const body_len = readBeU16(data, 2);
    const blen = body_len as usize;
    if (blen > data.len - EAPOL_HEADER_SIZE) {
        return null;
    }
    return EapolFrame {
        version: version,
        pkt_type: classifyEapolType(raw_type),
        raw_type: raw_type,
        body_len: body_len,
        body: data[EAPOL_HEADER_SIZE..(EAPOL_HEADER_SIZE + blen)],
    };
}

/// Returns `true` if `data` starts with a structurally valid IEEE 802.1X EAPOL frame.
pub fn isEapol(data: []const u8): bool {
    if (f := parseEapol(data)) {
        * = f;
        return true;
    }
    return false;
}

/// Strictly parses an RFC 3748 EAP packet.
/// Validates `code in 1..4`; enforces `length == 4` for Success/Failure, and
/// `5 <= length <= data.len` for Request/Response.
pub fn parseEap(data: []const u8): EapPacket? {
    if (data.len < EAP_HEADER_SIZE) {
        return null;
    }
    const raw_code = data[0];
    const identifier = data[1];
    const length = readBeU16(data, 2);
    const len_usize = length as usize;
    if (len_usize < EAP_HEADER_SIZE || len_usize > data.len) {
        return null;
    }

    if (raw_code == 3 || raw_code == 4) {
        if (length != 4) {
            return null;
        }
        let code = EapCode.success;
        if (raw_code == 4) {
            code = EapCode.failure;
        }
        return EapPacket {
            code: code,
            identifier: identifier,
            length: length,
            eap_type: null,
            type_data: "",
        };
    }

    if (raw_code == 1 || raw_code == 2) {
        if (len_usize < 5) {
            return null;
        }
        let code = EapCode.request;
        if (raw_code == 2) {
            code = EapCode.response;
        }
        return EapPacket {
            code: code,
            identifier: identifier,
            length: length,
            eap_type: data[4],
            type_data: data[5..len_usize],
        };
    }

    return null;
}

/// Returns `true` if `data` starts with a structurally valid RFC 3748 EAP packet.
pub fn isEap(data: []const u8): bool {
    if (p := parseEap(data)) {
        * = p;
        return true;
    }
    return false;
}

/// Decodes an RFC 3748 Section 5.7 Expanded Type payload (`type_data` following `eap_type == 254`):
/// `[3B BE vendor_id, 4B BE vendor_type, vendor_data...]`.
pub fn parseEapExpanded(type_data: []const u8): EapExpanded? {
    if (type_data.len < 7) {
        return null;
    }
    return EapExpanded {
        vendor_id: readBeU24(type_data, 0),
        vendor_type: readBeU32(type_data, 3),
        vendor_data: type_data[7..type_data.len],
    };
}
