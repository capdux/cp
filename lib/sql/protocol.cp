// Source: Absorbed from /data/dev/bun/src/sql/ | Synced: 2026-11-20T09:04:25Z
// lib/sql/protocol.cp
// Unified SQL Parameterized Statement, Postgres v3 Wire Frame, MySQL Packet Protocol,
// Generic SqlConnectionPool & SQL Transaction State Machine
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

pub enum SqlValue {
    Null,
    Int: i64,
    Float: f64,
    Text: []const u8,
    Blob: []const u8,
    Bool: bool,

    pub fn typeTag(): SqlType {
        return switch (_) {
            .Null => SqlType.Null,
            .Int => SqlType.Int64,
            .Float => SqlType.Float64,
            .Text => SqlType.Text,
            .Blob => SqlType.Blob,
            .Bool => SqlType.Boolean,
        };
    }

    pub fn isNull(): bool {
        return switch (_) {
            .Null => true,
            else => false,
        };
    }

    pub fn asInt(): i64? {
        return switch (_) {
            .Int v => v,
            else => null,
        };
    }

    pub fn asFloat(): f64? {
        return switch (_) {
            .Float v => v,
            .Int v => @floatFromInt(v),
            else => null,
        };
    }

    pub fn asText(): ([]const u8)? {
        return switch (_) {
            .Text v => v,
            else => null,
        };
    }

    pub fn asBlob(): ([]const u8)? {
        return switch (_) {
            .Blob v => v,
            .Text v => v,
            else => null,
        };
    }

    pub fn asBool(): bool? {
        return switch (_) {
            .Bool v => v,
            else => null,
        };
    }
}

pub struct SqlParam {
    pub kind: SqlType = SqlType.Null;
    pub int_val: i64 = 0;
    pub float_val: f64 = 0.0;
    pub text_val: []const u8 = "";
    pub blob_val: []const u8 = "";
    pub bool_val: bool = false;

    pub fn toValue(): SqlValue {
        return switch (_.kind) {
            .Null => .Null,
            .Int64 => .Int(_.int_val),
            .Float64 => .Float(_.float_val),
            .Text => .Text(_.text_val),
            .Blob => .Blob(_.blob_val),
            .Boolean => .Bool(_.bool_val),
        };
    }
}

pub struct SqlStatement {
    pub query: []const u8 = "";
    pub dialect: SqlDialect = SqlDialect.Postgres;
    pub params: [32]SqlParam = undefined;
    pub param_count: usize = 0;

    pub static fn prepare(query: []const u8, dialect: SqlDialect): SqlStatement {
        return SqlStatement{
            query: query,
            dialect: dialect,
            params: undefined,
            param_count: 0,
        };
    }

    pub fn bindInt(*_, val: i64): bool {
        if (_.param_count >= 32) return false;
        _.params[_.param_count] = SqlParam{
            kind: SqlType.Int64,
            int_val: val,
        };
        _.param_count += 1;
        return true;
    }

    pub fn bindFloat(*_, val: f64): bool {
        if (_.param_count >= 32) return false;
        _.params[_.param_count] = SqlParam{
            kind: SqlType.Float64,
            float_val: val,
        };
        _.param_count += 1;
        return true;
    }

    pub fn bindText(*_, val: []const u8): bool {
        if (_.param_count >= 32) return false;
        _.params[_.param_count] = SqlParam{
            kind: SqlType.Text,
            text_val: val,
        };
        _.param_count += 1;
        return true;
    }

    pub fn bindBlob(*_, val: []const u8): bool {
        if (_.param_count >= 32) return false;
        _.params[_.param_count] = SqlParam{
            kind: SqlType.Blob,
            blob_val: val,
        };
        _.param_count += 1;
        return true;
    }

    pub fn bindBool(*_, val: bool): bool {
        if (_.param_count >= 32) return false;
        _.params[_.param_count] = SqlParam{
            kind: SqlType.Boolean,
            bool_val: val,
        };
        _.param_count += 1;
        return true;
    }

