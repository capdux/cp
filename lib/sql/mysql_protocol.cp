// Source: /data/dev/bun/src/sql/mysql/protocol/Auth.cp
// lib/sql/mysql_protocol.cp
// Complete MySQL 4.1+ / 8.0 Wire Protocol Codecs: Auth (Native/CachingSHA2/Switch),
// SSLRequest, ColumnDefinition41, ResultSetHeader, COM_STMT_PREPARE/EXECUTE & Binary Row Decoder

const std = @import("std");
import { MySqlPacketHeader, SqlParam, SqlType } from "./protocol.cp";
import { MySqlCommandType, MySqlLenEnc, MySqlLenEncStr } from "./mysql.cp";
import {
    AnyMySQLError,
    MySqlFieldType,
    MySqlValueDateTime,
    MySqlValueTime,
} from "./mysql_types.cp";

pub struct MySqlBufferReader {
    pub data: []const u8 = "",
    pub pos: usize = 0,

    pub static fn init(buf: []const u8): MySqlBufferReader {
        return MySqlBufferReader{ data: buf, pos: 0 };
    }

    pub fn remaining(): usize {
        if (_.pos >= _.data.len) return 0;
        return _.data.len - _.pos;
    }

    pub fn read(*_, count: usize): ([]const u8)! {
        if (_.pos + count > _.data.len) return error.ShortRead;
        const s = _.data[_.pos .. _.pos + count];
        _.pos += count;
        return s;
    }

    pub fn skip(*_, count: usize): ! {
        if (_.pos + count > _.data.len) return error.ShortRead;
        _.pos += count;
    }

    pub fn u8Val(*_): u8! {
        if (_.pos >= _.data.len) return error.ShortRead;
        const v = _.data[_.pos];
        _.pos += 1;
        return v;
    }

    pub fn u16Val(*_): u16! {
        const s = _.read(2)?;
        return (s[0] as u16) | ((s[1] as u16) << 8);
    }

    pub fn u24Val(*_): u32! {
        const s = _.read(3)?;
        return (s[0] as u32) | ((s[1] as u32) << 8) | ((s[2] as u32) << 16);
    }

    pub fn u32Val(*_): u32! {
        const s = _.read(4)?;
        return (s[0] as u32) | ((s[1] as u32) << 8) | ((s[2] as u32) << 16) | ((s[3] as u32) << 24);
    }

    pub fn u64Val(*_): u64! {
        const s = _.read(8)?;
        return (s[0] as u64) |
            ((s[1] as u64) << 8) |
            ((s[2] as u64) << 16) |
            ((s[3] as u64) << 24) |
            ((s[4] as u64) << 32) |
            ((s[5] as u64) << 40) |
            ((s[6] as u64) << 48) |
            ((s[7] as u64) << 56);
    }

    pub fn lenEncInt(*_): u64! {
        if (_.pos >= _.data.len) return error.ShortRead;
        const info = MySqlLenEnc.decode(_.data[_.pos..]) ?? return error.ShortRead;
        _.pos += info.bytes_used;
        return info.value;
    }

    pub fn lenEncString(*_): ([]const u8)?! {
        if (_.pos >= _.data.len) return error.ShortRead;
        const info = MySqlLenEncStr.decode(_.data[_.pos..]) ?? return error.ShortRead;
        _.pos += info.bytes_used;
        if (info.is_null) return null;
        return info.value;
    }
}

pub struct MySqlAuthNative {
    /// Computes MySQL `mysql_native_password` scramble: SHA1(password) XOR SHA1(nonce[0..20] || SHA1(SHA1(password))).
    pub static fn scramble(password: []const u8, nonce: []const u8): ([20]u8)! {
        let result: [20]u8 = [0; 20];
        if (password.len == 0) return result;
        if (nonce.len < 20) return error.MissingAuthData;

        let stage1: [20]u8 = undefined;
        std.crypto.hash.Sha1.hash(password, &stage1, .{});

        let stage2: [20]u8 = undefined;
        std.crypto.hash.Sha1.hash(&stage1, &stage2, .{});

        let hasher = std.crypto.hash.Sha1.init(.{});
        hasher.update(nonce[0..20]);
        hasher.update(&stage2);
        let stage3: [20]u8 = undefined;
        hasher.final(&stage3);

        let i: usize = 0;
        while (i < 20) : (i += 1) {
            result[i] = stage3[i] ^ stage1[i];
        }
        return result;
    }
}

