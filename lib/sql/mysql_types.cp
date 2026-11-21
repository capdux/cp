// Source: Absorbed from /data/dev/bun/src/sql/mysql/MySQLTypes.cp | Synced: 2026-11-20T09:04:16Z
// lib/sql/mysql_types.cp
// MySQL AuthMethod, Capabilities, StatusFlags, CharacterSets, FieldTypes, Error Classifier & Temporal Values

const std = @import("std");

pub err AnyMySQLError {
    ConnectionClosed,
    InvalidAuthSwitchRequest,
    InvalidBinaryValue,
    InvalidErrorPacket,
    InvalidOKPacket,
    InvalidPrepareOKPacket,
    InvalidPublicKey,
    MissingAuthData,
    ShortRead,
    BufferTooSmall,
    UnsupportedPacket,
    FailedToEncryptPassword,
}

pub enum MySqlAuthMethod: u8 {
    mysql_native_password = 0,
    caching_sha2_password = 1,
    mysql_clear_password = 2,
    sha256_password = 3,
    auth_gssapi_client = 4,
    unknown = 255,

    pub static fn fromPluginName(plugin_name: []const u8): MySqlAuthMethod {
        if (std.mem.eql(u8, plugin_name, "mysql_native_password")) return .mysql_native_password;
        if (std.mem.eql(u8, plugin_name, "caching_sha2_password")) return .caching_sha2_password;
        if (std.mem.eql(u8, plugin_name, "mysql_clear_password")) return .mysql_clear_password;
        if (std.mem.eql(u8, plugin_name, "sha256_password")) return .sha256_password;
        if (std.mem.eql(u8, plugin_name, "auth_gssapi_client")) return .auth_gssapi_client;
        return .unknown;
    }

    pub fn pluginName(): []const u8 {
        return switch (_) {
            .mysql_native_password => "mysql_native_password",
            .caching_sha2_password => "caching_sha2_password",
            .mysql_clear_password => "mysql_clear_password",
            .sha256_password => "sha256_password",
            .auth_gssapi_client => "auth_gssapi_client",
            .unknown => "",
        };
    }
}

pub enum MySqlConnectionState: u8 {
    disconnected = 0,
    connecting = 1,
    awaiting_handshake = 2,
    authenticating = 3,
    ready = 4,
    querying = 5,
    closed = 6,
}

pub enum MySqlQueryStatus: u8 {
    pending = 0,
    reading_columns = 1,
    reading_rows = 2,
    done = 3,
    failed = 4,
}

pub enum MySqlSSLMode: u8 {
    disabled = 0,
    preferred = 1,
    required = 2,
    verify_ca = 3,
    verify_identity = 4,
}

pub enum MySqlTLSStatus: u8 {
    none = 0,
    requested = 1,
    active = 2,
}

pub struct MySqlCapabilityFlags {
    pub const CLIENT_LONG_PASSWORD: u32 = 1 << 0;
    pub const CLIENT_FOUND_ROWS: u32 = 1 << 1;
    pub const CLIENT_LONG_FLAG: u32 = 1 << 2;
    pub const CLIENT_CONNECT_WITH_DB: u32 = 1 << 3;
    pub const CLIENT_NO_SCHEMA: u32 = 1 << 4;
    pub const CLIENT_COMPRESS: u32 = 1 << 5;
    pub const CLIENT_ODBC: u32 = 1 << 6;
    pub const CLIENT_LOCAL_FILES: u32 = 1 << 7;
    pub const CLIENT_IGNORE_SPACE: u32 = 1 << 8;
    pub const CLIENT_PROTOCOL_41: u32 = 1 << 9;
    pub const CLIENT_INTERACTIVE: u32 = 1 << 10;
    pub const CLIENT_SSL: u32 = 1 << 11;
    pub const CLIENT_IGNORE_SIGPIPE: u32 = 1 << 12;
    pub const CLIENT_TRANSACTIONS: u32 = 1 << 13;
    pub const CLIENT_RESERVED: u32 = 1 << 14;
    pub const CLIENT_SECURE_CONNECTION: u32 = 1 << 15;
    pub const CLIENT_MULTI_STATEMENTS: u32 = 1 << 16;
    pub const CLIENT_MULTI_RESULTS: u32 = 1 << 17;
    pub const CLIENT_PS_MULTI_RESULTS: u32 = 1 << 18;
    pub const CLIENT_PLUGIN_AUTH: u32 = 1 << 19;
    pub const CLIENT_CONNECT_ATTRS: u32 = 1 << 20;
    pub const CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA: u32 = 1 << 21;
    pub const CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS: u32 = 1 << 22;
    pub const CLIENT_SESSION_TRACK: u32 = 1 << 23;
    pub const CLIENT_DEPRECATE_EOF: u32 = 1 << 24;
    pub const CLIENT_OPTIONAL_RESULTSET_METADATA: u32 = 1 << 25;
    pub const CLIENT_ZSTD_COMPRESSION_ALGORITHM: u32 = 1 << 26;
    pub const CLIENT_QUERY_ATTRIBUTES: u32 = 1 << 27;