    pub fn bindNull(*_): bool {
        if (_.param_count >= 32) return false;
        _.params[_.param_count] = SqlParam{
            kind: SqlType.Null,
        };
        _.param_count += 1;
        return true;
    }

    pub fn bindValue(*_, val: SqlValue): bool {
        return switch (val) {
            .Null => _.bindNull(),
            .Int v => _.bindInt(v),
            .Float v => _.bindFloat(v),
            .Text v => _.bindText(v),
            .Blob v => _.bindBlob(v),
            .Bool v => _.bindBool(v),
        };
    }

    pub fn getParam(idx: usize): SqlParam? {
        if (idx >= _.param_count) return null;
        return _.params[idx];
    }

    pub fn clearBindings(*_) {
        _.param_count = 0;
    }
}

pub enum PostgresTransactionStatus: u8 {
    Idle = 'I',
    InTransaction = 'T',
    Failed = 'E',
    Unknown = 0,

    pub static fn fromByte(b: u8): PostgresTransactionStatus {
        if (b == 'I') return PostgresTransactionStatus.Idle;
        if (b == 'T') return PostgresTransactionStatus.InTransaction;
        if (b == 'E') return PostgresTransactionStatus.Failed;
        return PostgresTransactionStatus.Unknown;
    }
}

pub struct PostgresWire {
    fn writeU32BE(out: []u8, pos: usize, val: u32) {
        out[pos] = ((val >> 24) & 0xFF) as u8;
        out[pos + 1] = ((val >> 16) & 0xFF) as u8;
        out[pos + 2] = ((val >> 8) & 0xFF) as u8;
        out[pos + 3] = (val & 0xFF) as u8;
    }

    fn writeU16BE(out: []u8, pos: usize, val: u16) {
        out[pos] = ((val >> 8) & 0xFF) as u8;
        out[pos + 1] = (val & 0xFF) as u8;
    }

