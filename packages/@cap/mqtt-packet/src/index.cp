// @cap/mqtt-packet — Zero-heap OASIS MQTT v3.1.1 binary control packet codec
// MIT License (clean-room CP implementation)

pub const PKT_CONNECT: u8 = 1;
pub const PKT_CONNACK: u8 = 2;
pub const PKT_PUBLISH: u8 = 3;
pub const PKT_PUBACK: u8 = 4;
pub const PKT_SUBSCRIBE: u8 = 8;
pub const PKT_PINGREQ: u8 = 12;
pub const PKT_PINGRESP: u8 = 13;
pub const PKT_DISCONNECT: u8 = 14;

pub const MQTT_OK: i32 = 0;
pub const MQTT_INCOMPLETE: i32 = 1;
pub const MQTT_INVALID: i32 = -1;

pub const MAX_REMAINING_LEN: u32 = 268435455;

pub struct MqttHeader {
    pub status: i32,
    pub pkt_type: u8,
    pub flags: u8,
    pub remaining_len: u32,
    pub header_len: usize,
    pub total_len: usize,
}

pub struct MqttPublish {
    pub status: i32,
    pub qos: u8,
    pub retain: bool,
    pub dup: bool,
    pub packet_id: u16,
    pub topic_start: usize,
    pub topic_len: usize,
    pub payload_start: usize,
    pub payload_len: usize,
    pub consumed: usize,
}

fn makeHeaderStatus(status: i32) -> MqttHeader {
    return MqttHeader {
        status: status,
        pkt_type: 0,
        flags: 0,
        remaining_len: 0,
        header_len: 0,
        total_len: 0,
    };
}

fn makePublishStatus(status: i32) -> MqttPublish {
    return MqttPublish {
        status: status,
        qos: 0,
        retain: false,
        dup: false,
        packet_id: 0,
        topic_start: 0,
        topic_len: 0,
        payload_start: 0,
        payload_len: 0,
        consumed: 0,
    };
}

pub fn encodeRemainingLen(len: u32, dst: []u8) -> i64 {
    if (len > MAX_REMAINING_LEN) {
        return -1;
    }
    let x: u32 = len;
    let pos: usize = 0;
    while (true) {
        if (pos >= dst.len) {
            return -1;
        }
        let b: u8 = (x % 128) as u8;
        x = x / 128;
        if (x > 0) {
            b = b | 0x80;
        }
        dst[pos] = b;
        pos += 1;
        if (x == 0) {
            break;
        }
    }
    return pos as i64;
}

pub fn decodeHeader(buf: []const u8) -> MqttHeader {
    if (buf.len < 2) {
        return makeHeaderStatus(MQTT_INCOMPLETE);
    }
    const first: u8 = buf[0];
    const pkt_type: u8 = (first >> 4) & 0x0F;
    const flags: u8 = first & 0x0F;
    if (pkt_type < 1 || pkt_type > 14) {
        return makeHeaderStatus(MQTT_INVALID);
    }

    let i: usize = 1;
    let mult: u32 = 1;
    let rem_len: u32 = 0;
    let count: usize = 0;

    while (true) {
        if (i >= buf.len) {
            return makeHeaderStatus(MQTT_INCOMPLETE);
        }
        const b: u8 = buf[i];
        i += 1;
        count += 1;
        rem_len = rem_len + (((b & 0x7F) as u32) * mult);
        if ((b & 0x80) == 0) {
            break;
        }
        if (count == 4) {
            return makeHeaderStatus(MQTT_INVALID);
        }
        mult = mult * 128;
    }

    const hdr_len: usize = i;
    const total_len: usize = hdr_len + (rem_len as usize);
    return MqttHeader {
        status: MQTT_OK,
        pkt_type: pkt_type,
        flags: flags,
        remaining_len: rem_len,
        header_len: hdr_len,
        total_len: total_len,
    };
}