    pub static fn defaultMask(): u32 {
        return CLIENT_LONG_PASSWORD |
            CLIENT_LONG_FLAG |
            CLIENT_CONNECT_WITH_DB |
            CLIENT_PROTOCOL_41 |
            CLIENT_TRANSACTIONS |
            CLIENT_SECURE_CONNECTION |
            CLIENT_MULTI_RESULTS |
            CLIENT_PS_MULTI_RESULTS |
            CLIENT_PLUGIN_AUTH |
            CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA |
            CLIENT_DEPRECATE_EOF;
    }
}

pub struct MySqlStatusFlags {
    pub raw: u16 = 0,

    pub const SERVER_STATUS_IN_TRANS: u16 = 0x0001;
    pub const SERVER_STATUS_AUTOCOMMIT: u16 = 0x0002;
    pub const SERVER_MORE_RESULTS_EXISTS: u16 = 0x0008;
    pub const SERVER_QUERY_NO_GOOD_INDEX_USED: u16 = 0x0010;
    pub const SERVER_QUERY_NO_INDEX_USED: u16 = 0x0020;
    pub const SERVER_STATUS_CURSOR_EXISTS: u16 = 0x0040;
    pub const SERVER_STATUS_LAST_ROW_SENT: u16 = 0x0080;
    pub const SERVER_STATUS_DB_DROPPED: u16 = 0x0100;
    pub const SERVER_STATUS_NO_BACKSLASH_ESCAPES: u16 = 0x0200;
    pub const SERVER_STATUS_METADATA_CHANGED: u16 = 0x0400;
    pub const SERVER_QUERY_WAS_SLOW: u16 = 0x0800;
    pub const SERVER_PS_OUT_PARAMS: u16 = 0x1000;
    pub const SERVER_STATUS_IN_TRANS_READONLY: u16 = 0x2000;
    pub const SERVER_SESSION_STATE_CHANGED: u16 = 0x4000;

    pub static fn fromInt(v: u16): MySqlStatusFlags {
        return MySqlStatusFlags{ raw: v };
    }

    pub fn inTransaction(): bool {
        return (_.raw & SERVER_STATUS_IN_TRANS) != 0;
    }

    pub fn autocommit(): bool {
        return (_.raw & SERVER_STATUS_AUTOCOMMIT) != 0;
    }

    pub fn moreResults(): bool {
        return (_.raw & SERVER_MORE_RESULTS_EXISTS) != 0;
    }

    pub fn cursorExists(): bool {
        return (_.raw & SERVER_STATUS_CURSOR_EXISTS) != 0;
    }
}

pub enum MySqlCharacterSet: u8 {
    big5_chinese_ci = 1,
    latin2_czech_cs = 2,
    dec8_swedish_ci = 3,
    cp850_general_ci = 4,
    latin1_german1_ci = 5,
    hp8_english_ci = 6,
    koi8r_general_ci = 7,
    latin1_swedish_ci = 8,
    latin2_general_ci = 9,
    swe7_swedish_ci = 10,
    ascii_general_ci = 11,
    utf8mb3_general_ci = 33,
    utf8mb4_general_ci = 45,
    utf8mb4_bin = 46,
    binary = 63,
    utf8mb4_unicode_ci = 224,
    utf8mb4_0900_ai_ci = 255,

    pub const default: MySqlCharacterSet = MySqlCharacterSet.utf8mb4_general_ci;
}