    /// Encodes a PostgreSQL v3 StartupMessage (4-byte BE total length + 0x00030000 version + "user\0<user>\0" + ["database\0<db>\0"] + "\0").
    pub static fn encodeStartupMessage(out: []u8, user: []const u8, database: []const u8): usize {
        if (user.len == 0) return 0;
        let total_len: usize = user.len + 15;
        if (database.len > 0) {
            total_len = total_len + database.len + 10;
        }
        if (out.len < total_len) return 0;

        writeU32BE(out, 0, total_len as u32);
        // Protocol v3.0 (196608 = 0x00030000)
        out[4] = 0;
        out[5] = 3;
        out[6] = 0;
        out[7] = 0;

        let pos: usize = 8;
        @memcpy(out[pos .. pos + 4], "user");
        pos += 4;
        out[pos] = 0;
        pos += 1;
        @memcpy(out[pos .. pos + user.len], user);
        pos += user.len;
        out[pos] = 0;
        pos += 1;

        if (database.len > 0) {
            @memcpy(out[pos .. pos + 8], "database");
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
        if (out.len < total_len) return 0;
        out[0] = 'p';
        writeU32BE(out, 1, payload_len as u32);
        @memcpy(out[5 .. 5 + password.len], password);
        out[5 + password.len] = 0;
        return total_len;
    }

    /// Encodes a PostgreSQL v3 MD5 PasswordMessage: `"md5" + md5(md5(password + user) + salt)`.
    pub static fn encodeMD5Password(out: []u8, user: []const u8, password: []const u8, salt: [4]u8): usize {
        let inner_buf: [256]u8 = undefined;
        if (password.len + user.len > inner_buf.len) return 0;
        @memcpy(inner_buf[0..password.len], password);
        @memcpy(inner_buf[password.len .. password.len + user.len], user);
        let d1: [16]u8 = undefined;
        std.crypto.hash.Md5.hash(inner_buf[0 .. password.len + user.len], &d1, .{});
        const h1 = std.fmt.bytesToHex(d1, .lower);

        let outer_buf: [36]u8 = undefined;
        @memcpy(outer_buf[0..32], h1[0..32]);
        outer_buf[32] = salt[0];
        outer_buf[33] = salt[1];
        outer_buf[34] = salt[2];
        outer_buf[35] = salt[3];
        let d2: [16]u8 = undefined;
        std.crypto.hash.Md5.hash(outer_buf[0..36], &d2, .{});
        const h2 = std.fmt.bytesToHex(d2, .lower);

        let final_pw: [35]u8 = undefined;
        @memcpy(final_pw[0..3], "md5");
        @memcpy(final_pw[3..35], h2[0..32]);
        return encodePasswordMessage(out, final_pw[0..35]);
    }

    /// Encodes a PostgreSQL v3 SASLInitialResponse (`'p'` + len + mechanism + `\0` + 4B data len + initial_data).
    pub static fn encodeSASLInitialResponse(out: []u8, mechanism: []const u8, initial_data: []const u8): usize {
        const payload_len = 4 + mechanism.len + 1 + 4 + initial_data.len;
        const total_len = 1 + payload_len;
        if (out.len < total_len) return 0;
        out[0] = 'p';
        writeU32BE(out, 1, payload_len as u32);
        let pos: usize = 5;
        @memcpy(out[pos .. pos + mechanism.len], mechanism);
        pos += mechanism.len;
        out[pos] = 0;
        pos += 1;
        writeU32BE(out, pos, initial_data.len as u32);
        pos += 4;
        if (initial_data.len > 0) {
            @memcpy(out[pos .. pos + initial_data.len], initial_data);
            pos += initial_data.len;
        }
        return pos;
    }

    /// Encodes a PostgreSQL v3 SASLResponse (`'p'` + 4B len + data).
    pub static fn encodeSASLResponse(out: []u8, data: []const u8): usize {
        const payload_len = 4 + data.len;
        const total_len = 1 + payload_len;
        if (out.len < total_len) return 0;
        out[0] = 'p';
        writeU32BE(out, 1, payload_len as u32);
        if (data.len > 0) {
            @memcpy(out[5 .. 5 + data.len], data);
        }
        return total_len;
    }

    /// Encodes a PostgreSQL v3 Frontend Simple Query message (`'Q'` + 4-byte BE length + query + `\0`).
    pub static fn encodeSimpleQuery(out: []u8, sql: []const u8): usize {
        const payload_len = 4 + sql.len + 1;
        const total_len = 1 + payload_len;
        if (out.len < total_len) return 0;
        out[0] = 'Q';
        writeU32BE(out, 1, payload_len as u32);
        @memcpy(out[5 .. 5 + sql.len], sql);
        out[5 + sql.len] = 0;
        return total_len;
    }

    /// Encodes Extended Query `'P'` (Parse): stmt_name\0 + query\0 + u16 param_count + [u32 oid...].
    pub static fn encodeParse(out: []u8, stmt_name: []const u8, query: []const u8, param_oids: []const u32): usize {
        const payload_len = 4 + (stmt_name.len + 1) + (query.len + 1) + 2 + (param_oids.len * 4);
        const total_len = 1 + payload_len;
        if (out.len < total_len) return 0;

        out[0] = 'P';
        writeU32BE(out, 1, payload_len as u32);
        let pos: usize = 5;
        if (stmt_name.len > 0) {
            @memcpy(out[pos .. pos + stmt_name.len], stmt_name);
            pos += stmt_name.len;
        }
        out[pos] = 0;
        pos += 1;

        if (query.len > 0) {
            @memcpy(out[pos .. pos + query.len], query);
            pos += query.len;
        }
        out[pos] = 0;
        pos += 1;

        writeU16BE(out, pos, param_oids.len as u16);
        pos += 2;

        for (oid of param_oids) {
            writeU32BE(out, pos, oid);
            pos += 4;
        }
        return pos;
    }

    /// Encodes Extended Query `'B'` (Bind): portal\0 + stmt\0 + param_formats + param_values + result_formats.
    pub static fn encodeBind(
        out: []u8,
        portal: []const u8,
        stmt_name: []const u8,
        param_formats: []const u16,
        params: []const SqlParam,
        result_formats: []const u16,
    ): usize {
        if (out.len < 16) return 0;
        out[0] = 'B';
        let pos: usize = 5; // Reserve 4 bytes at [1..5] for payload length

        if (pos + portal.len + 1 > out.len) return 0;
        if (portal.len > 0) {
            @memcpy(out[pos .. pos + portal.len], portal);
            pos += portal.len;
        }
        out[pos] = 0;
        pos += 1;

        if (pos + stmt_name.len + 1 > out.len) return 0;
        if (stmt_name.len > 0) {
            @memcpy(out[pos .. pos + stmt_name.len], stmt_name);
            pos += stmt_name.len;
        }
        out[pos] = 0;
        pos += 1;

        // Parameter format codes
        if (pos + 2 + param_formats.len * 2 > out.len) return 0;
        writeU16BE(out, pos, param_formats.len as u16);
        pos += 2;
        for (fmt of param_formats) {
            writeU16BE(out, pos, fmt);
            pos += 2;
        }

        // Parameter values (encoded in text format for universal PG compatibility)
        if (pos + 2 > out.len) return 0;
        writeU16BE(out, pos, params.len as u16);
        pos += 2;

        let num_buf: [64]u8 = undefined;
        for (p of params) {
            if (p.kind == SqlType.Null) {
                if (pos + 4 > out.len) return 0;
                writeU32BE(out, pos, 0xFFFFFFFF);
                pos += 4;
            } else {
                const bytes = switch (p.kind) {
                    .Int64 => std.fmt.bufPrint(&num_buf, "{d}", .{p.int_val}) catch return 0,
                    .Float64 => std.fmt.bufPrint(&num_buf, "{d}", .{p.float_val}) catch return 0,
                    .Boolean => if (p.bool_val) "t" else "f",
                    .Text => p.text_val,
                    .Blob => p.blob_val,
                    .Null => "",
                };
                if (pos + 4 + bytes.len > out.len) return 0;
                writeU32BE(out, pos, bytes.len as u32);
                pos += 4;
                if (bytes.len > 0) {
                    @memcpy(out[pos .. pos + bytes.len], bytes);
                    pos += bytes.len;
                }
            }
        }

        // Result format codes
        if (pos + 2 + result_formats.len * 2 > out.len) return 0;
        writeU16BE(out, pos, result_formats.len as u16);
        pos += 2;
        for (rf of result_formats) {
            writeU16BE(out, pos, rf);
            pos += 2;
        }

        writeU32BE(out, 1, (pos - 1) as u32);
        return pos;
    }

    /// Encodes Extended Query `'D'` (Describe): target `'S'` (statement) or `'P'` (portal) + name\0.
    pub static fn encodeDescribe(out: []u8, target: u8, name: []const u8): usize {
        const payload_len = 4 + 1 + name.len + 1;
        const total_len = 1 + payload_len;
        if (out.len < total_len) return 0;
        out[0] = 'D';
        writeU32BE(out, 1, payload_len as u32);
        out[5] = target;
        if (name.len > 0) {
            @memcpy(out[6 .. 6 + name.len], name);
        }
        out[6 + name.len] = 0;
        return total_len;
    }

    /// Encodes Extended Query `'E'` (Execute): portal\0 + 4-byte BE max_rows (0 = unlimited).
    pub static fn encodeExecute(out: []u8, portal: []const u8, max_rows: u32): usize {
        const payload_len = 4 + portal.len + 1 + 4;
        const total_len = 1 + payload_len;
        if (out.len < total_len) return 0;
        out[0] = 'E';
        writeU32BE(out, 1, payload_len as u32);
        let pos: usize = 5;
        if (portal.len > 0) {
            @memcpy(out[pos .. pos + portal.len], portal);
            pos += portal.len;
        }
        out[pos] = 0;
        pos += 1;
        writeU32BE(out, pos, max_rows);
        return total_len;
    }

    /// Encodes Extended Query `'C'` (Close): target `'S'` or `'P'` + name\0.
    pub static fn encodeClose(out: []u8, target: u8, name: []const u8): usize {
        const payload_len = 4 + 1 + name.len + 1;
        const total_len = 1 + payload_len;
        if (out.len < total_len) return 0;
        out[0] = 'C';
        writeU32BE(out, 1, payload_len as u32);
        out[5] = target;
        if (name.len > 0) {
            @memcpy(out[6 .. 6 + name.len], name);
        }
        out[6 + name.len] = 0;
        return total_len;
    }

    /// Encodes Extended Query `'S'` (Sync) - 5 bytes.
    pub static fn encodeSync(out: []u8): usize {
        if (out.len < 5) return 0;
        out[0] = 'S';
        writeU32BE(out, 1, 4);
        return 5;
    }

    /// Encodes `'H'` (Flush) - 5 bytes.
    pub static fn encodeFlush(out: []u8): usize {
        if (out.len < 5) return 0;
        out[0] = 'H';
        writeU32BE(out, 1, 4);
        return 5;
    }

    /// Encodes `'X'` (Terminate) - 5 bytes.
    pub static fn encodeTerminate(out: []u8): usize {
        if (out.len < 5) return 0;
        out[0] = 'X';
        writeU32BE(out, 1, 4);
        return 5;
    }

    /// Parses the 5-byte PostgreSQL v3 message header (1-byte tag + 4-byte BE payload length).
    pub static fn parseFrameLength(hdr: []const u8): u32? {
        if (hdr.len < 5) return null;
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
        if (out.len < 4) return 0;
        out[0] = (payload_len & 0xFF) as u8;
        out[1] = ((payload_len >> 8) & 0xFF) as u8;
        out[2] = ((payload_len >> 16) & 0xFF) as u8;
        out[3] = seq_id;
        return 4;
    }

    pub static fn decode(hdr: []const u8): MySqlPacketHeader? {
        if (hdr.len < 4) return null;
        const len = (hdr[0] as u32) |
            ((hdr[1] as u32) << 8) |
            ((hdr[2] as u32) << 16);
        return MySqlPacketHeader{
            payload_length: len,
            sequence_id: hdr[3],
        };
    }
}

// ============================================================================
// Generic SQL Connection Pool & Transaction State Machine
// ============================================================================

pub const MAX_SQL_POOL_SIZE: usize = 64;

pub enum SqlPoolSlotState: u8 {
    Empty = 0,
    Idle = 1,
    InUse = 2,
    Closed = 3,
}

pub struct SqlPoolSlot {
    pub conn_id: u64 = 0,
    pub dialect: SqlDialect = SqlDialect.Postgres,
    pub state: SqlPoolSlotState = SqlPoolSlotState.Empty,
    pub created_at_ms: i64 = 0,
    pub last_used_ms: i64 = 0,
    pub query_count: u64 = 0,
    pub healthy: bool = true,
}

pub struct SqlPoolStats {
    pub capacity: usize = 0,
    pub total: usize = 0,
    pub idle: usize = 0,
    pub in_use: usize = 0,
    pub acquires: u64 = 0,
    pub releases: u64 = 0,
    pub evictions: u64 = 0,
}

pub struct SqlConnectionPool {
    pub dialect: SqlDialect = SqlDialect.Postgres,
    pub capacity: usize = 16,
    pub idle_timeout_ms: i64 = 60000,
    pub slots: [MAX_SQL_POOL_SIZE]SqlPoolSlot = undefined,
    pub acquires: u64 = 0,
    pub releases: u64 = 0,
    pub evictions: u64 = 0,

