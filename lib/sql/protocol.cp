// Source: /data/dev/bun/src/sql/
// lib/sql/protocol.cp
// Unified SQL Parameterized Statement, Postgres v3 Wire Frame & MySQL Packet Protocol
// Absorbed from Bun src/sql/postgres/, src/sql/mysql/ & src/sql/shared/

const std = @import("std");

pub enum SqlDialect: u8 {
    Postgres = 0,
    MySql = 1,
    Sqlite = 2,
}

pub enum SqlType: u8 {
    Null = 0,
    Int64 = 1,
    Float64 = 2,
    Text = 3,
    Blob = 4,
    Boolean = 5,
}

pub struct SqlParam {
    pub kind: SqlType = SqlType.Null;
    pub int_val: i64 = 0;
    pub text_val: []const u8 = "";
    pub bool_val: bool = false;
}

pub struct SqlStatement {
    pub query: []const u8 = "";
    pub dialect: SqlDialect = SqlDialect.Postgres;
    pub params: [32]SqlParam = undefined;
    pub param_count: usize = 0;

    pub static fn prepare(query: []const u8, dialect: SqlDialect): SqlStatement {
        return SqlStatement {
            query: query,
            dialect: dialect,
            params: undefined,
            param_count: 0,
        };
    }

    pub fn bindInt(*_, val: i64): bool {
        if (_.param_count >= 32) { return false; }
        _.params[_.param_count] = SqlParam {
            kind: SqlType.Int64,
            int_val: val,
        };
        _.param_count += 1;
        return true;
    }

    pub fn bindText(*_, val: []const u8): bool {
        if (_.param_count >= 32) { return false; }
        _.params[_.param_count] = SqlParam {
            kind: SqlType.Text,
            text_val: val,
        };
        _.param_count += 1;
        return true;
    }

    pub fn bindBool(*_, val: bool): bool {
        if (_.param_count >= 32) { return false; }
        _.params[_.param_count] = SqlParam {
            kind: SqlType.Boolean,
            bool_val: val,
        };
        _.param_count += 1;
        return true;
    }

    pub fn bindNull(*_): bool {
        if (_.param_count >= 32) { return false; }
        _.params[_.param_count] = SqlParam {
            kind: SqlType.Null,
        };
        _.param_count += 1;
        return true;
    }

    pub fn getParam(idx: usize): SqlParam? {
        if (idx >= _.param_count) { return null; }
        return _.params[idx];
    }

    pub fn clearBindings(*_) {
        _.param_count = 0;
    }
}

pub struct PostgresWire {
    /// Encodes a PostgreSQL v3 StartupMessage (4-byte BE total length + 0x00030000 version + "user\0<user>\0" + ["database\0<db>\0"] + "\0").
    pub static fn encodeStartupMessage(out: []u8, user: []const u8, database: []const u8): usize {
        if (user.len == 0) { return 0; }
        let total_len: usize = user.len + 15;
        if (database.len > 0) {
            total_len = total_len + database.len + 10;
        }
        if (out.len < total_len) {
            return 0;
        }
        const tlen32 = total_len as u32;
        out[0] = ((tlen32 >> 24) & 0xFF) as u8;
        out[1] = ((tlen32 >> 16) & 0xFF) as u8;
        out[2] = ((tlen32 >> 8) & 0xFF) as u8;
        out[3] = (tlen32 & 0xFF) as u8;

        // Protocol v3.0 (196608 = 0x00030000)
        out[4] = 0;
        out[5] = 3;
        out[6] = 0;
        out[7] = 0;

        let pos: usize = 8;
        const user_key = "user";
        @memcpy(out[pos .. pos + 4], user_key);
        pos += 4;
        out[pos] = 0;
        pos += 1;
        @memcpy(out[pos .. pos + user.len], user);
        pos += user.len;
        out[pos] = 0;
        pos += 1;

        if (database.len > 0) {
            const db_key = "database";
            @memcpy(out[pos .. pos + 8], db_key);
            pos += 8;
            out[pos] = 0;
            pos += 1;
            @memcpy(out[pos .. pos + database.len], database);
            pos += database.len;
            out[pos] = 0;
            pos += 1;
        }

        out[pos] = 0;
        pos += 1;
        return pos;
    }

    /// Encodes a PostgreSQL v3 PasswordMessage (`'p'` + 4-byte BE length + password + `\0`).
    pub static fn encodePasswordMessage(out: []u8, password: []const u8): usize {
        const payload_len = 4 + password.len + 1;
        const total_len = 1 + payload_len;
        if (out.len < total_len) {
            return 0;
        }
        out[0] = 'p';
        const plen32 = payload_len as u32;
        out[1] = ((plen32 >> 24) & 0xFF) as u8;
        out[2] = ((plen32 >> 16) & 0xFF) as u8;
        out[3] = ((plen32 >> 8) & 0xFF) as u8;
        out[4] = (plen32 & 0xFF) as u8;
        @memcpy(out[5 .. 5 + password.len], password);
        out[5 + password.len] = 0;
        return total_len;
    }

    /// Encodes a PostgreSQL v3 Frontend Simple Query message (`'Q'` + 4-byte BE length + query + `\0`).
    pub static fn encodeSimpleQuery(out: []u8, sql: []const u8): usize {
        const payload_len = 4 + sql.len + 1;
        const total_len = 1 + payload_len;
        if (out.len < total_len) {
            return 0;
        }
        out[0] = 'Q';
        const plen32 = payload_len as u32;
        out[1] = ((plen32 >> 24) & 0xFF) as u8;
        out[2] = ((plen32 >> 16) & 0xFF) as u8;
        out[3] = ((plen32 >> 8) & 0xFF) as u8;
        out[4] = (plen32 & 0xFF) as u8;
        let i: usize = 0;
        while (i < sql.len) {
            out[5 + i] = sql[i];
            i += 1;
        }
        out[5 + sql.len] = 0;
        return total_len;
    }

    /// Parses the 5-byte PostgreSQL v3 message header (1-byte tag + 4-byte BE payload length).
    pub static fn parseFrameLength(hdr: []const u8): u32? {
        if (hdr.len < 5) {
            return null;
        }
        return ((hdr[1] as u32) << 24) |
               ((hdr[2] as u32) << 16) |
               ((hdr[3] as u32) << 8) |
               (hdr[4] as u32);
    }
}

pub struct MySqlPacketHeader {
    pub payload_length: u32 = 0;
    pub sequence_id: u8 = 0;

    /// Encodes a 4-byte MySQL wire packet header (3-byte LE payload length + 1-byte sequence_id).
    pub static fn encode(out: []u8, payload_len: u32, seq_id: u8): usize {
        if (out.len < 4) {
            return 0;
        }
        out[0] = (payload_len & 0xFF) as u8;
        out[1] = ((payload_len >> 8) & 0xFF) as u8;
        out[2] = ((payload_len >> 16) & 0xFF) as u8;
        out[3] = seq_id;
        return 4;
    }

    pub static fn decode(hdr: []const u8): MySqlPacketHeader? {
        if (hdr.len < 4) {
            return null;
        }
        const len = (hdr[0] as u32) |
                    ((hdr[1] as u32) << 8) |
                    ((hdr[2] as u32) << 16);
        return MySqlPacketHeader {
            payload_length: len,
            sequence_id: hdr[3],
        };
    }
}
