// Source: cp/lib/mqtt/types.cp (MQTT v3.1.1 / v5.0 Core Types & Error Definitions)
const std = @import("std");

pub const MqttError = error{
    MalformedVarInt,
    IncompletePacket,
    PacketTooLarge,
    InvalidPacketType,
    InvalidFlags,
    MalformedPacket,
    InvalidUtf8,
    InvalidTopicFilter,
    InvalidTopicName,
    InvalidQoS,
    MissingPacketId,
    UnexpectedPacketId,
    UnsupportedVersion,
    OutOfMemory,
};

pub const MqttVersion = enum(u8) {
    v3_1_1 = 4,
    v5_0 = 5,

    pub fn fromU8(val: u8) MqttError!MqttVersion {
        if (val == 4) {
            return MqttVersion.v3_1_1;
        }
        if (val == 5) {
            return MqttVersion.v5_0;
        }
        return MqttError.UnsupportedVersion;
    }
};

pub const QoS = enum(u8) {
    at_most_once = 0,
    at_least_once = 1,
    exactly_once = 2,

    pub fn fromU8(val: u8) MqttError!QoS {
        if (val == 0) {
            return QoS.at_most_once;
        }
        if (val == 1) {
            return QoS.at_least_once;
        }
        if (val == 2) {
            return QoS.exactly_once;
        }
        return MqttError.InvalidQoS;
    }
};

pub const PacketType = enum(u8) {
    connect = 1,
    connack = 2,
    publish = 3,
    puback = 4,
    pubrec = 5,
    pubrel = 6,
    pubcomp = 7,
    subscribe = 8,
    suback = 9,
    unsubscribe = 10,
    unsuback = 11,
    pingreq = 12,
    pingresp = 13,
    disconnect = 14,
    auth = 15,

    pub fn fromU8(val: u8) MqttError!PacketType {
        return switch (val) {
            1 => PacketType.connect,
            2 => PacketType.connack,
            3 => PacketType.publish,
            4 => PacketType.puback,
            5 => PacketType.pubrec,
            6 => PacketType.pubrel,
            7 => PacketType.pubcomp,
            8 => PacketType.subscribe,
            9 => PacketType.suback,
            10 => PacketType.unsubscribe,
            11 => PacketType.unsuback,
            12 => PacketType.pingreq,
            13 => PacketType.pingresp,
            14 => PacketType.disconnect,
            15 => PacketType.auth,
            else => MqttError.InvalidPacketType,
        };
    }
};

pub struct WillMessage {
    topic: []const u8,
    payload: []const u8,
    qos: QoS,
    retain: bool,
}

pub struct ConnectPacket {
    version: MqttVersion,
    client_id: []const u8,
    clean_session: bool,
    keep_alive: u16,
    will: WillMessage?,
    username: []const u8?,
    password: []const u8?,
}

pub struct ConnackPacket {
    session_present: bool,
    return_code: u8,
}

pub struct PublishPacket {
    dup: bool,
    qos: QoS,
    retain: bool,
    topic: []const u8,
    packet_id: u16?,
    payload: []const u8,
}

pub struct TopicSubscription {
    filter: []const u8,
    qos: QoS,
}

pub struct SubscribePacket {
    packet_id: u16,
    topics: []const TopicSubscription,
}

pub struct SubackPacket {
    packet_id: u16,
    return_codes: []const u8,
}

pub struct TopicFilter {
    filter: []const u8,
}

pub struct UnsubscribePacket {
    packet_id: u16,
    topics: []const TopicFilter,
}

pub struct DisconnectPacket {
    reason_code: u8,
}

pub const MqttPacket = union(enum) {
    connect: ConnectPacket,
    connack: ConnackPacket,
    publish: PublishPacket,
    puback: u16,
    pubrec: u16,
    pubrel: u16,
    pubcomp: u16,
    subscribe: SubscribePacket,
    suback: SubackPacket,
    unsubscribe: UnsubscribePacket,
    unsuback: u16,
    pingreq: void,
    pingresp: void,
    disconnect: DisconnectPacket,

    pub fn deinit(self: MqttPacket, alloc: std.mem.Allocator) void {
        switch (self) {
            .subscribe => |sub| {
                alloc.free(sub.topics);
            },
            .suback => |sa| {
                alloc.free(sa.return_codes);
            },
            .unsubscribe => |unsub| {
                alloc.free(unsub.topics);
            },
            else => {},
        }
    }
};
