// Source: /data/dev/bun/src/sql/postgres/types/Tag.cp
// lib/sql/pg_types.cp
// PostgreSQL Type OID Registry, CommandTag, Error Classification & Binary/Text Codecs

const std = @import("std");

pub err AnyPostgresError {
    ConnectionClosed,
    ExpectedRequest,
    ExpectedStatement,
    InvalidBackendKeyData,
    InvalidBinaryData,
    InvalidByteSequence,
    InvalidByteSequenceForEncoding,
    InvalidCharacter,
    InvalidMessage,
    InvalidMessageLength,
    InvalidQueryBinding,
    InvalidServerKey,
    InvalidServerSignature,
    InvalidTimeFormat,
    MultidimensionalArrayNotSupportedYet,
    NullsInArrayNotSupportedYet,
    OutOfMemory,
    Overflow,
    PBKDF2Failed,
    SASLSignatureMismatch,
    SASLSignatureInvalidBase64,
    ShortRead,
    TLSNotAvailable,
    TLSUpgradeFailed,
    TooManyParameters,
    UnexpectedMessage,
    UnknownAuthenticationMethod,
    UnsupportedAuthenticationMethod,
    UnsupportedByteaFormat,
    UnsupportedIntegerSize,
    UnsupportedArrayFormat,
    UnsupportedNumericFormat,
    UnknownFormatCode,
    BufferTooSmall,
}

pub enum PgSSLMode: u8 {
    disable = 0,
    prefer = 1,
    require = 2,
    verify_ca = 3,
    verify_full = 4,
}

pub enum PgStatus: u8 {
    idle = 0,
    connecting = 1,
    authenticating = 2,
    ready = 3,
    querying = 4,
    closed = 5,
    err_state = 6,
}

pub enum PgTLSStatus: u8 {
    none = 0,
    requested = 1,
    active = 2,
}

pub enum PgTypeTag: u32 {
    pg_bool = 16,
    bytea = 17,
    char = 18,
    name = 19,
    int8 = 20,
    int2 = 21,
    int2vector = 22,
    int4 = 23,
    text = 25,
    oid = 26,
    xid = 28,
    cid = 29,
    json = 114,
    xml = 142,
    xml_array = 143,
    json_array = 199,
    point = 600,
    lseg = 601,
    path = 602,
    box = 603,
    polygon = 604,
    line = 628,
    line_array = 629,
    cidr = 650,
    cidr_array = 651,
    float4 = 700,
    float8 = 701,
    circle = 718,
    circle_array = 719,
    macaddr8 = 774,
    macaddr8_array = 775,
    money = 790,
    money_array = 791,
    macaddr = 829,
    inet = 869,
    bool_array = 1000,
    bytea_array = 1001,
    char_array = 1002,
    name_array = 1003,
    int2_array = 1005,
    int2vector_array = 1006,
    int4_array = 1007,
    text_array = 1009,
    tid_array = 1010,
    xid_array = 1011,
    cid_array = 1012,
    bpchar_array = 1014,
    varchar_array = 1015,
    int8_array = 1016,
    point_array = 1017,
    lseg_array = 1018,
    path_array = 1019,
    box_array = 1020,
    float4_array = 1021,
    float8_array = 1022,
    polygon_array = 1027,
    oid_array = 1028,
    aclitem = 1033,
    aclitem_array = 1034,
    macaddr_array = 1040,
    inet_array = 1041,
    bpchar = 1042,
    varchar = 1043,
    date = 1082,
    time = 1083,
    timestamp = 1114,
    timestamp_array = 1115,
    date_array = 1182,
    time_array = 1183,
    timestamptz = 1184,
    timestamptz_array = 1185,
    interval = 1186,
    interval_array = 1187,
    numeric_array = 1231,
    pg_database = 1248,
    timetz = 1266,
    timetz_array = 1270,
    bit = 1560,
    bit_array = 1561,
    varbit = 1562,
    varbit_array = 1563,
    numeric = 1700,
    uuid = 2950,
    uuid_array = 2951,
    jsonb = 3802,
    jsonb_array = 3807,
    jsonpath = 4072,
    jsonpath_array = 4073,
    unknown = 0,

