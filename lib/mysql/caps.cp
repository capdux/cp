// Source: Absorbed from /data/dev/bun/src/sql/mysql/{Capabilities,StatusFlags}.cp

/// MySQL 4.1–8.4 协议 32-bit 客户端/服务端能力位枚举
pub enum CapabilityFlag: u32 {
    CLIENT_LONG_PASSWORD = 1,
    CLIENT_FOUND_ROWS = 2,
    CLIENT_LONG_FLAG = 4,
    CLIENT_CONNECT_WITH_DB = 8,
    CLIENT_NO_SCHEMA = 16,
    CLIENT_COMPRESS = 32,
    CLIENT_ODBC = 64,
    CLIENT_LOCAL_FILES = 128,
    CLIENT_IGNORE_SPACE = 256,
    CLIENT_PROTOCOL_41 = 512,
    CLIENT_INTERACTIVE = 1024,
    CLIENT_SSL = 2048,
    CLIENT_IGNORE_SIGPIPE = 4096,
    CLIENT_TRANSACTIONS = 8192,
    CLIENT_RESERVED = 16384,
    CLIENT_SECURE_CONNECTION = 32768,
    CLIENT_MULTI_STATEMENTS = 65536,
    CLIENT_MULTI_RESULTS = 131072,
    CLIENT_PS_MULTI_RESULTS = 262144,
    CLIENT_PLUGIN_AUTH = 524288,
    CLIENT_CONNECT_ATTRS = 1048576,
    CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA = 2097152,
    CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS = 4194304,
    CLIENT_SESSION_TRACK = 8388608,
    CLIENT_DEPRECATE_EOF = 16777216,
    CLIENT_OPTIONAL_RESULTSET_METADATA = 33554432,
    CLIENT_ZSTD_COMPRESSION_ALGORITHM = 67108864,
    CLIENT_QUERY_ATTRIBUTES = 134217728,
    MULTI_FACTOR_AUTHENTICATION = 268435456,
    CLIENT_CAPABILITY_EXTENSION = 536870912,
    CLIENT_SSL_VERIFY_SERVER_CERT = 1073741824,
    CLIENT_REMEMBER_OPTIONS = 2147483648,

    fn bitName(): string {
        return switch (_) {
            .CLIENT_LONG_PASSWORD => "CLIENT_LONG_PASSWORD",
            .CLIENT_FOUND_ROWS => "CLIENT_FOUND_ROWS",
            .CLIENT_LONG_FLAG => "CLIENT_LONG_FLAG",
            .CLIENT_CONNECT_WITH_DB => "CLIENT_CONNECT_WITH_DB",
            .CLIENT_NO_SCHEMA => "CLIENT_NO_SCHEMA",
            .CLIENT_COMPRESS => "CLIENT_COMPRESS",
            .CLIENT_ODBC => "CLIENT_ODBC",
            .CLIENT_LOCAL_FILES => "CLIENT_LOCAL_FILES",
            .CLIENT_IGNORE_SPACE => "CLIENT_IGNORE_SPACE",
            .CLIENT_PROTOCOL_41 => "CLIENT_PROTOCOL_41",
            .CLIENT_INTERACTIVE => "CLIENT_INTERACTIVE",
            .CLIENT_SSL => "CLIENT_SSL",
            .CLIENT_IGNORE_SIGPIPE => "CLIENT_IGNORE_SIGPIPE",
            .CLIENT_TRANSACTIONS => "CLIENT_TRANSACTIONS",
            .CLIENT_RESERVED => "CLIENT_RESERVED",
            .CLIENT_SECURE_CONNECTION => "CLIENT_SECURE_CONNECTION",
            .CLIENT_MULTI_STATEMENTS => "CLIENT_MULTI_STATEMENTS",
            .CLIENT_MULTI_RESULTS => "CLIENT_MULTI_RESULTS",
            .CLIENT_PS_MULTI_RESULTS => "CLIENT_PS_MULTI_RESULTS",
            .CLIENT_PLUGIN_AUTH => "CLIENT_PLUGIN_AUTH",
            .CLIENT_CONNECT_ATTRS => "CLIENT_CONNECT_ATTRS",
            .CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA => "CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA",
            .CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS => "CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS",
            .CLIENT_SESSION_TRACK => "CLIENT_SESSION_TRACK",
            .CLIENT_DEPRECATE_EOF => "CLIENT_DEPRECATE_EOF",
            .CLIENT_OPTIONAL_RESULTSET_METADATA => "CLIENT_OPTIONAL_RESULTSET_METADATA",
            .CLIENT_ZSTD_COMPRESSION_ALGORITHM => "CLIENT_ZSTD_COMPRESSION_ALGORITHM",
            .CLIENT_QUERY_ATTRIBUTES => "CLIENT_QUERY_ATTRIBUTES",
            .MULTI_FACTOR_AUTHENTICATION => "MULTI_FACTOR_AUTHENTICATION",
            .CLIENT_CAPABILITY_EXTENSION => "CLIENT_CAPABILITY_EXTENSION",
            .CLIENT_SSL_VERIFY_SERVER_CERT => "CLIENT_SSL_VERIFY_SERVER_CERT",
            .CLIENT_REMEMBER_OPTIONS => "CLIENT_REMEMBER_OPTIONS",
        };
    }
}

