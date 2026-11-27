// Origin: Pure-CP Stdlib (Zero-Alloc ASHRAE 135 BACnet/IP BVLC, NPDU, APDU & TLV Codec) | Created: 2026-11-22

pub const BACNET_UDP_PORT: u16 = 47808;
pub const BVLC_TYPE_IPV4: u8 = 0x81;
pub const BVLC_HEADER_SIZE: usize = 4;
pub const NPDU_VERSION: u8 = 0x01;

// BVLC Functions (ASHRAE 135 Annex J)
pub const BVLC_RESULT: u8 = 0x00;
pub const BVLC_FORWARDED_NPDU: u8 = 0x04;
pub const BVLC_REGISTER_FOREIGN_DEVICE: u8 = 0x05;
pub const BVLC_ORIGINAL_UNICAST: u8 = 0x0A;
pub const BVLC_ORIGINAL_BROADCAST: u8 = 0x0B;

// APDU Types (upper 4 bits of first APDU octet)
pub const APDU_CONFIRMED_REQ: u8 = 0;
pub const APDU_UNCONFIRMED_REQ: u8 = 1;
pub const APDU_SIMPLE_ACK: u8 = 2;
pub const APDU_COMPLEX_ACK: u8 = 3;
pub const APDU_SEGMENT_ACK: u8 = 4;
pub const APDU_ERROR: u8 = 5;
pub const APDU_REJECT: u8 = 6;
pub const APDU_ABORT: u8 = 7;

// Common Confirmed / Unconfirmed Services
pub const SVC_I_AM: u8 = 0;
pub const SVC_WHO_IS: u8 = 8;
pub const SVC_READ_PROPERTY: u8 = 12;
pub const SVC_READ_PROPERTY_MULTIPLE: u8 = 14;
pub const SVC_WRITE_PROPERTY: u8 = 15;

fn readU16Be(buf: []const u8, off: usize): u16 {
    const b0 = buf[off] as u16;
    const b1 = buf[off + 1] as u16;
    return (b0 << 8) | b1;
}

