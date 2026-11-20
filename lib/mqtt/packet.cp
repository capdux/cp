// Source: cp/lib/mqtt/packet.cp (Pure CP MQTT 3.1.1 Wire Codec and Incremental Stream Decoder)
// Pure CP zero-C-dependency MQTT 3.1.1 binary wire protocol codec and
// incremental streaming packet decoder with remaining-length bounds & OOM safety.

const std = @import("std");

/// Maximum MQTT 3.1.1 Remaining Length representable in 4 variable-length bytes (2^28 - 1).
pub const MAX_REMAINING_LENGTH: usize = 268435455;

/// Error set returned by MQTT codec and stream decoder operations.
pub err MqttError {
    InvalidPacketType,
    MalformedRemainingLength,
    InvalidProtocolName,
    InvalidQos,
    MissingPacketId,
    PacketTooLarge,
    Truncated,
    EmptyTopics,
    OutOfMemory,
}

/// MQTT 3.1.1 Control Packet types (values 1..14; 0 and 15 are forbidden).
pub enum MqttPacketType: u8 {
    Connect = 1,
    ConnAck = 2,
    Publish = 3,
    PubAck = 4,
    PubRec = 5,
    PubRel = 6,
    PubComp = 7,
    Subscribe = 8,
    SubAck = 9,
    Unsubscribe = 10,
    UnsubAck = 11,
    PingReq = 12,
    PingResp = 13,
    Disconnect = 14,
}

/// MQTT Quality of Service levels (0, 1, 2; 3 is reserved/invalid).
pub enum MqttQos: u8 {
    AtMostOnce = 0,
    AtLeastOnce = 1,
    ExactlyOnce = 2,
}

/// MQTT 3.1.1 CONNACK return codes.
pub enum ConnAckCode: u8 {
    Accepted = 0,
    UnacceptableProtocolVersion = 1,
    IdentifierRejected = 2,
    ServerUnavailable = 3,
    BadUserNameOrPassword = 4,
    NotAuthorized = 5,
}

/// Last Will and Testament configuration carried inside CONNECT packets.
pub struct MqttWill {
    topic: string = "";
    message: []const u8 = "";
    qos: MqttQos = .AtMostOnce;
    retain: bool = false;
}

/// MQTT CONNECT packet payload and flags.
pub struct ConnectPacket {
    protocol_name: string = "MQTT";
    protocol_level: u8 = 4;
    clean_session: bool = true;
    keep_alive: u16 = 60;
    client_id: string = "";
    will: MqttWill? = null;
    username: string? = null;
    password: ([]const u8)? = null;
}

/// MQTT CONNACK packet.
pub struct ConnAckPacket {
    session_present: bool = false;
    return_code: ConnAckCode = .Accepted;
}

/// MQTT PUBLISH packet.
pub struct PublishPacket {
    dup: bool = false;
    qos: MqttQos = .AtMostOnce;
    retain: bool = false;
    topic: string = "";
    packet_id: u16? = null;
    payload: []const u8 = "";
}

/// Shared payload for packet-ID-only control packets (PUBACK/PUBREC/PUBREL/PUBCOMP/UNSUBACK).
pub struct PacketIdOnly {
    packet_id: u16 = 0;
}

/// Subscription topic filter and requested QoS level.
pub struct TopicFilter {
    topic: string = "";
    qos: MqttQos = .AtMostOnce;
}

/// MQTT SUBSCRIBE packet.
pub struct SubscribePacket {
    packet_id: u16 = 0;
    topics: []const TopicFilter = [];
}

/// MQTT SUBACK packet.
pub struct SubAckPacket {
    packet_id: u16 = 0;
    return_codes: []const u8 = [];
}

/// MQTT UNSUBSCRIBE packet.
pub struct UnsubscribePacket {
    packet_id: u16 = 0;
    topics: []const string = [];
}

/// Result of decoding variable-length Remaining Length.
pub struct RemainingLengthResult {
    value: usize = 0;
    consumed: usize = 0;
}

/// Tagged union representing all MQTT 3.1.1 control packets.
pub enum MqttPacket {
    Connect: ConnectPacket,
    ConnAck: ConnAckPacket,
    Publish: PublishPacket,
    PubAck: PacketIdOnly,
    PubRec: PacketIdOnly,
    PubRel: PacketIdOnly,
    PubComp: PacketIdOnly,
    Subscribe: SubscribePacket,
    SubAck: SubAckPacket,
    Unsubscribe: UnsubscribePacket,
    UnsubAck: PacketIdOnly,
    PingReq,
    PingResp,
    Disconnect,

