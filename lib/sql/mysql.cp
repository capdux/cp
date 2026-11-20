// Source: /data/dev/bun/src/sql/
// lib/sql/mysql.cp
// MySQL Wire Protocol: Length-Encoded Integer/String, HandshakeV10, Command Encoders,
// OK_Packet, ERR_Packet, EOF_Packet & Capabilities
// Absorbed from Bun src/sql/mysql/Capabilities.cp & src/sql/mysql/protocol/

const std = @import("std");
import { MySqlPacketHeader } from "./protocol";

pub enum MySqlCommandType: u8 {
    COM_QUIT = 0x01,
    COM_INIT_DB = 0x02,
    COM_QUERY = 0x03,
    COM_PING = 0x0E,
    COM_STMT_PREPARE = 0x16,
    COM_STMT_EXECUTE = 0x17,
    COM_STMT_CLOSE = 0x19,
}

pub struct MySqlLenEnc {
    pub value: u64 = 0;
    pub bytes_used: usize = 0;

    /// Encodes a u64 as a MySQL Length-Encoded Integer into `out`.
    pub static fn encode(out: []u8, val: u64): usize {
        if (val < 251) {
            if (out.len < 1) return 0;
            out[0] = (val & 0xFF) as u8;
            return 1;
        }
        if (val < 65536) {
            if (out.len < 3) return 0;
            out[0] = 0xFC;
            out[1] = (val & 0xFF) as u8;
            out[2] = ((val >> 8) & 0xFF) as u8;
            return 3;
        }
        if (val < 16777216) {
            if (out.len < 4) return 0;
            out[0] = 0xFD;
            out[1] = (val & 0xFF) as u8;
            out[2] = ((val >> 8) & 0xFF) as u8;
            out[3] = ((val >> 16) & 0xFF) as u8;
            return 4;
        }
        if (out.len < 9) return 0;
        out[0] = 0xFE;
        out[1] = (val & 0xFF) as u8;
        out[2] = ((val >> 8) & 0xFF) as u8;
        out[3] = ((val >> 16) & 0xFF) as u8;
        out[4] = ((val >> 24) & 0xFF) as u8;
        out[5] = ((val >> 32) & 0xFF) as u8;
        out[6] = ((val >> 40) & 0xFF) as u8;
        out[7] = ((val >> 48) & 0xFF) as u8;
        out[8] = ((val >> 56) & 0xFF) as u8;
        return 9;
    }

    /// Decodes a MySQL Length-Encoded Integer from `buf`.
    pub static fn decode(buf: []const u8): MySqlLenEnc? {
        if (buf.len == 0) return null;
        const first = buf[0];
        if (first < 0xFB) {
            return MySqlLenEnc{
                value: first as u64,
                bytes_used: 1,
            };
        }
        if (first == 0xFC) {
            if (buf.len < 3) return null;
            const v = (buf[1] as u64) | ((buf[2] as u64) << 8);
            return MySqlLenEnc{
                value: v,
                bytes_used: 3,
            };
        }
        if (first == 0xFD) {
            if (buf.len < 4) return null;
            const v = (buf[1] as u64) | ((buf[2] as u64) << 8) | ((buf[3] as u64) << 16);
            return MySqlLenEnc{
                value: v,
                bytes_used: 4,
            };
        }
        if (first == 0xFE) {
            if (buf.len < 9) return null;
            const v = (buf[1] as u64) |
                ((buf[2] as u64) << 8) |
                ((buf[3] as u64) << 16) |
                ((buf[4] as u64) << 24) |
                ((buf[5] as u64) << 32) |
                ((buf[6] as u64) << 40) |
                ((buf[7] as u64) << 48) |
                ((buf[8] as u64) << 56);
            return MySqlLenEnc{
                value: v,
                bytes_used: 9,
            };
        }
        return null;
    }
}

pub struct MySqlLenEncStr {
    pub value: []const u8 = "",
    pub is_null: bool = false,
    pub bytes_used: usize = 0,

    pub static fn encode(out: []u8, s: []const u8): usize {
        const len_bytes = MySqlLenEnc.encode(out, s.len as u64);
        if (len_bytes == 0 || out.len < len_bytes + s.len) return 0;
        if (s.len > 0) {
            @memcpy(out[len_bytes .. len_bytes + s.len], s);
        }
        return len_bytes + s.len;
    }