pub enum MySqlFastAuthStatus: u8 {
    success = 0x03,
    continue_auth = 0x04,
    unknown = 0xFF,

    pub static fn fromByte(b: u8): MySqlFastAuthStatus {
        if (b == 0x03) return .success;
        if (b == 0x04) return .continue_auth;
        return .unknown;
    }
}

pub struct MySqlAuthCachingSha2 {
    /// Computes MySQL 8.0 `caching_sha2_password` scramble:
    /// XOR(SHA256(password), SHA256(SHA256(SHA256(password)) || nonce))
    pub static fn scramble(password: []const u8, nonce: []const u8): ([32]u8)! {
        let result: [32]u8 = [0; 32];
        if (password.len == 0) return result;
        if (nonce.len < 20) return error.MissingAuthData;

        let digest1: [32]u8 = undefined;
        std.crypto.hash.sha2.Sha256.hash(password, &digest1, .{});

        let digest2: [32]u8 = undefined;
        std.crypto.hash.sha2.Sha256.hash(&digest1, &digest2, .{});

        let hasher = std.crypto.hash.sha2.Sha256.init(.{});
        hasher.update(&digest2);
        hasher.update(nonce[0..20]);
        let digest3: [32]u8 = undefined;
        hasher.final(&digest3);

        let i: usize = 0;
        while (i < 32) : (i += 1) {
            result[i] = digest1[i] ^ digest3[i];
        }
        return result;
    }

    pub static fn encodePublicKeyRequest(out: []u8, seq_id: u8): usize {
        if (out.len < 5) return 0;
        * = MySqlPacketHeader.encode(out, 1, seq_id);
        out[4] = 0x02;
        return 5;
    }

    pub static fn parseFastAuthResponse(payload: []const u8): MySqlFastAuthStatus? {
        if (payload.len < 2 || payload[0] != 0x01) return null;
        return MySqlFastAuthStatus.fromByte(payload[1]);
    }
}

pub struct MySqlAuthSwitchRequest {
    pub plugin_name: []const u8 = "",
    pub plugin_data: []const u8 = "",

    /// Parses a MySQL `AuthSwitchRequest` payload (`0xFE` + plugin_name\0 + auth_plugin_data).
    pub static fn parse(payload: []const u8): MySqlAuthSwitchRequest? {
        if (payload.len < 2 || payload[0] != 0xFE) return null;
        const nul = std.mem.indexOfScalarPos(u8, payload, 1, 0) ?? return null;
        const name = payload[1..nul];
        let data: []const u8 = "";
        if (nul + 1 < payload.len) {
            let end = payload.len;
            if (end > nul + 1 && payload[end - 1] == 0) end -= 1;
            data = payload[nul + 1 .. end];
        }
        return MySqlAuthSwitchRequest{
            plugin_name: name,
            plugin_data: data,
        };
    }

    pub static fn encodeResponse(out: []u8, auth_data: []const u8, seq_id: u8): usize {
        if (out.len < 4 + auth_data.len) return 0;
        * = MySqlPacketHeader.encode(out, auth_data.len as u32, seq_id);
        if (auth_data.len > 0) {
            @memcpy(out[4 .. 4 + auth_data.len], auth_data);
        }
        return 4 + auth_data.len;
    }
}