    /// Releases heap-allocated slices owned by decoded packets.
    pub fn deinit(*_, alloc: std.mem.Allocator) {
        switch (_.*) {
            .Connect c => {
                if (c.protocol_name.len > 0) alloc.free(c.protocol_name);
                if (c.client_id.len > 0) alloc.free(c.client_id);
                if (w := c.will) {
                    if (w.topic.len > 0) alloc.free(w.topic);
                    if (w.message.len > 0) alloc.free(w.message);
                }
                if (u := c.username) {
                    if (u.len > 0) alloc.free(u);
                }
                if (pw := c.password) {
                    if (pw.len > 0) alloc.free(pw);
                }
            },
            .Publish p => {
                if (p.topic.len > 0) alloc.free(p.topic);
                if (p.payload.len > 0) alloc.free(p.payload);
            },
            .Subscribe s => {
                for (tf of s.topics) {
                    if (tf.topic.len > 0) alloc.free(tf.topic);
                }
                if (s.topics.len > 0) alloc.free(s.topics);
            },
            .SubAck sa => {
                if (sa.return_codes.len > 0) alloc.free(sa.return_codes);
            },
            .Unsubscribe u => {
                for (t of u.topics) {
                    if (t.len > 0) alloc.free(t);
                }
                if (u.topics.len > 0) alloc.free(u.topics);
            },
            else => {},
        }
    }
}

/// Result of decoding a single MQTT packet from a byte buffer.
pub struct DecodeResult {
    packet: MqttPacket;
    consumed: usize;
}

/// Pure CP MQTT 3.1.1 wire protocol encoder and decoder.
pub static class MqttCodec {
    /// Encodes `len` (0..268,435,455) into MQTT variable-length format in `out_buf`.
    /// Returns the number of bytes written (1..4).
    pub fn encodeRemainingLength(len: usize, out_buf: []u8): usize ! MqttError {
        if (len > MAX_REMAINING_LENGTH) {
            return MqttError.PacketTooLarge;
        }
        let x: usize = len;
        let idx: usize = 0;
        while (true) {
            if (idx >= out_buf.len) {
                return MqttError.Truncated;
            }
            let encoded_byte: u8 = (x % 128) as u8;
            x = x / 128;
            if (x > 0) {
                encoded_byte = encoded_byte | 0x80;
            }
            out_buf[idx] = encoded_byte;
            idx += 1;
            if (x == 0) {
                break;
            }
        }
        return idx;
    }

    /// Decodes MQTT variable-length Remaining Length from `bytes` (max 4 bytes).
    /// Returns `MalformedRemainingLength` if the 4th byte has continuation bit `0x80` set.
    pub fn decodeRemainingLength(bytes: []const u8): RemainingLengthResult ! MqttError {
        let multiplier: usize = 1;
        let value: usize = 0;
        let idx: usize = 0;
        while (true) {
            if (idx >= bytes.len) {
                return MqttError.Truncated;
            }
            const encoded_byte: u8 = bytes[idx];
            value += ((encoded_byte & 0x7F) as usize) * multiplier;
            idx += 1;
            if ((encoded_byte & 0x80) == 0) {
                return RemainingLengthResult{ value: value, consumed: idx };
            }
            if (idx >= 4) {
                return MqttError.MalformedRemainingLength;
            }
            multiplier *= 128;
        }
    }

    fn writeU16(list: *std.ArrayList(u8), alloc: std.mem.Allocator, v: u16) ! MqttError {
        list.append(alloc, ((v >> 8) & 0xFF) as u8) catch return MqttError.OutOfMemory;
        list.append(alloc, (v & 0xFF) as u8) catch return MqttError.OutOfMemory;
    }

    fn writeLenPrefixed(list: *std.ArrayList(u8), alloc: std.mem.Allocator, data: []const u8) ! MqttError {
        if (data.len > 65535) {
            return MqttError.PacketTooLarge;
        }
        writeU16(list, alloc, data.len as u16)?;
        if (data.len > 0) {
            list.appendSlice(alloc, data) catch return MqttError.OutOfMemory;
        }
    }

