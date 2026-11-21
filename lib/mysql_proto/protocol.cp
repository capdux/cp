// Source: Absorbed from /data/dev/bun/src/sql/mysql/Capabilities.cp, StatusFlags.cp, protocol/CharacterSet.cp

/// MySQL 协议客户端/服务端能力位掩码常量 (32-bit)
pub const CAP_CLIENT_LONG_PASSWORD: u32 = 1 << 0;
pub const CAP_CLIENT_FOUND_ROWS: u32 = 1 << 1;
pub const CAP_CLIENT_LONG_FLAG: u32 = 1 << 2;
pub const CAP_CLIENT_CONNECT_WITH_DB: u32 = 1 << 3;
pub const CAP_CLIENT_NO_SCHEMA: u32 = 1 << 4;
pub const CAP_CLIENT_COMPRESS: u32 = 1 << 5;
pub const CAP_CLIENT_ODBC: u32 = 1 << 6;
pub const CAP_CLIENT_LOCAL_FILES: u32 = 1 << 7;
pub const CAP_CLIENT_IGNORE_SPACE: u32 = 1 << 8;
pub const CAP_CLIENT_PROTOCOL_41: u32 = 1 << 9;
pub const CAP_CLIENT_INTERACTIVE: u32 = 1 << 10;
pub const CAP_CLIENT_SSL: u32 = 1 << 11;
pub const CAP_CLIENT_IGNORE_SIGPIPE: u32 = 1 << 12;
pub const CAP_CLIENT_TRANSACTIONS: u32 = 1 << 13;
pub const CAP_CLIENT_RESERVED: u32 = 1 << 14;
pub const CAP_CLIENT_SECURE_CONNECTION: u32 = 1 << 15;
pub const CAP_CLIENT_MULTI_STATEMENTS: u32 = 1 << 16;
pub const CAP_CLIENT_MULTI_RESULTS: u32 = 1 << 17;
pub const CAP_CLIENT_PS_MULTI_RESULTS: u32 = 1 << 18;
pub const CAP_CLIENT_PLUGIN_AUTH: u32 = 1 << 19;
pub const CAP_CLIENT_CONNECT_ATTRS: u32 = 1 << 20;
pub const CAP_CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA: u32 = 1 << 21;
pub const CAP_CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS: u32 = 1 << 22;
pub const CAP_CLIENT_SESSION_TRACK: u32 = 1 << 23;
pub const CAP_CLIENT_DEPRECATE_EOF: u32 = 1 << 24;
pub const CAP_CLIENT_OPTIONAL_RESULTSET_METADATA: u32 = 1 << 25;
pub const CAP_CLIENT_ZSTD_COMPRESSION_ALGORITHM: u32 = 1 << 26;
pub const CAP_CLIENT_QUERY_ATTRIBUTES: u32 = 1 << 27;
pub const CAP_MULTI_FACTOR_AUTHENTICATION: u32 = 1 << 28;
pub const CAP_CLIENT_CAPABILITY_EXTENSION: u32 = 1 << 29;
pub const CAP_CLIENT_SSL_VERIFY_SERVER_CERT: u32 = 1 << 30;
pub const CAP_CLIENT_REMEMBER_OPTIONS: u32 = 1 << 31;

/// MySQL 握手能力标志位集合 (32-bit)
pub struct Capabilities {
    CLIENT_LONG_PASSWORD: bool = false,
    CLIENT_FOUND_ROWS: bool = false,
    CLIENT_LONG_FLAG: bool = false,
    CLIENT_CONNECT_WITH_DB: bool = false,
    CLIENT_NO_SCHEMA: bool = false,
    CLIENT_COMPRESS: bool = false,
    CLIENT_ODBC: bool = false,
    CLIENT_LOCAL_FILES: bool = false,
    CLIENT_IGNORE_SPACE: bool = false,
    CLIENT_PROTOCOL_41: bool = false,
    CLIENT_INTERACTIVE: bool = false,
    CLIENT_SSL: bool = false,
    CLIENT_IGNORE_SIGPIPE: bool = false,
    CLIENT_TRANSACTIONS: bool = false,
    CLIENT_RESERVED: bool = false,
    CLIENT_SECURE_CONNECTION: bool = false,
    CLIENT_MULTI_STATEMENTS: bool = false,
    CLIENT_MULTI_RESULTS: bool = false,
    CLIENT_PS_MULTI_RESULTS: bool = false,
    CLIENT_PLUGIN_AUTH: bool = false,
    CLIENT_CONNECT_ATTRS: bool = false,
    CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA: bool = false,
    CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS: bool = false,
    CLIENT_SESSION_TRACK: bool = false,
    CLIENT_DEPRECATE_EOF: bool = false,
    CLIENT_OPTIONAL_RESULTSET_METADATA: bool = false,
    CLIENT_ZSTD_COMPRESSION_ALGORITHM: bool = false,
    CLIENT_QUERY_ATTRIBUTES: bool = false,
    MULTI_FACTOR_AUTHENTICATION: bool = false,
    CLIENT_CAPABILITY_EXTENSION: bool = false,
    CLIENT_SSL_VERIFY_SERVER_CERT: bool = false,
    CLIENT_REMEMBER_OPTIONS: bool = false,