    pub static fn decode(buf: []const u8): MySqlLenEncStr? {
        if (buf.len == 0) return null;
        if (buf[0] == 0xFB) {
            return MySqlLenEncStr{
                value: "",
                is_null: true,
                bytes_used: 1,
            };
        }
        const len_info = MySqlLenEnc.decode(buf) ?? return null;
        const slen = len_info.value as usize;
        const total = len_info.bytes_used + slen;
        if (buf.len < total) return null;
        return MySqlLenEncStr{
            value: buf[len_info.bytes_used .. total],
            is_null: false,
            bytes_used: total,
        };
    }
}

pub struct MySqlCapabilities {
    pub flags: u32 = 0;

    pub static fn flagLongPassword(): u32 { return 1; }
    pub static fn flagFoundRows(): u32 { return 2; }
    pub static fn flagLongFlag(): u32 { return 4; }
    pub static fn flagConnectWithDb(): u32 { return 8; }
    pub static fn flagProtocol41(): u32 { return 512; }
    pub static fn flagSsl(): u32 { return 2048; }
    pub static fn flagTransactions(): u32 { return 8192; }
    pub static fn flagSecureConnection(): u32 { return 32768; }
    pub static fn flagMultiStatements(): u32 { return 65536; }
    pub static fn flagMultiResults(): u32 { return 131072; }
    pub static fn flagPluginAuth(): u32 { return 524288; }
    pub static fn flagDeprecateEof(): u32 { return 16777216; }

    pub static fn defaultClient(): MySqlCapabilities {
        return MySqlCapabilities{
            flags: 1 | 4 | 8 | 512 | 8192 | 32768 | 131072 | 524288 | 16777216,
        };
    }

    pub fn has(flag: u32): bool {
        return (_.flags & flag) != 0;
    }

    pub fn enable(*_, flag: u32) {
        _.flags = _.flags | flag;
    }

    pub fn disable(*_, flag: u32) {
        _.flags = _.flags & (~flag);
    }
}

pub struct MySqlHandshakeV10 {
    pub protocol_version: u8 = 10,
    pub server_version: []const u8 = "",
    pub connection_id: u32 = 0,
    pub auth_plugin_data_part_1: [8]u8 = undefined,
    pub auth_plugin_data_part_2: []const u8 = "",
    pub capability_flags: u32 = 0,
    pub character_set: u8 = 0,
    pub status_flags: u16 = 0,
    pub auth_plugin_name: []const u8 = "",

    /// Parses a MySQL Initial Handshake v10 packet payload (after the 4-byte MySqlPacketHeader).
    pub static fn parse(payload: []const u8): MySqlHandshakeV10? {
        if (payload.len < 4 || payload[0] != 10) return null;
        const nul = std.mem.indexOfScalarPos(u8, payload, 1, 0) ?? return null;
        const server_ver = payload[1..nul];
        let pos = nul + 1;

        // 4B connection_id + 8B auth_part_1 + 1B filler + 2B cap_low + 1B charset + 2B status + 2B cap_high + 1B auth_len + 10B reserved = 31 bytes
        if (pos + 31 > payload.len) return null;

        const conn_id = (payload[pos] as u32) |
            ((payload[pos + 1] as u32) << 8) |
            ((payload[pos + 2] as u32) << 16) |
            ((payload[pos + 3] as u32) << 24);
        pos += 4;

        let part1: [8]u8 = undefined;
        @memcpy(&part1, payload[pos .. pos + 8]);
        pos += 8;

        pos += 1; // filler 0x00

        const cap_low = (payload[pos] as u32) | ((payload[pos + 1] as u32) << 8);
        pos += 2;
        const charset = payload[pos];
        pos += 1;
        const status = (payload[pos] as u16) | ((payload[pos + 1] as u16) << 8);
        pos += 2;
        const cap_high = (payload[pos] as u32) | ((payload[pos + 1] as u32) << 8);
        pos += 2;
        const caps = (cap_high << 16) | cap_low;

        let auth_len = payload[pos] as usize;
        pos += 1;
        pos += 10; // 10 reserved zero bytes

        let part2: []const u8 = "";
        if ((caps & MySqlCapabilities.flagSecureConnection()) != 0) {
            const rem_len: usize = if (auth_len > 8) @max(13, auth_len - 8) else 13;
            if (pos + rem_len > payload.len) return null;
            // Strip trailing NUL from scramble if present
            let actual = rem_len;
            if (actual > 0 && payload[pos + actual - 1] == 0) {
                actual -= 1;
            }
            part2 = payload[pos .. pos + actual];
            pos += rem_len;
        }

        let plugin_name: []const u8 = "";
        if ((caps & MySqlCapabilities.flagPluginAuth()) != 0 && pos < payload.len) {
            const p_nul = std.mem.indexOfScalarPos(u8, payload, pos, 0) ?? payload.len;
            plugin_name = payload[pos..p_nul];
        }

        return MySqlHandshakeV10{
            protocol_version: 10,
            server_version: server_ver,
            connection_id: conn_id,
            auth_plugin_data_part_1: part1,
            auth_plugin_data_part_2: part2,
            capability_flags: caps,
            character_set: charset,
            status_flags: status,
            auth_plugin_name: plugin_name,
        };
    }

