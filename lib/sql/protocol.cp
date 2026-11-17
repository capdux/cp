// Source: Bun src/sql/postgres/
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
    kind: SqlType = SqlType.Null;
    int_val: i64 = 0;
    text_val: []const u8 = "";
    bool_val: bool = false;
}

pub struct SqlStatement {
    query: []const u8 = "";
    dialect: SqlDialect = SqlDialect.Postgres;
    params: [32]SqlParam = undefined;
    param_count: usize = 0;

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

    pub fn bindNull(*_): bool {
        if (_.param_count >= 32) { return false; }
        _.params[_.param_count] = SqlParam {
            kind: SqlType.Null,
        };
        _.param_count += 1;
        return true;
    }

    pub fn clearBindings(*_) {
        _.param_count = 0;
    }
}

pub struct PostgresWire {
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
    payload_length: u32 = 0;
    sequence_id: u8 = 0;

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
