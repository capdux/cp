// Source: Absorbed from /data/dev/bun/src/sql/mysql/protocol/ | Synced: 2026-11-22T12:49:17Z (packet framing, wire auth, text resultset & COM_* protocol)
const std = @import("std");

pub const MAX_DEFAULT_PACKET_BYTES: usize = 16 * 1024 * 1024;

pub enum MysqlError {
    UnexpectedEof,
    InvalidPacketHeader,
    PacketTooLarge,
    InvalidLengthEncodedInt,
    InvalidProtocolVersion,
    InvalidPacketType,
    MalformedPacket,
    OutOfMemory,
}

pub struct LenEncInt {
    value: u64,
    is_null: bool,
}

pub struct MysqlPacket {
    sequence_id: u8,
    payload: []u8,

    pub fn deinit(alloc: std.mem.Allocator) {
        alloc.free(_.payload);
    }
}

pub fn readLenEncInt(data: []const u8, offset: *usize): !LenEncInt {
    if (offset.* >= data.len) {
        return error.UnexpectedEof;
    }
    const first = data[offset.*];
    offset.* += 1;

    if (first < 0xFB) {
        return LenEncInt{ .value = @as(u64, first), .is_null = false };
    }
    if (first == 0xFB) {
        return LenEncInt{ .value = 0, .is_null = true };
    }
    if (first == 0xFC) {
        if (offset.* + 2 > data.len) {
            return error.UnexpectedEof;
        }
        const v = std.mem.readInt(u16, data[offset.*..][0..2], .little);
        offset.* += 2;
        return LenEncInt{ .value = @as(u64, v), .is_null = false };
    }
    if (first == 0xFD) {
        if (offset.* + 3 > data.len) {
            return error.UnexpectedEof;
        }
        const b0 = @as(u64, data[offset.*]);
        const b1 = @as(u64, data[offset.* + 1]);
        const b2 = @as(u64, data[offset.* + 2]);
        offset.* += 3;
        return LenEncInt{ .value = b0 | (b1 << 8) | (b2 << 16), .is_null = false };
    }
    if (first == 0xFE) {
        if (offset.* + 8 > data.len) {
            return error.UnexpectedEof;
        }
        const v = std.mem.readInt(u64, data[offset.*..][0..8], .little);
        offset.* += 8;
        return LenEncInt{ .value = v, .is_null = false };
    }
    return error.InvalidLengthEncodedInt;
}

pub fn writeLenEncInt(alloc: std.mem.Allocator, out: *std.ArrayList(u8), value: u64): !void {
    if (value < 251) {
        try out.append(alloc, @truncate(value));
        return;
    }
    if (value < 65536) {
        try out.append(alloc, 0xFC);
        let buf: [2]u8 = undefined;
        std.mem.writeInt(u16, &buf, @truncate(value), .little);
        try out.appendSlice(alloc, &buf);
        return;
    }
    if (value < 16777216) {
        try out.append(alloc, 0xFD);
        try out.append(alloc, @truncate(value));
        try out.append(alloc, @truncate(value >> 8));
        try out.append(alloc, @truncate(value >> 16));
        return;
    }
    try out.append(alloc, 0xFE);
    let buf: [8]u8 = undefined;
    std.mem.writeInt(u64, &buf, value, .little);
    try out.appendSlice(alloc, &buf);
}

pub fn writeLenEncNull(alloc: std.mem.Allocator, out: *std.ArrayList(u8)): !void {
    try out.append(alloc, 0xFB);
}

pub fn readLenEncStr(data: []const u8, offset: *usize): !(([]const u8)?) {
    const len_info = try readLenEncInt(data, offset);
    if (len_info.is_null) {
        return null;
    }
    const str_len: usize = @intCast(len_info.value);
    if (offset.* + str_len > data.len) {
        return error.UnexpectedEof;
    }
    const slice = data[offset.* .. offset.* + str_len];
    offset.* += str_len;
    return slice;
}

pub fn writeLenEncStr(alloc: std.mem.Allocator, out: *std.ArrayList(u8), value: []const u8): !void {
    try writeLenEncInt(alloc, out, @as(u64, value.len));
    try out.appendSlice(alloc, value);
}

pub fn readNullTermStr(data: []const u8, offset: *usize): !([]const u8) {
    const start = offset.*;
    while (offset.* < data.len) {
        if (data[offset.*] == 0) {
            const slice = data[start..offset.*];
            offset.* += 1;
            return slice;
        }
        offset.* += 1;
    }
    return error.UnexpectedEof;
}