    pub static fn fromOid(oid_val: u32): PgTypeTag {
        return switch (oid_val) {
            16 => .pg_bool,
            17 => .bytea,
            18 => .char,
            19 => .name,
            20 => .int8,
            21 => .int2,
            22 => .int2vector,
            23 => .int4,
            25 => .text,
            26 => .oid,
            28 => .xid,
            29 => .cid,
            114 => .json,
            142 => .xml,
            143 => .xml_array,
            199 => .json_array,
            600 => .point,
            601 => .lseg,
            602 => .path,
            603 => .box,
            604 => .polygon,
            628 => .line,
            629 => .line_array,
            650 => .cidr,
            651 => .cidr_array,
            700 => .float4,
            701 => .float8,
            718 => .circle,
            719 => .circle_array,
            774 => .macaddr8,
            775 => .macaddr8_array,
            790 => .money,
            791 => .money_array,
            829 => .macaddr,
            869 => .inet,
            1000 => .bool_array,
            1001 => .bytea_array,
            1002 => .char_array,
            1003 => .name_array,
            1005 => .int2_array,
            1006 => .int2vector_array,
            1007 => .int4_array,
            1009 => .text_array,
            1010 => .tid_array,
            1011 => .xid_array,
            1012 => .cid_array,
            1014 => .bpchar_array,
            1015 => .varchar_array,
            1016 => .int8_array,
            1017 => .point_array,
            1018 => .lseg_array,
            1019 => .path_array,
            1020 => .box_array,
            1021 => .float4_array,
            1022 => .float8_array,
            1027 => .polygon_array,
            1028 => .oid_array,
            1033 => .aclitem,
            1034 => .aclitem_array,
            1040 => .macaddr_array,
            1041 => .inet_array,
            1042 => .bpchar,
            1043 => .varchar,
            1082 => .date,
            1083 => .time,
            1114 => .timestamp,
            1115 => .timestamp_array,
            1182 => .date_array,
            1183 => .time_array,
            1184 => .timestamptz,
            1185 => .timestamptz_array,
            1186 => .interval,
            1187 => .interval_array,
            1231 => .numeric_array,
            1248 => .pg_database,
            1266 => .timetz,
            1270 => .timetz_array,
            1560 => .bit,
            1561 => .bit_array,
            1562 => .varbit,
            1563 => .varbit_array,
            1700 => .numeric,
            2950 => .uuid,
            2951 => .uuid_array,
            3802 => .jsonb,
            3807 => .jsonb_array,
            4072 => .jsonpath,
            4073 => .jsonpath_array,
            else => .unknown,
        };
    }

    pub fn toOid(): u32 {
        return @intFromEnum(_);
    }

    pub fn isBinaryFormatSupported(): bool {
        return switch (_) {
            .pg_bool, .bytea, .int2, .int4, .int8, .float4, .float8,
            .date, .time, .timestamp, .timestamptz, .uuid, .jsonb, .numeric,
            .int4_array, .float4_array, .int8_array, .text_array
            => true,
            else => false,
        };
    }

    pub fn formatCode(): u16 {
        return if (_.isBinaryFormatSupported()) 1 else 0;
    }

    pub fn isArray(): bool {
        return switch (_) {
            .bool_array, .bytea_array, .char_array, .name_array, .int2_array,
            .int2vector_array, .int4_array, .text_array, .int8_array,
            .float4_array, .float8_array, .varchar_array, .date_array,
            .time_array, .timestamp_array, .timestamptz_array, .numeric_array,
            .uuid_array, .jsonb_array, .json_array
            => true,
            else => false,
        };
    }
}

pub enum PgCommandKind: u8 {
    INSERT = 0,
    DELETE = 1,
    UPDATE = 2,
    MERGE = 3,
    SELECT = 4,
    MOVE = 5,
    FETCH = 6,
    COPY = 7,
    OTHER = 8,
}

pub struct PgCommandTag {
    pub kind: PgCommandKind = .OTHER,
    pub rows: u64 = 0,
    pub raw: []const u8 = "",