pub enum MySqlFieldType: u8 {
    MYSQL_TYPE_DECIMAL = 0x00,
    MYSQL_TYPE_TINY = 0x01,
    MYSQL_TYPE_SHORT = 0x02,
    MYSQL_TYPE_LONG = 0x03,
    MYSQL_TYPE_FLOAT = 0x04,
    MYSQL_TYPE_DOUBLE = 0x05,
    MYSQL_TYPE_NULL = 0x06,
    MYSQL_TYPE_TIMESTAMP = 0x07,
    MYSQL_TYPE_LONGLONG = 0x08,
    MYSQL_TYPE_INT24 = 0x09,
    MYSQL_TYPE_DATE = 0x0a,
    MYSQL_TYPE_TIME = 0x0b,
    MYSQL_TYPE_DATETIME = 0x0c,
    MYSQL_TYPE_YEAR = 0x0d,
    MYSQL_TYPE_NEWDATE = 0x0e,
    MYSQL_TYPE_VARCHAR = 0x0f,
    MYSQL_TYPE_BIT = 0x10,
    MYSQL_TYPE_TIMESTAMP2 = 0x11,
    MYSQL_TYPE_DATETIME2 = 0x12,
    MYSQL_TYPE_TIME2 = 0x13,
    MYSQL_TYPE_JSON = 0xf5,
    MYSQL_TYPE_NEWDECIMAL = 0xf6,
    MYSQL_TYPE_ENUM = 0xf7,
    MYSQL_TYPE_SET = 0xf8,
    MYSQL_TYPE_TINY_BLOB = 0xf9,
    MYSQL_TYPE_MEDIUM_BLOB = 0xfa,
    MYSQL_TYPE_LONG_BLOB = 0xfb,
    MYSQL_TYPE_BLOB = 0xfc,
    MYSQL_TYPE_VAR_STRING = 0xfd,
    MYSQL_TYPE_STRING = 0xfe,
    MYSQL_TYPE_GEOMETRY = 0xff,

    pub static fn fromByte(b: u8): MySqlFieldType {
        return switch (b) {
            0x00 => .MYSQL_TYPE_DECIMAL,
            0x01 => .MYSQL_TYPE_TINY,
            0x02 => .MYSQL_TYPE_SHORT,
            0x03 => .MYSQL_TYPE_LONG,
            0x04 => .MYSQL_TYPE_FLOAT,
            0x05 => .MYSQL_TYPE_DOUBLE,
            0x06 => .MYSQL_TYPE_NULL,
            0x07 => .MYSQL_TYPE_TIMESTAMP,
            0x08 => .MYSQL_TYPE_LONGLONG,
            0x09 => .MYSQL_TYPE_INT24,
            0x0a => .MYSQL_TYPE_DATE,
            0x0b => .MYSQL_TYPE_TIME,
            0x0c => .MYSQL_TYPE_DATETIME,
            0x0d => .MYSQL_TYPE_YEAR,
            0x0e => .MYSQL_TYPE_NEWDATE,
            0x0f => .MYSQL_TYPE_VARCHAR,
            0x10 => .MYSQL_TYPE_BIT,
            0x11 => .MYSQL_TYPE_TIMESTAMP2,
            0x12 => .MYSQL_TYPE_DATETIME2,
            0x13 => .MYSQL_TYPE_TIME2,
            0xf5 => .MYSQL_TYPE_JSON,
            0xf6 => .MYSQL_TYPE_NEWDECIMAL,
            0xf7 => .MYSQL_TYPE_ENUM,
            0xf8 => .MYSQL_TYPE_SET,
            0xf9 => .MYSQL_TYPE_TINY_BLOB,
            0xfa => .MYSQL_TYPE_MEDIUM_BLOB,
            0xfb => .MYSQL_TYPE_LONG_BLOB,
            0xfc => .MYSQL_TYPE_BLOB,
            0xfd => .MYSQL_TYPE_VAR_STRING,
            0xfe => .MYSQL_TYPE_STRING,
            0xff => .MYSQL_TYPE_GEOMETRY,
            else => .MYSQL_TYPE_STRING,
        };
    }

