// Source: cp/lib/mqtt/stream.cp (Incremental Streaming MQTT Frame Decoder)
const std = @import("std");
import { MqttError, MqttPacket } from "./types.cp";
import { decodeVarByteInt, decodePacket } from "./codec.cp";

pub struct MqttStreamDecoder {
    alloc: std.mem.Allocator,
    buf: std.ArrayList(u8),
    read_pos: usize,
    max_packet_bytes: usize,

    pub fn init(alloc: std.mem.Allocator, max_packet_bytes: usize): MqttStreamDecoder {
        return MqttStreamDecoder{
            .alloc = alloc,
            .buf = std.ArrayList(u8).empty,
            .read_pos = 0,
            .max_packet_bytes = max_packet_bytes,
        };
    }

    pub fn deinit(*_) {
        _.buf.deinit(_.alloc);
        _.read_pos = 0;
    }

    fn compact(*_) {
        if (_.read_pos == 0) {
            return;
        }
        const rem = _.buf.items.len - _.read_pos;
        if (rem == 0) {
            _.buf.clearRetainingCapacity();
        } else {
            std.mem.copyForwards(u8, _.buf.items[0..rem], _.buf.items[_.read_pos..]);
            _.buf.items.len = rem;
        }
        _.read_pos = 0;
    }

    pub fn feed(*_, chunk: []const u8): MqttError!void {
        if (_.read_pos > 0) {
            _.compact();
        }
        if (_.buf.items.len + chunk.len > _.max_packet_bytes * 4 and _.buf.items.len > _.max_packet_bytes) {
            return MqttError.PacketTooLarge;
        }
        try _.buf.appendSlice(_.alloc, chunk);
    }

    pub fn nextPacket(*_): MqttError!MqttPacket? {
        const available = _.buf.items[_.read_pos..];
        if (available.len < 2) {
            return null;
        }

        const var_res = decodeVarByteInt(available[1..]) catch |e| {
            if (e == MqttError.IncompletePacket) {
                return null;
            }
            return e;
        };

        const total_len = 1 + var_res.bytes_read + var_res.value;
        if (total_len > _.max_packet_bytes) {
            return MqttError.PacketTooLarge;
        }

        if (available.len < total_len) {
            return null;
        }

        const pkt = try decodePacket(_.alloc, available[0..total_len]);
        _.read_pos += total_len;
        return pkt;
    }
}