    pub static fn init(dialect: SqlDialect, capacity: usize = 16, idle_timeout_ms: i64 = 60000): SqlConnectionPool {
        const pool_cap = if (capacity == 0 || capacity > MAX_SQL_POOL_SIZE) MAX_SQL_POOL_SIZE else capacity;
        let pool = SqlConnectionPool{
            dialect: dialect,
            capacity: pool_cap,
            idle_timeout_ms: idle_timeout_ms,
            slots: undefined,
            acquires: 0,
            releases: 0,
            evictions: 0,
        };
        let i: usize = 0;
        while (i < MAX_SQL_POOL_SIZE) : (i += 1) {
            pool.slots[i] = SqlPoolSlot{};
        }
        return pool;
    }

    pub fn activeTotal(): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.capacity) : (i += 1) {
            if (_.slots[i].state == SqlPoolSlotState.Idle || _.slots[i].state == SqlPoolSlotState.InUse) {
                count += 1;
            }
        }
        return count;
    }

    pub fn registerConnection(*_, conn_id: u64, now_ms: i64, start_in_use: bool = true): bool {
        if (_.activeTotal() >= _.capacity) return false;
        let i: usize = 0;
        while (i < _.capacity) : (i += 1) {
            if (_.slots[i].state == SqlPoolSlotState.Empty || _.slots[i].state == SqlPoolSlotState.Closed) {
                _.slots[i] = SqlPoolSlot{
                    conn_id: conn_id,
                    dialect: _.dialect,
                    state: if (start_in_use) SqlPoolSlotState.InUse else SqlPoolSlotState.Idle,
                    created_at_ms: now_ms,
                    last_used_ms: now_ms,
                    query_count: 0,
                    healthy: true,
                };
                return true;
            }
        }
        return false;
    }

    /// Borrows an idle, non-expired connection ID from the pool.
    pub fn acquire(*_, now_ms: i64): u64? {
        let best_idx: usize? = null;
        let best_ts: i64 = -1;
        let i: usize = 0;
        while (i < _.capacity) : (i += 1) {
            if (_.slots[i].state == SqlPoolSlotState.Idle) {
                if (!_.slots[i].healthy || (_.idle_timeout_ms > 0 && (now_ms - _.slots[i].last_used_ms) > _.idle_timeout_ms)) {
                    _.slots[i] = SqlPoolSlot{};
                    _.evictions += 1;
                    continue;
                }
                if (best_idx == null || _.slots[i].last_used_ms >= best_ts) {
                    best_idx = i;
                    best_ts = _.slots[i].last_used_ms;
                }
            }
        }
        if (idx := best_idx) {
            _.slots[idx].state = SqlPoolSlotState.InUse;
            _.slots[idx].last_used_ms = now_ms;
            _.slots[idx].query_count += 1;
            _.acquires += 1;
            return _.slots[idx].conn_id;
        }
        return null;
    }

    /// Returns a borrowed connection to Idle (or evicts if unhealthy / failed transaction).
    pub fn releaseConnection(*_, conn_id: u64, now_ms: i64, healthy: bool = true): bool {
        let i: usize = 0;
        while (i < _.capacity) : (i += 1) {
            if (_.slots[i].conn_id == conn_id && (_.slots[i].state == SqlPoolSlotState.InUse || _.slots[i].state == SqlPoolSlotState.Idle)) {
                _.releases += 1;
                if (!healthy) {
                    _.slots[i] = SqlPoolSlot{};
                    _.evictions += 1;
                    return false;
                }
                _.slots[i].state = SqlPoolSlotState.Idle;
                _.slots[i].last_used_ms = now_ms;
                return true;
            }
        }
        return false;
    }

    pub fn evictIdle(*_, now_ms: i64): usize {
        if (_.idle_timeout_ms <= 0) return 0;
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.capacity) : (i += 1) {
            if (_.slots[i].state == SqlPoolSlotState.Idle && (now_ms - _.slots[i].last_used_ms) > _.idle_timeout_ms) {
                _.slots[i] = SqlPoolSlot{};
                _.evictions += 1;
                count += 1;
            }
        }
        return count;
    }

    pub fn closeConnection(*_, conn_id: u64): bool {
        let i: usize = 0;
        while (i < _.capacity) : (i += 1) {
            if (_.slots[i].conn_id == conn_id && _.slots[i].state != SqlPoolSlotState.Empty) {
                _.slots[i] = SqlPoolSlot{};
                return true;
            }
        }
        return false;
    }

    pub fn stats(): SqlPoolStats {
        let total: usize = 0;
        let idle: usize = 0;
        let in_use: usize = 0;
        let i: usize = 0;
        while (i < _.capacity) : (i += 1) {
            if (_.slots[i].state == SqlPoolSlotState.Idle) {
                idle += 1;
                total += 1;
            } else if (_.slots[i].state == SqlPoolSlotState.InUse) {
                in_use += 1;
                total += 1;
            }
        }
        return SqlPoolStats{
            capacity: _.capacity,
            total: total,
            idle: idle,
            in_use: in_use,
            acquires: _.acquires,
            releases: _.releases,
            evictions: _.evictions,
        };
    }
}