    pub fn isBinaryFormatSupported(): bool {
        return switch (_) {
            .MYSQL_TYPE_TINY,
            .MYSQL_TYPE_SHORT,
            .MYSQL_TYPE_LONG,
            .MYSQL_TYPE_LONGLONG,
            .MYSQL_TYPE_FLOAT,
            .MYSQL_TYPE_DOUBLE,
            .MYSQL_TYPE_TIME,
            .MYSQL_TYPE_DATE,
            .MYSQL_TYPE_DATETIME,
            .MYSQL_TYPE_TIMESTAMP
            => true,
            else => false,
        };
    }
}

pub struct MySqlErrorClassifier {
    pub static fn codeName(errno: u16): []const u8 {
        return switch (errno) {
            1040 => "ER_CON_COUNT_ERROR",
            1044 => "ER_DBACCESS_DENIED_ERROR",
            1045 => "ER_ACCESS_DENIED_ERROR",
            1049 => "ER_BAD_DB_ERROR",
            1054 => "ER_BAD_FIELD_ERROR",
            1062 => "ER_DUP_ENTRY",
            1064 => "ER_PARSE_ERROR",
            1146 => "ER_NO_SUCH_TABLE",
            1205 => "ER_LOCK_WAIT_TIMEOUT",
            1213 => "ER_LOCK_DEADLOCK",
            1451 => "ER_ROW_IS_REFERENCED_2",
            1452 => "ER_NO_REFERENCED_ROW_2",
            2002 => "CR_CONNECTION_ERROR",
            2003 => "CR_CONN_HOST_ERROR",
            2006 => "CR_SERVER_GONE_ERROR",
            2013 => "CR_SERVER_LOST",
            else => "MYSQL_ERR",
        };
    }

    pub static fn isRetryable(errno: u16): bool {
        return errno == 1205 || errno == 1213 || errno == 2006 || errno == 2013 || errno == 1040;
    }
}

pub struct MySqlValueDateTime {
    pub year: u16 = 0,
    pub month: u8 = 0,
    pub day: u8 = 0,
    pub hour: u8 = 0,
    pub minute: u8 = 0,
    pub second: u8 = 0,
    pub microsecond: u32 = 0,

    pub static fn fromBinary(val: []const u8): MySqlValueDateTime! {
        if (val.len == 0) return MySqlValueDateTime{};
        if (val.len == 4) {
            return MySqlValueDateTime{
                year: (val[0] as u16) | ((val[1] as u16) << 8),
                month: val[2],
                day: val[3],
            };
        }
        if (val.len == 7) {
            return MySqlValueDateTime{
                year: (val[0] as u16) | ((val[1] as u16) << 8),
                month: val[2],
                day: val[3],
                hour: val[4],
                minute: val[5],
                second: val[6],
            };
        }
        if (val.len == 11) {
            const us = (val[7] as u32) | ((val[8] as u32) << 8) | ((val[9] as u32) << 16) | ((val[10] as u32) << 24);
            return MySqlValueDateTime{
                year: (val[0] as u16) | ((val[1] as u16) << 8),
                month: val[2],
                day: val[3],
                hour: val[4],
                minute: val[5],
                second: val[6],
                microsecond: us,
            };
        }
        return error.InvalidBinaryValue;
    }

    pub fn toBinary(field_type: MySqlFieldType, buffer: []u8): usize {
        if (field_type == .MYSQL_TYPE_DATE) {
            if (buffer.len < 5) return 0;
            buffer[0] = 4;
            buffer[1] = (_.year & 0xFF) as u8;
            buffer[2] = ((_.year >> 8) & 0xFF) as u8;
            buffer[3] = _.month;
            buffer[4] = _.day;
            return 5;
        }
        if (_.microsecond == 0) {
            if (buffer.len < 8) return 0;
            buffer[0] = 7;
            buffer[1] = (_.year & 0xFF) as u8;
            buffer[2] = ((_.year >> 8) & 0xFF) as u8;
            buffer[3] = _.month;
            buffer[4] = _.day;
            buffer[5] = _.hour;
            buffer[6] = _.minute;
            buffer[7] = _.second;
            return 8;
        }
        if (buffer.len < 12) return 0;
        buffer[0] = 11;
        buffer[1] = (_.year & 0xFF) as u8;
        buffer[2] = ((_.year >> 8) & 0xFF) as u8;
        buffer[3] = _.month;
        buffer[4] = _.day;
        buffer[5] = _.hour;
        buffer[6] = _.minute;
        buffer[7] = _.second;
        buffer[8] = (_.microsecond & 0xFF) as u8;
        buffer[9] = ((_.microsecond >> 8) & 0xFF) as u8;
        buffer[10] = ((_.microsecond >> 16) & 0xFF) as u8;
        buffer[11] = ((_.microsecond >> 24) & 0xFF) as u8;
        return 12;
    }