    pub static fn parse(raw_tag: []const u8): PgCommandTag {
        let end = raw_tag.len;
        while (end > 0 && raw_tag[end - 1] == 0) end -= 1;
        const tag = raw_tag[0..end];

        const first_sp = std.mem.indexOfScalar(u8, tag, ' ') ?? {
            return PgCommandTag{ kind: .OTHER, rows: 0, raw: tag };
        };
        const verb = tag[0..first_sp];
        const rest = tag[@min(first_sp + 1, tag.len)..];

        if (std.mem.eql(u8, verb, "INSERT")) {
            const second_sp = std.mem.indexOfScalar(u8, rest, ' ') ?? {
                return PgCommandTag{ kind: .OTHER, rows: 0, raw: tag };
            };
            const num_s = rest[@min(second_sp + 1, rest.len)..];
            const n = std.fmt.parseInt(u64, num_s, 10) catch 0;
            return PgCommandTag{ kind: .INSERT, rows: n, raw: tag };
        }

        const kind: PgCommandKind = if (std.mem.eql(u8, verb, "DELETE"))
            .DELETE
        else if (std.mem.eql(u8, verb, "UPDATE"))
            .UPDATE
        else if (std.mem.eql(u8, verb, "MERGE"))
            .MERGE
        else if (std.mem.eql(u8, verb, "SELECT"))
            .SELECT
        else if (std.mem.eql(u8, verb, "MOVE"))
            .MOVE
        else if (std.mem.eql(u8, verb, "FETCH"))
            .FETCH
        else if (std.mem.eql(u8, verb, "COPY"))
            .COPY
        else
            .OTHER;

        if (kind == .OTHER) {
            return PgCommandTag{ kind: .OTHER, rows: 0, raw: tag };
        }
        const n = std.fmt.parseInt(u64, rest, 10) catch 0;
        return PgCommandTag{ kind: kind, rows: n, raw: tag };
    }
}

pub enum PgErrorCategory: u8 {
    SuccessfulCompletion = 0,
    Warning = 1,
    NoData = 2,
    ConnectionException = 3,
    DataException = 4,
    IntegrityConstraintViolation = 5,
    InvalidAuthorization = 6,
    TransactionRollback = 7,
    SyntaxOrAccessError = 8,
    InsufficientResources = 9,
    OperatorIntervention = 10,
    SystemError = 11,
    InternalError = 12,
    Unknown = 255,
}

pub struct PgErrorClassifier {
    pub static fn classify(sql_state: []const u8): PgErrorCategory {
        if (sql_state.len < 2) return .Unknown;
        const c0 = sql_state[0];
        const c1 = sql_state[1];
        if (c0 == '0' && c1 == '0') return .SuccessfulCompletion;
        if (c0 == '0' && c1 == '1') return .Warning;
        if (c0 == '0' && c1 == '2') return .NoData;
        if (c0 == '0' && c1 == '8') return .ConnectionException;
        if (c0 == '2' && c1 == '2') return .DataException;
        if (c0 == '2' && c1 == '3') return .IntegrityConstraintViolation;
        if (c0 == '2' && c1 == '8') return .InvalidAuthorization;
        if (c0 == '4' && c1 == '0') return .TransactionRollback;
        if (c0 == '4' && c1 == '2') return .SyntaxOrAccessError;
        if (c0 == '5' && c1 == '3') return .InsufficientResources;
        if (c0 == '5' && c1 == '7') return .OperatorIntervention;
        if (c0 == '5' && c1 == '8') return .SystemError;
        if (c0 == 'X' && c1 == 'X') return .InternalError;
        return .Unknown;
    }

    pub static fn isRetryable(sql_state: []const u8): bool {
        const cat = classify(sql_state);
        return cat == .TransactionRollback || cat == .ConnectionException || cat == .InsufficientResources;
    }