    /// 编码为 MySQL 32 位能力位掩码
    fn toU32(): u32 {
        let v: u32 = 0;
        if (_.CLIENT_LONG_PASSWORD) { v |= CAP_CLIENT_LONG_PASSWORD; }
        if (_.CLIENT_FOUND_ROWS) { v |= CAP_CLIENT_FOUND_ROWS; }
        if (_.CLIENT_LONG_FLAG) { v |= CAP_CLIENT_LONG_FLAG; }
        if (_.CLIENT_CONNECT_WITH_DB) { v |= CAP_CLIENT_CONNECT_WITH_DB; }
        if (_.CLIENT_NO_SCHEMA) { v |= CAP_CLIENT_NO_SCHEMA; }
        if (_.CLIENT_COMPRESS) { v |= CAP_CLIENT_COMPRESS; }
        if (_.CLIENT_ODBC) { v |= CAP_CLIENT_ODBC; }
        if (_.CLIENT_LOCAL_FILES) { v |= CAP_CLIENT_LOCAL_FILES; }
        if (_.CLIENT_IGNORE_SPACE) { v |= CAP_CLIENT_IGNORE_SPACE; }
        if (_.CLIENT_PROTOCOL_41) { v |= CAP_CLIENT_PROTOCOL_41; }
        if (_.CLIENT_INTERACTIVE) { v |= CAP_CLIENT_INTERACTIVE; }
        if (_.CLIENT_SSL) { v |= CAP_CLIENT_SSL; }
        if (_.CLIENT_IGNORE_SIGPIPE) { v |= CAP_CLIENT_IGNORE_SIGPIPE; }
        if (_.CLIENT_TRANSACTIONS) { v |= CAP_CLIENT_TRANSACTIONS; }
        if (_.CLIENT_RESERVED) { v |= CAP_CLIENT_RESERVED; }
        if (_.CLIENT_SECURE_CONNECTION) { v |= CAP_CLIENT_SECURE_CONNECTION; }
        if (_.CLIENT_MULTI_STATEMENTS) { v |= CAP_CLIENT_MULTI_STATEMENTS; }
        if (_.CLIENT_MULTI_RESULTS) { v |= CAP_CLIENT_MULTI_RESULTS; }
        if (_.CLIENT_PS_MULTI_RESULTS) { v |= CAP_CLIENT_PS_MULTI_RESULTS; }
        if (_.CLIENT_PLUGIN_AUTH) { v |= CAP_CLIENT_PLUGIN_AUTH; }
        if (_.CLIENT_CONNECT_ATTRS) { v |= CAP_CLIENT_CONNECT_ATTRS; }
        if (_.CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA) { v |= CAP_CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA; }
        if (_.CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS) { v |= CAP_CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS; }
        if (_.CLIENT_SESSION_TRACK) { v |= CAP_CLIENT_SESSION_TRACK; }
        if (_.CLIENT_DEPRECATE_EOF) { v |= CAP_CLIENT_DEPRECATE_EOF; }
        if (_.CLIENT_OPTIONAL_RESULTSET_METADATA) { v |= CAP_CLIENT_OPTIONAL_RESULTSET_METADATA; }
        if (_.CLIENT_ZSTD_COMPRESSION_ALGORITHM) { v |= CAP_CLIENT_ZSTD_COMPRESSION_ALGORITHM; }
        if (_.CLIENT_QUERY_ATTRIBUTES) { v |= CAP_CLIENT_QUERY_ATTRIBUTES; }
        if (_.MULTI_FACTOR_AUTHENTICATION) { v |= CAP_MULTI_FACTOR_AUTHENTICATION; }
        if (_.CLIENT_CAPABILITY_EXTENSION) { v |= CAP_CLIENT_CAPABILITY_EXTENSION; }
        if (_.CLIENT_SSL_VERIFY_SERVER_CERT) { v |= CAP_CLIENT_SSL_VERIFY_SERVER_CERT; }
        if (_.CLIENT_REMEMBER_OPTIONS) { v |= CAP_CLIENT_REMEMBER_OPTIONS; }
        return v;
    }

