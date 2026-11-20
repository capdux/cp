// Source: cp/lib/mysql_wire/protocol.cp (MySQL OK/ERR/EOF, COM_*, ColumnDefinition41 & TextResultRow)
const std = @import("std");
import {
    readLenEncInt,
    writeLenEncInt,
    writeLenEncNull,
    readLenEncStr,
    writeLenEncStr,
} from "./packet.cp";

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

    pub fn decode(alloc: std.mem.Allocator, payload: []const u8): !OkPacket {
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

    pub fn decode(alloc: std.mem.Allocator, payload: []const u8): !ColumnDef41 {
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