    fn readU16(bytes: []const u8, offset: *usize): u16 ! MqttError {
        if (offset.* + 2 > bytes.len) {
            return MqttError.Truncated;
        }
        const hi: u16 = bytes[offset.*];
        const lo: u16 = bytes[offset.* + 1];
        offset.* += 2;
        return (hi << 8) | lo;
    }

    fn readLenPrefixedOwned(alloc: std.mem.Allocator, bytes: []const u8, offset: *usize): []u8 ! MqttError {
        const slen: usize = readU16(bytes, offset)?;
        if (offset.* + slen > bytes.len) {
            return MqttError.Truncated;
        }
        if (slen == 0) {
            return "";
        }
        const out = alloc.dupe(u8, bytes[offset.* .. offset.* + slen]) catch return MqttError.OutOfMemory;
        offset.* += slen;
        return out;
    }

    fn parseQos(raw: u8): MqttQos ! MqttError {
        if (raw == 0) return MqttQos.AtMostOnce;
        if (raw == 1) return MqttQos.AtLeastOnce;
        if (raw == 2) return MqttQos.ExactlyOnce;
        return MqttError.InvalidQos;
    }

    fn parsePacketType(raw: u8): MqttPacketType ! MqttError {
        switch (raw) {
            1 => return MqttPacketType.Connect,
            2 => return MqttPacketType.ConnAck,
            3 => return MqttPacketType.Publish,
            4 => return MqttPacketType.PubAck,
            5 => return MqttPacketType.PubRec,
            6 => return MqttPacketType.PubRel,
            7 => return MqttPacketType.PubComp,
            8 => return MqttPacketType.Subscribe,
            9 => return MqttPacketType.SubAck,
            10 => return MqttPacketType.Unsubscribe,
            11 => return MqttPacketType.UnsubAck,
            12 => return MqttPacketType.PingReq,
            13 => return MqttPacketType.PingResp,
            14 => return MqttPacketType.Disconnect,
            else => return MqttError.InvalidPacketType,
        }
    }

    fn parseConnAckCode(raw: u8): ConnAckCode ! MqttError {
        switch (raw) {
            0 => return ConnAckCode.Accepted,
            1 => return ConnAckCode.UnacceptableProtocolVersion,
            2 => return ConnAckCode.IdentifierRejected,
            3 => return ConnAckCode.ServerUnavailable,
            4 => return ConnAckCode.BadUserNameOrPassword,
            5 => return ConnAckCode.NotAuthorized,
            else => return MqttError.InvalidPacketType,
        }
    }