    /// 从 32 位位掩码解码为 Capabilities
    static fn fromU32(flags: u32): Capabilities {
        return Capabilities{
            CLIENT_LONG_PASSWORD: (flags & CAP_CLIENT_LONG_PASSWORD) != 0,
            CLIENT_FOUND_ROWS: (flags & CAP_CLIENT_FOUND_ROWS) != 0,
            CLIENT_LONG_FLAG: (flags & CAP_CLIENT_LONG_FLAG) != 0,
            CLIENT_CONNECT_WITH_DB: (flags & CAP_CLIENT_CONNECT_WITH_DB) != 0,
            CLIENT_NO_SCHEMA: (flags & CAP_CLIENT_NO_SCHEMA) != 0,
            CLIENT_COMPRESS: (flags & CAP_CLIENT_COMPRESS) != 0,
            CLIENT_ODBC: (flags & CAP_CLIENT_ODBC) != 0,
            CLIENT_LOCAL_FILES: (flags & CAP_CLIENT_LOCAL_FILES) != 0,
            CLIENT_IGNORE_SPACE: (flags & CAP_CLIENT_IGNORE_SPACE) != 0,
            CLIENT_PROTOCOL_41: (flags & CAP_CLIENT_PROTOCOL_41) != 0,
            CLIENT_INTERACTIVE: (flags & CAP_CLIENT_INTERACTIVE) != 0,
            CLIENT_SSL: (flags & CAP_CLIENT_SSL) != 0,
            CLIENT_IGNORE_SIGPIPE: (flags & CAP_CLIENT_IGNORE_SIGPIPE) != 0,
            CLIENT_TRANSACTIONS: (flags & CAP_CLIENT_TRANSACTIONS) != 0,
            CLIENT_RESERVED: (flags & CAP_CLIENT_RESERVED) != 0,
            CLIENT_SECURE_CONNECTION: (flags & CAP_CLIENT_SECURE_CONNECTION) != 0,
            CLIENT_MULTI_STATEMENTS: (flags & CAP_CLIENT_MULTI_STATEMENTS) != 0,
            CLIENT_MULTI_RESULTS: (flags & CAP_CLIENT_MULTI_RESULTS) != 0,
            CLIENT_PS_MULTI_RESULTS: (flags & CAP_CLIENT_PS_MULTI_RESULTS) != 0,
            CLIENT_PLUGIN_AUTH: (flags & CAP_CLIENT_PLUGIN_AUTH) != 0,
            CLIENT_CONNECT_ATTRS: (flags & CAP_CLIENT_CONNECT_ATTRS) != 0,
            CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA: (flags & CAP_CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA) != 0,
            CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS: (flags & CAP_CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS) != 0,
            CLIENT_SESSION_TRACK: (flags & CAP_CLIENT_SESSION_TRACK) != 0,
            CLIENT_DEPRECATE_EOF: (flags & CAP_CLIENT_DEPRECATE_EOF) != 0,
            CLIENT_OPTIONAL_RESULTSET_METADATA: (flags & CAP_CLIENT_OPTIONAL_RESULTSET_METADATA) != 0,
            CLIENT_ZSTD_COMPRESSION_ALGORITHM: (flags & CAP_CLIENT_ZSTD_COMPRESSION_ALGORITHM) != 0,
            CLIENT_QUERY_ATTRIBUTES: (flags & CAP_CLIENT_QUERY_ATTRIBUTES) != 0,
            MULTI_FACTOR_AUTHENTICATION: (flags & CAP_MULTI_FACTOR_AUTHENTICATION) != 0,
            CLIENT_CAPABILITY_EXTENSION: (flags & CAP_CLIENT_CAPABILITY_EXTENSION) != 0,
            CLIENT_SSL_VERIFY_SERVER_CERT: (flags & CAP_CLIENT_SSL_VERIFY_SERVER_CERT) != 0,
            CLIENT_REMEMBER_OPTIONS: (flags & CAP_CLIENT_REMEMBER_OPTIONS) != 0,
        };
    }

    /// 构造默认客户端握手能力集
    static fn defaultClient(ssl: bool, has_db_name: bool): Capabilities {
        return Capabilities{
            CLIENT_PROTOCOL_41: true,
            CLIENT_PLUGIN_AUTH: true,
            CLIENT_SECURE_CONNECTION: true,
            CLIENT_CONNECT_WITH_DB: has_db_name,
            CLIENT_DEPRECATE_EOF: true,
            CLIENT_SSL: ssl,
            CLIENT_MULTI_STATEMENTS: true,
            CLIENT_MULTI_RESULTS: true,
        };
    }

    /// 检测是否包含指定能力位掩码（支持单个或组合位掩码；0 掩码返回 false）
    fn hasFlag(mask: u32): bool {
        if (mask == 0) {
            return false;
        }
        return (_.toU32() & mask) == mask;
    }