pub enum SqlTransactionState: u8 {
    Idle = 0,
    InTransaction = 1,
    Failed = 2,
}

pub enum SqlIsolationLevel: u8 {
    ReadUncommitted = 0,
    ReadCommitted = 1,
    RepeatableRead = 2,
    Serializable = 3,

    pub fn toSql(): []const u8 {
        return switch (_) {
            .ReadUncommitted => "READ UNCOMMITTED",
            .ReadCommitted => "READ COMMITTED",
            .RepeatableRead => "REPEATABLE READ",
            .Serializable => "SERIALIZABLE",
        };
    }
}

pub struct SqlTransactionStateMachine {
    pub dialect: SqlDialect = SqlDialect.Postgres,
    pub state: SqlTransactionState = SqlTransactionState.Idle,
    pub savepoints: [16][]const u8 = undefined,
    pub savepoint_depth: usize = 0,
    pub read_only: bool = false,
    pub isolation: SqlIsolationLevel? = null,

    pub static fn init(dialect: SqlDialect): SqlTransactionStateMachine {
        let sm = SqlTransactionStateMachine{
            dialect: dialect,
            state: SqlTransactionState.Idle,
            savepoints: undefined,
            savepoint_depth: 0,
            read_only: false,
            isolation: null,
        };
        let i: usize = 0;
        while (i < 16) : (i += 1) {
            sm.savepoints[i] = "";
        }
        return sm;
    }

