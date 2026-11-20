// Source: cp/lib/mqtt/codec.cp (MQTT v3.1.1 / v5.0 Binary Packet & Variable Byte Int Codec)
const std = @import("std");
import {
    MqttError,
    MqttVersion,
    QoS,
    PacketType,
    WillMessage,
    ConnectPacket,
    ConnackPacket,
    PublishPacket,
    TopicSubscription,
    TopicFilter,
    SubscribePacket,
    SubackPacket,
    UnsubscribePacket,
    DisconnectPacket,
    MqttPacket,
} from "./types.cp";
import { validateTopicName, validateTopicFilter } from "./topic.cp";

pub const MAX_VAR_INT: usize = 268435455;

pub struct VarIntResult {
    value: usize,
    bytes_read: usize,
}

pub fn encodeVarByteInt(value: usize, out: *[4]u8): MqttError!usize {
    if (value > MAX_VAR_INT) {
        return MqttError.MalformedVarInt;
    }
    let x = value;
    let i: usize = 0;
    while (true) {
        let encoded_byte: u8 = @intCast(x % 128);
        x = x / 128;
        if (x > 0) {
            encoded_byte = encoded_byte | 0x80;
        }
        out[i] = encoded_byte;
        i += 1;
        if (x == 0) {
            break;
        }
    }
    return i;
}

pub fn decodeVarByteInt(data: []const u8): MqttError!VarIntResult {
    if (data.len == 0) {
        return MqttError.IncompletePacket;
    }
    let multiplier: usize = 1;
    let value: usize = 0;
    let i: usize = 0;
    while (i < 4) {
        if (i >= data.len) {
            return MqttError.IncompletePacket;
        }
        const encoded_byte = data[i];
        value += @as(usize, encoded_byte & 0x7F) * multiplier;
        if ((encoded_byte & 0x80) == 0) {
            return VarIntResult{
                .value = value,
                .bytes_read = i + 1,
            };
        }
        if (i == 3) {
            return MqttError.MalformedVarInt;
        }
        multiplier *= 128;
        i += 1;
    }
    return MqttError.MalformedVarInt;
}

pub fn validateMqttUtf8(s: []const u8): MqttError!void {
    if (s.len > 65535) {
        return MqttError.InvalidUtf8;
    }
    if (!std.unicode.utf8ValidateSlice(s)) {
        return MqttError.InvalidUtf8;
    }
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == 0) {
            return MqttError.InvalidUtf8;
        }
        i += 1;
    }
}

fn appendU16(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), val: u16): MqttError!void {
    try buf.append(alloc, @intCast((val >> 8) & 0xFF));
    try buf.append(alloc, @intCast(val & 0xFF));
}

fn appendMqttString(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), s: []const u8): MqttError!void {
    try validateMqttUtf8(s);
    try appendU16(alloc, buf, @intCast(s.len));
    try buf.appendSlice(alloc, s);
}

fn appendBinaryData(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), data: []const u8): MqttError!void {
    if (data.len > 65535) {
        return MqttError.MalformedPacket;
    }
    try appendU16(alloc, buf, @intCast(data.len));
    try buf.appendSlice(alloc, data);
}

fn readU16(data: []const u8, offset: *usize): MqttError!u16 {
    if (offset.* + 2 > data.len) {
        return MqttError.MalformedPacket;
    }
    const hi: u16 = data[offset.*];
    const lo: u16 = data[offset.* + 1];
    offset.* += 2;
    return (hi << 8) | lo;
}

fn readMqttString(data: []const u8, offset: *usize): MqttError![]const u8 {
    const len = try readU16(data, offset);
    if (offset.* + len > data.len) {
        return MqttError.MalformedPacket;
    }
    const s = data[offset.* .. offset.* + len];
    offset.* += len;
    try validateMqttUtf8(s);
    return s;
}

fn readBinaryData(data: []const u8, offset: *usize): MqttError![]const u8 {
    const len = try readU16(data, offset);
    if (offset.* + len > data.len) {
        return MqttError.MalformedPacket;
    }
    const s = data[offset.* .. offset.* + len];
    offset.* += len;
    return s;
}