    /// 计算与另一能力集的交集（常用于客户端/服务端握手协商）
    fn intersect(other: Capabilities): Capabilities {
        return Capabilities.fromU32(_.toU32() & other.toU32());
    }

    /// 计算与另一能力集的并集
    fn unionWith(other: Capabilities): Capabilities {
        return Capabilities.fromU32(_.toU32() | other.toU32());
    }

    /// 清除当前驱动不支持的压缩、MFA 与扩展能力位
    fn rejectUnsupported(*_) {
        _.CLIENT_ZSTD_COMPRESSION_ALGORITHM = false;
        _.MULTI_FACTOR_AUTHENTICATION = false;
        _.CLIENT_CAPABILITY_EXTENSION = false;
        _.CLIENT_SSL_VERIFY_SERVER_CERT = false;
        _.CLIENT_REMEMBER_OPTIONS = false;
        _.CLIENT_COMPRESS = false;
        _.CLIENT_INTERACTIVE = false;
        _.CLIENT_IGNORE_SIGPIPE = false;
        _.CLIENT_NO_SCHEMA = false;
        _.CLIENT_ODBC = false;
        _.CLIENT_LOCAL_FILES = false;
        _.CLIENT_OPTIONAL_RESULTSET_METADATA = false;
        _.CLIENT_QUERY_ATTRIBUTES = false;
    }
}

/// MySQL 服务端状态标志枚举 (16-bit)
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

/// MySQL 服务端状态标志位集合 (16-bit)
pub struct StatusFlags {
    raw: u16 = 0,

    /// 返回底层 16 位状态掩码
    fn toU16(): u16 {
        return _.raw;
    }

    /// 从 16 位状态掩码构造 StatusFlags
    static fn fromU16(flags: u16): StatusFlags {
        return StatusFlags{ raw: flags };
    }

    /// 检测是否置位指定 StatusFlag
    fn has(flag: StatusFlag): bool {
        const mask = flag as u16;
        return (_.raw & mask) != 0;
    }

    /// 置位指定 StatusFlag
    fn set(*_, flag: StatusFlag) {
        _.raw |= flag as u16;
    }

    /// 清除指定 StatusFlag
    fn clear(*_, flag: StatusFlag) {
        _.raw &= ~(flag as u16);
    }

    /// 当前连接是否处于事务中（含只读事务）
    fn inTransaction(): bool {
        return _.has(.SERVER_STATUS_IN_TRANS) || _.has(.SERVER_STATUS_IN_TRANS_READONLY);
    }

    /// 当前连接是否处于只读事务中
    fn inReadOnlyTransaction(): bool {
        return _.has(.SERVER_STATUS_IN_TRANS_READONLY);
    }

    /// 是否开启自动提交模式
    fn isAutoCommit(): bool {
        return _.has(.SERVER_STATUS_AUTOCOMMIT);
    }

    /// 是否还有更多结果集待读取
    fn hasMoreResults(): bool {
        return _.has(.SERVER_MORE_RESULTS_EXISTS);
    }

    /// 是否存在服务端游标
    fn cursorExists(): bool {
        return _.has(.SERVER_STATUS_CURSOR_EXISTS);
    }

    /// 是否已发送最后一行
    fn lastRowSent(): bool {
        return _.has(.SERVER_STATUS_LAST_ROW_SENT);
    }

    /// 查询是否被标记为慢查询
    fn isSlowQuery(): bool {
        return _.has(.SERVER_QUERY_WAS_SLOW);
    }

    /// 会话状态是否发生变更
    fn isSessionStateChanged(): bool {
        return _.has(.SERVER_SESSION_STATE_CHANGED);
    }
}