pub fn encodePacketFrame(alloc: std.mem.Allocator, sequence_id: u8, payload: []const u8): !([]u8) {
    if (payload.len > 0xFFFFFF) {
        return error.PacketTooLarge;
    }
    const total = 4 + payload.len;
    let out = try alloc.alloc(u8, total);
    const plen: u32 = @intCast(payload.len);
    out[0] = @truncate(plen);
    out[1] = @truncate(plen >> 8);
    out[2] = @truncate(plen >> 16);
    out[3] = sequence_id;
    if (payload.len > 0) {
        @memcpy(out[4..total], payload);
    }
    return out;
}

pub struct MysqlStreamFramer {
    buf: std.ArrayList(u8),
    read_pos: usize,
    max_packet_bytes: usize,

    pub static fn init(max_packet_bytes: usize): MysqlStreamFramer {
        return MysqlStreamFramer{
            .buf = std.ArrayList(u8).empty,
            .read_pos = 0,
            .max_packet_bytes = max_packet_bytes,
        };
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        _.buf.deinit(alloc);
    }

    pub fn pendingBytes(): usize {
        return _.buf.items.len - _.read_pos;
    }

    pub fn feed(*_, alloc: std.mem.Allocator, chunk: []const u8): !void {
        if (chunk.len == 0) {
            return;
        }
        if (_.read_pos > 0 and _.read_pos == _.buf.items.len) {
            _.buf.clearRetainingCapacity();
            _.read_pos = 0;
        } else if (_.read_pos > 4096) {
            const rem = _.buf.items.len - _.read_pos;
            std.mem.copyForwards(u8, _.buf.items[0..rem], _.buf.items[_.read_pos..]);
            _.buf.items.len = rem;
            _.read_pos = 0;
        }
        try _.buf.appendSlice(alloc, chunk);
    }

    pub fn nextPacket(*_, alloc: std.mem.Allocator): !(MysqlPacket?) {
        const avail = _.buf.items.len - _.read_pos;
        if (avail < 4) {
            return null;
        }
        const hdr = _.buf.items[_.read_pos .. _.read_pos + 4];
        const payload_len: usize = @as(usize, hdr[0]) |
            (@as(usize, hdr[1]) << 8) |
            (@as(usize, hdr[2]) << 16);
        if (payload_len > _.max_packet_bytes) {
            return error.PacketTooLarge;
        }
        const frame_len = 4 + payload_len;
        if (avail < frame_len) {
            return null;
        }
        const seq_id = hdr[3];
        const payload = try alloc.alloc(u8, payload_len);
        if (payload_len > 0) {
            @memcpy(payload, _.buf.items[_.read_pos + 4 .. _.read_pos + frame_len]);
        }
        _.read_pos += frame_len;
        if (_.read_pos == _.buf.items.len) {
            _.buf.clearRetainingCapacity();
            _.read_pos = 0;
        }
        return MysqlPacket{
            .sequence_id = seq_id,
            .payload = payload,
        };
    }
}

