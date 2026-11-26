// Type definitions for CP

export class MqttHeader {
    status: i32,
    pkt_type: u8,
    flags: u8,
    remaining_len: u32,
    header_len: usize,
    total_len: usize,
}

export class MqttPublish {
    status: i32,
    qos: u8,
    retain: bool,
    dup: bool,
    packet_id: u16,
    topic_start: usize,
    topic_len: usize,
    payload_start: usize,
    payload_len: usize,
    consumed: usize,
}

export fn makeHeaderStatus(status: i32): MqttHeader;
export fn makePublishStatus(status: i32): MqttPublish;
export fn encodeRemainingLen(len: u32, dst: []u8): i64;
export fn decodeHeader(buf: &[]u8): MqttHeader;
export fn encodePublish(topic: &[]u8, payload: &[]u8, qos: u8, retain: bool, packet_id: u16, dst: []u8): i64;
export fn decodePublish(buf: &[]u8): MqttPublish;
export fn encodeSimple(pkt_type: u8, dst: []u8): i64;
export fn encodePuback(packet_id: u16, dst: []u8): i64;