/// MySQL Collation / CharacterSet 枚举（完整覆盖标准 collation ID 1..250 及 MySQL 8.0 常用 0900 collation）
pub enum CharacterSet: u16 {
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
    ujis_japanese_ci = 12,
    sjis_japanese_ci = 13,
    cp1251_bulgarian_ci = 14,
    latin1_danish_ci = 15,
    hebrew_general_ci = 16,
    tis620_thai_ci = 18,
    euckr_korean_ci = 19,
    latin7_estonian_cs = 20,
    latin2_hungarian_ci = 21,
    koi8u_general_ci = 22,
    cp1251_ukrainian_ci = 23,
    gb2312_chinese_ci = 24,
    greek_general_ci = 25,
    cp1250_general_ci = 26,
    latin2_croatian_ci = 27,
    gbk_chinese_ci = 28,
    cp1257_lithuanian_ci = 29,
    latin5_turkish_ci = 30,
    latin1_german2_ci = 31,
    armscii8_general_ci = 32,
    utf8mb3_general_ci = 33,
    cp1250_czech_cs = 34,
    ucs2_general_ci = 35,
    cp866_general_ci = 36,
    keybcs2_general_ci = 37,
    macce_general_ci = 38,
    macroman_general_ci = 39,
    cp852_general_ci = 40,
    latin7_general_ci = 41,
    latin7_general_cs = 42,
    macce_bin = 43,
    cp1250_croatian_ci = 44,
    utf8mb4_general_ci = 45,
    utf8mb4_bin = 46,
    latin1_bin = 47,
    latin1_general_ci = 48,
    latin1_general_cs = 49,
    cp1251_bin = 50,
    cp1251_general_ci = 51,
    cp1251_general_cs = 52,
    macroman_bin = 53,
    utf16_general_ci = 54,
    utf16_bin = 55,
    utf16le_general_ci = 56,
    cp1256_general_ci = 57,
    cp1257_bin = 58,
    cp1257_general_ci = 59,
    utf32_general_ci = 60,
    utf32_bin = 61,
    utf16le_bin = 62,
    binary = 63,
    armscii8_bin = 64,
    ascii_bin = 65,
    cp1250_bin = 66,
    cp1256_bin = 67,
    cp866_bin = 68,
    dec8_bin = 69,
    greek_bin = 70,
    hebrew_bin = 71,
    hp8_bin = 72,
    keybcs2_bin = 73,
    koi8r_bin = 74,
    koi8u_bin = 75,
    utf8mb3_tolower_ci = 76,
    latin2_bin = 77,
    latin5_bin = 78,
    latin7_bin = 79,
    cp850_bin = 80,
    cp852_bin = 81,
    swe7_bin = 82,
    utf8mb3_bin = 83,
    big5_bin = 84,
    euckr_bin = 85,
    gb2312_bin = 86,
    gbk_bin = 87,
    sjis_bin = 88,
    tis620_bin = 89,
    ucs2_bin = 90,
    ujis_bin = 91,
    geostd8_general_ci = 92,
    geostd8_bin = 93,
    latin1_spanish_ci = 94,
    cp932_japanese_ci = 95,
    cp932_bin = 96,
    eucjpms_japanese_ci = 97,
    eucjpms_bin = 98,
    cp1250_polish_ci = 99,
    utf16_unicode_ci = 101,
    utf16_icelandic_ci = 102,
    utf16_latvian_ci = 103,
    utf16_romanian_ci = 104,
    utf16_slovenian_ci = 105,
    utf16_polish_ci = 106,
    utf16_estonian_ci = 107,
    utf16_spanish_ci = 108,
    utf16_swedish_ci = 109,
    utf16_turkish_ci = 110,
    utf16_czech_ci = 111,
    utf16_danish_ci = 112,
    utf16_lithuanian_ci = 113,
    utf16_slovak_ci = 114,
    utf16_spanish2_ci = 115,
    utf16_roman_ci = 116,
    utf16_persian_ci = 117,
    utf16_esperanto_ci = 118,
    utf16_hungarian_ci = 119,
    utf16_sinhala_ci = 120,
    utf16_german2_ci = 121,
    utf16_croatian_ci = 122,
    utf16_unicode_520_ci = 123,
    utf16_vietnamese_ci = 124,
    ucs2_unicode_ci = 128,
    ucs2_icelandic_ci = 129,
    ucs2_latvian_ci = 130,
    ucs2_romanian_ci = 131,
    ucs2_slovenian_ci = 132,
    ucs2_polish_ci = 133,
    ucs2_estonian_ci = 134,
    ucs2_spanish_ci = 135,
    ucs2_swedish_ci = 136,
    ucs2_turkish_ci = 137,
    ucs2_czech_ci = 138,
    ucs2_danish_ci = 139,
    ucs2_lithuanian_ci = 140,
    ucs2_slovak_ci = 141,
    ucs2_spanish2_ci = 142,
    ucs2_roman_ci = 143,
    ucs2_persian_ci = 144,
    ucs2_esperanto_ci = 145,
    ucs2_hungarian_ci = 146,
    ucs2_sinhala_ci = 147,
    ucs2_german2_ci = 148,
    ucs2_croatian_ci = 149,
    ucs2_unicode_520_ci = 150,
    ucs2_vietnamese_ci = 151,
    ucs2_general_mysql500_ci = 159,
    utf32_unicode_ci = 160,
    utf32_icelandic_ci = 161,
    utf32_latvian_ci = 162,
    utf32_romanian_ci = 163,
    utf32_slovenian_ci = 164,
    utf32_polish_ci = 165,
    utf32_estonian_ci = 166,
    utf32_spanish_ci = 167,
    utf32_swedish_ci = 168,
    utf32_turkish_ci = 169,
    utf32_czech_ci = 170,
    utf32_danish_ci = 171,
    utf32_lithuanian_ci = 172,
    utf32_slovak_ci = 173,
    utf32_spanish2_ci = 174,
    utf32_roman_ci = 175,
    utf32_persian_ci = 176,
    utf32_esperanto_ci = 177,
    utf32_hungarian_ci = 178,
    utf32_sinhala_ci = 179,
    utf32_german2_ci = 180,
    utf32_croatian_ci = 181,
    utf32_unicode_520_ci = 182,
    utf32_vietnamese_ci = 183,
    utf8mb3_unicode_ci = 192,
    utf8mb3_icelandic_ci = 193,
    utf8mb3_latvian_ci = 194,
    utf8mb3_romanian_ci = 195,
    utf8mb3_slovenian_ci = 196,
    utf8mb3_polish_ci = 197,
    utf8mb3_estonian_ci = 198,
    utf8mb3_spanish_ci = 199,
    utf8mb3_swedish_ci = 200,
    utf8mb3_turkish_ci = 201,
    utf8mb3_czech_ci = 202,
    utf8mb3_danish_ci = 203,
    utf8mb3_lithuanian_ci = 204,
    utf8mb3_slovak_ci = 205,
    utf8mb3_spanish2_ci = 206,
    utf8mb3_roman_ci = 207,
    utf8mb3_persian_ci = 208,
    utf8mb3_esperanto_ci = 209,
    utf8mb3_hungarian_ci = 210,
    utf8mb3_sinhala_ci = 211,
    utf8mb3_german2_ci = 212,
    utf8mb3_croatian_ci = 213,
    utf8mb3_unicode_520_ci = 214,
    utf8mb3_vietnamese_ci = 215,
    utf8mb3_general_mysql500_ci = 223,
    utf8mb4_unicode_ci = 224,
    utf8mb4_icelandic_ci = 225,
    utf8mb4_latvian_ci = 226,
    utf8mb4_romanian_ci = 227,
    utf8mb4_slovenian_ci = 228,
    utf8mb4_polish_ci = 229,
    utf8mb4_estonian_ci = 230,
    utf8mb4_spanish_ci = 231,
    utf8mb4_swedish_ci = 232,
    utf8mb4_turkish_ci = 233,
    utf8mb4_czech_ci = 234,
    utf8mb4_danish_ci = 235,
    utf8mb4_lithuanian_ci = 236,
    utf8mb4_slovak_ci = 237,
    utf8mb4_spanish2_ci = 238,
    utf8mb4_roman_ci = 239,
    utf8mb4_persian_ci = 240,
    utf8mb4_esperanto_ci = 241,
    utf8mb4_hungarian_ci = 242,
    utf8mb4_sinhala_ci = 243,
    utf8mb4_german2_ci = 244,
    utf8mb4_croatian_ci = 245,
    utf8mb4_unicode_520_ci = 246,
    utf8mb4_vietnamese_ci = 247,
    gb18030_chinese_ci = 248,
    gb18030_bin = 249,
    gb18030_unicode_520_ci = 250,
    utf8mb4_0900_ai_ci = 255,
    utf8mb4_0900_as_cs = 278,
    utf8mb4_0900_bin = 309,