pub fn encodePacket(alloc: std.mem.Allocator, packet: MqttPacket): MqttError![]u8 {
    let body = std.ArrayList(u8).empty;
    defer body.deinit(alloc);

    let header_byte: u8 = 0;

    switch (packet) {
        .connect => |conn| {
            header_byte = @intFromEnum(PacketType.connect) << 4;
            // Protocol Name "MQTT"
            try appendMqttString(alloc, &body, "MQTT");
            // Protocol Level
            try body.append(alloc, @intFromEnum(conn.version));
            // Connect Flags
            let flags: u8 = 0;
            if (conn.clean_session) {
                flags |= 0x02;
            }
            if (w := conn.will) {
                try validateTopicName(w.topic);
                flags |= 0x04;
                flags |= (@intFromEnum(w.qos) & 0x03) << 3;
                if (w.retain) {
                    flags |= 0x20;
                }
            }
            if (conn.password != null) {
                if (conn.username == null and conn.version == MqttVersion.v3_1_1) {
                    return MqttError.MalformedPacket;
                }
                flags |= 0x40;
            }
            if (conn.username != null) {
                flags |= 0x80;
            }
            try body.append(alloc, flags);
            // Keep Alive
            try appendU16(alloc, &body, conn.keep_alive);
            // v5.0 Properties (length 0)
            if (conn.version == MqttVersion.v5_0) {
                try body.append(alloc, 0);
            }
            // Payload: Client ID
            try appendMqttString(alloc, &body, conn.client_id);
            if (w := conn.will) {
                if (conn.version == MqttVersion.v5_0) {
                    try body.append(alloc, 0); // Will Properties length 0
                }
                try appendMqttString(alloc, &body, w.topic);
                try appendBinaryData(alloc, &body, w.payload);
            }
            if (u := conn.username) {
                try appendMqttString(alloc, &body, u);
            }
            if (p := conn.password) {
                try appendBinaryData(alloc, &body, p);
            }
        },
        .connack => |ca| {
            header_byte = @intFromEnum(PacketType.connack) << 4;
            try body.append(alloc, if (ca.session_present) 1 else 0);
            try body.append(alloc, ca.return_code);
        },
        .publish => |pub_pkt| {
            try validateTopicName(pub_pkt.topic);
            if (pub_pkt.qos == QoS.at_most_once) {
                if (pub_pkt.packet_id != null) {
                    return MqttError.UnexpectedPacketId;
                }
            } else {
                if (pub_pkt.packet_id == null or pub_pkt.packet_id.? == 0) {
                    return MqttError.MissingPacketId;
                }
            }
            let flags: u8 = (@intFromEnum(pub_pkt.qos) & 0x03) << 1;
            if (pub_pkt.dup) {
                flags |= 0x08;
            }
            if (pub_pkt.retain) {
                flags |= 0x01;
            }
            header_byte = (@intFromEnum(PacketType.publish) << 4) | flags;
            try appendMqttString(alloc, &body, pub_pkt.topic);
            if (pid := pub_pkt.packet_id) {
                try appendU16(alloc, &body, pid);
            }
            try body.appendSlice(alloc, pub_pkt.payload);
        },
        .puback => |pid| {
            if (pid == 0) return MqttError.MissingPacketId;
            header_byte = @intFromEnum(PacketType.puback) << 4;
            try appendU16(alloc, &body, pid);
        },
        .pubrec => |pid| {
            if (pid == 0) return MqttError.MissingPacketId;
            header_byte = @intFromEnum(PacketType.pubrec) << 4;
            try appendU16(alloc, &body, pid);
        },
        .pubrel => |pid| {
            if (pid == 0) return MqttError.MissingPacketId;
            header_byte = (@intFromEnum(PacketType.pubrel) << 4) | 0x02;
            try appendU16(alloc, &body, pid);
        },
        .pubcomp => |pid| {
            if (pid == 0) return MqttError.MissingPacketId;
            header_byte = @intFromEnum(PacketType.pubcomp) << 4;
            try appendU16(alloc, &body, pid);
        },
        .subscribe => |sub| {
            if (sub.packet_id == 0) return MqttError.MissingPacketId;
            if (sub.topics.len == 0) return MqttError.MalformedPacket;
            header_byte = (@intFromEnum(PacketType.subscribe) << 4) | 0x02;
            try appendU16(alloc, &body, sub.packet_id);
            for (t of sub.topics) {
                try validateTopicFilter(t.filter);
                try appendMqttString(alloc, &body, t.filter);
                try body.append(alloc, @intFromEnum(t.qos));
            }
        },
        .suback => |sa| {
            if (sa.packet_id == 0) return MqttError.MissingPacketId;
            if (sa.return_codes.len == 0) return MqttError.MalformedPacket;
            header_byte = @intFromEnum(PacketType.suback) << 4;
            try appendU16(alloc, &body, sa.packet_id);
            try body.appendSlice(alloc, sa.return_codes);
        },
        .unsubscribe => |unsub| {
            if (unsub.packet_id == 0) return MqttError.MissingPacketId;
            if (unsub.topics.len == 0) return MqttError.MalformedPacket;
            header_byte = (@intFromEnum(PacketType.unsubscribe) << 4) | 0x02;
            try appendU16(alloc, &body, unsub.packet_id);
            for (t of unsub.topics) {
                try validateTopicFilter(t.filter);
                try appendMqttString(alloc, &body, t.filter);
            }
        },
        .unsuback => |pid| {
            if (pid == 0) return MqttError.MissingPacketId;
            header_byte = @intFromEnum(PacketType.unsuback) << 4;
            try appendU16(alloc, &body, pid);
        },
        .pingreq => {
            header_byte = @intFromEnum(PacketType.pingreq) << 4;
        },
        .pingresp => {
            header_byte = @intFromEnum(PacketType.pingresp) << 4;
        },
        .disconnect => |disc| {
            header_byte = @intFromEnum(PacketType.disconnect) << 4;
            if (disc.reason_code != 0) {
                try body.append(alloc, disc.reason_code);
            }
        },
    }

    let var_len_buf: [4]u8 = undefined;
    const var_len_size = try encodeVarByteInt(body.items.len, &var_len_buf);

    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);
    try out.append(alloc, header_byte);
    try out.appendSlice(alloc, var_len_buf[0..var_len_size]);
    try out.appendSlice(alloc, body.items);
    return try out.toOwnedSlice(alloc);
}

