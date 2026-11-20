// Source: cp/lib/mysql_wire/auth.cp (MySQL HandshakeV10, HandshakeResponse41 & mysql_native_password)
const std = @import("std");
import { readLenEncStr, writeLenEncStr, readNullTermStr } from "./packet.cp";

pub const CLIENT_LONG_PASSWORD: u32 = 0x00000001;
pub const CLIENT_FOUND_ROWS: u32 = 0x00000002;
pub const CLIENT_LONG_FLAG: u32 = 0x00000004;
pub const CLIENT_CONNECT_WITH_DB: u32 = 0x00000008;
pub const CLIENT_PROTOCOL_41: u32 = 0x00000200;
pub const CLIENT_TRANSACTIONS: u32 = 0x00002000;
pub const CLIENT_SECURE_CONNECTION: u32 = 0x00008000;
pub const CLIENT_PLUGIN_AUTH: u32 = 0x00080000;

pub const DEFAULT_CLIENT_CAPABILITIES: u32 =
    CLIENT_LONG_PASSWORD |
    CLIENT_FOUND_ROWS |
    CLIENT_LONG_FLAG |
    CLIENT_PROTOCOL_41 |
    CLIENT_TRANSACTIONS |
    CLIENT_SECURE_CONNECTION |
    CLIENT_PLUGIN_AUTH;

pub struct HandshakeV10 {
    protocol_version: u8,
    server_version: []u8,
    connection_id: u32,
    auth_plugin_data: [20]u8,
    capability_flags: u32,
    character_set: u8,
    status_flags: u16,
    auth_plugin_name: []u8,

    pub fn deinit(alloc: std.mem.Allocator) {
        alloc.free(_.server_version);
        alloc.free(_.auth_plugin_name);
    }

    pub fn encode(alloc: std.mem.Allocator): !([]u8) {
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(alloc);

        try out.append(alloc, _.protocol_version);
        try out.appendSlice(alloc, _.server_version);
        try out.append(alloc, 0);

        let conn_buf: [4]u8 = undefined;
        std.mem.writeInt(u32, &conn_buf, _.connection_id, .little);
        try out.appendSlice(alloc, &conn_buf);

        try out.appendSlice(alloc, _.auth_plugin_data[0..8]);
        try out.append(alloc, 0);

        const cap_low: u16 = @truncate(_.capability_flags);
        let cap_low_buf: [2]u8 = undefined;
        std.mem.writeInt(u16, &cap_low_buf, cap_low, .little);
        try out.appendSlice(alloc, &cap_low_buf);

        try out.append(alloc, _.character_set);

        let status_buf: [2]u8 = undefined;
        std.mem.writeInt(u16, &status_buf, _.status_flags, .little);
        try out.appendSlice(alloc, &status_buf);

        const cap_high: u16 = @truncate(_.capability_flags >> 16);
        let cap_high_buf: [2]u8 = undefined;
        std.mem.writeInt(u16, &cap_high_buf, cap_high, .little);
        try out.appendSlice(alloc, &cap_high_buf);

        try out.append(alloc, 21);

        let reserved = std.mem.zeroes([10]u8);
        try out.appendSlice(alloc, &reserved);

        try out.appendSlice(alloc, _.auth_plugin_data[8..20]);
        try out.append(alloc, 0);

        try out.appendSlice(alloc, _.auth_plugin_name);
        try out.append(alloc, 0);

        return try out.toOwnedSlice(alloc);
    }

    pub fn decode(alloc: std.mem.Allocator, payload: []const u8): !HandshakeV10 {
        let offset: usize = 0;
        if (offset >= payload.len) {
            return error.UnexpectedEof;
        }
        const proto = payload[offset];
        offset += 1;
        if (proto != 10) {
            return error.InvalidProtocolVersion;
        }

        const srv_ver_slice = try readNullTermStr(payload, &offset);
        if (offset + 4 + 8 + 1 + 2 + 1 + 2 + 2 + 1 + 10 > payload.len) {
            return error.UnexpectedEof;
        }

        const conn_id = std.mem.readInt(u32, payload[offset..][0..4], .little);
        offset += 4;

        let scramble = std.mem.zeroes([20]u8);
        @memcpy(scramble[0..8], payload[offset .. offset + 8]);
        offset += 8;

        // filler
        offset += 1;

        const cap_low = std.mem.readInt(u16, payload[offset..][0..2], .little);
        offset += 2;

        const charset = payload[offset];
        offset += 1;

        const status = std.mem.readInt(u16, payload[offset..][0..2], .little);
        offset += 2;

        const cap_high = std.mem.readInt(u16, payload[offset..][0..2], .little);
        offset += 2;

        const caps = (@as(u32, cap_high) << 16) | @as(u32, cap_low);

        const auth_data_len = payload[offset];
        offset += 1;

        // 10 reserved bytes
        offset += 10;

        if ((caps & CLIENT_SECURE_CONNECTION) != 0) {
            const part2_len: usize = if (auth_data_len > 8) @max(13, @as(usize, auth_data_len) - 8) else 13;
            if (offset + part2_len > payload.len) {
                return error.UnexpectedEof;
            }
            @memcpy(scramble[8..20], payload[offset .. offset + 12]);
            offset += part2_len;
        }

        let plugin_slice: []const u8 = "mysql_native_password";
        if ((caps & CLIENT_PLUGIN_AUTH) != 0 and offset < payload.len) {
            plugin_slice = try readNullTermStr(payload, &offset);
        }

        const srv_ver = try alloc.dupe(u8, srv_ver_slice);
        errdefer alloc.free(srv_ver);
        const plugin_name = try alloc.dupe(u8, plugin_slice);

        return HandshakeV10{
            .protocol_version = proto,
            .server_version = srv_ver,
            .connection_id = conn_id,
            .auth_plugin_data = scramble,
            .capability_flags = caps,
            .character_set = charset,
            .status_flags = status,
            .auth_plugin_name = plugin_name,
        };
    }
}