    pub static fn categoryName(cat: PgErrorCategory): []const u8 {
        return switch (cat) {
            .SuccessfulCompletion => "SuccessfulCompletion",
            .Warning => "Warning",
            .NoData => "NoData",
            .ConnectionException => "ConnectionException",
            .DataException => "DataException",
            .IntegrityConstraintViolation => "IntegrityConstraintViolation",
            .InvalidAuthorization => "InvalidAuthorization",
            .TransactionRollback => "TransactionRollback",
            .SyntaxOrAccessError => "SyntaxOrAccessError",
            .InsufficientResources => "InsufficientResources",
            .OperatorIntervention => "OperatorIntervention",
            .SystemError => "SystemError",
            .InternalError => "InternalError",
            .Unknown => "Unknown",
        };
    }
}

/// Postgres epoch (2000-01-01T00:00:00Z) offset from Unix epoch (1970-01-01) in milliseconds & microseconds.
pub const PG_EPOCH_UNIX_MS: i64 = 946684800000;
pub const PG_EPOCH_UNIX_DAYS: i32 = 10957;

pub struct PgBinaryArrayHeader {
    pub ndim: i32 = 0,
    pub has_null: i32 = 0,
    pub element_oid: u32 = 0,
    pub len: i32 = 0,
    pub lbound: i32 = 1,
    pub data_offset: usize = 0,
}

pub struct PgBinaryCodec {
    // --- Integer & Float Binary Codecs (Big-Endian) ---

    pub static fn encodeInt2(out: []u8, val: i16): usize {
        if (out.len < 2) return 0;
        const u: u16 = @bitCast(val);
        out[0] = ((u >> 8) & 0xFF) as u8;
        out[1] = (u & 0xFF) as u8;
        return 2;
    }

    pub static fn decodeInt2(bytes: []const u8): i16! {
        if (bytes.len < 2) return error.InvalidBinaryData;
        const u: u16 = ((bytes[0] as u16) << 8) | (bytes[1] as u16);
        return @bitCast(u);
    }

    pub static fn encodeInt4(out: []u8, val: i32): usize {
        if (out.len < 4) return 0;
        const u: u32 = @bitCast(val);
        out[0] = ((u >> 24) & 0xFF) as u8;
        out[1] = ((u >> 16) & 0xFF) as u8;
        out[2] = ((u >> 8) & 0xFF) as u8;
        out[3] = (u & 0xFF) as u8;
        return 4;
    }

    pub static fn decodeInt4(bytes: []const u8): i32! {
        if (bytes.len < 4) return error.InvalidBinaryData;
        const u: u32 = ((bytes[0] as u32) << 24) | ((bytes[1] as u32) << 16) | ((bytes[2] as u32) << 8) | (bytes[3] as u32);
        return @bitCast(u);
    }

    pub static fn encodeInt8(out: []u8, val: i64): usize {
        if (out.len < 8) return 0;
        const u: u64 = @bitCast(val);
        let i: usize = 0;
        while (i < 8) : (i += 1) {
            const shift: u6 = @intCast(56 - (i * 8));
            out[i] = ((u >> shift) & 0xFF) as u8;
        }
        return 8;
    }

    pub static fn decodeInt8(bytes: []const u8): i64! {
        if (bytes.len < 8) return error.InvalidBinaryData;
        let u: u64 = 0;
        let i: usize = 0;
        while (i < 8) : (i += 1) {
            u = (u << 8) | (bytes[i] as u64);
        }
        return @bitCast(u);
    }

    pub static fn encodeFloat4(out: []u8, val: f32): usize {
        const u: u32 = @bitCast(val);
        return encodeInt4(out, @bitCast(u));
    }

    pub static fn decodeFloat4(bytes: []const u8): f32! {
        const i = decodeInt4(bytes)?;
        const u: u32 = @bitCast(i);
        return @bitCast(u);
    }

    pub static fn encodeFloat8(out: []u8, val: f64): usize {
        const u: u64 = @bitCast(val);
        return encodeInt8(out, @bitCast(u));
    }

    pub static fn decodeFloat8(bytes: []const u8): f64! {
        const i = decodeInt8(bytes)?;
        const u: u64 = @bitCast(i);
        return @bitCast(u);
    }

    // --- Bool ---

    pub static fn encodeBool(out: []u8, val: bool): usize {
        if (out.len < 1) return 0;
        out[0] = if (val) 1 else 0;
        return 1;
    }