    /// Encodes an `MqttPacket` into a newly allocated wire-format byte slice.
    pub fn encode(alloc: std.mem.Allocator, pkt: MqttPacket): []u8 ! MqttError {
        let body: std.ArrayList(u8) = .empty;
        defer body.deinit(alloc);

        let header_byte: u8 = 0;

        switch (pkt) {
            .Connect c => {
                if (!std.mem.eql(u8, c.protocol_name, "MQTT")) {
                    return MqttError.InvalidProtocolName;
                }
                header_byte = (MqttPacketType.Connect as u8) << 4;
                writeLenPrefixed(*body, alloc, c.protocol_name)?;
                body.append(alloc, c.protocol_level) catch return MqttError.OutOfMemory;

                let flags: u8 = 0;
                if (c.clean_session) flags |= 0x02;
                if (w := c.will) {
                    flags |= 0x04;
                    flags |= ((w.qos as u8) & 0x03) << 3;
                    if (w.retain) flags |= 0x20;
                }
                if (c.username != null) flags |= 0x80;
                if (c.password != null) flags |= 0x40;

                body.append(alloc, flags) catch return MqttError.OutOfMemory;
                writeU16(*body, alloc, c.keep_alive)?;

                writeLenPrefixed(*body, alloc, c.client_id)?;
                if (w := c.will) {
                    writeLenPrefixed(*body, alloc, w.topic)?;
                    writeLenPrefixed(*body, alloc, w.message)?;
                }
                if (u := c.username) {
                    writeLenPrefixed(*body, alloc, u)?;
                }
                if (pw := c.password) {
                    writeLenPrefixed(*body, alloc, pw)?;
                }
            },
            .ConnAck ca => {
                header_byte = (MqttPacketType.ConnAck as u8) << 4;
                const sp: u8 = if (ca.session_present) 1 else 0;
                body.append(alloc, sp) catch return MqttError.OutOfMemory;
                body.append(alloc, ca.return_code as u8) catch return MqttError.OutOfMemory;
            },
            .Publish p => {
                let flags: u8 = 0;
                if (p.dup) flags |= 0x08;
                flags |= ((p.qos as u8) & 0x03) << 1;
                if (p.retain) flags |= 0x01;
                header_byte = ((MqttPacketType.Publish as u8) << 4) | flags;

                writeLenPrefixed(*body, alloc, p.topic)?;
                if (p.qos != .AtMostOnce) {
                    if (p.packet_id == null) {
                        return MqttError.MissingPacketId;
                    }
                    const pid = p.packet_id.?;
                    if (pid == 0) {
                        return MqttError.MissingPacketId;
                    }
                    writeU16(*body, alloc, pid)?;
                }
                if (p.payload.len > 0) {
                    body.appendSlice(alloc, p.payload) catch return MqttError.OutOfMemory;
                }
            },
            .PubAck ack => {
                if (ack.packet_id == 0) return MqttError.MissingPacketId;
                header_byte = (MqttPacketType.PubAck as u8) << 4;
                writeU16(*body, alloc, ack.packet_id)?;
            },
            .PubRec rec => {
                if (rec.packet_id == 0) return MqttError.MissingPacketId;
                header_byte = (MqttPacketType.PubRec as u8) << 4;
                writeU16(*body, alloc, rec.packet_id)?;
            },
            .PubRel rel => {
                if (rel.packet_id == 0) return MqttError.MissingPacketId;
                header_byte = ((MqttPacketType.PubRel as u8) << 4) | 0x02;
                writeU16(*body, alloc, rel.packet_id)?;
            },
            .PubComp comp => {
                if (comp.packet_id == 0) return MqttError.MissingPacketId;
                header_byte = (MqttPacketType.PubComp as u8) << 4;
                writeU16(*body, alloc, comp.packet_id)?;
            },
            .Subscribe sub => {
                if (sub.packet_id == 0) return MqttError.MissingPacketId;
                if (sub.topics.len == 0) return MqttError.EmptyTopics;
                header_byte = ((MqttPacketType.Subscribe as u8) << 4) | 0x02;
                writeU16(*body, alloc, sub.packet_id)?;
                for (tf of sub.topics) {
                    writeLenPrefixed(*body, alloc, tf.topic)?;
                    body.append(alloc, tf.qos as u8) catch return MqttError.OutOfMemory;
                }
            },
            .SubAck sa => {
                if (sa.packet_id == 0) return MqttError.MissingPacketId;
                if (sa.return_codes.len == 0) return MqttError.EmptyTopics;
                header_byte = (MqttPacketType.SubAck as u8) << 4;
                writeU16(*body, alloc, sa.packet_id)?;
                body.appendSlice(alloc, sa.return_codes) catch return MqttError.OutOfMemory;
            },
            .Unsubscribe unsub => {
                if (unsub.packet_id == 0) return MqttError.MissingPacketId;
                if (unsub.topics.len == 0) return MqttError.EmptyTopics;
                header_byte = ((MqttPacketType.Unsubscribe as u8) << 4) | 0x02;
                writeU16(*body, alloc, unsub.packet_id)?;
                for (t of unsub.topics) {
                    writeLenPrefixed(*body, alloc, t)?;
                }
            },
            .UnsubAck ua => {
                if (ua.packet_id == 0) return MqttError.MissingPacketId;
                header_byte = (MqttPacketType.UnsubAck as u8) << 4;
                writeU16(*body, alloc, ua.packet_id)?;
            },
            .PingReq => {
                header_byte = (MqttPacketType.PingReq as u8) << 4;
            },
            .PingResp => {
                header_byte = (MqttPacketType.PingResp as u8) << 4;
            },
            .Disconnect => {
                header_byte = (MqttPacketType.Disconnect as u8) << 4;
            },
        }

        let rem_buf: [4]u8 = [0; 4];
        const rem_len_bytes = encodeRemainingLength(body.items.len, rem_buf[0..])?;

        const total_len: usize = (1 as usize) + rem_len_bytes + body.items.len;
        let out = alloc.alloc(u8, total_len) catch return MqttError.OutOfMemory;
        out[0] = header_byte;
        std.mem.copyForwards(u8, out[1 .. 1 + rem_len_bytes], rem_buf[0..rem_len_bytes]);
        if (body.items.len > 0) {
            std.mem.copyForwards(u8, out[1 + rem_len_bytes ..], body.items);
        }
        return out;
    }