    pub fn formatIso(out: []u8): ([]u8)! {
        if (_.microsecond > 0) {
            return std.fmt.bufPrint(
                out,
                "{d:0>4}-{d:0>2}-{d:0>2}T{d:0>2}:{d:0>2}:{d:0>2}.{d:0>6}",
                .{ _.year, _.month, _.day, _.hour, _.minute, _.second, _.microsecond },
            );
        }
        return std.fmt.bufPrint(
            out,
            "{d:0>4}-{d:0>2}-{d:0>2}T{d:0>2}:{d:0>2}:{d:0>2}",
            .{ _.year, _.month, _.day, _.hour, _.minute, _.second },
        );
    }
}

pub struct MySqlValueTime {
    pub negative: bool = false,
    pub days: u32 = 0,
    pub hours: u8 = 0,
    pub minutes: u8 = 0,
    pub seconds: u8 = 0,
    pub microseconds: u32 = 0,

    pub static fn fromBinary(val: []const u8): MySqlValueTime! {
        if (val.len == 0) return MySqlValueTime{};
        if (val.len != 8 && val.len != 12) return error.InvalidBinaryValue;
        let t = MySqlValueTime{
            negative: val[0] != 0,
            days: (val[1] as u32) | ((val[2] as u32) << 8) | ((val[3] as u32) << 16) | ((val[4] as u32) << 24),
            hours: val[5],
            minutes: val[6],
            seconds: val[7],
        };
        if (val.len == 12) {
            t.microseconds = (val[8] as u32) | ((val[9] as u32) << 8) | ((val[10] as u32) << 16) | ((val[11] as u32) << 24);
        }
        return t;
    }

    pub fn toBinary(buffer: []u8): usize {
        if (_.microseconds == 0) {
            if (buffer.len < 9) return 0;
            buffer[0] = 8;
            buffer[1] = if (_.negative) 1 else 0;
            buffer[2] = (_.days & 0xFF) as u8;
            buffer[3] = ((_.days >> 8) & 0xFF) as u8;
            buffer[4] = ((_.days >> 16) & 0xFF) as u8;
            buffer[5] = ((_.days >> 24) & 0xFF) as u8;
            buffer[6] = _.hours;
            buffer[7] = _.minutes;
            buffer[8] = _.seconds;
            return 9;
        }
        if (buffer.len < 13) return 0;
        buffer[0] = 12;
        buffer[1] = if (_.negative) 1 else 0;
        buffer[2] = (_.days & 0xFF) as u8;
        buffer[3] = ((_.days >> 8) & 0xFF) as u8;
        buffer[4] = ((_.days >> 16) & 0xFF) as u8;
        buffer[5] = ((_.days >> 24) & 0xFF) as u8;
        buffer[6] = _.hours;
        buffer[7] = _.minutes;
        buffer[8] = _.seconds;
        buffer[9] = (_.microseconds & 0xFF) as u8;
        buffer[10] = ((_.microseconds >> 8) & 0xFF) as u8;
        buffer[11] = ((_.microseconds >> 16) & 0xFF) as u8;
        buffer[12] = ((_.microseconds >> 24) & 0xFF) as u8;
        return 13;
    }

    pub fn formatHms(out: []u8): ([]u8)! {
        const total_hours = (_.hours as u32) + _.days * 24;
        const sign = if (_.negative) "-" else "";
        if (total_hours > 99) {
            return std.fmt.bufPrint(out, "{s}{d:0>3}:{d:0>2}:{d:0>2}", .{ sign, total_hours, _.minutes, _.seconds });
        }
        return std.fmt.bufPrint(out, "{s}{d:0>2}:{d:0>2}:{d:0>2}", .{ sign, total_hours, _.minutes, _.seconds });
    }
}
