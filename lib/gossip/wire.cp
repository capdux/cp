// Source: cp/lib/gossip/wire.cp (Compact binary wire codec for SWIM GossipMessage)

const std = @import("std");
import { GossipError, MemberState, GossipUpdate } from "./types.cp";

pub const WIRE_MAGIC_0: u8 = 0x47; // 'G'
pub const WIRE_MAGIC_1: u8 = 0x50; // 'P'
pub const WIRE_VERSION: u8 = 1;

pub enum MessageKind {
    Ping = 1,
    Ack = 2,
    PingReq = 3,
    Sync = 4,

    pub fn fromU8(v: u8) -> GossipError!MessageKind {
        switch (v) {
            1 => return MessageKind.Ping,
            2 => return MessageKind.Ack,
            3 => return MessageKind.PingReq,
            4 => return MessageKind.Sync,
            else => return GossipError.UnknownTag,
        }
    }
}

pub struct GossipMessage {
    kind: MessageKind,
    seq: u64,
    sender_id: string,
    sender_addr: string,
    target_id: string,
    target_addr: string,
    updates: []GossipUpdate,
}

pub struct OutboundMessage {
    target_id: string,
    target_addr: string,
    msg: GossipMessage,
}

fn appendU16(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, val: u16) -> GossipError!void {
    buf.append(alloc, @intCast(val & 0xFF)) catch return GossipError.OutOfMemory;
    buf.append(alloc, @intCast((val >> 8) & 0xFF)) catch return GossipError.OutOfMemory;
}

fn appendU64(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, val: u64) -> GossipError!void {
    let i: u6 = 0;
    while (i < 8) {
        const shift: u6 = @intCast(i * 8);
        buf.append(alloc, @intCast((val >> shift) & 0xFF)) catch return GossipError.OutOfMemory;
        i += 1;
    }
}

fn appendStr(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, s: string) -> GossipError!void {
    if (s.len > 65535) {
        return GossipError.InvalidMessage;
    }
    try appendU16(buf, alloc, @intCast(s.len));
    buf.appendSlice(alloc, s) catch return GossipError.OutOfMemory;
}

pub fn encodeMessage(alloc: std.mem.Allocator, msg: GossipMessage) -> GossipError![]u8 {
    let buf = std.ArrayList(u8).empty;
    errdefer buf.deinit(alloc);

    buf.append(alloc, WIRE_MAGIC_0) catch return GossipError.OutOfMemory;
    buf.append(alloc, WIRE_MAGIC_1) catch return GossipError.OutOfMemory;
    buf.append(alloc, WIRE_VERSION) catch return GossipError.OutOfMemory;
    buf.append(alloc, @intFromEnum(msg.kind)) catch return GossipError.OutOfMemory;

    try appendU64(&buf, alloc, msg.seq);
    try appendStr(&buf, alloc, msg.sender_id);
    try appendStr(&buf, alloc, msg.sender_addr);
    try appendStr(&buf, alloc, msg.target_id);
    try appendStr(&buf, alloc, msg.target_addr);

    if (msg.updates.len > 65535) {
        return GossipError.InvalidMessage;
    }
    try appendU16(&buf, alloc, @intCast(msg.updates.len));

    for (upd of msg.updates) {
        try appendStr(&buf, alloc, upd.node_id);
        try appendStr(&buf, alloc, upd.addr);
        buf.append(alloc, @intFromEnum(upd.state)) catch return GossipError.OutOfMemory;
        try appendU64(&buf, alloc, upd.incarnation);
    }

    return buf.toOwnedSlice(alloc) catch return GossipError.OutOfMemory;
}

struct Cursor {
    data: []const u8,
    pos: usize,

    fn readU8(*_) -> GossipError!u8 {
        if (_.pos >= _.data.len) {
            return GossipError.TruncatedMessage;
        }
        const b = _.data[_.pos];
        _.pos += 1;
        return b;
    }