    const default = CharacterSet.utf8mb4_general_ci;

    /// 返回底层 MySQL collation ID
    fn id(): u16 {
        return _ as u16;
    }

    /// 返回完整 collation 名称（如 "utf8mb4_general_ci"）
    fn name(): string {
        return @tagName(_);
    }

    /// 返回所属基础字符集名称（如 "utf8mb4" / "latin1" / "binary"）
    fn charset(): string {
        return switch (_) {
            .big5_chinese_ci | .big5_bin => "big5",
            .dec8_swedish_ci | .dec8_bin => "dec8",
            .cp850_general_ci | .cp850_bin => "cp850",
            .hp8_english_ci | .hp8_bin => "hp8",
            .koi8r_general_ci | .koi8r_bin => "koi8r",
            .latin1_german1_ci | .latin1_swedish_ci | .latin1_danish_ci | .latin1_german2_ci | .latin1_bin | .latin1_general_ci | .latin1_general_cs | .latin1_spanish_ci => "latin1",
            .latin2_czech_cs | .latin2_general_ci | .latin2_hungarian_ci | .latin2_croatian_ci | .latin2_bin => "latin2",
            .swe7_swedish_ci | .swe7_bin => "swe7",
            .ascii_general_ci | .ascii_bin => "ascii",
            .ujis_japanese_ci | .ujis_bin => "ujis",
            .sjis_japanese_ci | .sjis_bin => "sjis",
            .hebrew_general_ci | .hebrew_bin => "hebrew",
            .tis620_thai_ci | .tis620_bin => "tis620",
            .euckr_korean_ci | .euckr_bin => "euckr",
            .koi8u_general_ci | .koi8u_bin => "koi8u",
            .gb2312_chinese_ci | .gb2312_bin => "gb2312",
            .greek_general_ci | .greek_bin => "greek",
            .cp1250_general_ci | .cp1250_czech_cs | .cp1250_croatian_ci | .cp1250_bin | .cp1250_polish_ci => "cp1250",
            .gbk_chinese_ci | .gbk_bin => "gbk",
            .latin5_turkish_ci | .latin5_bin => "latin5",
            .armscii8_general_ci | .armscii8_bin => "armscii8",
            .cp866_general_ci | .cp866_bin => "cp866",
            .keybcs2_general_ci | .keybcs2_bin => "keybcs2",
            .macce_general_ci | .macce_bin => "macce",
            .macroman_general_ci | .macroman_bin => "macroman",
            .cp852_general_ci | .cp852_bin => "cp852",
            .latin7_estonian_cs | .latin7_general_ci | .latin7_general_cs | .latin7_bin => "latin7",
            .cp1251_bulgarian_ci | .cp1251_ukrainian_ci | .cp1251_bin | .cp1251_general_ci | .cp1251_general_cs => "cp1251",
            .utf16le_general_ci | .utf16le_bin => "utf16le",
            .cp1256_general_ci | .cp1256_bin => "cp1256",
            .cp1257_lithuanian_ci | .cp1257_bin | .cp1257_general_ci => "cp1257",
            .binary => "binary",
            .geostd8_general_ci | .geostd8_bin => "geostd8",
            .cp932_japanese_ci | .cp932_bin => "cp932",
            .eucjpms_japanese_ci | .eucjpms_bin => "eucjpms",
            .gb18030_chinese_ci | .gb18030_bin | .gb18030_unicode_520_ci => "gb18030",
            .utf16_general_ci | .utf16_bin | .utf16_unicode_ci | .utf16_icelandic_ci | .utf16_latvian_ci | .utf16_romanian_ci | .utf16_slovenian_ci | .utf16_polish_ci | .utf16_estonian_ci | .utf16_spanish_ci | .utf16_swedish_ci | .utf16_turkish_ci | .utf16_czech_ci | .utf16_danish_ci | .utf16_lithuanian_ci | .utf16_slovak_ci | .utf16_spanish2_ci | .utf16_roman_ci | .utf16_persian_ci | .utf16_esperanto_ci | .utf16_hungarian_ci | .utf16_sinhala_ci | .utf16_german2_ci | .utf16_croatian_ci | .utf16_unicode_520_ci | .utf16_vietnamese_ci => "utf16",
            .ucs2_general_ci | .ucs2_bin | .ucs2_unicode_ci | .ucs2_icelandic_ci | .ucs2_latvian_ci | .ucs2_romanian_ci | .ucs2_slovenian_ci | .ucs2_polish_ci | .ucs2_estonian_ci | .ucs2_spanish_ci | .ucs2_swedish_ci | .ucs2_turkish_ci | .ucs2_czech_ci | .ucs2_danish_ci | .ucs2_lithuanian_ci | .ucs2_slovak_ci | .ucs2_spanish2_ci | .ucs2_roman_ci | .ucs2_persian_ci | .ucs2_esperanto_ci | .ucs2_hungarian_ci | .ucs2_sinhala_ci | .ucs2_german2_ci | .ucs2_croatian_ci | .ucs2_unicode_520_ci | .ucs2_vietnamese_ci | .ucs2_general_mysql500_ci => "ucs2",
            .utf32_general_ci | .utf32_bin | .utf32_unicode_ci | .utf32_icelandic_ci | .utf32_latvian_ci | .utf32_romanian_ci | .utf32_slovenian_ci | .utf32_polish_ci | .utf32_estonian_ci | .utf32_spanish_ci | .utf32_swedish_ci | .utf32_turkish_ci | .utf32_czech_ci | .utf32_danish_ci | .utf32_lithuanian_ci | .utf32_slovak_ci | .utf32_spanish2_ci | .utf32_roman_ci | .utf32_persian_ci | .utf32_esperanto_ci | .utf32_hungarian_ci | .utf32_sinhala_ci | .utf32_german2_ci | .utf32_croatian_ci | .utf32_unicode_520_ci | .utf32_vietnamese_ci => "utf32",
            .utf8mb3_general_ci | .utf8mb3_tolower_ci | .utf8mb3_bin | .utf8mb3_unicode_ci | .utf8mb3_icelandic_ci | .utf8mb3_latvian_ci | .utf8mb3_romanian_ci | .utf8mb3_slovenian_ci | .utf8mb3_polish_ci | .utf8mb3_estonian_ci | .utf8mb3_spanish_ci | .utf8mb3_swedish_ci | .utf8mb3_turkish_ci | .utf8mb3_czech_ci | .utf8mb3_danish_ci | .utf8mb3_lithuanian_ci | .utf8mb3_slovak_ci | .utf8mb3_spanish2_ci | .utf8mb3_roman_ci | .utf8mb3_persian_ci | .utf8mb3_esperanto_ci | .utf8mb3_hungarian_ci | .utf8mb3_sinhala_ci | .utf8mb3_german2_ci | .utf8mb3_croatian_ci | .utf8mb3_unicode_520_ci | .utf8mb3_vietnamese_ci | .utf8mb3_general_mysql500_ci => "utf8mb3",
            .utf8mb4_general_ci | .utf8mb4_bin | .utf8mb4_unicode_ci | .utf8mb4_icelandic_ci | .utf8mb4_latvian_ci | .utf8mb4_romanian_ci | .utf8mb4_slovenian_ci | .utf8mb4_polish_ci | .utf8mb4_estonian_ci | .utf8mb4_spanish_ci | .utf8mb4_swedish_ci | .utf8mb4_turkish_ci | .utf8mb4_czech_ci | .utf8mb4_danish_ci | .utf8mb4_lithuanian_ci | .utf8mb4_slovak_ci | .utf8mb4_spanish2_ci | .utf8mb4_roman_ci | .utf8mb4_persian_ci | .utf8mb4_esperanto_ci | .utf8mb4_hungarian_ci | .utf8mb4_sinhala_ci | .utf8mb4_german2_ci | .utf8mb4_croatian_ci | .utf8mb4_unicode_520_ci | .utf8mb4_vietnamese_ci | .utf8mb4_0900_ai_ci | .utf8mb4_0900_as_cs | .utf8mb4_0900_bin => "utf8mb4",
        };
    }