fn readU32Be(buf: []const u8, off: usize): u32 {
    const b0 = buf[off] as u32;
    const b1 = buf[off + 1] as u32;
    const b2 = buf[off + 2] as u32;
    const b3 = buf[off + 3] as u32;
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

fn writeU16Be(buf: []u8, off: usize, v: u16) {
    buf[off] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

/// Fast prefix check for BACnet/IP BVLC packets.
pub fn isBvlc(data: []const u8): bool {
    if (data.len < BVLC_HEADER_SIZE) {
        return false;
    }
    if (data[0] != BVLC_TYPE_IPV4) {
        return false;
    }
    const len = readU16Be(data, 2) as usize;
    return len >= BVLC_HEADER_SIZE && len <= data.len;
}

/// Parsed BACnet Virtual Link Control (BVLC) header and payload.
pub struct BvlcPacket {
    bvlc_type: u8,
    function: u8,
    length: u16,
    forwarded_ip: [4]u8,
    forwarded_port: u16,
    has_forwarded: bool,
    payload: []const u8,
}

/// Strictly parses a BACnet/IP BVLC packet (including Forwarded-NPDU 6-byte originating address).
pub fn parseBvlc(data: []const u8): BvlcPacket? {
    if (data.len < BVLC_HEADER_SIZE) {
        return null;
    }
    const bvlc_type = data[0];
    if (bvlc_type != BVLC_TYPE_IPV4) {
        return null;
    }
    const function = data[1];
    const length = readU16Be(data, 2);
    const len_usize = length as usize;
    if (len_usize < BVLC_HEADER_SIZE || len_usize > data.len) {
        return null;
    }

    if (function == BVLC_FORWARDED_NPDU) {
        if (len_usize < BVLC_HEADER_SIZE + 6) {
            return null;
        }
        const fwd_ip: [4]u8 = [data[4], data[5], data[6], data[7]];
        const fwd_port = readU16Be(data, 8);
        return BvlcPacket{
            .bvlc_type = bvlc_type,
            .function = function,
            .length = length,
            .forwarded_ip = fwd_ip,
            .forwarded_port = fwd_port,
            .has_forwarded = true,
            .payload = data[10..len_usize],
        };
    }

    const zero_ip: [4]u8 = [0, 0, 0, 0];
    return BvlcPacket{
        .bvlc_type = bvlc_type,
        .function = function,
        .length = length,
        .forwarded_ip = zero_ip,
        .forwarded_port = 0,
        .has_forwarded = false,
        .payload = data[BVLC_HEADER_SIZE..len_usize],
    };
}

/// Zero-alloc encoder for a BVLC Original-Unicast-NPDU (`0x0A`) packet.
pub fn encodeBvlcUnicast(npdu: []const u8, out: []u8): usize? {
    const total = BVLC_HEADER_SIZE + npdu.len;
    if (total > 65535 || out.len < total) {
        return null;
    }
    out[0] = BVLC_TYPE_IPV4;
    out[1] = BVLC_ORIGINAL_UNICAST;
    writeU16Be(out, 2, total as u16);
    let i: usize = 0;
    while (i < npdu.len) {
        out[BVLC_HEADER_SIZE + i] = npdu[i];
        i += 1;
    }
    return total;
}

/// Zero-alloc encoder for a BVLC Original-Broadcast-NPDU (`0x0B`) packet.
pub fn encodeBvlcBroadcast(npdu: []const u8, out: []u8): usize? {
    const total = BVLC_HEADER_SIZE + npdu.len;
    if (total > 65535 || out.len < total) {
        return null;
    }
    out[0] = BVLC_TYPE_IPV4;
    out[1] = BVLC_ORIGINAL_BROADCAST;
    writeU16Be(out, 2, total as u16);
    let i: usize = 0;
    while (i < npdu.len) {
        out[BVLC_HEADER_SIZE + i] = npdu[i];
        i += 1;
    }
    return total;
}

/// Parsed BACnet Network Protocol Data Unit (NPDU, ASHRAE 135 §6.2.2).
pub struct NpduPacket {
    version: u8,
    control: u8,
    dnet: u16?,
    dadr: []const u8,
    snet: u16?,
    sadr: []const u8,
    hop_count: u8?,
    apdu: []const u8,

    fn isNetworkMessage(): bool {
        return (_.control & 0x80) != 0;
    }

    fn expectingReply(): bool {
        return (_.control & 0x04) != 0;
    }

    fn priority(): u8 {
        return _.control & 0x03;
    }
}

/// Strictly parses a BACnet NPDU (version must equal `0x01`, bounds-checked DNET/DADR/SNET/SADR/HopCount).
pub fn parseNpdu(data: []const u8): NpduPacket? {
    if (data.len < 2) {
        return null;
    }
    const version = data[0];
    if (version != NPDU_VERSION) {
        return null;
    }
    const control = data[1];
    let pos: usize = 2;

    let dnet: u16? = null;
    let dadr_start: usize = 0;
    let dadr_end: usize = 0;
    const has_dest = (control & 0x20) != 0;
    if (has_dest) {
        if (pos + 3 > data.len) {
            return null;
        }
        dnet = readU16Be(data, pos);
        pos += 2;
        const dlen = data[pos] as usize;
        pos += 1;
        if (pos + dlen > data.len) {
            return null;
        }
        dadr_start = pos;
        dadr_end = pos + dlen;
        pos += dlen;
    }

    let snet: u16? = null;
    let sadr_start: usize = 0;
    let sadr_end: usize = 0;
    const has_src = (control & 0x08) != 0;
    if (has_src) {
        if (pos + 3 > data.len) {
            return null;
        }
        snet = readU16Be(data, pos);
        pos += 2;
        const slen = data[pos] as usize;
        pos += 1;
        if (pos + slen > data.len) {
            return null;
        }
        sadr_start = pos;
        sadr_end = pos + slen;
        pos += slen;
    }

    let hop_count: u8? = null;
    if (has_dest) {
        if (pos + 1 > data.len) {
            return null;
        }
        hop_count = data[pos];
        pos += 1;
    }

    return NpduPacket{
        .version = version,
        .control = control,
        .dnet = dnet,
        .dadr = data[dadr_start..dadr_end],
        .snet = snet,
        .sadr = data[sadr_start..sadr_end],
        .hop_count = hop_count,
        .apdu = data[pos..],
    };
}

/// Parsed BACnet Application Protocol Data Unit (APDU) header.
pub struct ApduHeader {
    pdu_type: u8,
    invoke_id: u8?,
    service_choice: u8,
    payload: []const u8,
}

/// Strictly parses a BACnet APDU header across all 8 standard APDU types (`0..7`).
pub fn parseApdu(data: []const u8): ApduHeader? {
    if (data.len < 2) {
        return null;
    }
    const b0 = data[0];
    const pdu_type = (b0 >> 4) & 0x0F;

    if (pdu_type == APDU_CONFIRMED_REQ) {
        const segmented = (b0 & 0x08) != 0;
        const hdr_len: usize = if (segmented) 6 else 4;
        if (data.len < hdr_len) {
            return null;
        }
        return ApduHeader{
            .pdu_type = pdu_type,
            .invoke_id = data[2],
            .service_choice = data[hdr_len - 1],
            .payload = data[hdr_len..],
        };
    }

    if (pdu_type == APDU_UNCONFIRMED_REQ) {
        return ApduHeader{
            .pdu_type = pdu_type,
            .invoke_id = null,
            .service_choice = data[1],
            .payload = data[2..],
        };
    }

    if (pdu_type == APDU_SIMPLE_ACK) {
        if (data.len < 3) {
            return null;
        }
        return ApduHeader{
            .pdu_type = pdu_type,
            .invoke_id = data[1],
            .service_choice = data[2],
            .payload = data[3..],
        };
    }

    if (pdu_type == APDU_COMPLEX_ACK) {
        const segmented = (b0 & 0x08) != 0;
        const hdr_len: usize = if (segmented) 5 else 3;
        if (data.len < hdr_len) {
            return null;
        }
        return ApduHeader{
            .pdu_type = pdu_type,
            .invoke_id = data[1],
            .service_choice = data[hdr_len - 1],
            .payload = data[hdr_len..],
        };
    }

    if (pdu_type == APDU_SEGMENT_ACK) {
        if (data.len < 4) {
            return null;
        }
        return ApduHeader{
            .pdu_type = pdu_type,
            .invoke_id = data[1],
            .service_choice = 0,
            .payload = data[2..],
        };
    }

    if (pdu_type == APDU_ERROR || pdu_type == APDU_REJECT || pdu_type == APDU_ABORT) {
        if (data.len < 3) {
            return null;
        }
        return ApduHeader{
            .pdu_type = pdu_type,
            .invoke_id = data[1],
            .service_choice = data[2],
            .payload = data[3..],
        };
    }

    return null;
}

/// BACnet 32-bit Object Identifier (10-bit object type + 22-bit instance number).
pub struct BacnetObjectId {
    object_type: u16,
    instance: u32,
}

/// Decodes a 32-bit BACnet Object Identifier.
pub fn decodeObjectId(raw: u32): BacnetObjectId {
    return BacnetObjectId{
        .object_type = ((raw >> 22) & 0x3FF) as u16,
        .instance = raw & 0x3FFFFF,
    };
}

/// Encodes a 10-bit object type (`0..1023`) and 22-bit instance (`0..4194303`) into a 32-bit Object ID.
pub fn encodeObjectId(obj_type: u16, instance: u32): u32? {
    if (obj_type > 0x3FF || instance > 0x3FFFFF) {
        return null;
    }
    return ((obj_type as u32) << 22) | instance;
}

/// Single decoded BACnet Application or Context-Specific TLV tag.
pub struct BacnetTag {
    tag_number: u8,
    is_context: bool,
    is_opening: bool,
    is_closing: bool,
    value: []const u8,
}

/// Zero-copy iterator over BACnet Application/Context TLV tags (ASHRAE 135 §20.2.1).
/// Supports standard lengths `0..4`, extended `tag_number == 15`, extended lengths (`lvt == 5` + 1B/2B/4B),
/// and Context Opening (`lvt == 6`) / Closing (`lvt == 7`) tags.
pub struct BacnetTagIter {
    data: []const u8,
    offset: usize,
    failed: bool,

    static fn init(buf: []const u8): BacnetTagIter {
        return BacnetTagIter{
            .data = buf,
            .offset = 0,
            .failed = false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): BacnetTag? {
        if (_.failed || _.offset >= _.data.len) {
            return null;
        }
        const b0 = _.data[_.offset];
        _.offset += 1;

        let tag_number = (b0 >> 4) & 0x0F;
        const is_context = (b0 & 0x08) != 0;
        const lvt = b0 & 0x07;

        // Extended tag number when upper 4 bits == 15
        if (tag_number == 15) {
            if (_.offset >= _.data.len) {
                _.failed = true;
                return null;
            }
            tag_number = _.data[_.offset];
            _.offset += 1;
        }

        // Context Opening (lvt == 6) / Closing (lvt == 7) tags carry no value bytes
        if (is_context && lvt == 6) {
            return BacnetTag{
                .tag_number = tag_number,
                .is_context = true,
                .is_opening = true,
                .is_closing = false,
                .value = _.data[_.offset.._.offset],
            };
        }
        if (is_context && lvt == 7) {
            return BacnetTag{
                .tag_number = tag_number,
                .is_context = true,
                .is_opening = false,
                .is_closing = true,
                .value = _.data[_.offset.._.offset],
            };
        }

        // Application tag lvt 6/7 are reserved/invalid
        if (!is_context && (lvt == 6 || lvt == 7)) {
            _.failed = true;
            return null;
        }

        // Application Boolean (tag 1): boolean value is encoded directly in lvt (0/1), 0 trailing bytes
        if (!is_context && tag_number == 1) {
            return BacnetTag{
                .tag_number = tag_number,
                .is_context = false,
                .is_opening = false,
                .is_closing = false,
                .value = _.data[_.offset.._.offset],
            };
        }

        let val_len: usize = 0;
        if (lvt <= 4) {
            val_len = lvt as usize;
        } else {
            // Extended length (lvt == 5)
            if (_.offset >= _.data.len) {
                _.failed = true;
                return null;
            }
            const ext = _.data[_.offset];
            _.offset += 1;
            if (ext < 254) {
                val_len = ext as usize;
            } else if (ext == 254) {
                if (_.offset + 2 > _.data.len) {
                    _.failed = true;
                    return null;
                }
                val_len = readU16Be(_.data, _.offset) as usize;
                _.offset += 2;
            } else {
                // ext == 255 -> 4-byte BE length
                if (_.offset + 4 > _.data.len) {
                    _.failed = true;
                    return null;
                }
                val_len = readU32Be(_.data, _.offset) as usize;
                _.offset += 4;
            }
        }

        if (val_len > _.data.len - _.offset) {
            _.failed = true;
            return null;
        }

        const start = _.offset;
        _.offset += val_len;
        return BacnetTag{
            .tag_number = tag_number,
            .is_context = is_context,
            .is_opening = false,
            .is_closing = false,
            .value = _.data[start.._.offset],
        };
    }
}