pub struct MySqlSSLRequest {
    pub static fn encode(out: []u8, caps: u32, max_packet_size: u32, charset: u8, seq_id: u8 = 1): usize {
        if (out.len < 36) return 0;
        * = MySqlPacketHeader.encode(out, 32, seq_id);
        out[4] = (caps & 0xFF) as u8;
        out[5] = ((caps >> 8) & 0xFF) as u8;
        out[6] = ((caps >> 16) & 0xFF) as u8;
        out[7] = ((caps >> 24) & 0xFF) as u8;
        out[8] = (max_packet_size & 0xFF) as u8;
        out[9] = ((max_packet_size >> 8) & 0xFF) as u8;
        out[10] = ((max_packet_size >> 16) & 0xFF) as u8;
        out[11] = ((max_packet_size >> 24) & 0xFF) as u8;
        out[12] = charset;
        let i: usize = 13;
        while (i < 36) : (i += 1) out[i] = 0;
        return 36;
    }
}

pub struct MySqlColumnFlags {
    pub raw: u16 = 0,

    pub const NOT_NULL: u16 = 1 << 0;
    pub const PRI_KEY: u16 = 1 << 1;
    pub const UNIQUE_KEY: u16 = 1 << 2;
    pub const MULTIPLE_KEY: u16 = 1 << 3;
    pub const BLOB: u16 = 1 << 4;
    pub const UNSIGNED: u16 = 1 << 5;
    pub const ZEROFILL: u16 = 1 << 6;
    pub const BINARY: u16 = 1 << 7;
    pub const ENUM: u16 = 1 << 8;
    pub const AUTO_INCREMENT: u16 = 1 << 9;
    pub const TIMESTAMP: u16 = 1 << 10;
    pub const SET: u16 = 1 << 11;

    pub fn isNotNull(): bool { return (_.raw & NOT_NULL) != 0; }
    pub fn isPrimaryKey(): bool { return (_.raw & PRI_KEY) != 0; }
    pub fn isUnsigned(): bool { return (_.raw & UNSIGNED) != 0; }
    pub fn isBinary(): bool { return (_.raw & BINARY) != 0; }
}

pub struct MySqlColumnDefinition41 {
    pub catalog: []const u8 = "",
    pub schema: []const u8 = "",
    pub table: []const u8 = "",
    pub org_table: []const u8 = "",
    pub name: []const u8 = "",
    pub org_name: []const u8 = "",
    pub character_set: u16 = 0,
    pub column_length: u32 = 0,
    pub column_type: MySqlFieldType = .MYSQL_TYPE_NULL,
    pub flags: MySqlColumnFlags = MySqlColumnFlags{},
    pub decimals: u8 = 0,

    /// Parses a MySQL 4.1 `ColumnDefinition41` packet payload.
    pub static fn parse(payload: []const u8): MySqlColumnDefinition41? {
        let r = MySqlBufferReader.init(payload);
        const cat = (r.lenEncString() catch return null) ?? "";
        const sch = (r.lenEncString() catch return null) ?? "";
        const tbl = (r.lenEncString() catch return null) ?? "";
        const org_tbl = (r.lenEncString() catch return null) ?? "";
        const col_name = (r.lenEncString() catch return null) ?? "";
        const org_col_name = (r.lenEncString() catch return null) ?? "";
        * = r.lenEncInt() catch return null; // length of fixed-length fields (0x0C)
        const cset = r.u16Val() catch return null;
        const col_len = r.u32Val() catch return null;
        const col_type_b = r.u8Val() catch return null;
        const flg = r.u16Val() catch return null;
        const decs = r.u8Val() catch return null;
        return MySqlColumnDefinition41{
            catalog: cat,
            schema: sch,
            table: tbl,
            org_table: org_tbl,
            name: col_name,
            org_name: org_col_name,
            character_set: cset,
            column_length: col_len,
            column_type: MySqlFieldType.fromByte(col_type_b),
            flags: MySqlColumnFlags{ raw: flg },
            decimals: decs,
        };
    }
}

pub struct MySqlResultSetHeader {
    pub column_count: u64 = 0,
    pub metadata_follows: bool = true,

    pub static fn parse(payload: []const u8): MySqlResultSetHeader? {
        let r = MySqlBufferReader.init(payload);
        const cnt = r.lenEncInt() catch return null;
        let follows = true;
        if (r.remaining() >= 1) {
            const b = r.u8Val() catch 1;
            follows = b != 0;
        }
        return MySqlResultSetHeader{
            column_count: cnt,
            metadata_follows: follows,
        };
    }
}