    /// Decodes a single MQTT 3.1.1 packet from `buf`, returning the owned `MqttPacket` and bytes consumed.
    pub fn decodeOne(alloc: std.mem.Allocator, buf: []const u8): DecodeResult ! MqttError {
        if (buf.len < 2) {
            return MqttError.Truncated;
        }
        const first_byte: u8 = buf[0];
        const ptype = parsePacketType((first_byte >> 4) & 0x0F)?;
        const flags: u8 = first_byte & 0x0F;

        const rem = decodeRemainingLength(buf[1..])?;
        const header_len: usize = (1 as usize) + rem.consumed;
        const total_len: usize = header_len + rem.value;
        if (buf.len < total_len) {
            return MqttError.Truncated;
        }

        const body = buf[header_len..total_len];
        let off: usize = 0;

        switch (ptype) {
            .Connect => {
                if (flags != 0) return MqttError.InvalidPacketType;
                const proto_name = readLenPrefixedOwned(alloc, body, *off)?;
                errdefer if (proto_name.len > 0) alloc.free(proto_name);
                if (!std.mem.eql(u8, proto_name, "MQTT")) {
                    return MqttError.InvalidProtocolName;
                }
                if (off + 4 > body.len) return MqttError.Truncated;
                const proto_level = body[off];
                off += 1;
                const conn_flags = body[off];
                off += 1;
                if ((conn_flags & 0x01) != 0) return MqttError.InvalidPacketType;
                const keep_alive = readU16(body, *off)?;

                const clean_session = (conn_flags & 0x02) != 0;
                const has_will = (conn_flags & 0x04) != 0;
                const will_qos_raw: u8 = (conn_flags >> 3) & 0x03;
                const will_qos = parseQos(will_qos_raw)?;
                const will_retain = (conn_flags & 0x20) != 0;
                if (!has_will && (will_qos_raw != 0 || will_retain)) {
                    return MqttError.InvalidPacketType;
                }
                const has_user = (conn_flags & 0x80) != 0;
                const has_pass = (conn_flags & 0x40) != 0;

                const client_id = readLenPrefixedOwned(alloc, body, *off)?;
                errdefer if (client_id.len > 0) alloc.free(client_id);

                let will: MqttWill? = null;
                errdefer if (w := will) {
                    if (w.topic.len > 0) alloc.free(w.topic);
                    if (w.message.len > 0) alloc.free(w.message);
                }
                if (has_will) {
                    const wtopic = readLenPrefixedOwned(alloc, body, *off)?;
                    errdefer if (wtopic.len > 0) alloc.free(wtopic);
                    const wmsg = readLenPrefixedOwned(alloc, body, *off)?;
                    will = MqttWill{
                        topic: wtopic,
                        message: wmsg,
                        qos: will_qos,
                        retain: will_retain,
                    };
                }

                let username: string? = null;
                errdefer if (u := username) {
                    if (u.len > 0) alloc.free(u);
                }
                if (has_user) {
                    username = readLenPrefixedOwned(alloc, body, *off)?;
                }

                let password: ([]const u8)? = null;
                errdefer if (pw := password) {
                    if (pw.len > 0) alloc.free(pw);
                }
                if (has_pass) {
                    password = readLenPrefixedOwned(alloc, body, *off)?;
                }

                if (off != body.len) return MqttError.MalformedRemainingLength;

                return DecodeResult{
                    packet: MqttPacket{
                        Connect: ConnectPacket{
                            protocol_name: proto_name,
                            protocol_level: proto_level,
                            clean_session: clean_session,
                            keep_alive: keep_alive,
                            client_id: client_id,
                            will: will,
                            username: username,
                            password: password,
                        },
                    },
                    consumed: total_len,
                };
            },
            .ConnAck => {
                if (flags != 0) return MqttError.InvalidPacketType;
                if (body.len != 2) return MqttError.MalformedRemainingLength;
                if ((body[0] & 0xFE) != 0) return MqttError.InvalidPacketType;
                const sp = (body[0] & 0x01) != 0;
                const rc = parseConnAckCode(body[1])?;
                return DecodeResult{
                    packet: MqttPacket{
                        ConnAck: ConnAckPacket{
                            session_present: sp,
                            return_code: rc,
                        },
                    },
                    consumed: total_len,
                };
            },
            .Publish => {
                const dup = (flags & 0x08) != 0;
                const qos = parseQos((flags >> 1) & 0x03)?;
                const retain = (flags & 0x01) != 0;

                const topic = readLenPrefixedOwned(alloc, body, *off)?;
                errdefer if (topic.len > 0) alloc.free(topic);

                let pid: u16? = null;
                if (qos != .AtMostOnce) {
                    const read_pid = readU16(body, *off)?;
                    if (read_pid == 0) return MqttError.MissingPacketId;
                    pid = read_pid;
                }

                const payload_slice = body[off..];
                const payload = if (payload_slice.len > 0)
                    (alloc.dupe(u8, payload_slice) catch return MqttError.OutOfMemory)
                else
                    "";

                return DecodeResult{
                    packet: MqttPacket{
                        Publish: PublishPacket{
                            dup: dup,
                            qos: qos,
                            retain: retain,
                            topic: topic,
                            packet_id: pid,
                            payload: payload,
                        },
                    },
                    consumed: total_len,
                };
            },
            .PubAck | .PubRec | .PubRel | .PubComp | .UnsubAck => {
                const expected_flags: u8 = if (ptype == .PubRel) 0x02 else 0x00;
                if (flags != expected_flags) return MqttError.InvalidPacketType;
                if (body.len != 2) return MqttError.MalformedRemainingLength;
                const pid = readU16(body, *off)?;
                if (pid == 0) return MqttError.MissingPacketId;
                const info = PacketIdOnly{ packet_id: pid };
                switch (ptype) {
                    .PubAck => return DecodeResult{ packet: MqttPacket{ PubAck: info }, consumed: total_len },
                    .PubRec => return DecodeResult{ packet: MqttPacket{ PubRec: info }, consumed: total_len },
                    .PubRel => return DecodeResult{ packet: MqttPacket{ PubRel: info }, consumed: total_len },
                    .PubComp => return DecodeResult{ packet: MqttPacket{ PubComp: info }, consumed: total_len },
                    else => return DecodeResult{ packet: MqttPacket{ UnsubAck: info }, consumed: total_len },
                }
            },
            .Subscribe => {
                if (flags != 0x02) return MqttError.InvalidPacketType;
                const pid = readU16(body, *off)?;
                if (pid == 0) return MqttError.MissingPacketId;

                let list: std.ArrayList(TopicFilter) = .empty;
                errdefer {
                    for (tf of list.items) {
                        if (tf.topic.len > 0) alloc.free(tf.topic);
                    }
                    list.deinit(alloc);
                }

                while (off < body.len) {
                    const t = readLenPrefixedOwned(alloc, body, *off)?;
                    errdefer if (t.len > 0) alloc.free(t);
                    if (off >= body.len) return MqttError.Truncated;
                    const qraw = body[off];
                    off += 1;
                    if (qraw > 2) return MqttError.InvalidQos;
                    const q = parseQos(qraw)?;
                    list.append(alloc, TopicFilter{ topic: t, qos: q }) catch return MqttError.OutOfMemory;
                }
                if (list.items.len == 0) return MqttError.EmptyTopics;

                const owned_topics = list.toOwnedSlice(alloc) catch return MqttError.OutOfMemory;
                return DecodeResult{
                    packet: MqttPacket{
                        Subscribe: SubscribePacket{
                            packet_id: pid,
                            topics: owned_topics,
                        },
                    },
                    consumed: total_len,
                };
            },
            .SubAck => {
                if (flags != 0) return MqttError.InvalidPacketType;
                const pid = readU16(body, *off)?;
                if (pid == 0) return MqttError.MissingPacketId;
                const rc_slice = body[off..];
                if (rc_slice.len == 0) return MqttError.EmptyTopics;
                const owned_codes = alloc.dupe(u8, rc_slice) catch return MqttError.OutOfMemory;
                return DecodeResult{
                    packet: MqttPacket{
                        SubAck: SubAckPacket{
                            packet_id: pid,
                            return_codes: owned_codes,
                        },
                    },
                    consumed: total_len,
                };
            },
            .Unsubscribe => {
                if (flags != 0x02) return MqttError.InvalidPacketType;
                const pid = readU16(body, *off)?;
                if (pid == 0) return MqttError.MissingPacketId;

                let list: std.ArrayList(string) = .empty;
                errdefer {
                    for (t of list.items) {
                        if (t.len > 0) alloc.free(t);
                    }
                    list.deinit(alloc);
                }

                while (off < body.len) {
                    const t = readLenPrefixedOwned(alloc, body, *off)?;
                    list.append(alloc, t) catch {
                        if (t.len > 0) alloc.free(t);
                        return MqttError.OutOfMemory;
                    };
                }
                if (list.items.len == 0) return MqttError.EmptyTopics;

                const owned_topics = list.toOwnedSlice(alloc) catch return MqttError.OutOfMemory;
                return DecodeResult{
                    packet: MqttPacket{
                        Unsubscribe: UnsubscribePacket{
                            packet_id: pid,
                            topics: owned_topics,
                        },
                    },
                    consumed: total_len,
                };
            },
            .PingReq => {
                if (flags != 0) return MqttError.InvalidPacketType;
                if (body.len != 0) return MqttError.MalformedRemainingLength;
                return DecodeResult{ packet: .PingReq, consumed: total_len };
            },
            .PingResp => {
                if (flags != 0) return MqttError.InvalidPacketType;
                if (body.len != 0) return MqttError.MalformedRemainingLength;
                return DecodeResult{ packet: .PingResp, consumed: total_len };
            },
            .Disconnect => {
                if (flags != 0) return MqttError.InvalidPacketType;
                if (body.len != 0) return MqttError.MalformedRemainingLength;
                return DecodeResult{ packet: .Disconnect, consumed: total_len };
            },
        }
    }
}