    fn readU16(*_) -> GossipError!u16 {
        if (_.pos + 2 > _.data.len) {
            return GossipError.TruncatedMessage;
        }
        const b0: u16 = _.data[_.pos];
        const b1: u16 = _.data[_.pos + 1];
        _.pos += 2;
        return b0 | (b1 << 8);
    }

    fn readU64(*_) -> GossipError!u64 {
        if (_.pos + 8 > _.data.len) {
            return GossipError.TruncatedMessage;
        }
        let out: u64 = 0;
        let i: u6 = 0;
        while (i < 8) {
            const b: u64 = _.data[_.pos + i];
            const shift: u6 = @intCast(i * 8);
            out = out | (b << shift);
            i += 1;
        }
        _.pos += 8;
        return out;
    }

    fn readDupStr(*_, alloc: std.mem.Allocator) -> GossipError![]u8 {
        const len: usize = try _.readU16();
        if (_.pos + len > _.data.len) {
            return GossipError.TruncatedMessage;
        }
        const slice = _.data[_.pos .. _.pos + len];
        _.pos += len;
        return alloc.dupe(u8, slice) catch return GossipError.OutOfMemory;
    }
}

pub fn decodeMessage(alloc: std.mem.Allocator, data: []const u8) -> GossipError!GossipMessage {
    if (data.len < 12) {
        return GossipError.TruncatedMessage;
    }
    let cur = Cursor{ .data = data, .pos = 0 };

    const m0 = try cur.readU8();
    const m1 = try cur.readU8();
    if (m0 != WIRE_MAGIC_0 or m1 != WIRE_MAGIC_1) {
        return GossipError.InvalidMessage;
    }
    const ver = try cur.readU8();
    if (ver != WIRE_VERSION) {
        return GossipError.InvalidMessage;
    }
    const kind_raw = try cur.readU8();
    const kind = try MessageKind.fromU8(kind_raw);
    const seq = try cur.readU64();

    const sender_id = try cur.readDupStr(alloc);
    errdefer alloc.free(sender_id);

    const sender_addr = try cur.readDupStr(alloc);
    errdefer alloc.free(sender_addr);

    const target_id = try cur.readDupStr(alloc);
    errdefer alloc.free(target_id);

    const target_addr = try cur.readDupStr(alloc);
    errdefer alloc.free(target_addr);

    const upd_count: usize = try cur.readU16();
    let updates_list = std.ArrayList(GossipUpdate).empty;
    errdefer {
        for (u of updates_list.items) {
            alloc.free(u.node_id);
            alloc.free(u.addr);
        }
        updates_list.deinit(alloc);
    }

    let i: usize = 0;
    while (i < upd_count) {
        const nid = try cur.readDupStr(alloc);
        errdefer alloc.free(nid);

        const naddr = try cur.readDupStr(alloc);
        errdefer alloc.free(naddr);

        const st_raw = try cur.readU8();
        const st = try MemberState.fromU8(st_raw);
        const inc = try cur.readU64();

        updates_list.append(alloc, GossipUpdate{
            .node_id = nid,
            .addr = naddr,
            .state = st,
            .incarnation = inc,
        }) catch return GossipError.OutOfMemory;
        i += 1;
    }

    if (cur.pos != cur.data.len) {
        return GossipError.InvalidMessage;
    }

    const updates = updates_list.toOwnedSlice(alloc) catch return GossipError.OutOfMemory;
    return GossipMessage{
        .kind = kind,
        .seq = seq,
        .sender_id = sender_id,
        .sender_addr = sender_addr,
        .target_id = target_id,
        .target_addr = target_addr,
        .updates = updates,
    };
}

pub fn freeMessage(alloc: std.mem.Allocator, msg: *GossipMessage) {
    alloc.free(msg.sender_id);
    alloc.free(msg.sender_addr);
    alloc.free(msg.target_id);
    alloc.free(msg.target_addr);
    for (u of msg.updates) {
        alloc.free(u.node_id);
        alloc.free(u.addr);
    }
    alloc.free(msg.updates);
}