pub fn computeNativePasswordAuth(
    alloc: std.mem.Allocator,
    password: []const u8,
    scramble_20: [20]u8,
): !([]u8) {
    if (password.len == 0) {
        return try alloc.alloc(u8, 0);
    }

    let stage1: [20]u8 = undefined;
    let h1 = std.crypto.hash.Sha1.init(.{});
    h1.update(password);
    h1.final(&stage1);

    let stage2: [20]u8 = undefined;
    let h2 = std.crypto.hash.Sha1.init(.{});
    h2.update(&stage1);
    h2.final(&stage2);

    let challenge: [20]u8 = undefined;
    let h3 = std.crypto.hash.Sha1.init(.{});
    h3.update(&scramble_20);
    h3.update(&stage2);
    h3.final(&challenge);

    let out = try alloc.alloc(u8, 20);
    let i: usize = 0;
    while (i < 20) {
        out[i] = stage1[i] ^ challenge[i];
        i += 1;
    }
    return out;
}

pub struct HandshakeResponse41 {
    capability_flags: u32,
    max_packet_size: u32,
    character_set: u8,
    username: []u8,
    auth_response: []u8,
    database: ([]u8)?,
    auth_plugin_name: ([]u8)?,

    pub fn deinit(alloc: std.mem.Allocator) {
        alloc.free(_.username);
        alloc.free(_.auth_response);
        if (db := _.database) {
            alloc.free(db);
        }
        if (pn := _.auth_plugin_name) {
            alloc.free(pn);
        }
    }

    pub fn encode(alloc: std.mem.Allocator): !([]u8) {
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(alloc);

        let caps = _.capability_flags | CLIENT_PROTOCOL_41;
        if (_.database != null) {
            caps |= CLIENT_CONNECT_WITH_DB;
        }
        if (_.auth_plugin_name != null) {
            caps |= CLIENT_PLUGIN_AUTH;
        }

        let cap_buf: [4]u8 = undefined;
        std.mem.writeInt(u32, &cap_buf, caps, .little);
        try out.appendSlice(alloc, &cap_buf);

        let max_buf: [4]u8 = undefined;
        std.mem.writeInt(u32, &max_buf, _.max_packet_size, .little);
        try out.appendSlice(alloc, &max_buf);

        try out.append(alloc, _.character_set);

        let reserved = std.mem.zeroes([23]u8);
        try out.appendSlice(alloc, &reserved);

        try out.appendSlice(alloc, _.username);
        try out.append(alloc, 0);

        if ((caps & CLIENT_SECURE_CONNECTION) != 0) {
            try writeLenEncStr(alloc, &out, _.auth_response);
        } else {
            try out.appendSlice(alloc, _.auth_response);
            try out.append(alloc, 0);
        }

        if (db := _.database) {
            try out.appendSlice(alloc, db);
            try out.append(alloc, 0);
        }

        if (pn := _.auth_plugin_name) {
            try out.appendSlice(alloc, pn);
            try out.append(alloc, 0);
        }

        return try out.toOwnedSlice(alloc);
    }

    pub fn decode(alloc: std.mem.Allocator, payload: []const u8): !HandshakeResponse41 {
        if (payload.len < 32) {
            return error.UnexpectedEof;
        }
        let offset: usize = 0;
        const caps = std.mem.readInt(u32, payload[offset..][0..4], .little);
        offset += 4;
        const max_pkt = std.mem.readInt(u32, payload[offset..][0..4], .little);
        offset += 4;
        const charset = payload[offset];
        offset += 1;
        offset += 23; // reserved

        const user_slice = try readNullTermStr(payload, &offset);
        const auth_slice: []const u8 = if ((caps & CLIENT_SECURE_CONNECTION) != 0)
            (try readLenEncStr(payload, &offset)) orelse ""
        else
            try readNullTermStr(payload, &offset);

        let db_slice: ([]const u8)? = null;
        if ((caps & CLIENT_CONNECT_WITH_DB) != 0 and offset < payload.len) {
            db_slice = try readNullTermStr(payload, &offset);
        }

        let plugin_slice: ([]const u8)? = null;
        if ((caps & CLIENT_PLUGIN_AUTH) != 0 and offset < payload.len) {
            plugin_slice = try readNullTermStr(payload, &offset);
        }

        const username = try alloc.dupe(u8, user_slice);
        errdefer alloc.free(username);
        const auth_response = try alloc.dupe(u8, auth_slice);
        errdefer alloc.free(auth_response);

        let db_owned: ([]u8)? = null;
        if (d := db_slice) {
            db_owned = try alloc.dupe(u8, d);
        }
        errdefer if (d := db_owned) alloc.free(d);

        let plugin_owned: ([]u8)? = null;
        if (p := plugin_slice) {
            plugin_owned = try alloc.dupe(u8, p);
        }

        return HandshakeResponse41{
            .capability_flags = caps,
            .max_packet_size = max_pkt,
            .character_set = charset,
            .username = username,
            .auth_response = auth_response,
            .database = db_owned,
            .auth_plugin_name = plugin_owned,
        };
    }
}