/// Incremental streaming MQTT 3.1.1 decoder supporting partial/fragmented reads,
/// coalesced multi-packet frames, and `max_packet_bytes` DoS protection.
pub struct MqttStreamDecoder {
    priv allocator: std.mem.Allocator;
    priv buf: std.ArrayList(u8) = .empty;
    priv read_pos: usize = 0;
    priv max_packet_bytes: usize;

    /// Initializes a stream decoder with `alloc` and maximum allowed packet frame size `max_packet_bytes`.
    pub static fn init(alloc: std.mem.Allocator, max_packet_bytes: usize): MqttStreamDecoder {
        return MqttStreamDecoder{
            allocator: alloc,
            buf: .empty,
            read_pos: 0,
            max_packet_bytes: max_packet_bytes,
        };
    }

    /// Releases the internal stream buffer.
    pub fn deinit(*_) {
        _.buf.deinit(_.allocator);
        _.read_pos = 0;
    }

    /// Feeds an arbitrary chunk of incoming wire bytes into the decoder buffer.
    pub fn feed(*_, chunk: []const u8) ! MqttError {
        if (chunk.len > 0) {
            if (_.read_pos == _.buf.items.len) {
                _.buf.items.len = 0;
                _.read_pos = 0;
            } else if (_.read_pos >= 4096 && _.read_pos * 2 >= _.buf.items.len) {
                const rem = _.buf.items.len - _.read_pos;
                std.mem.copyForwards(u8, _.buf.items[0..rem], _.buf.items[_.read_pos..]);
                _.buf.items.len = rem;
                _.read_pos = 0;
            }
            _.buf.appendSlice(_.allocator, chunk) catch return MqttError.OutOfMemory;
        }
    }

    /// Returns the number of unconsumed bytes currently buffered.
    pub fn bufferedLen(): usize {
        return _.buf.items.len - _.read_pos;
    }

    /// Attempts to decode the next complete packet from the internal buffer.
    /// Returns `null` if more bytes are required.
    pub fn nextPacket(*_, alloc: std.mem.Allocator): MqttPacket? ! MqttError {
        const avail = _.buf.items[_.read_pos..];
        if (avail.len < 2) {
            return null;
        }

        const rem = MqttCodec.decodeRemainingLength(avail[1..]) catch e => {
            if (e == MqttError.Truncated) {
                return null;
            }
            return e;
        };

        const total_len: usize = (1 as usize) + rem.consumed + rem.value;
        if (total_len > _.max_packet_bytes) {
            return MqttError.PacketTooLarge;
        }
        if (avail.len < total_len) {
            return null;
        }

        const res = MqttCodec.decodeOne(alloc, avail[0..total_len])?;
        _.read_pos += res.consumed;
        if (_.read_pos == _.buf.items.len) {
            _.buf.items.len = 0;
            _.read_pos = 0;
        }
        return res.packet;
    }
}