pub struct MySqlPrepareOK {
    pub status: u8 = 0,
    pub statement_id: u32 = 0,
    pub num_columns: u16 = 0,
    pub num_params: u16 = 0,
    pub warning_count: u16 = 0,

    /// Parses a MySQL `COM_STMT_PREPARE_OK` response payload (`0x00` + 4B stmt_id + 2B cols + 2B params + 1B filler + 2B warnings).
    pub static fn parse(payload: []const u8): MySqlPrepareOK? {
        let r = MySqlBufferReader.init(payload);
        const st = r.u8Val() catch return null;
        if (st != 0) return null;
        const sid = r.u32Val() catch return null;
        const ncols = r.u16Val() catch return null;
        const nparams = r.u16Val() catch return null;
        r.skip(1) catch return null;
        const warns = r.u16Val() catch 0;
        return MySqlPrepareOK{
            status: st,
            statement_id: sid,
            num_columns: ncols,
            num_params: nparams,
            warning_count: warns,
        };
    }
}

pub struct MySqlPreparedProtocol {
    pub static fn encodePrepare(out: []u8, sql: []const u8, seq_id: u8 = 0): usize {
        const payload_len = 1 + sql.len;
        if (out.len < 4 + payload_len) return 0;
        * = MySqlPacketHeader.encode(out, payload_len as u32, seq_id);
        out[4] = @intFromEnum(MySqlCommandType.COM_STMT_PREPARE);
        if (sql.len > 0) @memcpy(out[5 .. 5 + sql.len], sql);
        return 4 + payload_len;
    }

    pub static fn encodeClose(out: []u8, statement_id: u32, seq_id: u8 = 0): usize {
        if (out.len < 9) return 0;
        * = MySqlPacketHeader.encode(out, 5, seq_id);
        out[4] = @intFromEnum(MySqlCommandType.COM_STMT_CLOSE);
        out[5] = (statement_id & 0xFF) as u8;
        out[6] = ((statement_id >> 8) & 0xFF) as u8;
        out[7] = ((statement_id >> 16) & 0xFF) as u8;
        out[8] = ((statement_id >> 24) & 0xFF) as u8;
        return 9;
    }