const CAP_NAMES: [32]string = [
    "CLIENT_LONG_PASSWORD",
    "CLIENT_FOUND_ROWS",
    "CLIENT_LONG_FLAG",
    "CLIENT_CONNECT_WITH_DB",
    "CLIENT_NO_SCHEMA",
    "CLIENT_COMPRESS",
    "CLIENT_ODBC",
    "CLIENT_LOCAL_FILES",
    "CLIENT_IGNORE_SPACE",
    "CLIENT_PROTOCOL_41",
    "CLIENT_INTERACTIVE",
    "CLIENT_SSL",
    "CLIENT_IGNORE_SIGPIPE",
    "CLIENT_TRANSACTIONS",
    "CLIENT_RESERVED",
    "CLIENT_SECURE_CONNECTION",
    "CLIENT_MULTI_STATEMENTS",
    "CLIENT_MULTI_RESULTS",
    "CLIENT_PS_MULTI_RESULTS",
    "CLIENT_PLUGIN_AUTH",
    "CLIENT_CONNECT_ATTRS",
    "CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA",
    "CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS",
    "CLIENT_SESSION_TRACK",
    "CLIENT_DEPRECATE_EOF",
    "CLIENT_OPTIONAL_RESULTSET_METADATA",
    "CLIENT_ZSTD_COMPRESSION_ALGORITHM",
    "CLIENT_QUERY_ATTRIBUTES",
    "MULTI_FACTOR_AUTHENTICATION",
    "CLIENT_CAPABILITY_EXTENSION",
    "CLIENT_SSL_VERIFY_SERVER_CERT",
    "CLIENT_REMEMBER_OPTIONS",
];

const UNSUPPORTED_CAPS_MASK: u32 =
    @intFromEnum(CapabilityFlag.CLIENT_ZSTD_COMPRESSION_ALGORITHM) |
    @intFromEnum(CapabilityFlag.MULTI_FACTOR_AUTHENTICATION) |
    @intFromEnum(CapabilityFlag.CLIENT_CAPABILITY_EXTENSION) |
    @intFromEnum(CapabilityFlag.CLIENT_SSL_VERIFY_SERVER_CERT) |
    @intFromEnum(CapabilityFlag.CLIENT_REMEMBER_OPTIONS) |
    @intFromEnum(CapabilityFlag.CLIENT_COMPRESS) |
    @intFromEnum(CapabilityFlag.CLIENT_INTERACTIVE) |
    @intFromEnum(CapabilityFlag.CLIENT_IGNORE_SIGPIPE) |
    @intFromEnum(CapabilityFlag.CLIENT_NO_SCHEMA) |
    @intFromEnum(CapabilityFlag.CLIENT_ODBC) |
    @intFromEnum(CapabilityFlag.CLIENT_LOCAL_FILES) |
    @intFromEnum(CapabilityFlag.CLIENT_OPTIONAL_RESULTSET_METADATA) |
    @intFromEnum(CapabilityFlag.CLIENT_QUERY_ATTRIBUTES);

const DEFAULT_CLIENT_CAPS_MASK: u32 =
    @intFromEnum(CapabilityFlag.CLIENT_LONG_PASSWORD) |
    @intFromEnum(CapabilityFlag.CLIENT_LONG_FLAG) |
    @intFromEnum(CapabilityFlag.CLIENT_CONNECT_WITH_DB) |
    @intFromEnum(CapabilityFlag.CLIENT_PROTOCOL_41) |
    @intFromEnum(CapabilityFlag.CLIENT_TRANSACTIONS) |
    @intFromEnum(CapabilityFlag.CLIENT_SECURE_CONNECTION) |
    @intFromEnum(CapabilityFlag.CLIENT_MULTI_RESULTS) |
    @intFromEnum(CapabilityFlag.CLIENT_PS_MULTI_RESULTS) |
    @intFromEnum(CapabilityFlag.CLIENT_PLUGIN_AUTH) |
    @intFromEnum(CapabilityFlag.CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA) |
    @intFromEnum(CapabilityFlag.CLIENT_SESSION_TRACK) |
    @intFromEnum(CapabilityFlag.CLIENT_DEPRECATE_EOF);