    /// 返回该字符集单字符最大字节数 (MySQL MAXLEN: 1 / 2 / 3 / 4)
    fn maxBytesPerChar(): u8 {
        const v = _ as u16;
        // 4 字节字符集：utf8mb4, utf16, utf16le, utf32, gb18030
        if (v == 45 || v == 46 || v == 54 || v == 55 || v == 56 || v == 60 || v == 61 || v == 62) {
            return 4;
        }
        if ((v >= 101 && v <= 124) || (v >= 160 && v <= 183) || (v >= 224 && v <= 250) || v == 255 || v == 278 || v == 309) {
            return 4;
        }
        // 3 字节字符集：utf8mb3, ujis, eucjpms
        if (v == 12 || v == 33 || v == 76 || v == 83 || v == 91 || v == 97 || v == 98 || (v >= 192 && v <= 215) || v == 223) {
            return 3;
        }
        // 2 字节字符集：big5, sjis, euckr, gb2312, gbk, ucs2, cp932
        if (v == 1 || v == 13 || v == 19 || v == 24 || v == 28 || v == 35 || v == 84 || v == 85 || v == 86 || v == 87 || v == 88 || v == 90 || v == 95 || v == 96 || (v >= 128 && v <= 151) || v == 159) {
            return 2;
        }
        // 其余均为单字节字符集（latin1/latin2/ascii/binary/cp125x 等）
        return 1;
    }