    fn isValidIdent(name: []const u8): bool {
        if (name.len == 0) return false;
        for (ch of name) {
            const ok = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_';
            if (!ok) return false;
        }
        return true;
    }

    pub fn begin(*_, isolation: SqlIsolationLevel? = null, read_only: bool = false, out: []u8): ([]const u8)! {
        if (_.state != SqlTransactionState.Idle) return error.TransactionAlreadyActive;

        let sql: []const u8 = "";
        if (_.dialect == SqlDialect.Sqlite) {
            sql = std.fmt.bufPrint(out, "BEGIN IMMEDIATE", .{}) catch return error.BufferTooSmall;
        } else if (_.dialect == SqlDialect.MySql) {
            if (read_only) {
                sql = std.fmt.bufPrint(out, "START TRANSACTION READ ONLY", .{}) catch return error.BufferTooSmall;
            } else {
                sql = std.fmt.bufPrint(out, "START TRANSACTION", .{}) catch return error.BufferTooSmall;
            }
        } else {
            if (iso := isolation) {
                if (read_only) {
                    sql = std.fmt.bufPrint(out, "BEGIN ISOLATION LEVEL {s} READ ONLY", .{iso.toSql()}) catch return error.BufferTooSmall;
                } else {
                    sql = std.fmt.bufPrint(out, "BEGIN ISOLATION LEVEL {s}", .{iso.toSql()}) catch return error.BufferTooSmall;
                }
            } else if (read_only) {
                sql = std.fmt.bufPrint(out, "BEGIN READ ONLY", .{}) catch return error.BufferTooSmall;
            } else {
                sql = std.fmt.bufPrint(out, "BEGIN", .{}) catch return error.BufferTooSmall;
            }
        }

        _.state = SqlTransactionState.InTransaction;
        _.isolation = isolation;
        _.read_only = read_only;
        _.savepoint_depth = 0;
        return sql;
    }