/// MySQL 32-bit 能力位集合（零堆分配）
pub struct Capabilities {
    raw: u32 = 0,

    static fn fromInt(v: u32): Capabilities {
        return Capabilities{ raw: v };
    }

    fn toInt(): u32 {
        return _.raw;
    }

    fn has(flag: CapabilityFlag): bool {
        return (_.raw & @as(u32, @intFromEnum(flag))) != 0;
    }

    fn set(*_, flag: CapabilityFlag) {
        _.raw |= @as(u32, @intFromEnum(flag));
    }

    fn clear(*_, flag: CapabilityFlag) {
        _.raw &= ~@as(u32, @intFromEnum(flag));
    }

    /// 客户端与服务端握手能力协商交集
    fn intersect(server_caps: Capabilities): Capabilities {
        return Capabilities{ raw: _.raw & server_caps.raw };
    }

    /// 默认安全现代客户端能力集
    static fn defaultClient(): Capabilities {
        return Capabilities{ raw: DEFAULT_CLIENT_CAPS_MASK };
    }

    /// 剔除 ZSTD / MFA / ODBC / LOCAL_FILES 等未支持或高风险扩展位
    fn rejectUnsupported(*_) {
        _.raw &= ~UNSUPPORTED_CAPS_MASK;
    }

    /// 格式化为逗号分隔的人类可读能力标志列表写入 `out`
    fn formatList(out: []u8): []const u8 {
        let pos: usize = 0;
        let first = true;
        let bit: usize = 0;
        while (bit < 32) {
            const shift: u5 = @intCast(bit);
            const mask: u32 = @as(u32, 1) << shift;
            if ((_.raw & mask) != 0) {
                if (!first) {
                    if (pos + 2 <= out.len) {
                        out[pos] = ',';
                        out[pos + 1] = ' ';
                        pos += 2;
                    } else {
                        break;
                    }
                }
                first = false;
                const label = CAP_NAMES[bit];
                let k: usize = 0;
                while (k < label.len && pos < out.len) {
                    out[pos] = label[k];
                    pos += 1;
                    k += 1;
                }
            }
            bit += 1;
        }
        return out[0..pos];
    }
}

/// MySQL 16-bit 服务器状态标志枚举（全部 14 个标准状态位）
pub enum StatusFlag: u16 {
    SERVER_STATUS_IN_TRANS = 1,
    SERVER_STATUS_AUTOCOMMIT = 2,
    SERVER_MORE_RESULTS_EXISTS = 8,
    SERVER_STATUS_NO_GOOD_INDEX_USED = 16,
    SERVER_STATUS_NO_INDEX_USED = 32,
    SERVER_STATUS_CURSOR_EXISTS = 64,
    SERVER_STATUS_LAST_ROW_SENT = 128,
    SERVER_STATUS_DB_DROPPED = 256,
    SERVER_STATUS_NO_BACKSLASH_ESCAPES = 512,
    SERVER_STATUS_METADATA_CHANGED = 1024,
    SERVER_QUERY_WAS_SLOW = 2048,
    SERVER_PS_OUT_PARAMS = 4096,
    SERVER_STATUS_IN_TRANS_READONLY = 8192,
    SERVER_SESSION_STATE_CHANGED = 16384,
}

/// MySQL 16-bit 服务器状态标志集合
pub struct StatusFlags {
    raw: u16 = 0,

    static fn fromInt(v: u16): StatusFlags {
        return StatusFlags{ raw: v };
    }

    fn toInt(): u16 {
        return _.raw;
    }

    fn has(flag: StatusFlag): bool {
        return (_.raw & @as(u16, @intFromEnum(flag))) != 0;
    }

    fn inTransaction(): bool {
        return _.has(.SERVER_STATUS_IN_TRANS);
    }

    fn isAutoCommit(): bool {
        return _.has(.SERVER_STATUS_AUTOCOMMIT);
    }

    fn hasMoreResults(): bool {
        return _.has(.SERVER_MORE_RESULTS_EXISTS);
    }

    fn isReadOnlyTransaction(): bool {
        return _.has(.SERVER_STATUS_IN_TRANS_READONLY);
    }
}