    pub fn copyScramble(out: *[20]u8): usize {
        @memcpy(out[0..8], _.auth_plugin_data_part_1[0..8]);
        const p2_len: usize = @min(12 as usize, _.auth_plugin_data_part_2.len);
        if (p2_len > 0) {
            @memcpy(out[8 .. 8 + p2_len], _.auth_plugin_data_part_2[0..p2_len]);
        }
        return 8 + p2_len;
    }
}

pub struct MySqlOkPacket {
    pub affected_rows: u64 = 0;
    pub last_insert_id: u64 = 0;
    pub status_flags: u16 = 0;
    pub warnings: u16 = 0;

    /// Parses a MySQL 4.1+ `OK_Packet` payload (header byte `0x00` or `0xFE`).
    pub static fn parse(payload: []const u8): MySqlOkPacket? {
        if (payload.len < 7 || (payload[0] != 0x00 && payload[0] != 0xFE)) return null;
        let pos: usize = 1;
        const aff = MySqlLenEnc.decode(payload[pos..]) ?? return null;
        pos += aff.bytes_used;

        const lid = MySqlLenEnc.decode(payload[pos..]) ?? return null;
        pos += lid.bytes_used;

        let status: u16 = 0;
        let warn: u16 = 0;
        if (pos + 2 <= payload.len) {
            status = (payload[pos] as u16) | ((payload[pos + 1] as u16) << 8);
            pos += 2;
        }
        if (pos + 2 <= payload.len) {
            warn = (payload[pos] as u16) | ((payload[pos + 1] as u16) << 8);
        }

        return MySqlOkPacket{
            affected_rows: aff.value,
            last_insert_id: lid.value,
            status_flags: status,
            warnings: warn,
        };
    }
}

pub struct MySqlErrPacket {
    pub error_code: u16 = 0;
    pub sql_state: []const u8 = "";
    pub message: []const u8 = "";

    /// Parses a MySQL 4.1+ `ERR_Packet` payload (header byte `0xFF`).
    pub static fn parse(payload: []const u8): MySqlErrPacket? {
        if (payload.len < 3 || payload[0] != 0xFF) return null;
        const code = (payload[1] as u16) | ((payload[2] as u16) << 8);
        if (payload.len >= 9 && payload[3] == '#') {
            return MySqlErrPacket{
                error_code: code,
                sql_state: payload[4..9],
                message: payload[9..payload.len],
            };
        }
        return MySqlErrPacket{
            error_code: code,
            sql_state: "HY000",
            message: payload[3..payload.len],
        };
    }
}

pub struct MySqlEofPacket {
    pub warnings: u16 = 0;
    pub status_flags: u16 = 0;

    /// Parses a MySQL 4.1 `EOF_Packet` payload (header byte `0xFE`, length < 9).
    pub static fn parse(payload: []const u8): MySqlEofPacket? {
        if (payload.len < 5 || payload.len >= 9 || payload[0] != 0xFE) return null;
        const warnings = (payload[1] as u16) | ((payload[2] as u16) << 8);
        const status = (payload[3] as u16) | ((payload[4] as u16) << 8);
        return MySqlEofPacket{
            warnings: warnings,
            status_flags: status,
        };
    }
}

pub struct MySqlCommandEncoder {
    /// Encodes a `COM_QUERY` (0x03) packet with 4-byte MySqlPacketHeader.
    pub static fn encodeQuery(out: []u8, sql: []const u8, seq_id: u8 = 0): usize {
        const payload_len = 1 + sql.len;
        if (out.len < 4 + payload_len) return 0;
        * = MySqlPacketHeader.encode(out, payload_len as u32, seq_id);
        out[4] = MySqlCommandType.COM_QUERY as u8;
        if (sql.len > 0) {
            @memcpy(out[5 .. 5 + sql.len], sql);
        }
        return 4 + payload_len;
    }