pub fn decodePacket(alloc: std.mem.Allocator, data: []const u8): MqttError!MqttPacket {
    if (data.len < 2) {
        return MqttError.IncompletePacket;
    }
    const header_byte = data[0];
    const ptype = try PacketType.fromU8(header_byte >> 4);
    const flags = header_byte & 0x0F;

    const var_res = try decodeVarByteInt(data[1..]);
    const header_len = 1 + var_res.bytes_read;
    const total_len = header_len + var_res.value;
    if (data.len < total_len) {
        return MqttError.IncompletePacket;
    }

    const body = data[header_len..total_len];
    let offset: usize = 0;

    switch (ptype) {
        .connect => {
            if (flags != 0) return MqttError.InvalidFlags;
            const proto = try readMqttString(body, &offset);
            if (!std.mem.eql(u8, proto, "MQTT")) {
                return MqttError.MalformedPacket;
            }
            if (offset + 4 > body.len) return MqttError.MalformedPacket;
            const version = try MqttVersion.fromU8(body[offset]);
            offset += 1;
            const cflags = body[offset];
            offset += 1;
            if ((cflags & 0x01) != 0) return MqttError.MalformedPacket;
            const keep_alive = try readU16(body, &offset);

            if (version == MqttVersion.v5_0) {
                const prop_len = try decodeVarByteInt(body[offset..]);
                offset += prop_len.bytes_read;
                if (offset + prop_len.value > body.len) return MqttError.MalformedPacket;
                offset += prop_len.value;
            }

            const client_id = try readMqttString(body, &offset);
            const clean_session = (cflags & 0x02) != 0;
            const will_flag = (cflags & 0x04) != 0;
            const will_qos = try QoS.fromU8((cflags >> 3) & 0x03);
            const will_retain = (cflags & 0x20) != 0;
            const pass_flag = (cflags & 0x40) != 0;
            const user_flag = (cflags & 0x80) != 0;

            let will_msg: WillMessage? = null;
            if (will_flag) {
                if (version == MqttVersion.v5_0) {
                    const wprop_len = try decodeVarByteInt(body[offset..]);
                    offset += wprop_len.bytes_read;
                    if (offset + wprop_len.value > body.len) return MqttError.MalformedPacket;
                    offset += wprop_len.value;
                }
                const wtopic = try readMqttString(body, &offset);
                try validateTopicName(wtopic);
                const wpayload = try readBinaryData(body, &offset);
                will_msg = WillMessage{
                    .topic = wtopic,
                    .payload = wpayload,
                    .qos = will_qos,
                    .retain = will_retain,
                };
            }

            let username: ([]const u8)? = null;
            if (user_flag) {
                username = try readMqttString(body, &offset);
            }
            let password: ([]const u8)? = null;
            if (pass_flag) {
                password = try readBinaryData(body, &offset);
            }
            if (offset != body.len) return MqttError.MalformedPacket;

            return MqttPacket{
                .connect = ConnectPacket{
                    .version = version,
                    .client_id = client_id,
                    .clean_session = clean_session,
                    .keep_alive = keep_alive,
                    .will = will_msg,
                    .username = username,
                    .password = password,
                },
            };
        },
        .connack => {
            if (flags != 0 or body.len < 2) return MqttError.MalformedPacket;
            const ack_flags = body[0];
            if ((ack_flags & 0xFE) != 0) return MqttError.MalformedPacket;
            return MqttPacket{
                .connack = ConnackPacket{
                    .session_present = (ack_flags & 0x01) != 0,
                    .return_code = body[1],
                },
            };
        },
        .publish => {
            const dup = (flags & 0x08) != 0;
            const qos = try QoS.fromU8((flags >> 1) & 0x03);
            const retain = (flags & 0x01) != 0;
            const topic = try readMqttString(body, &offset);
            try validateTopicName(topic);

            let packet_id: u16? = null;
            if (qos != QoS.at_most_once) {
                const pid = try readU16(body, &offset);
                if (pid == 0) return MqttError.MissingPacketId;
                packet_id = pid;
            }
            const payload = body[offset..];
            return MqttPacket{
                .publish = PublishPacket{
                    .dup = dup,
                    .qos = qos,
                    .retain = retain,
                    .topic = topic,
                    .packet_id = packet_id,
                    .payload = payload,
                },
            };
        },
        .puback => {
            if (flags != 0) return MqttError.InvalidFlags;
            if (body.len != 2) return MqttError.MalformedPacket;
            const pid = try readU16(body, &offset);
            if (pid == 0) return MqttError.MissingPacketId;
            return MqttPacket{ .puback = pid };
        },
        .pubrec => {
            if (flags != 0) return MqttError.InvalidFlags;
            if (body.len != 2) return MqttError.MalformedPacket;
            const pid = try readU16(body, &offset);
            if (pid == 0) return MqttError.MissingPacketId;
            return MqttPacket{ .pubrec = pid };
        },
        .pubrel => {
            if (flags != 0x02) return MqttError.InvalidFlags;
            if (body.len != 2) return MqttError.MalformedPacket;
            const pid = try readU16(body, &offset);
            if (pid == 0) return MqttError.MissingPacketId;
            return MqttPacket{ .pubrel = pid };
        },
        .pubcomp => {
            if (flags != 0) return MqttError.InvalidFlags;
            if (body.len != 2) return MqttError.MalformedPacket;
            const pid = try readU16(body, &offset);
            if (pid == 0) return MqttError.MissingPacketId;
            return MqttPacket{ .pubcomp = pid };
        },
        .subscribe => {
            if (flags != 0x02) return MqttError.InvalidFlags;
            const pid = try readU16(body, &offset);
            if (pid == 0) return MqttError.MissingPacketId;
            let topics = std.ArrayList(TopicSubscription).empty;
            errdefer topics.deinit(alloc);
            while (offset < body.len) {
                const filter = try readMqttString(body, &offset);
                try validateTopicFilter(filter);
                if (offset >= body.len) return MqttError.MalformedPacket;
                const qos = try QoS.fromU8(body[offset] & 0x03);
                offset += 1;
                try topics.append(alloc, TopicSubscription{
                    .filter = filter,
                    .qos = qos,
                });
            }
            if (topics.items.len == 0) return MqttError.MalformedPacket;
            return MqttPacket{
                .subscribe = SubscribePacket{
                    .packet_id = pid,
                    .topics = try topics.toOwnedSlice(alloc),
                },
            };
        },
        .suback => {
            if (flags != 0) return MqttError.InvalidFlags;
            const pid = try readU16(body, &offset);
            if (pid == 0) return MqttError.MissingPacketId;
            if (offset >= body.len) return MqttError.MalformedPacket;
            const codes = try alloc.dupe(u8, body[offset..]);
            return MqttPacket{
                .suback = SubackPacket{
                    .packet_id = pid,
                    .return_codes = codes,
                },
            };
        },
        .unsubscribe => {
            if (flags != 0x02) return MqttError.InvalidFlags;
            const pid = try readU16(body, &offset);
            if (pid == 0) return MqttError.MissingPacketId;
            let topics = std.ArrayList(TopicFilter).empty;
            errdefer topics.deinit(alloc);
            while (offset < body.len) {
                const filter = try readMqttString(body, &offset);
                try validateTopicFilter(filter);
                try topics.append(alloc, TopicFilter{ .filter = filter });
            }
            if (topics.items.len == 0) return MqttError.MalformedPacket;
            return MqttPacket{
                .unsubscribe = UnsubscribePacket{
                    .packet_id = pid,
                    .topics = try topics.toOwnedSlice(alloc),
                },
            };
        },
        .unsuback => {
            if (flags != 0) return MqttError.InvalidFlags;
            if (body.len != 2) return MqttError.MalformedPacket;
            const pid = try readU16(body, &offset);
            if (pid == 0) return MqttError.MissingPacketId;
            return MqttPacket{ .unsuback = pid };
        },
        .pingreq => {
            if (flags != 0 or body.len != 0) return MqttError.MalformedPacket;
            return MqttPacket.pingreq;
        },
        .pingresp => {
            if (flags != 0 or body.len != 0) return MqttError.MalformedPacket;
            return MqttPacket.pingresp;
        },
        .disconnect => {
            if (flags != 0) return MqttError.InvalidFlags;
            const reason: u8 = if (body.len > 0) body[0] else 0;
            return MqttPacket{
                .disconnect = DisconnectPacket{
                    .reason_code = reason,
                },
            };
        },
        .auth => return MqttError.InvalidPacketType,
    }
}