    pub fn commit(*_, out: []u8): ([]const u8)! {
        if (_.state == SqlTransactionState.Idle) return error.NoActiveTransaction;
        if (_.state == SqlTransactionState.Failed) return error.TransactionAborted;
        const sql = std.fmt.bufPrint(out, "COMMIT", .{}) catch return error.BufferTooSmall;
        _.state = SqlTransactionState.Idle;
        _.savepoint_depth = 0;
        return sql;
    }

    pub fn rollback(*_, out: []u8): ([]const u8)! {
        if (_.state == SqlTransactionState.Idle) return error.NoActiveTransaction;
        const sql = std.fmt.bufPrint(out, "ROLLBACK", .{}) catch return error.BufferTooSmall;
        _.state = SqlTransactionState.Idle;
        _.savepoint_depth = 0;
        return sql;
    }

    pub fn savepoint(*_, name: []const u8, out: []u8): ([]const u8)! {
        if (_.state != SqlTransactionState.InTransaction) return error.NoActiveTransaction;
        if (!isValidIdent(name)) return error.InvalidSavepointName;
        if (_.savepoint_depth >= 16) return error.TooManySavepoints;
        const sql = std.fmt.bufPrint(out, "SAVEPOINT {s}", .{name}) catch return error.BufferTooSmall;
        _.savepoints[_.savepoint_depth] = name;
        _.savepoint_depth += 1;
        return sql;
    }