pub fn encodePublish(
    topic: []const u8,
    payload: []const u8,
    qos: u8,
    retain: bool,
    packet_id: u16,
    dst: []u8,
) -> i64 {
    if (qos > 2 || topic.len == 0 || topic.len > 65535) {
        return -1;
    }
    let rem: usize = topic.len + payload.len + 2;
    if (qos > 0) {
        rem += 2;
    }
    if (rem > (MAX_REMAINING_LEN as usize)) {
        return -1;
    }
    if (dst.len < 2) {
        return -1;
    }

    let flags: u8 = (qos & 3) << 1;
    if (retain) {
        flags = flags | 1;
    }
    dst[0] = (PKT_PUBLISH << 4) | flags;

    const rl_written: i64 = encodeRemainingLen(rem as u32, dst[1..dst.len]);
    if (rl_written < 0) {
        return -1;
    }
    let pos: usize = (rl_written as usize) + 1;
    if (rem > dst.len - pos) {
        return -1;
    }

    const tlen: u16 = topic.len as u16;
    dst[pos] = ((tlen >> 8) & 0xFF) as u8;
    dst[pos + 1] = (tlen & 0xFF) as u8;
    pos += 2;

    let i: usize = 0;
    while (i < topic.len) {
        dst[pos] = topic[i];
        pos += 1;
        i += 1;
    }

    if (qos > 0) {
        dst[pos] = ((packet_id >> 8) & 0xFF) as u8;
        dst[pos + 1] = (packet_id & 0xFF) as u8;
        pos += 2;
    }

    i = 0;
    while (i < payload.len) {
        dst[pos] = payload[i];
        pos += 1;
        i += 1;
    }

    return pos as i64;
}

pub fn decodePublish(buf: []const u8) -> MqttPublish {
    const h: MqttHeader = decodeHeader(buf);
    if (h.status != MQTT_OK) {
        return makePublishStatus(h.status);
    }
    if (h.pkt_type != PKT_PUBLISH) {
        return makePublishStatus(MQTT_INVALID);
    }

    const dup: bool = (h.flags & 0x08) != 0;
    const qos: u8 = (h.flags >> 1) & 0x03;
    const retain: bool = (h.flags & 0x01) != 0;
    if (qos == 3) {
        return makePublishStatus(MQTT_INVALID);
    }

    if (h.total_len > buf.len) {
        return makePublishStatus(MQTT_INCOMPLETE);
    }

    let pos: usize = h.header_len;
    if (pos + 2 > h.total_len) {
        return makePublishStatus(MQTT_INVALID);
    }
    const topic_len: usize = (((buf[pos] as usize) << 8) | (buf[pos + 1] as usize));
    pos += 2;
    if (topic_len == 0 || pos + topic_len > h.total_len) {
        return makePublishStatus(MQTT_INVALID);
    }
    const topic_start: usize = pos;
    pos += topic_len;

    let pid: u16 = 0;
    if (qos > 0) {
        if (pos + 2 > h.total_len) {
            return makePublishStatus(MQTT_INVALID);
        }
        pid = (((buf[pos] as u16) << 8) | (buf[pos + 1] as u16));
        pos += 2;
    }

    return MqttPublish {
        status: MQTT_OK,
        qos: qos,
        retain: retain,
        dup: dup,
        packet_id: pid,
        topic_start: topic_start,
        topic_len: topic_len,
        payload_start: pos,
        payload_len: h.total_len - pos,
        consumed: h.total_len,
    };
}

pub fn encodeSimple(pkt_type: u8, dst: []u8) -> i64 {
    if (pkt_type != PKT_PINGREQ && pkt_type != PKT_PINGRESP && pkt_type != PKT_DISCONNECT) {
        return -1;
    }
    if (dst.len < 2) {
        return -1;
    }
    dst[0] = pkt_type << 4;
    dst[1] = 0;
    return 2;
}

pub fn encodePuback(packet_id: u16, dst: []u8) -> i64 {
    if (dst.len < 4) {
        return -1;
    }
    dst[0] = PKT_PUBACK << 4;
    dst[1] = 2;
    dst[2] = ((packet_id >> 8) & 0xFF) as u8;
    dst[3] = (packet_id & 0xFF) as u8;
    return 4;
}