    /// Encodes a MySQL `COM_STMT_EXECUTE` (0x17) packet with NULL bitmap, param type headers & binary/lenenc values.
    pub static fn encodeExecute(
        out: []u8,
        statement_id: u32,
        flags: u8,
        params: []const SqlParam,
        seq_id: u8 = 0,
    ): usize {
        if (out.len < 15) return 0;
        let pos: usize = 4;
        out[pos] = @intFromEnum(MySqlCommandType.COM_STMT_EXECUTE);
        pos += 1;
        out[pos] = (statement_id & 0xFF) as u8;
        out[pos + 1] = ((statement_id >> 8) & 0xFF) as u8;
        out[pos + 2] = ((statement_id >> 16) & 0xFF) as u8;
        out[pos + 3] = ((statement_id >> 24) & 0xFF) as u8;
        pos += 4;
        out[pos] = flags;
        pos += 1;
        // iteration_count = 1
        out[pos] = 1;
        out[pos + 1] = 0;
        out[pos + 2] = 0;
        out[pos + 3] = 0;
        pos += 4;

        if (params.len > 0) {
            const bitmap_bytes = (params.len + 7) / 8;
            if (pos + bitmap_bytes + 1 + params.len * 2 > out.len) return 0;
            @memset(out[pos .. pos + bitmap_bytes], 0);
            let i: usize = 0;
            while (i < params.len) : (i += 1) {
                if (params[i].kind == .Null) {
                    const bit_sh: u3 = @intCast(i & 7);
                    out[pos + (i >> 3)] |= (1 as u8) << bit_sh;
                }
            }
            pos += bitmap_bytes;

            // new_params_bind_flag = 1
            out[pos] = 1;
            pos += 1;

            for (p of params) {
                const ft: u8 = switch (p.kind) {
                    .Null => @intFromEnum(MySqlFieldType.MYSQL_TYPE_NULL),
                    .Int64 => @intFromEnum(MySqlFieldType.MYSQL_TYPE_LONGLONG),
                    .Float64 => @intFromEnum(MySqlFieldType.MYSQL_TYPE_DOUBLE),
                    .Boolean => @intFromEnum(MySqlFieldType.MYSQL_TYPE_TINY),
                    .Text => @intFromEnum(MySqlFieldType.MYSQL_TYPE_VAR_STRING),
                    .Blob => @intFromEnum(MySqlFieldType.MYSQL_TYPE_BLOB),
                };
                out[pos] = ft;
                out[pos + 1] = 0; // signed
                pos += 2;
            }

            for (p of params) {
                if (p.kind == .Null) continue;
                if (p.kind == .Boolean) {
                    if (pos + 1 > out.len) return 0;
                    out[pos] = if (p.bool_val) 1 else 0;
                    pos += 1;
                } else if (p.kind == .Int64) {
                    if (pos + 8 > out.len) return 0;
                    const u: u64 = @bitCast(p.int_val);
                    let k: usize = 0;
                    while (k < 8) : (k += 1) {
                        const shift: u6 = @intCast(k * 8);
                        out[pos + k] = ((u >> shift) & 0xFF) as u8;
                    }
                    pos += 8;
                } else if (p.kind == .Float64) {
                    if (pos + 8 > out.len) return 0;
                    const u: u64 = @bitCast(p.float_val);
                    let k: usize = 0;
                    while (k < 8) : (k += 1) {
                        const shift: u6 = @intCast(k * 8);
                        out[pos + k] = ((u >> shift) & 0xFF) as u8;
                    }
                    pos += 8;
                } else if (p.kind == .Text) {
                    const n = MySqlLenEncStr.encode(out[pos..], p.text_val);
                    if (n == 0) return 0;
                    pos += n;
                } else if (p.kind == .Blob) {
                    const n = MySqlLenEncStr.encode(out[pos..], p.blob_val);
                    if (n == 0) return 0;
                    pos += n;
                }
            }
        }

        * = MySqlPacketHeader.encode(out, (pos - 4) as u32, seq_id);
        return pos;
    }
}

pub enum MySqlDecodedKind: u8 {
    Null = 0,
    Int64 = 1,
    UInt64 = 2,
    Float64 = 3,
    Bool = 4,
    String = 5,
    Blob = 6,
    DateTime = 7,
    Time = 8,
}

pub struct MySqlDecodedCell {
    pub kind: MySqlDecodedKind = .Null,
    pub int_val: i64 = 0,
    pub uint_val: u64 = 0,
    pub float_val: f64 = 0.0,
    pub bool_val: bool = false,
    pub bytes_val: []const u8 = "",
    pub datetime_val: MySqlValueDateTime = MySqlValueDateTime{},
    pub time_val: MySqlValueTime = MySqlValueTime{},
}