    /// Encodes a `COM_PING` (0x0E) packet with 4-byte MySqlPacketHeader.
    pub static fn encodePing(out: []u8, seq_id: u8 = 0): usize {
        if (out.len < 5) return 0;
        * = MySqlPacketHeader.encode(out, 1, seq_id);
        out[4] = MySqlCommandType.COM_PING as u8;
        return 5;
    }

    /// Encodes a `COM_QUIT` (0x01) packet with 4-byte MySqlPacketHeader.
    pub static fn encodeQuit(out: []u8, seq_id: u8 = 0): usize {
        if (out.len < 5) return 0;
        * = MySqlPacketHeader.encode(out, 1, seq_id);
        out[4] = MySqlCommandType.COM_QUIT as u8;
        return 5;
    }

    /// Encodes a `COM_INIT_DB` (0x02) packet with 4-byte MySqlPacketHeader.
    pub static fn encodeInitDb(out: []u8, database: []const u8, seq_id: u8 = 0): usize {
        const payload_len = 1 + database.len;
        if (out.len < 4 + payload_len) return 0;
        * = MySqlPacketHeader.encode(out, payload_len as u32, seq_id);
        out[4] = MySqlCommandType.COM_INIT_DB as u8;
        if (database.len > 0) {
            @memcpy(out[5 .. 5 + database.len], database);
        }
        return 4 + payload_len;
    }

    /// Encodes a `HandshakeResponse41` client authentication response packet.
    pub static fn encodeHandshakeResponse41(
        out: []u8,
        caps: u32,
        max_packet_size: u32,
        charset: u8,
        user: []const u8,
        auth_response: []const u8,
        database: []const u8,
        plugin_name: []const u8,
        seq_id: u8 = 1,
    ): usize {
        if (out.len < 40) return 0;
        let pos: usize = 4;
        // 4B capability_flags LE
        out[pos] = (caps & 0xFF) as u8;
        out[pos + 1] = ((caps >> 8) & 0xFF) as u8;
        out[pos + 2] = ((caps >> 16) & 0xFF) as u8;
        out[pos + 3] = ((caps >> 24) & 0xFF) as u8;
        pos += 4;
        // 4B max_packet_size LE
        out[pos] = (max_packet_size & 0xFF) as u8;
        out[pos + 1] = ((max_packet_size >> 8) & 0xFF) as u8;
        out[pos + 2] = ((max_packet_size >> 16) & 0xFF) as u8;
        out[pos + 3] = ((max_packet_size >> 24) & 0xFF) as u8;
        pos += 4;
        // 1B character_set + 23B filler
        out[pos] = charset;
        pos += 1;
        let f: usize = 0;
        while (f < 23) : (f += 1) {
            out[pos + f] = 0;
        }
        pos += 23;

        // user\0
        if (pos + user.len + 1 > out.len) return 0;
        if (user.len > 0) {
            @memcpy(out[pos .. pos + user.len], user);
            pos += user.len;
        }
        out[pos] = 0;
        pos += 1;

        // auth_response (1B length + bytes)
        if (pos + 1 + auth_response.len > out.len) return 0;
        out[pos] = (auth_response.len & 0xFF) as u8;
        pos += 1;
        if (auth_response.len > 0) {
            @memcpy(out[pos .. pos + auth_response.len], auth_response);
            pos += auth_response.len;
        }

        // optional database\0 if CLIENT_CONNECT_WITH_DB
        if ((caps & MySqlCapabilities.flagConnectWithDb()) != 0 && database.len > 0) {
            if (pos + database.len + 1 > out.len) return 0;
            @memcpy(out[pos .. pos + database.len], database);
            pos += database.len;
            out[pos] = 0;
            pos += 1;
        }

        // optional plugin_name\0 if CLIENT_PLUGIN_AUTH
        if ((caps & MySqlCapabilities.flagPluginAuth()) != 0 && plugin_name.len > 0) {
            if (pos + plugin_name.len + 1 > out.len) return 0;
            @memcpy(out[pos .. pos + plugin_name.len], plugin_name);
            pos += plugin_name.len;
            out[pos] = 0;
            pos += 1;
        }

        * = MySqlPacketHeader.encode(out, (pos - 4) as u32, seq_id);
        return pos;
    }
}
