// Origin: Pure-CP Stdlib (Zero-Alloc RFC 5880 BFD Wire Protocol Parser) | Created: 2026-11-22
// Pure CP zero-heap-allocation RFC 5880 Bidirectional Forwarding Detection (BFD) parser:
// 24-byte control header, diagnostic/state/flag decoding, Simple Password / Keyed MD5 /
// Meticulous Keyed MD5 / Keyed SHA-1 / Meticulous Keyed SHA-1 authentication sections,
// and RFC 5880 Detection Time calculation.

/// Standard BFD single-hop control UDP port (`3784`, RFC 5881).
pub const BFD_UDP_PORT_SINGLE_HOP: u16 = 3784;

/// Standard BFD echo UDP port (`3785`, RFC 5881).
pub const BFD_UDP_PORT_ECHO: u16 = 3785;

/// Standard BFD multi-hop control UDP port (`4784`, RFC 5883).
pub const BFD_UDP_PORT_MULTI_HOP: u16 = 4784;

/// BFD protocol version (`1`, RFC 5880).
pub const BFD_VERSION: u8 = 1;

/// Minimum BFD control packet length in bytes (`24`, RFC 5880 Section 4.1).
pub const BFD_MIN_LEN: usize = 24;

/// BFD Diagnostic codes (RFC 5880 Section 4.1).
pub enum BfdDiag: u8 {
    none = 0,
    control_detection_time_expired = 1,
    echo_function_failed = 2,
    neighbor_signaled_session_down = 3,
    forwarding_plane_reset = 4,
    path_down = 5,
    concatenated_path_down = 6,
    administratively_down = 7,
    reverse_concatenated_path_down = 8,
    unknown = 255,
}

/// BFD Session States (RFC 5880 Section 4.1).
pub enum BfdState: u8 {
    admin_down = 0,
    down = 1,
    init = 2,
    up = 3,
}

/// BFD Authentication Types (RFC 5880 Section 4.1).
pub enum BfdAuthType: u8 {
    simple_password = 1,
    keyed_md5 = 2,
    meticulous_keyed_md5 = 3,
    keyed_sha1 = 4,
    meticulous_keyed_sha1 = 5,
}

/// Zero-copy decoded RFC 5880 BFD Authentication Section.
pub struct BfdAuth {
    auth_type: BfdAuthType = BfdAuthType.simple_password,
    auth_len: u8 = 0,
    auth_key_id: u8 = 0,
    sequence_number: u32 = 0,
    data: []const u8 = "",
}

/// Zero-copy decoded RFC 5880 BFD Control Packet.
pub struct BfdPacket {
    version: u8 = BFD_VERSION,
    diag: BfdDiag = BfdDiag.none,
    raw_diag: u8 = 0,
    state: BfdState = BfdState.down,
    poll: bool = false,
    final_flag: bool = false,
    control_plane_independent: bool = false,
    auth_present: bool = false,
    demand: bool = false,
    multipoint: bool = false,
    detect_mult: u8 = 1,
    length: u8 = 24,
    my_discriminator: u32 = 0,
    your_discriminator: u32 = 0,
    desired_min_tx_us: u32 = 0,
    required_min_rx_us: u32 = 0,
    required_min_echo_rx_us: u32 = 0,
    auth: BfdAuth? = null,
}

// ---------------------------------------------------------------------------
// Internal Helpers
// ---------------------------------------------------------------------------

fn readBeU32(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24)
        | ((data[off + 1] as u32) << 16)
        | ((data[off + 2] as u32) << 8)
        | (data[off + 3] as u32);
}

fn classifyDiag(raw: u8): BfdDiag {
    if (raw == 0) { return BfdDiag.none; }
    if (raw == 1) { return BfdDiag.control_detection_time_expired; }
    if (raw == 2) { return BfdDiag.echo_function_failed; }
    if (raw == 3) { return BfdDiag.neighbor_signaled_session_down; }
    if (raw == 4) { return BfdDiag.forwarding_plane_reset; }
    if (raw == 5) { return BfdDiag.path_down; }
    if (raw == 6) { return BfdDiag.concatenated_path_down; }
    if (raw == 7) { return BfdDiag.administratively_down; }
    if (raw == 8) { return BfdDiag.reverse_concatenated_path_down; }
    return BfdDiag.unknown;
}

fn classifyState(raw: u8): BfdState {
    if (raw == 0) { return BfdState.admin_down; }
    if (raw == 1) { return BfdState.down; }
    if (raw == 2) { return BfdState.init; }
    return BfdState.up;
}

