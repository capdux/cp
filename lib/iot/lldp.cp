// Origin: Pure-CP Stdlib (Zero-Alloc IEEE 802.1AB LLDP TLV & Neighbor Codec) | Created: 2026-11-22
// Pure CP zero-heap-allocation IEEE 802.1AB-2016 Link Layer Discovery Protocol (LLDP) codec:
// 2-byte big-endian 7-bit type / 9-bit length TLV iterator, strict mandatory-order
// LLDPDU neighbor parser, Org-Specific (type 127) OUI/subtype decoder, and zero-alloc LldpWriter.

/// IEEE 802.1AB LLDP EtherType (`0x88CC`).
pub const LLDP_ETHERTYPE: u16 = 0x88CC;

// Standard LLDP TLV Types (IEEE 802.1AB Table 8-1)
pub const TLV_END: u8 = 0;
pub const TLV_CHASSIS_ID: u8 = 1;
pub const TLV_PORT_ID: u8 = 2;
pub const TLV_TTL: u8 = 3;
pub const TLV_PORT_DESC: u8 = 4;
pub const TLV_SYSTEM_NAME: u8 = 5;
pub const TLV_SYSTEM_DESC: u8 = 6;
pub const TLV_SYSTEM_CAPS: u8 = 7;
pub const TLV_MGMT_ADDR: u8 = 8;
pub const TLV_ORG_SPECIFIC: u8 = 127;

// System Capabilities Bitmask (IEEE 802.1AB Table 8-4)
pub const CAP_REPEATER: u16 = 0x0002;
pub const CAP_BRIDGE: u16 = 0x0004;
pub const CAP_WLAN_AP: u16 = 0x0008;
pub const CAP_ROUTER: u16 = 0x0010;
pub const CAP_TELEPHONE: u16 = 0x0020;
pub const CAP_STATION_ONLY: u16 = 0x0080;

/// Zero-copy decoded IEEE 802.1AB LLDP TLV element.
pub struct LldpTlv {
    tlv_type: u8 = 0,
    length: u16 = 0,
    value: []const u8 = "",
}

/// Zero-copy decoded Organizationally Specific TLV (type `127`).
pub struct LldpOrgTlv {
    oui: [3]u8 = [0; 3],
    subtype: u8 = 0,
    info: []const u8 = "",
}

/// Zero-copy decoded LLDPDU neighbor summary.
pub struct LldpNeighbor {
    chassis_subtype: u8 = 0,
    chassis_id: []const u8 = "",
    port_subtype: u8 = 0,
    port_id: []const u8 = "",
    ttl_sec: u16 = 0,
    port_desc: []const u8 = "",
    system_name: []const u8 = "",
    system_desc: []const u8 = "",
    caps_supported: u16 = 0,
    caps_enabled: u16 = 0,
    has_end_tlv: bool = false,
    org_tlv_count: u32 = 0,

    fn isRouter(): bool {
        return (_.caps_enabled & CAP_ROUTER) != 0;
    }

    fn isBridge(): bool {
        return (_.caps_enabled & CAP_BRIDGE) != 0;
    }

    fn isWlanAp(): bool {
        return (_.caps_enabled & CAP_WLAN_AP) != 0;
    }
}