// --- Wire Auth (HandshakeV10 / HandshakeResponse41 / Native Password) ---
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

    pub static fn decode(alloc: std.mem.Allocator, payload: []const u8): !HandshakeResponse41 {
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

// --- Wire Protocol (OK/ERR/EOF, COM_*, ColumnDef41 & TextResultRow) ---
pub const COM_QUIT: u8 = 0x01;
pub const COM_QUERY: u8 = 0x03;
pub const COM_PING: u8 = 0x0E;

pub const OK_HEADER: u8 = 0x00;
pub const EOF_HEADER: u8 = 0xFE;
pub const ERR_HEADER: u8 = 0xFF;

pub struct OkPacket {
    affected_rows: u64,
    last_insert_id: u64,
    status_flags: u16,
    warnings: u16,
    info: []u8,

    pub fn deinit(alloc: std.mem.Allocator) {
        alloc.free(_.info);
    }

    pub fn encode(alloc: std.mem.Allocator): !([]u8) {
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(alloc);

        try out.append(alloc, OK_HEADER);
        try writeLenEncInt(alloc, &out, _.affected_rows);
        try writeLenEncInt(alloc, &out, _.last_insert_id);

        let st_buf: [2]u8 = undefined;
        std.mem.writeInt(u16, &st_buf, _.status_flags, .little);
        try out.appendSlice(alloc, &st_buf);

        let warn_buf: [2]u8 = undefined;
        std.mem.writeInt(u16, &warn_buf, _.warnings, .little);
        try out.appendSlice(alloc, &warn_buf);

        if (_.info.len > 0) {
            try out.appendSlice(alloc, _.info);
        }
        return try out.toOwnedSlice(alloc);
    }

    pub static fn decode(alloc: std.mem.Allocator, payload: []const u8): !OkPacket {
        if (payload.len < 7 or payload[0] != OK_HEADER) {
            return error.InvalidPacketType;
        }
        let offset: usize = 1;
        const aff = try readLenEncInt(payload, &offset);
        const ins = try readLenEncInt(payload, &offset);
        if (offset + 4 > payload.len) {
            return error.UnexpectedEof;
        }
        const status = std.mem.readInt(u16, payload[offset..][0..2], .little);
        offset += 2;
        const warnings = std.mem.readInt(u16, payload[offset..][0..2], .little);
        offset += 2;
        const info = try alloc.dupe(u8, payload[offset..]);

        return OkPacket{
            .affected_rows = aff.value,
            .last_insert_id = ins.value,
            .status_flags = status,
            .warnings = warnings,
            .info = info,
        };
    }
}

pub struct ErrPacket {
    error_code: u16,
    sql_state_marker: u8,
    sql_state: [5]u8,
    error_message: []u8,

    pub fn deinit(alloc: std.mem.Allocator) {
        alloc.free(_.error_message);
    }

    pub fn encode(alloc: std.mem.Allocator): !([]u8) {
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(alloc);

        try out.append(alloc, ERR_HEADER);
        let code_buf: [2]u8 = undefined;
        std.mem.writeInt(u16, &code_buf, _.error_code, .little);
        try out.appendSlice(alloc, &code_buf);

        try out.append(alloc, _.sql_state_marker);
        try out.appendSlice(alloc, &_.sql_state);
        try out.appendSlice(alloc, _.error_message);
        return try out.toOwnedSlice(alloc);
    }

    pub fn decode(alloc: std.mem.Allocator, payload: []const u8): !ErrPacket {
        if (payload.len < 3 or payload[0] != ERR_HEADER) {
            return error.InvalidPacketType;
        }
        let offset: usize = 1;
        const code = std.mem.readInt(u16, payload[offset..][0..2], .little);
        offset += 2;

        let marker: u8 = '#';
        let state: [5]u8 = ['H', 'Y', '0', '0', '0'];
        if (offset + 6 <= payload.len and payload[offset] == '#') {
            marker = payload[offset];
            offset += 1;
            @memcpy(&state, payload[offset .. offset + 5]);
            offset += 5;
        }
        const msg = try alloc.dupe(u8, payload[offset..]);

        return ErrPacket{
            .error_code = code,
            .sql_state_marker = marker,
            .sql_state = state,
            .error_message = msg,
        };
    }
}

pub struct EofPacket {
    warnings: u16,
    status_flags: u16,

    pub fn encode(alloc: std.mem.Allocator): !([]u8) {
        let out = try alloc.alloc(u8, 5);
        out[0] = EOF_HEADER;
        std.mem.writeInt(u16, out[1..3], _.warnings, .little);
        std.mem.writeInt(u16, out[3..5], _.status_flags, .little);
        return out;
    }

    pub fn decode(payload: []const u8): !EofPacket {
        if (payload.len < 5 or payload[0] != EOF_HEADER) {
            return error.InvalidPacketType;
        }
        return EofPacket{
            .warnings = std.mem.readInt(u16, payload[1..3], .little),
            .status_flags = std.mem.readInt(u16, payload[3..5], .little),
        };
    }
}

pub struct ComPacket {
    command: u8,
    argument: []u8,

    pub fn deinit(alloc: std.mem.Allocator) {
        alloc.free(_.argument);
    }

    pub fn encodeComQuery(alloc: std.mem.Allocator, sql: []const u8): !([]u8) {
        let out = try alloc.alloc(u8, 1 + sql.len);
        out[0] = COM_QUERY;
        if (sql.len > 0) {
            @memcpy(out[1..], sql);
        }
        return out;
    }

    pub fn encodeComPing(alloc: std.mem.Allocator): !([]u8) {
        let out = try alloc.alloc(u8, 1);
        out[0] = COM_PING;
        return out;
    }

    pub fn encodeComQuit(alloc: std.mem.Allocator): !([]u8) {
        let out = try alloc.alloc(u8, 1);
        out[0] = COM_QUIT;
        return out;
    }

    pub fn decode(alloc: std.mem.Allocator, payload: []const u8): !ComPacket {
        if (payload.len < 1) {
            return error.UnexpectedEof;
        }
        const arg = try alloc.dupe(u8, payload[1..]);
        return ComPacket{
            .command = payload[0],
            .argument = arg,
        };
    }
}

pub struct ColumnDef41 {
    catalog: []u8,
    schema: []u8,
    table: []u8,
    org_table: []u8,
    name: []u8,
    org_name: []u8,
    character_set: u16,
    column_length: u32,
    column_type: u8,
    flags: u16,
    decimals: u8,

    pub fn deinit(alloc: std.mem.Allocator) {
        alloc.free(_.catalog);
        alloc.free(_.schema);
        alloc.free(_.table);
        alloc.free(_.org_table);
        alloc.free(_.name);
        alloc.free(_.org_name);
    }

    pub fn encode(alloc: std.mem.Allocator): !([]u8) {
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(alloc);

        try writeLenEncStr(alloc, &out, _.catalog);
        try writeLenEncStr(alloc, &out, _.schema);
        try writeLenEncStr(alloc, &out, _.table);
        try writeLenEncStr(alloc, &out, _.org_table);
        try writeLenEncStr(alloc, &out, _.name);
        try writeLenEncStr(alloc, &out, _.org_name);

        try out.append(alloc, 0x0C);

        let cs_buf: [2]u8 = undefined;
        std.mem.writeInt(u16, &cs_buf, _.character_set, .little);
        try out.appendSlice(alloc, &cs_buf);

        let len_buf: [4]u8 = undefined;
        std.mem.writeInt(u32, &len_buf, _.column_length, .little);
        try out.appendSlice(alloc, &len_buf);

        try out.append(alloc, _.column_type);

        let fl_buf: [2]u8 = undefined;
        std.mem.writeInt(u16, &fl_buf, _.flags, .little);
        try out.appendSlice(alloc, &fl_buf);

        try out.append(alloc, _.decimals);
        try out.append(alloc, 0);
        try out.append(alloc, 0);

        return try out.toOwnedSlice(alloc);
    }

    pub static fn decode(alloc: std.mem.Allocator, payload: []const u8): !ColumnDef41 {
        let offset: usize = 0;
        const catalog_s = (try readLenEncStr(payload, &offset)) orelse "";
        const schema_s = (try readLenEncStr(payload, &offset)) orelse "";
        const table_s = (try readLenEncStr(payload, &offset)) orelse "";
        const org_table_s = (try readLenEncStr(payload, &offset)) orelse "";
        const name_s = (try readLenEncStr(payload, &offset)) orelse "";
        const org_name_s = (try readLenEncStr(payload, &offset)) orelse "";

        if (offset + 1 + 2 + 4 + 1 + 2 + 1 + 2 > payload.len) {
            return error.UnexpectedEof;
        }
        // fixed-length fields length (0x0C)
        offset += 1;

        const charset = std.mem.readInt(u16, payload[offset..][0..2], .little);
        offset += 2;
        const col_len = std.mem.readInt(u32, payload[offset..][0..4], .little);
        offset += 4;
        const col_type = payload[offset];
        offset += 1;
        const flags = std.mem.readInt(u16, payload[offset..][0..2], .little);
        offset += 2;
        const decimals = payload[offset];

        const catalog = try alloc.dupe(u8, catalog_s);
        errdefer alloc.free(catalog);
        const schema = try alloc.dupe(u8, schema_s);
        errdefer alloc.free(schema);
        const table = try alloc.dupe(u8, table_s);
        errdefer alloc.free(table);
        const org_table = try alloc.dupe(u8, org_table_s);
        errdefer alloc.free(org_table);
        const name = try alloc.dupe(u8, name_s);
        errdefer alloc.free(name);
        const org_name = try alloc.dupe(u8, org_name_s);

        return ColumnDef41{
            .catalog = catalog,
            .schema = schema,
            .table = table,
            .org_table = org_table,
            .name = name,
            .org_name = org_name,
            .character_set = charset,
            .column_length = col_len,
            .column_type = col_type,
            .flags = flags,
            .decimals = decimals,
        };
    }
}

pub type CellValue = ([]u8)?;

pub struct TextResultRow {
    values: []CellValue,

    pub fn deinit(alloc: std.mem.Allocator) {
        let i: usize = 0;
        while (i < _.values.len) {
            if (v := _.values[i]) {
                alloc.free(v);
            }
            i += 1;
        }
        alloc.free(_.values);
    }

    pub fn encode(alloc: std.mem.Allocator): !([]u8) {
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(alloc);

        let i: usize = 0;
        while (i < _.values.len) {
            if (v := _.values[i]) {
                try writeLenEncStr(alloc, &out, v);
            } else {
                try writeLenEncNull(alloc, &out);
            }
            i += 1;
        }
        return try out.toOwnedSlice(alloc);
    }

    pub fn decode(alloc: std.mem.Allocator, payload: []const u8, col_count: usize): !TextResultRow {
        let vals = try alloc.alloc(CellValue, col_count);
        let filled: usize = 0;
        errdefer {
            let j: usize = 0;
            while (j < filled) {
                if (v := vals[j]) {
                    alloc.free(v);
                }
                j += 1;
            }
            alloc.free(vals);
        }

        let offset: usize = 0;
        while (filled < col_count) {
            const maybe_slice = try readLenEncStr(payload, &offset);
            if (s := maybe_slice) {
                vals[filled] = try alloc.dupe(u8, s);
            } else {
                vals[filled] = null;
            }
            filled += 1;
        }

        return TextResultRow{ .values = vals };
    }
}