    pub static fn decodeBool(bytes: []const u8): bool! {
        if (bytes.len == 0) return error.InvalidBinaryData;
        if (bytes.len == 1) {
            if (bytes[0] == 1 || bytes[0] == 't' || bytes[0] == 'T' || bytes[0] == '1') return true;
            if (bytes[0] == 0 || bytes[0] == 'f' || bytes[0] == 'F' || bytes[0] == '0') return false;
            return error.InvalidBinaryData;
        }
        if (std.ascii.eqlIgnoreCase(bytes, "true")) return true;
        if (std.ascii.eqlIgnoreCase(bytes, "false")) return false;
        return error.InvalidBinaryData;
    }

    // --- Bytea (\x hex format) ---

    pub static fn decodeByteaText(bytes: []const u8, out: []u8): ([]u8)! {
        if (bytes.len >= 2 && bytes[0] == '\\' && bytes[1] == 'x') {
            const hex = bytes[2..];
            if ((hex.len % 2) != 0) return error.UnsupportedByteaFormat;
            const needed = hex.len / 2;
            if (out.len < needed) return error.BufferTooSmall;
            let i: usize = 0;
            while (i < needed) : (i += 1) {
                const hi = hexVal(hex[i * 2]) ?? return error.InvalidCharacter;
                const lo = hexVal(hex[i * 2 + 1]) ?? return error.InvalidCharacter;
                out[i] = (hi << 4) | lo;
            }
            return out[0..needed];
        }
        if (out.len < bytes.len) return error.BufferTooSmall;
        @memcpy(out[0..bytes.len], bytes);
        return out[0..bytes.len];
    }

    pub static fn encodeByteaHex(raw: []const u8, out: []u8): ([]u8)! {
        const needed = 2 + raw.len * 2;
        if (out.len < needed) return error.BufferTooSmall;
        out[0] = '\\';
        out[1] = 'x';
        const hex_chars = "0123456789abcdef";
        let i: usize = 0;
        while (i < raw.len) : (i += 1) {
            out[2 + i * 2] = hex_chars[(raw[i] >> 4) & 0x0F];
            out[2 + i * 2 + 1] = hex_chars[raw[i] & 0x0F];
        }
        return out[0..needed];
    }

    // --- UUID (16-byte binary <-> 36-byte string) ---

    pub static fn formatUuid(raw16: []const u8, out: *[36]u8): ([]const u8)! {
        if (raw16.len != 16) return error.InvalidBinaryData;
        const hex_chars = "0123456789abcdef";
        let pos: usize = 0;
        let i: usize = 0;
        while (i < 16) : (i += 1) {
            if (i == 4 || i == 6 || i == 8 || i == 10) {
                out[pos] = '-';
                pos += 1;
            }
            out[pos] = hex_chars[(raw16[i] >> 4) & 0x0F];
            out[pos + 1] = hex_chars[raw16[i] & 0x0F];
            pos += 2;
        }
        return out[0..36];
    }

    pub static fn parseUuid(uuid_str: []const u8, out: *[16]u8): ! {
        if (uuid_str.len != 36) return error.InvalidCharacter;
        let out_idx: usize = 0;
        let i: usize = 0;
        while (i < 36) {
            if (uuid_str[i] == '-') {
                i += 1;
                continue;
            }
            if (i + 1 >= 36 || out_idx >= 16) return error.InvalidCharacter;
            const hi = hexVal(uuid_str[i]) ?? return error.InvalidCharacter;
            const lo = hexVal(uuid_str[i + 1]) ?? return error.InvalidCharacter;
            out[out_idx] = (hi << 4) | lo;
            out_idx += 1;
            i += 2;
        }
        if (out_idx != 16) return error.InvalidCharacter;
    }

    // --- JSONB (1-byte version 0x01 + UTF-8 JSON text) ---

    pub static fn encodeJsonb(json_bytes: []const u8, out: []u8): ([]u8)! {
        if (out.len < 1 + json_bytes.len) return error.BufferTooSmall;
        out[0] = 0x01;
        if (json_bytes.len > 0) {
            @memcpy(out[1 .. 1 + json_bytes.len], json_bytes);
        }
        return out[0 .. 1 + json_bytes.len];
    }