fn readBeU16(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

// ---------------------------------------------------------------------------
// Zero-Alloc LLDP TLV Iterator
// ---------------------------------------------------------------------------

/// Zero-allocation iterator over IEEE 802.1AB LLDP TLVs.
/// Terminates cleanly on `TLV_END (0)` with `length == 0`, and sets `hasError()` on
/// non-zero `TLV_END` length or truncated/out-of-bounds TLV payloads.
pub struct LldpTlvIter {
    data: []const u8 = "",
    pos: usize = 0,
    seen_end: bool = false,
    failed: bool = false,

    pub fn init(data: []const u8): LldpTlvIter {
        return LldpTlvIter {
            data: data,
            pos: 0,
            seen_end: false,
            failed: false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn hasEndTlv(): bool {
        return _.seen_end;
    }

    fn next(*_): LldpTlv? {
        if (_.failed || _.seen_end || _.pos >= _.data.len) {
            return null;
        }
        const rem = _.data.len - _.pos;
        if (rem < 2) {
            _.failed = true;
            return null;
        }
        const b0 = _.data[_.pos];
        const b1 = _.data[_.pos + 1];
        const tlv_type: u8 = b0 >> 1;
        const length: u16 = (((b0 & 1) as u16) << 8) | (b1 as u16);
        const len_usize = length as usize;

        if (tlv_type == TLV_END) {
            if (length != 0) {
                _.failed = true;
                return null;
            }
            _.pos += 2;
            _.seen_end = true;
            return null;
        }

        if (rem - 2 < len_usize) {
            _.failed = true;
            return null;
        }
        const val = _.data[(_.pos + 2)..(_.pos + 2 + len_usize)];
        _.pos += 2 + len_usize;

        return LldpTlv {
            tlv_type: tlv_type,
            length: length,
            value: val,
        };
    }
}

// ---------------------------------------------------------------------------
// Public LLDPDU & Org-Specific TLV Parsers
// ---------------------------------------------------------------------------

/// Decodes an Organizationally Specific TLV (`tlv_type == 127`) into OUI, subtype, and info slice.
pub fn parseLldpOrgTlv(tlv: LldpTlv): LldpOrgTlv? {
    if (tlv.tlv_type != TLV_ORG_SPECIFIC || tlv.value.len < 4) {
        return null;
    }
    return LldpOrgTlv {
        oui: [tlv.value[0], tlv.value[1], tlv.value[2]],
        subtype: tlv.value[3],
        info: tlv.value[4..tlv.value.len],
    };
}

/// Strictly parses an IEEE 802.1AB LLDPDU payload (excluding the 14-byte Ethernet header).
/// Enforces mandatory leading order: `Chassis ID (1)` -> `Port ID (2)` -> `TTL (3)`.
pub fn parseLldpdu(data: []const u8): LldpNeighbor? {
    let it = LldpTlvIter.init(data);

    // 1. Mandatory Chassis ID TLV (type 1)
    let chassis_subtype: u8 = 0;
    let chassis_id: []const u8 = "";
    if (t1 := it.next()) {
        if (t1.tlv_type != TLV_CHASSIS_ID || t1.value.len < 2 || t1.value.len > 256) {
            return null;
        }
        chassis_subtype = t1.value[0];
        chassis_id = t1.value[1..t1.value.len];
    } else {
        return null;
    }

    // 2. Mandatory Port ID TLV (type 2)
    let port_subtype: u8 = 0;
    let port_id: []const u8 = "";
    if (t2 := it.next()) {
        if (t2.tlv_type != TLV_PORT_ID || t2.value.len < 2 || t2.value.len > 256) {
            return null;
        }
        port_subtype = t2.value[0];
        port_id = t2.value[1..t2.value.len];
    } else {
        return null;
    }

    // 3. Mandatory Time To Live TLV (type 3, strictly 2 bytes)
    let ttl_sec: u16 = 0;
    if (t3 := it.next()) {
        if (t3.tlv_type != TLV_TTL || t3.value.len != 2) {
            return null;
        }
        ttl_sec = readBeU16(t3.value, 0);
    } else {
        return null;
    }

    // 4. Optional TLVs until End-of-LLDPDU (0) or EOF
    let port_desc: []const u8 = "";
    let system_name: []const u8 = "";
    let system_desc: []const u8 = "";
    let caps_supported: u16 = 0;
    let caps_enabled: u16 = 0;
    let org_tlv_count: u32 = 0;

    while (tlv := it.next()) {
        if (tlv.tlv_type == TLV_PORT_DESC) {
            port_desc = tlv.value;
        } else if (tlv.tlv_type == TLV_SYSTEM_NAME) {
            system_name = tlv.value;
        } else if (tlv.tlv_type == TLV_SYSTEM_DESC) {
            system_desc = tlv.value;
        } else if (tlv.tlv_type == TLV_SYSTEM_CAPS) {
            if (tlv.value.len != 4) {
                return null;
            }
            caps_supported = readBeU16(tlv.value, 0);
            caps_enabled = readBeU16(tlv.value, 2);
        } else if (tlv.tlv_type == TLV_ORG_SPECIFIC) {
            if (tlv.value.len < 4) {
                return null;
            }
            org_tlv_count += 1;
        }
    }

    if (it.hasError()) {
        return null;
    }

    return LldpNeighbor {
        chassis_subtype: chassis_subtype,
        chassis_id: chassis_id,
        port_subtype: port_subtype,
        port_id: port_id,
        ttl_sec: ttl_sec,
        port_desc: port_desc,
        system_name: system_name,
        system_desc: system_desc,
        caps_supported: caps_supported,
        caps_enabled: caps_enabled,
        has_end_tlv: it.hasEndTlv(),
        org_tlv_count: org_tlv_count,
    };
}

// ---------------------------------------------------------------------------
// Zero-Alloc LLDPDU Writer
// ---------------------------------------------------------------------------

/// Zero-allocation IEEE 802.1AB LLDPDU encoder writing directly into a caller-supplied buffer.
pub struct LldpWriter {
    buf: []u8,
    pos: usize = 0,
    failed: bool = false,

    pub fn init(buf: []u8): LldpWriter {
        return LldpWriter {
            buf: buf,
            pos: 0,
            failed: false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn writeHeader(*_, tlv_type: u8, len: usize): bool {
        if (_.failed || tlv_type > 127 || len > 511 || _.buf.len - _.pos < 2 + len) {
            _.failed = true;
            return false;
        }
        const len_u16 = len as u16;
        _.buf[_.pos] = (tlv_type << 1) | (((len_u16 >> 8) & 1) as u8);
        _.buf[_.pos + 1] = (len_u16 & 0xFF) as u8;
        _.pos += 2;
        return true;
    }

    fn writeBytes(*_, s: []const u8) {
        let i: usize = 0;
        while (i < s.len) {
            _.buf[_.pos + i] = s[i];
            i += 1;
        }
        _.pos += s.len;
    }

    /// Writes a Chassis ID TLV (`type = 1`).
    fn writeChassisId(*_, subtype: u8, id: []const u8): bool {
        if (id.len == 0 || id.len > 255) {
            _.failed = true;
            return false;
        }
        if (!_.writeHeader(TLV_CHASSIS_ID, 1 + id.len)) {
            return false;
        }
        _.buf[_.pos] = subtype;
        _.pos += 1;
        _.writeBytes(id);
        return true;
    }

    /// Writes a Port ID TLV (`type = 2`).
    fn writePortId(*_, subtype: u8, id: []const u8): bool {
        if (id.len == 0 || id.len > 255) {
            _.failed = true;
            return false;
        }
        if (!_.writeHeader(TLV_PORT_ID, 1 + id.len)) {
            return false;
        }
        _.buf[_.pos] = subtype;
        _.pos += 1;
        _.writeBytes(id);
        return true;
    }

    /// Writes a Time To Live TLV (`type = 3`, 2-byte big-endian seconds).
    fn writeTtl(*_, ttl_sec: u16): bool {
        if (!_.writeHeader(TLV_TTL, 2)) {
            return false;
        }
        _.buf[_.pos] = ((ttl_sec >> 8) & 0xFF) as u8;
        _.buf[_.pos + 1] = (ttl_sec & 0xFF) as u8;
        _.pos += 2;
        return true;
    }

    /// Writes a string/octet TLV (such as Port Description, System Name, or System Description).
    /// Supports full 9-bit LLDP TLV lengths up to 511 bytes.
    fn writeStringTlv(*_, tlv_type: u8, text: []const u8): bool {
        if (tlv_type == TLV_END || !_.writeHeader(tlv_type, text.len)) {
            _.failed = true;
            return false;
        }
        _.writeBytes(text);
        return true;
    }

    /// Writes a System Capabilities TLV (`type = 7`, 2B supported + 2B enabled).
    fn writeSystemCaps(*_, supported: u16, enabled: u16): bool {
        if (!_.writeHeader(TLV_SYSTEM_CAPS, 4)) {
            return false;
        }
        _.buf[_.pos] = ((supported >> 8) & 0xFF) as u8;
        _.buf[_.pos + 1] = (supported & 0xFF) as u8;
        _.buf[_.pos + 2] = ((enabled >> 8) & 0xFF) as u8;
        _.buf[_.pos + 3] = (enabled & 0xFF) as u8;
        _.pos += 4;
        return true;
    }

    /// Writes an Organizationally Specific TLV (`type = 127`, 3B OUI + 1B subtype + info).
    fn writeOrgTlv(*_, oui: [3]u8, subtype: u8, info: []const u8): bool {
        if (4 + info.len > 511) {
            _.failed = true;
            return false;
        }
        if (!_.writeHeader(TLV_ORG_SPECIFIC, 4 + info.len)) {
            return false;
        }
        _.buf[_.pos] = oui[0];
        _.buf[_.pos + 1] = oui[1];
        _.buf[_.pos + 2] = oui[2];
        _.buf[_.pos + 3] = subtype;
        _.pos += 4;
        _.writeBytes(info);
        return true;
    }

    /// Appends the mandatory 2-byte `0x00 0x00` End-of-LLDPDU TLV and returns the encoded slice.
    fn finish(*_): ([]const u8)? {
        if (_.failed || _.buf.len - _.pos < 2) {
            _.failed = true;
            return null;
        }
        _.buf[_.pos] = 0;
        _.buf[_.pos + 1] = 0;
        _.pos += 2;
        return _.buf[0.._.pos];
    }
}