    pub fn rollbackTo(*_, name: []const u8, out: []u8): ([]const u8)! {
        if (_.state == SqlTransactionState.Idle) return error.NoActiveTransaction;
        if (!isValidIdent(name)) return error.InvalidSavepointName;

        let found_idx: usize? = null;
        let i: usize = _.savepoint_depth;
        while (i > 0) {
            i -= 1;
            if (std.mem.eql(u8, _.savepoints[i], name)) {
                found_idx = i;
                break;
            }
        }
        const idx = found_idx ?? return error.SavepointNotFound;
        const sql = std.fmt.bufPrint(out, "ROLLBACK TO SAVEPOINT {s}", .{name}) catch return error.BufferTooSmall;
        _.savepoint_depth = idx + 1;
        _.state = SqlTransactionState.InTransaction;
        return sql;
    }

    pub fn releaseSavepoint(*_, name: []const u8, out: []u8): ([]const u8)! {
        if (_.state != SqlTransactionState.InTransaction) return error.NoActiveTransaction;
        if (!isValidIdent(name)) return error.InvalidSavepointName;

        let found_idx: usize? = null;
        let i: usize = _.savepoint_depth;
        while (i > 0) {
            i -= 1;
            if (std.mem.eql(u8, _.savepoints[i], name)) {
                found_idx = i;
                break;
            }
        }
        const idx = found_idx ?? return error.SavepointNotFound;
        const sql = std.fmt.bufPrint(out, "RELEASE SAVEPOINT {s}", .{name}) catch return error.BufferTooSmall;
        _.savepoint_depth = idx;
        return sql;
    }

    pub fn markFailed(*_) {
        if (_.state == SqlTransactionState.InTransaction) {
            _.state = SqlTransactionState.Failed;
        }
    }

    pub fn syncFromPostgresReady(*_, status: PostgresTransactionStatus) {
        switch (status) {
            .Idle => {
                _.state = SqlTransactionState.Idle;
                _.savepoint_depth = 0;
            },
            .InTransaction => {
                _.state = SqlTransactionState.InTransaction;
            },
            .Failed => {
                _.state = SqlTransactionState.Failed;
            },
            .Unknown => {},
        }
    }
}