    pub static fn decodeJsonb(bytes: []const u8): ([]const u8)! {
        if (bytes.len == 0 || bytes[0] != 0x01) return error.InvalidBinaryData;
        return bytes[1..];
    }

    // --- Date & Timestamp (PostgreSQL 2000-01-01 Epoch <-> Unix ms) ---

    pub static fn encodeTimestampUnixMs(out: []u8, unix_ms: i64): usize {
        const pg_us = (unix_ms - PG_EPOCH_UNIX_MS) * 1000;
        return encodeInt8(out, pg_us);
    }

    pub static fn decodeTimestampUnixMs(bytes: []const u8): i64! {
        const pg_us = decodeInt8(bytes)?;
        return @divFloor(pg_us, 1000) + PG_EPOCH_UNIX_MS;
    }

    pub static fn encodeDateUnixDays(out: []u8, unix_days: i32): usize {
        return encodeInt4(out, unix_days - PG_EPOCH_UNIX_DAYS);
    }

    pub static fn decodeDateUnixDays(bytes: []const u8): i32! {
        const pg_days = decodeInt4(bytes)?;
        return pg_days + PG_EPOCH_UNIX_DAYS;
    }

    // --- NUMERIC Binary Decoder (base-10000 digits -> ASCII decimal string) ---

    pub static fn decodeNumericText(bytes: []const u8, out: []u8): ([]u8)! {
        if (bytes.len < 8) return error.UnsupportedNumericFormat;
        const ndigits = (((bytes[0] as u16) << 8) | (bytes[1] as u16)) as usize;
        const weight: i16 = @bitCast(((bytes[2] as u16) << 8) | (bytes[3] as u16));
        const sign: u16 = ((bytes[4] as u16) << 8) | (bytes[5] as u16);
        const dscale: u16 = ((bytes[6] as u16) << 8) | (bytes[7] as u16);

        if (sign == 0xC000) {
            if (out.len < 3) return error.BufferTooSmall;
            @memcpy(out[0..3], "NaN");
            return out[0..3];
        }
        if (sign == 0xD000) {
            if (out.len < 8) return error.BufferTooSmall;
            @memcpy(out[0..8], "Infinity");
            return out[0..8];
        }
        if (sign == 0xF000) {
            if (out.len < 9) return error.BufferTooSmall;
            @memcpy(out[0..9], "-Infinity");
            return out[0..9];
        }
        if (sign != 0x0000 && sign != 0x4000) return error.UnsupportedNumericFormat;
        if (bytes.len < 8 + ndigits * 2) return error.InvalidBinaryData;

        let pos: usize = 0;
        if (sign == 0x4000) {
            if (pos >= out.len) return error.BufferTooSmall;
            out[pos] = '-';
            pos += 1;
        }

        if (ndigits == 0) {
            if (pos >= out.len) return error.BufferTooSmall;
            out[pos] = '0';
            pos += 1;
            if (dscale > 0) {
                if (pos >= out.len) return error.BufferTooSmall;
                out[pos] = '.';
                pos += 1;
                let z: u16 = 0;
                while (z < dscale) : (z += 1) {
                    if (pos >= out.len) return error.BufferTooSmall;
                    out[pos] = '0';
                    pos += 1;
                }
            }
            return out[0..pos];
        }

        // Integer part (weight >= 0 -> groups 0..weight)
        let digit_idx: usize = 0;
        if (weight < 0) {
            if (pos >= out.len) return error.BufferTooSmall;
            out[pos] = '0';
            pos += 1;
        } else {
            let w: i32 = weight as i32;
            let first = true;
            while (w >= 0) : (w -= 1) {
                let grp: u16 = 0;
                if (digit_idx < ndigits) {
                    const off = 8 + digit_idx * 2;
                    grp = ((bytes[off] as u16) << 8) | (bytes[off + 1] as u16);
                    digit_idx += 1;
                }
                if (first) {
                    const s = std.fmt.bufPrint(out[pos..], "{d}", .{grp}) catch return error.BufferTooSmall;
                    pos += s.len;
                    first = false;
                } else {
                    const s = std.fmt.bufPrint(out[pos..], "{d:0>4}", .{grp}) catch return error.BufferTooSmall;
                    pos += s.len;
                }
            }
        }

        // Fractional part
        if (dscale > 0) {
            if (pos >= out.len) return error.BufferTooSmall;
            out[pos] = '.';
            pos += 1;

            let frac_written: usize = 0;
            const target_frac = dscale as usize;

            if (weight < -1) {
                let lead_zero_groups = (-weight - 1) as usize;
                while (lead_zero_groups > 0 && frac_written < target_frac) {
                    const take_n = @min(4 as usize, target_frac - frac_written);
                    let k: usize = 0;
                    while (k < take_n) : (k += 1) {
                        if (pos >= out.len) return error.BufferTooSmall;
                        out[pos] = '0';
                        pos += 1;
                    }
                    frac_written += take_n;
                    lead_zero_groups -= 1;
                }
            }

            while (frac_written < target_frac) {
                let grp: u16 = 0;
                if (digit_idx < ndigits) {
                    const off = 8 + digit_idx * 2;
                    grp = ((bytes[off] as u16) << 8) | (bytes[off + 1] as u16);
                    digit_idx += 1;
                }
                let gbuf: [4]u8 = undefined;
                * = std.fmt.bufPrint(&gbuf, "{d:0>4}", .{grp}) catch return error.BufferTooSmall;
                const take_n = @min(4 as usize, target_frac - frac_written);
                if (pos + take_n > out.len) return error.BufferTooSmall;
                @memcpy(out[pos .. pos + take_n], gbuf[0..take_n]);
                pos += take_n;
                frac_written += take_n;
            }
        }

        return out[0..pos];
    }