    /// 判断是否为 UTF-8 家族字符集（utf8mb4 或 utf8mb3）
    fn isUtf8(): bool {
        const v = _ as u16;
        return v == 33 || v == 45 || v == 46 || v == 76 || v == 83 || (v >= 192 && v <= 215) || v == 223 || (v >= 224 && v <= 247) || v == 255 || v == 278 || v == 309;
    }

    /// 判断是否为完整的 4 字节 UTF-8（utf8mb4）
    fn isUtf8mb4(): bool {
        const v = _ as u16;
        return v == 45 || v == 46 || (v >= 224 && v <= 247) || v == 255 || v == 278 || v == 309;
    }

    /// 判断是否为 binary 字符集
    fn isBinary(): bool {
        return _ == .binary;
    }

    /// 从 collation ID 安全查找 CharacterSet；未知或保留空洞 ID 返回 null
    static fn fromId(collation_id: u16): CharacterSet? {
        if (collation_id >= 1 && collation_id <= 99 && collation_id != 17) {
            return collation_id as CharacterSet;
        }
        if ((collation_id >= 101 && collation_id <= 124) || (collation_id >= 128 && collation_id <= 151) || collation_id == 159) {
            return collation_id as CharacterSet;
        }
        if ((collation_id >= 160 && collation_id <= 183) || (collation_id >= 192 && collation_id <= 215) || collation_id == 223) {
            return collation_id as CharacterSet;
        }
        if ((collation_id >= 224 && collation_id <= 250) || collation_id == 255 || collation_id == 278 || collation_id == 309) {
            return collation_id as CharacterSet;
        }
        return null;
    }
}