pub struct MySqlBinaryRowDecoder {
    /// Decodes a MySQL Binary Protocol Resultset Row (`0x00` + NULL bitmap `(N+9)/8` bytes + binary column values).
    pub static fn decodeRow(
        payload: []const u8,
        cols: []const MySqlColumnDefinition41,
        out_cells: []MySqlDecodedCell,
    ): usize! {
        if (payload.len == 0 || payload[0] != 0x00) return error.InvalidBinaryValue;
        const n = cols.len;
        if (out_cells.len < n) return error.BufferTooSmall;
        const bitmap_len = (n + 7 + 2) / 8;
        if (payload.len < 1 + bitmap_len) return error.ShortRead;

        const bitmap = payload[1 .. 1 + bitmap_len];
        let r = MySqlBufferReader.init(payload[1 + bitmap_len ..]);

        let i: usize = 0;
        while (i < n) : (i += 1) {
            const bit_pos = i + 2;
            const bit_sh: u3 = @intCast(bit_pos & 7);
            const is_null = (bitmap[bit_pos >> 3] & ((1 as u8) << bit_sh)) != 0;
            if (is_null) {
                out_cells[i] = MySqlDecodedCell{ kind: .Null };
                continue;
            }
            const col = cols[i];
            const unsigned = col.flags.isUnsigned();
            switch (col.column_type) {
                .MYSQL_TYPE_TINY => {
                    const b = r.u8Val()?;
                    if (unsigned) {
                        out_cells[i] = MySqlDecodedCell{ kind: .UInt64, uint_val: b as u64 };
                    } else {
                        const iv: i8 = @bitCast(b);
                        out_cells[i] = MySqlDecodedCell{ kind: .Int64, int_val: iv as i64 };
                    }
                },
                .MYSQL_TYPE_SHORT => {
                    const v = r.u16Val()?;
                    if (unsigned) {
                        out_cells[i] = MySqlDecodedCell{ kind: .UInt64, uint_val: v as u64 };
                    } else {
                        out_cells[i] = MySqlDecodedCell{ kind: .Int64, int_val: (@as(i16, @bitCast(v))) as i64 };
                    }
                },
                .MYSQL_TYPE_LONG => {
                    const v = r.u32Val()?;
                    if (unsigned) {
                        out_cells[i] = MySqlDecodedCell{ kind: .UInt64, uint_val: v as u64 };
                    } else {
                        out_cells[i] = MySqlDecodedCell{ kind: .Int64, int_val: (@as(i32, @bitCast(v))) as i64 };
                    }
                },
                .MYSQL_TYPE_LONGLONG => {
                    const v = r.u64Val()?;
                    if (unsigned) {
                        out_cells[i] = MySqlDecodedCell{ kind: .UInt64, uint_val: v };
                    } else {
                        out_cells[i] = MySqlDecodedCell{ kind: .Int64, int_val: @bitCast(v) };
                    }
                },
                .MYSQL_TYPE_FLOAT => {
                    const u = r.u32Val()?;
                    const f: f32 = @bitCast(u);
                    out_cells[i] = MySqlDecodedCell{ kind: .Float64, float_val: f as f64 };
                },
                .MYSQL_TYPE_DOUBLE => {
                    const u = r.u64Val()?;
                    const f: f64 = @bitCast(u);
                    out_cells[i] = MySqlDecodedCell{ kind: .Float64, float_val: f };
                },
                .MYSQL_TYPE_DATE, .MYSQL_TYPE_DATETIME, .MYSQL_TYPE_TIMESTAMP => {
                    const len_b = r.u8Val()?;
                    const raw = r.read(len_b as usize)?;
                    const dt = MySqlValueDateTime.fromBinary(raw)?;
                    out_cells[i] = MySqlDecodedCell{ kind: .DateTime, datetime_val: dt };
                },
                .MYSQL_TYPE_TIME => {
                    const len_b = r.u8Val()?;
                    const raw = r.read(len_b as usize)?;
                    const tv = MySqlValueTime.fromBinary(raw)?;
                    out_cells[i] = MySqlDecodedCell{ kind: .Time, time_val: tv };
                },
                .MYSQL_TYPE_BIT => {
                    const raw = (r.lenEncString()?) ?? "";
                    if (col.column_length == 1) {
                        out_cells[i] = MySqlDecodedCell{ kind: .Bool, bool_val: raw.len > 0 && raw[0] == 1 };
                    } else {
                        out_cells[i] = MySqlDecodedCell{ kind: .Blob, bytes_val: raw };
                    }
                },
                else => {
                    const raw = (r.lenEncString()?) ?? "";
                    if (col.flags.isBinary() && col.character_set == 63) {
                        out_cells[i] = MySqlDecodedCell{ kind: .Blob, bytes_val: raw };
                    } else {
                        out_cells[i] = MySqlDecodedCell{ kind: .String, bytes_val: raw };
                    }
                },
            }
        }
        return n;
    }
}