    // --- Single-Dimension Binary Array Header & Int4/Float4 Array Decoder ---

    pub static fn decodeArrayHeader(bytes: []const u8): PgBinaryArrayHeader! {
        if (bytes.len < 12) return error.InvalidBinaryData;
        const ndim = decodeInt4(bytes[0..4])?;
        if (ndim == 0) {
            const has_null = decodeInt4(bytes[4..8])?;
            const elem_oid = @as(u32, @bitCast(decodeInt4(bytes[8..12])?));
            return PgBinaryArrayHeader{
                ndim: 0,
                has_null: has_null,
                element_oid: elem_oid,
                len: 0,
                lbound: 1,
                data_offset: 12,
            };
        }
        if (ndim > 1) return error.MultidimensionalArrayNotSupportedYet;
        if (bytes.len < 20) return error.InvalidBinaryData;
        const has_null = decodeInt4(bytes[4..8])?;
        const elem_oid = @as(u32, @bitCast(decodeInt4(bytes[8..12])?));
        const len = decodeInt4(bytes[12..16])?;
        const lbound = decodeInt4(bytes[16..20])?;
        return PgBinaryArrayHeader{
            ndim: ndim,
            has_null: has_null,
            element_oid: elem_oid,
            len: len,
            lbound: lbound,
            data_offset: 20,
        };
    }

    pub static fn decodeInt4Array(bytes: []const u8, out: []i32): ([]i32)! {
        const hdr = decodeArrayHeader(bytes)?;
        if (hdr.len <= 0) return out[0..0];
        const count = hdr.len as usize;
        if (out.len < count) return error.BufferTooSmall;
        let pos = hdr.data_offset;
        let i: usize = 0;
        while (i < count) : (i += 1) {
            if (pos + 4 > bytes.len) return error.InvalidBinaryData;
            const elem_len = decodeInt4(bytes[pos .. pos + 4])?;
            pos += 4;
            if (elem_len < 0) return error.NullsInArrayNotSupportedYet;
            if (elem_len != 4 || pos + 4 > bytes.len) return error.InvalidBinaryData;
            out[i] = decodeInt4(bytes[pos .. pos + 4])?;
            pos += 4;
        }
        return out[0..count];
    }

    static fn hexVal(c: u8): u8? {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return null;
    }
}