fn parseAuthSection(sec: []const u8): BfdAuth? {
    if (sec.len < 3) {
        return null;
    }
    const raw_type = sec[0];
    const auth_len = sec[1];
    if ((auth_len as usize) != sec.len) {
        return null;
    }
    const key_id = sec[2];

    if (raw_type == 1) {
        // Simple Password (RFC 5880 Section 4.2): min 3 bytes (Type + Len + KeyID + 0..16B password)
        if (auth_len < 3) {
            return null;
        }
        return BfdAuth {
            auth_type: BfdAuthType.simple_password,
            auth_len: auth_len,
            auth_key_id: key_id,
            sequence_number: 0,
            data: sec[3..sec.len],
        };
    }

    if (raw_type == 2 || raw_type == 3) {
        // Keyed MD5 / Meticulous Keyed MD5 (RFC 5880 Section 4.3): min 24 bytes (8B hdr + 16B digest)
        if (auth_len < 24) {
            return null;
        }
        let atype = BfdAuthType.keyed_md5;
        if (raw_type == 3) {
            atype = BfdAuthType.meticulous_keyed_md5;
        }
        return BfdAuth {
            auth_type: atype,
            auth_len: auth_len,
            auth_key_id: key_id,
            sequence_number: readBeU32(sec, 4),
            data: sec[8..sec.len],
        };
    }

    if (raw_type == 4 || raw_type == 5) {
        // Keyed SHA1 / Meticulous Keyed SHA1 (RFC 5880 Section 4.4): min 28 bytes (8B hdr + 20B digest)
        if (auth_len < 28) {
            return null;
        }
        let atype = BfdAuthType.keyed_sha1;
        if (raw_type == 5) {
            atype = BfdAuthType.meticulous_keyed_sha1;
        }
        return BfdAuth {
            auth_type: atype,
            auth_len: auth_len,
            auth_key_id: key_id,
            sequence_number: readBeU32(sec, 4),
            data: sec[8..sec.len],
        };
    }

    return null;
}

// ---------------------------------------------------------------------------
// Public BFD Parsing & Helper API
// ---------------------------------------------------------------------------

/// Strictly parses an RFC 5880 BFD Control Packet from `data`.
/// Validates `version == 1`, `detect_mult != 0`, `multipoint == false`,
/// `24 <= length <= data.len`, `my_discriminator != 0`, and optional Authentication Section.
pub fn parseBfd(data: []const u8): BfdPacket? {
    if (data.len < BFD_MIN_LEN) {
        return null;
    }
    const version: u8 = (data[0] >> 5) & 0x07;
    if (version != BFD_VERSION) {
        return null;
    }
    const raw_diag: u8 = data[0] & 0x1F;
    const b1 = data[1];
    const state = classifyState((b1 >> 6) & 0x03);
    const poll = (b1 & 0x20) != 0;
    const final_flag = (b1 & 0x10) != 0;
    const cpi = (b1 & 0x08) != 0;
    const auth_present = (b1 & 0x04) != 0;
    const demand = (b1 & 0x02) != 0;
    const multipoint = (b1 & 0x01) != 0;

    if (multipoint) {
        return null;
    }
    const detect_mult = data[2];
    if (detect_mult == 0) {
        return null;
    }
    const length = data[3];
    const len_usize = length as usize;
    if (len_usize < BFD_MIN_LEN || len_usize > data.len) {
        return null;
    }

    const my_disc = readBeU32(data, 4);
    if (my_disc == 0) {
        return null;
    }
    const your_disc = readBeU32(data, 8);
    const desired_tx = readBeU32(data, 12);
    const required_rx = readBeU32(data, 16);
    const required_echo_rx = readBeU32(data, 20);

    let auth: BfdAuth? = null;
    if (auth_present) {
        if (len_usize < BFD_MIN_LEN + 3) {
            return null;
        }
        if (parsed_auth := parseAuthSection(data[BFD_MIN_LEN..len_usize])) {
            auth = parsed_auth;
        } else {
            return null;
        }
    } else if (len_usize != BFD_MIN_LEN) {
        return null;
    }

    return BfdPacket {
        version: version,
        diag: classifyDiag(raw_diag),
        raw_diag: raw_diag,
        state: state,
        poll: poll,
        final_flag: final_flag,
        control_plane_independent: cpi,
        auth_present: auth_present,
        demand: demand,
        multipoint: multipoint,
        detect_mult: detect_mult,
        length: length,
        my_discriminator: my_disc,
        your_discriminator: your_disc,
        desired_min_tx_us: desired_tx,
        required_min_rx_us: required_rx,
        required_min_echo_rx_us: required_echo_rx,
        auth: auth,
    };
}

/// Returns `true` if `data` is a structurally valid RFC 5880 BFD Control Packet.
pub fn isBfd(data: []const u8): bool {
    if (pkt := parseBfd(data)) {
        * = pkt;
        return true;
    }
    return false;
}

/// Computes the negotiated BFD Detection Time in microseconds per RFC 5880 Section 6.8.4:
/// `detect_mult * max(remote_min_tx, local_min_rx)`.
pub fn detectionTimeUs(remote_min_tx: u32, local_min_rx: u32, detect_mult: u8): u64 {
    let interval: u64 = remote_min_tx as u64;
    if ((local_min_rx as u64) > interval) {
        interval = local_min_rx as u64;
    }
    return interval * (detect_mult as u64);
}
