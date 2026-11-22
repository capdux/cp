// Source: Absorbed from /data/dev/bun/src/sql/mysql/protocol/CharacterSet.cp | Synced: 2026-11-22T12:04:00Z
// Pure-CP zero-allocation MySQL 5.7/8.0 CharacterSet & Collation ID registry (1..255+),
// encoding family classifier, max-bytes-per-char calculator, binary-column detector,
// handshake byte mapper, and case-insensitive charset/collation name resolver.

/// 默认 MySQL 握手字符集 ID (`utf8mb4_general_ci = 45`，对齐 Bun `CharacterSet.default`)
pub const MYSQL_CHARSET_DEFAULT_ID: u16 = 45;
/// MySQL 8.0+ 默认字符集 ID (`utf8mb4_0900_ai_ci = 255`)
pub const MYSQL_CHARSET_UTF8MB4_0900_AI_CI_ID: u16 = 255;
/// `utf8mb4_bin = 46`（UTF-8 文本，按二进制码点比较）
pub const MYSQL_CHARSET_UTF8MB4_BIN_ID: u16 = 46;
/// `utf8mb4_unicode_ci = 224`
pub const MYSQL_CHARSET_UTF8MB4_UNICODE_CI_ID: u16 = 224;
/// `utf8mb3_general_ci = 33`（旧版 3 字节 UTF-8）
pub const MYSQL_CHARSET_UTF8MB3_GENERAL_CI_ID: u16 = 33;
/// `binary = 63`（原始二进制字节流，用于 `BLOB` / `VARBINARY` 列判定）
pub const MYSQL_CHARSET_BINARY_ID: u16 = 63;
/// `latin1_swedish_ci = 8`
pub const MYSQL_CHARSET_LATIN1_SWEDISH_CI_ID: u16 = 8;
/// `ascii_general_ci = 11`
pub const MYSQL_CHARSET_ASCII_GENERAL_CI_ID: u16 = 11;
/// `gbk_chinese_ci = 28`
pub const MYSQL_CHARSET_GBK_CHINESE_CI_ID: u16 = 28;
/// `gb18030_chinese_ci = 248`
pub const MYSQL_CHARSET_GB18030_CHINESE_CI_ID: u16 = 248;

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn eqlAsciiIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// MySQL 字符集编码族分类
pub enum MySqlCharsetFamily: u8 {
    Unknown = 0,
    Utf8mb4 = 1,
    Utf8mb3 = 2,
    Binary = 3,
    Ascii = 4,
    Latin1 = 5,
    Latin2 = 6,
    Latin5 = 7,
    Latin7 = 8,
    Utf16 = 9,
    Utf16le = 10,
    Utf32 = 11,
    Ucs2 = 12,
    Big5 = 13,
    Gb2312 = 14,
    Gbk = 15,
    Gb18030 = 16,
    Sjis = 17,
    Cp932 = 18,
    Ujis = 19,
    Eucjpms = 20,
    Euckr = 21,
    Cp1250 = 22,
    Cp1251 = 23,
    Cp1256 = 24,
    Cp1257 = 25,
    Cp850 = 26,
    Cp852 = 27,
    Cp866 = 28,
    Koi8r = 29,
    Koi8u = 30,
    Greek = 31,
    Hebrew = 32,
    Tis620 = 33,
    Macce = 34,
    Macroman = 35,
    Dec8 = 36,
    Hp8 = 37,
    Swe7 = 38,
    Armscii8 = 39,
    Keybcs2 = 40,
    Geostd8 = 41,

    fn asStr(): []const u8 {
        return switch (_) {
            .Unknown => "unknown",
            .Utf8mb4 => "utf8mb4",
            .Utf8mb3 => "utf8mb3",
            .Binary => "binary",
            .Ascii => "ascii",
            .Latin1 => "latin1",
            .Latin2 => "latin2",
            .Latin5 => "latin5",
            .Latin7 => "latin7",
            .Utf16 => "utf16",
            .Utf16le => "utf16le",
            .Utf32 => "utf32",
            .Ucs2 => "ucs2",
            .Big5 => "big5",
            .Gb2312 => "gb2312",
            .Gbk => "gbk",
            .Gb18030 => "gb18030",
            .Sjis => "sjis",
            .Cp932 => "cp932",
            .Ujis => "ujis",
            .Eucjpms => "eucjpms",
            .Euckr => "euckr",
            .Cp1250 => "cp1250",
            .Cp1251 => "cp1251",
            .Cp1256 => "cp1256",
            .Cp1257 => "cp1257",
            .Cp850 => "cp850",
            .Cp852 => "cp852",
            .Cp866 => "cp866",
            .Koi8r => "koi8r",
            .Koi8u => "koi8u",
            .Greek => "greek",
            .Hebrew => "hebrew",
            .Tis620 => "tis620",
            .Macce => "macce",
            .Macroman => "macroman",
            .Dec8 => "dec8",
            .Hp8 => "hp8",
            .Swe7 => "swe7",
            .Armscii8 => "armscii8",
            .Keybcs2 => "keybcs2",
            .Geostd8 => "geostd8",
        };
    }

    /// 返回该编码族单字符最大字节宽度（1 / 2 / 3 / 4）
    fn maxBytesPerChar(): u8 {
        return switch (_) {
            .Utf8mb4, .Utf16, .Utf16le, .Utf32, .Gb18030 => 4,
            .Utf8mb3, .Ujis, .Eucjpms => 3,
            .Ucs2, .Big5, .Gb2312, .Gbk, .Sjis, .Cp932, .Euckr => 2,
            else => 1,
        };
    }

    /// 是否直接兼容 UTF-8 字符串解码（无须额外转码）
    fn isUtf8Compatible(): bool {
        return _ == .Utf8mb4 || _ == .Utf8mb3 || _ == .Ascii;
    }

    fn isBinary(): bool {
        return _ == .Binary;
    }

    /// 返回该编码族对应的默认 Collation ID
    fn defaultCollationId(): u16 {
        return switch (_) {
            .Utf8mb4 => MYSQL_CHARSET_DEFAULT_ID,
            .Utf8mb3 => MYSQL_CHARSET_UTF8MB3_GENERAL_CI_ID,
            .Binary => MYSQL_CHARSET_BINARY_ID,
            .Ascii => MYSQL_CHARSET_ASCII_GENERAL_CI_ID,
            .Latin1 => MYSQL_CHARSET_LATIN1_SWEDISH_CI_ID,
            .Latin2 => 9,
            .Latin5 => 30,
            .Latin7 => 41,
            .Utf16 => 54,
            .Utf16le => 56,
            .Utf32 => 60,
            .Ucs2 => 35,
            .Big5 => 1,
            .Gb2312 => 24,
            .Gbk => MYSQL_CHARSET_GBK_CHINESE_CI_ID,
            .Gb18030 => MYSQL_CHARSET_GB18030_CHINESE_CI_ID,
            .Sjis => 13,
            .Cp932 => 95,
            .Ujis => 12,
            .Eucjpms => 97,
            .Euckr => 19,
            .Cp1250 => 26,
            .Cp1251 => 51,
            .Cp1256 => 57,
            .Cp1257 => 59,
            .Cp850 => 4,
            .Cp852 => 40,
            .Cp866 => 36,
            .Koi8r => 7,
            .Koi8u => 22,
            .Greek => 25,
            .Hebrew => 16,
            .Tis620 => 18,
            .Macce => 38,
            .Macroman => 39,
            .Dec8 => 3,
            .Hp8 => 6,
            .Swe7 => 10,
            .Armscii8 => 32,
            .Keybcs2 => 37,
            .Geostd8 => 92,
            .Unknown => 0,
        };
    }
}

/// MySQL Collation 元数据视图
pub struct MySqlCollationInfo {
    id: u16 = 0,
    collation_name: []const u8 = "(unknown)",
    family: MySqlCharsetFamily = .Unknown,
    is_binary_collation: bool = false,
    is_case_sensitive: bool = false,

    fn charsetName(): []const u8 {
        return _.family.asStr();
    }

    fn maxBytesPerChar(): u8 {
        return _.family.maxBytesPerChar();
    }

    fn isKnown(): bool {
        return _.family != .Unknown;
    }

    fn isUtf8Compatible(): bool {
        return _.family.isUtf8Compatible();
    }

    /// 是否为原始二进制字节列（仅 `binary = 63`，区别于 `utf8mb4_bin = 46` 等文本二进制排序）
    fn isRawBinary(): bool {
        return _.id == MYSQL_CHARSET_BINARY_ID && _.family == .Binary;
    }

    /// 转换为 MySQL HandshakeResponse41 的 1 字节 `character_set` 协议字段（超出 255 时按规范回退到 `utf8mb4_0900_ai_ci` = 255）
    fn handshakeByte(): u8 {
        if (_.id == 0) {
            return @intCast(MYSQL_CHARSET_DEFAULT_ID);
        }
        if (_.id <= 255) {
            return @intCast(_.id);
        }
        if (_.family == .Utf8mb4) {
            return @intCast(MYSQL_CHARSET_UTF8MB4_0900_AI_CI_ID);
        }
        return @intCast(MYSQL_CHARSET_DEFAULT_ID);
    }
}

fn mkInfo(
    id: u16,
    cname: []const u8,
    fam: MySqlCharsetFamily,
    is_bin: bool,
    is_cs: bool,
): MySqlCollationInfo {
    return MySqlCollationInfo{
        id: id,
        collation_name: cname,
        family: fam,
        is_binary_collation: is_bin,
        is_case_sensitive: is_cs || is_bin,
    };
}

/// 按 MySQL 协议 Collation ID（1..255+）查询完整元数据（对齐 Bun `CharacterSet.cp` 全量表 + MySQL 8.0 `utf8mb4_0900_*`）
pub fn lookupMySqlCollation(id: u16): MySqlCollationInfo {
    return switch (id) {
        1 => mkInfo(1, "big5_chinese_ci", .Big5, false, false),
        2 => mkInfo(2, "latin2_czech_cs", .Latin2, false, true),
        3 => mkInfo(3, "dec8_swedish_ci", .Dec8, false, false),
        4 => mkInfo(4, "cp850_general_ci", .Cp850, false, false),
        5 => mkInfo(5, "latin1_german1_ci", .Latin1, false, false),
        6 => mkInfo(6, "hp8_english_ci", .Hp8, false, false),
        7 => mkInfo(7, "koi8r_general_ci", .Koi8r, false, false),
        8 => mkInfo(8, "latin1_swedish_ci", .Latin1, false, false),
        9 => mkInfo(9, "latin2_general_ci", .Latin2, false, false),
        10 => mkInfo(10, "swe7_swedish_ci", .Swe7, false, false),
        11 => mkInfo(11, "ascii_general_ci", .Ascii, false, false),
        12 => mkInfo(12, "ujis_japanese_ci", .Ujis, false, false),
        13 => mkInfo(13, "sjis_japanese_ci", .Sjis, false, false),
        14 => mkInfo(14, "cp1251_bulgarian_ci", .Cp1251, false, false),
        15 => mkInfo(15, "latin1_danish_ci", .Latin1, false, false),
        16 => mkInfo(16, "hebrew_general_ci", .Hebrew, false, false),
        18 => mkInfo(18, "tis620_thai_ci", .Tis620, false, false),
        19 => mkInfo(19, "euckr_korean_ci", .Euckr, false, false),
        20 => mkInfo(20, "latin7_estonian_cs", .Latin7, false, true),
        21 => mkInfo(21, "latin2_hungarian_ci", .Latin2, false, false),
        22 => mkInfo(22, "koi8u_general_ci", .Koi8u, false, false),
        23 => mkInfo(23, "cp1251_ukrainian_ci", .Cp1251, false, false),
        24 => mkInfo(24, "gb2312_chinese_ci", .Gb2312, false, false),
        25 => mkInfo(25, "greek_general_ci", .Greek, false, false),
        26 => mkInfo(26, "cp1250_general_ci", .Cp1250, false, false),
        27 => mkInfo(27, "latin2_croatian_ci", .Latin2, false, false),
        28 => mkInfo(28, "gbk_chinese_ci", .Gbk, false, false),
        29 => mkInfo(29, "cp1257_lithuanian_ci", .Cp1257, false, false),
        30 => mkInfo(30, "latin5_turkish_ci", .Latin5, false, false),
        31 => mkInfo(31, "latin1_german2_ci", .Latin1, false, false),
        32 => mkInfo(32, "armscii8_general_ci", .Armscii8, false, false),
        33 => mkInfo(33, "utf8mb3_general_ci", .Utf8mb3, false, false),
        34 => mkInfo(34, "cp1250_czech_cs", .Cp1250, false, true),
        35 => mkInfo(35, "ucs2_general_ci", .Ucs2, false, false),
        36 => mkInfo(36, "cp866_general_ci", .Cp866, false, false),
        37 => mkInfo(37, "keybcs2_general_ci", .Keybcs2, false, false),
        38 => mkInfo(38, "macce_general_ci", .Macce, false, false),
        39 => mkInfo(39, "macroman_general_ci", .Macroman, false, false),
        40 => mkInfo(40, "cp852_general_ci", .Cp852, false, false),
        41 => mkInfo(41, "latin7_general_ci", .Latin7, false, false),
        42 => mkInfo(42, "latin7_general_cs", .Latin7, false, true),
        43 => mkInfo(43, "macce_bin", .Macce, true, true),
        44 => mkInfo(44, "cp1250_croatian_ci", .Cp1250, false, false),
        45 => mkInfo(45, "utf8mb4_general_ci", .Utf8mb4, false, false),
        46 => mkInfo(46, "utf8mb4_bin", .Utf8mb4, true, true),
        47 => mkInfo(47, "latin1_bin", .Latin1, true, true),
        48 => mkInfo(48, "latin1_general_ci", .Latin1, false, false),
        49 => mkInfo(49, "latin1_general_cs", .Latin1, false, true),
        50 => mkInfo(50, "cp1251_bin", .Cp1251, true, true),
        51 => mkInfo(51, "cp1251_general_ci", .Cp1251, false, false),
        52 => mkInfo(52, "cp1251_general_cs", .Cp1251, false, true),
        53 => mkInfo(53, "macroman_bin", .Macroman, true, true),
        54 => mkInfo(54, "utf16_general_ci", .Utf16, false, false),
        55 => mkInfo(55, "utf16_bin", .Utf16, true, true),
        56 => mkInfo(56, "utf16le_general_ci", .Utf16le, false, false),
        57 => mkInfo(57, "cp1256_general_ci", .Cp1256, false, false),
        58 => mkInfo(58, "cp1257_bin", .Cp1257, true, true),
        59 => mkInfo(59, "cp1257_general_ci", .Cp1257, false, false),
        60 => mkInfo(60, "utf32_general_ci", .Utf32, false, false),
        61 => mkInfo(61, "utf32_bin", .Utf32, true, true),
        62 => mkInfo(62, "utf16le_bin", .Utf16le, true, true),
        63 => mkInfo(63, "binary", .Binary, true, true),
        64 => mkInfo(64, "armscii8_bin", .Armscii8, true, true),
        65 => mkInfo(65, "ascii_bin", .Ascii, true, true),
        66 => mkInfo(66, "cp1250_bin", .Cp1250, true, true),
        67 => mkInfo(67, "cp1256_bin", .Cp1256, true, true),
        68 => mkInfo(68, "cp866_bin", .Cp866, true, true),
        69 => mkInfo(69, "dec8_bin", .Dec8, true, true),
        70 => mkInfo(70, "greek_bin", .Greek, true, true),
        71 => mkInfo(71, "hebrew_bin", .Hebrew, true, true),
        72 => mkInfo(72, "hp8_bin", .Hp8, true, true),
        73 => mkInfo(73, "keybcs2_bin", .Keybcs2, true, true),
        74 => mkInfo(74, "koi8r_bin", .Koi8r, true, true),
        75 => mkInfo(75, "koi8u_bin", .Koi8u, true, true),
        76 => mkInfo(76, "utf8mb3_tolower_ci", .Utf8mb3, false, false),
        77 => mkInfo(77, "latin2_bin", .Latin2, true, true),
        78 => mkInfo(78, "latin5_bin", .Latin5, true, true),
        79 => mkInfo(79, "latin7_bin", .Latin7, true, true),
        80 => mkInfo(80, "cp850_bin", .Cp850, true, true),
        81 => mkInfo(81, "cp852_bin", .Cp852, true, true),
        82 => mkInfo(82, "swe7_bin", .Swe7, true, true),
        83 => mkInfo(83, "utf8mb3_bin", .Utf8mb3, true, true),
        84 => mkInfo(84, "big5_bin", .Big5, true, true),
        85 => mkInfo(85, "euckr_bin", .Euckr, true, true),
        86 => mkInfo(86, "gb2312_bin", .Gb2312, true, true),
        87 => mkInfo(87, "gbk_bin", .Gbk, true, true),
        88 => mkInfo(88, "sjis_bin", .Sjis, true, true),
        89 => mkInfo(89, "tis620_bin", .Tis620, true, true),
        90 => mkInfo(90, "ucs2_bin", .Ucs2, true, true),
        91 => mkInfo(91, "ujis_bin", .Ujis, true, true),
        92 => mkInfo(92, "geostd8_general_ci", .Geostd8, false, false),
        93 => mkInfo(93, "geostd8_bin", .Geostd8, true, true),
        94 => mkInfo(94, "latin1_spanish_ci", .Latin1, false, false),
        95 => mkInfo(95, "cp932_japanese_ci", .Cp932, false, false),
        96 => mkInfo(96, "cp932_bin", .Cp932, true, true),
        97 => mkInfo(97, "eucjpms_japanese_ci", .Eucjpms, false, false),
        98 => mkInfo(98, "eucjpms_bin", .Eucjpms, true, true),
        99 => mkInfo(99, "cp1250_polish_ci", .Cp1250, false, false),
        101 => mkInfo(101, "utf16_unicode_ci", .Utf16, false, false),
        102 => mkInfo(102, "utf16_icelandic_ci", .Utf16, false, false),
        103 => mkInfo(103, "utf16_latvian_ci", .Utf16, false, false),
        104 => mkInfo(104, "utf16_romanian_ci", .Utf16, false, false),
        105 => mkInfo(105, "utf16_slovenian_ci", .Utf16, false, false),
        106 => mkInfo(106, "utf16_polish_ci", .Utf16, false, false),
        107 => mkInfo(107, "utf16_estonian_ci", .Utf16, false, false),
        108 => mkInfo(108, "utf16_spanish_ci", .Utf16, false, false),
        109 => mkInfo(109, "utf16_swedish_ci", .Utf16, false, false),
        110 => mkInfo(110, "utf16_turkish_ci", .Utf16, false, false),
        111 => mkInfo(111, "utf16_czech_ci", .Utf16, false, false),
        112 => mkInfo(112, "utf16_danish_ci", .Utf16, false, false),
        113 => mkInfo(113, "utf16_lithuanian_ci", .Utf16, false, false),
        114 => mkInfo(114, "utf16_slovak_ci", .Utf16, false, false),
        115 => mkInfo(115, "utf16_spanish2_ci", .Utf16, false, false),
        116 => mkInfo(116, "utf16_roman_ci", .Utf16, false, false),
        117 => mkInfo(117, "utf16_persian_ci", .Utf16, false, false),
        118 => mkInfo(118, "utf16_esperanto_ci", .Utf16, false, false),
        119 => mkInfo(119, "utf16_hungarian_ci", .Utf16, false, false),
        120 => mkInfo(120, "utf16_sinhala_ci", .Utf16, false, false),
        121 => mkInfo(121, "utf16_german2_ci", .Utf16, false, false),
        122 => mkInfo(122, "utf16_croatian_ci", .Utf16, false, false),
        123 => mkInfo(123, "utf16_unicode_520_ci", .Utf16, false, false),
        124 => mkInfo(124, "utf16_vietnamese_ci", .Utf16, false, false),
        128 => mkInfo(128, "ucs2_unicode_ci", .Ucs2, false, false),
        129 => mkInfo(129, "ucs2_icelandic_ci", .Ucs2, false, false),
        130 => mkInfo(130, "ucs2_latvian_ci", .Ucs2, false, false),
        131 => mkInfo(131, "ucs2_romanian_ci", .Ucs2, false, false),
        132 => mkInfo(132, "ucs2_slovenian_ci", .Ucs2, false, false),
        133 => mkInfo(133, "ucs2_polish_ci", .Ucs2, false, false),
        134 => mkInfo(134, "ucs2_estonian_ci", .Ucs2, false, false),
        135 => mkInfo(135, "ucs2_spanish_ci", .Ucs2, false, false),
        136 => mkInfo(136, "ucs2_swedish_ci", .Ucs2, false, false),
        137 => mkInfo(137, "ucs2_turkish_ci", .Ucs2, false, false),
        138 => mkInfo(138, "ucs2_czech_ci", .Ucs2, false, false),
        139 => mkInfo(139, "ucs2_danish_ci", .Ucs2, false, false),
        140 => mkInfo(140, "ucs2_lithuanian_ci", .Ucs2, false, false),
        141 => mkInfo(141, "ucs2_slovak_ci", .Ucs2, false, false),
        142 => mkInfo(142, "ucs2_spanish2_ci", .Ucs2, false, false),
        143 => mkInfo(143, "ucs2_roman_ci", .Ucs2, false, false),
        144 => mkInfo(144, "ucs2_persian_ci", .Ucs2, false, false),
        145 => mkInfo(145, "ucs2_esperanto_ci", .Ucs2, false, false),
        146 => mkInfo(146, "ucs2_hungarian_ci", .Ucs2, false, false),
        147 => mkInfo(147, "ucs2_sinhala_ci", .Ucs2, false, false),
        148 => mkInfo(148, "ucs2_german2_ci", .Ucs2, false, false),
        149 => mkInfo(149, "ucs2_croatian_ci", .Ucs2, false, false),
        150 => mkInfo(150, "ucs2_unicode_520_ci", .Ucs2, false, false),
        151 => mkInfo(151, "ucs2_vietnamese_ci", .Ucs2, false, false),
        159 => mkInfo(159, "ucs2_general_mysql500_ci", .Ucs2, false, false),
        160 => mkInfo(160, "utf32_unicode_ci", .Utf32, false, false),
        161 => mkInfo(161, "utf32_icelandic_ci", .Utf32, false, false),
        162 => mkInfo(162, "utf32_latvian_ci", .Utf32, false, false),
        163 => mkInfo(163, "utf32_romanian_ci", .Utf32, false, false),
        164 => mkInfo(164, "utf32_slovenian_ci", .Utf32, false, false),
        165 => mkInfo(165, "utf32_polish_ci", .Utf32, false, false),
        166 => mkInfo(166, "utf32_estonian_ci", .Utf32, false, false),
        167 => mkInfo(167, "utf32_spanish_ci", .Utf32, false, false),
        168 => mkInfo(168, "utf32_swedish_ci", .Utf32, false, false),
        169 => mkInfo(169, "utf32_turkish_ci", .Utf32, false, false),
        170 => mkInfo(170, "utf32_czech_ci", .Utf32, false, false),
        171 => mkInfo(171, "utf32_danish_ci", .Utf32, false, false),
        172 => mkInfo(172, "utf32_lithuanian_ci", .Utf32, false, false),
        173 => mkInfo(173, "utf32_slovak_ci", .Utf32, false, false),
        174 => mkInfo(174, "utf32_spanish2_ci", .Utf32, false, false),
        175 => mkInfo(175, "utf32_roman_ci", .Utf32, false, false),
        176 => mkInfo(176, "utf32_persian_ci", .Utf32, false, false),
        177 => mkInfo(177, "utf32_esperanto_ci", .Utf32, false, false),
        178 => mkInfo(178, "utf32_hungarian_ci", .Utf32, false, false),
        179 => mkInfo(179, "utf32_sinhala_ci", .Utf32, false, false),
        180 => mkInfo(180, "utf32_german2_ci", .Utf32, false, false),
        181 => mkInfo(181, "utf32_croatian_ci", .Utf32, false, false),
        182 => mkInfo(182, "utf32_unicode_520_ci", .Utf32, false, false),
        183 => mkInfo(183, "utf32_vietnamese_ci", .Utf32, false, false),
        192 => mkInfo(192, "utf8mb3_unicode_ci", .Utf8mb3, false, false),
        193 => mkInfo(193, "utf8mb3_icelandic_ci", .Utf8mb3, false, false),
        194 => mkInfo(194, "utf8mb3_latvian_ci", .Utf8mb3, false, false),
        195 => mkInfo(195, "utf8mb3_romanian_ci", .Utf8mb3, false, false),
        196 => mkInfo(196, "utf8mb3_slovenian_ci", .Utf8mb3, false, false),
        197 => mkInfo(197, "utf8mb3_polish_ci", .Utf8mb3, false, false),
        198 => mkInfo(198, "utf8mb3_estonian_ci", .Utf8mb3, false, false),
        199 => mkInfo(199, "utf8mb3_spanish_ci", .Utf8mb3, false, false),
        200 => mkInfo(200, "utf8mb3_swedish_ci", .Utf8mb3, false, false),
        201 => mkInfo(201, "utf8mb3_turkish_ci", .Utf8mb3, false, false),
        202 => mkInfo(202, "utf8mb3_czech_ci", .Utf8mb3, false, false),
        203 => mkInfo(203, "utf8mb3_danish_ci", .Utf8mb3, false, false),
        204 => mkInfo(204, "utf8mb3_lithuanian_ci", .Utf8mb3, false, false),
        205 => mkInfo(205, "utf8mb3_slovak_ci", .Utf8mb3, false, false),
        206 => mkInfo(206, "utf8mb3_spanish2_ci", .Utf8mb3, false, false),
        207 => mkInfo(207, "utf8mb3_roman_ci", .Utf8mb3, false, false),
        208 => mkInfo(208, "utf8mb3_persian_ci", .Utf8mb3, false, false),
        209 => mkInfo(209, "utf8mb3_esperanto_ci", .Utf8mb3, false, false),
        210 => mkInfo(210, "utf8mb3_hungarian_ci", .Utf8mb3, false, false),
        211 => mkInfo(211, "utf8mb3_sinhala_ci", .Utf8mb3, false, false),
        212 => mkInfo(212, "utf8mb3_german2_ci", .Utf8mb3, false, false),
        213 => mkInfo(213, "utf8mb3_croatian_ci", .Utf8mb3, false, false),
        214 => mkInfo(214, "utf8mb3_unicode_520_ci", .Utf8mb3, false, false),
        215 => mkInfo(215, "utf8mb3_vietnamese_ci", .Utf8mb3, false, false),
        223 => mkInfo(223, "utf8mb3_general_mysql500_ci", .Utf8mb3, false, false),
        224 => mkInfo(224, "utf8mb4_unicode_ci", .Utf8mb4, false, false),
        225 => mkInfo(225, "utf8mb4_icelandic_ci", .Utf8mb4, false, false),
        226 => mkInfo(226, "utf8mb4_latvian_ci", .Utf8mb4, false, false),
        227 => mkInfo(227, "utf8mb4_romanian_ci", .Utf8mb4, false, false),
        228 => mkInfo(228, "utf8mb4_slovenian_ci", .Utf8mb4, false, false),
        229 => mkInfo(229, "utf8mb4_polish_ci", .Utf8mb4, false, false),
        230 => mkInfo(230, "utf8mb4_estonian_ci", .Utf8mb4, false, false),
        231 => mkInfo(231, "utf8mb4_spanish_ci", .Utf8mb4, false, false),
        232 => mkInfo(232, "utf8mb4_swedish_ci", .Utf8mb4, false, false),
        233 => mkInfo(233, "utf8mb4_turkish_ci", .Utf8mb4, false, false),
        234 => mkInfo(234, "utf8mb4_czech_ci", .Utf8mb4, false, false),
        235 => mkInfo(235, "utf8mb4_danish_ci", .Utf8mb4, false, false),
        236 => mkInfo(236, "utf8mb4_lithuanian_ci", .Utf8mb4, false, false),
        237 => mkInfo(237, "utf8mb4_slovak_ci", .Utf8mb4, false, false),
        238 => mkInfo(238, "utf8mb4_spanish2_ci", .Utf8mb4, false, false),
        239 => mkInfo(239, "utf8mb4_roman_ci", .Utf8mb4, false, false),
        240 => mkInfo(240, "utf8mb4_persian_ci", .Utf8mb4, false, false),
        241 => mkInfo(241, "utf8mb4_esperanto_ci", .Utf8mb4, false, false),
        242 => mkInfo(242, "utf8mb4_hungarian_ci", .Utf8mb4, false, false),
        243 => mkInfo(243, "utf8mb4_sinhala_ci", .Utf8mb4, false, false),
        244 => mkInfo(244, "utf8mb4_german2_ci", .Utf8mb4, false, false),
        245 => mkInfo(245, "utf8mb4_croatian_ci", .Utf8mb4, false, false),
        246 => mkInfo(246, "utf8mb4_unicode_520_ci", .Utf8mb4, false, false),
        247 => mkInfo(247, "utf8mb4_vietnamese_ci", .Utf8mb4, false, false),
        248 => mkInfo(248, "gb18030_chinese_ci", .Gb18030, false, false),
        249 => mkInfo(249, "gb18030_bin", .Gb18030, true, true),
        250 => mkInfo(250, "gb18030_unicode_520_ci", .Gb18030, false, false),
        // MySQL 8.0+ UCA 9.0.0 扩展 collation
        255 => mkInfo(255, "utf8mb4_0900_ai_ci", .Utf8mb4, false, false),
        278 => mkInfo(278, "utf8mb4_0900_as_cs", .Utf8mb4, false, true),
        305 => mkInfo(305, "utf8mb4_0900_as_ci", .Utf8mb4, false, false),
        309 => mkInfo(309, "utf8mb4_0900_bin", .Utf8mb4, true, true),
        else => MySqlCollationInfo{
            id: id,
            collation_name: "(unknown)",
            family: .Unknown,
            is_binary_collation: false,
            is_case_sensitive: false,
        },
    };
}

/// 按字符集别名或完整 collation 名称解析（ASCII 大小写无关）
pub fn lookupMySqlCollationByName(raw_name: []const u8): MySqlCollationInfo? {
    if (eqlAsciiIgnoreCase(raw_name, "utf8mb4") ||
        eqlAsciiIgnoreCase(raw_name, "utf8mb4_general_ci"))
    {
        return lookupMySqlCollation(45);
    }
    if (eqlAsciiIgnoreCase(raw_name, "utf8mb4_0900_ai_ci")) {
        return lookupMySqlCollation(255);
    }
    if (eqlAsciiIgnoreCase(raw_name, "utf8mb4_0900_as_cs")) {
        return lookupMySqlCollation(278);
    }
    if (eqlAsciiIgnoreCase(raw_name, "utf8mb4_0900_bin")) {
        return lookupMySqlCollation(309);
    }
    if (eqlAsciiIgnoreCase(raw_name, "utf8mb4_bin")) {
        return lookupMySqlCollation(46);
    }
    if (eqlAsciiIgnoreCase(raw_name, "utf8mb4_unicode_ci")) {
        return lookupMySqlCollation(224);
    }
    if (eqlAsciiIgnoreCase(raw_name, "utf8mb4_unicode_520_ci")) {
        return lookupMySqlCollation(246);
    }
    if (eqlAsciiIgnoreCase(raw_name, "utf8") ||
        eqlAsciiIgnoreCase(raw_name, "utf8mb3") ||
        eqlAsciiIgnoreCase(raw_name, "utf8mb3_general_ci") ||
        eqlAsciiIgnoreCase(raw_name, "utf8_general_ci"))
    {
        return lookupMySqlCollation(33);
    }
    if (eqlAsciiIgnoreCase(raw_name, "utf8mb3_bin") ||
        eqlAsciiIgnoreCase(raw_name, "utf8_bin"))
    {
        return lookupMySqlCollation(83);
    }
    if (eqlAsciiIgnoreCase(raw_name, "binary")) {
        return lookupMySqlCollation(63);
    }
    if (eqlAsciiIgnoreCase(raw_name, "ascii") ||
        eqlAsciiIgnoreCase(raw_name, "ascii_general_ci"))
    {
        return lookupMySqlCollation(11);
    }
    if (eqlAsciiIgnoreCase(raw_name, "ascii_bin")) {
        return lookupMySqlCollation(65);
    }
    if (eqlAsciiIgnoreCase(raw_name, "latin1") ||
        eqlAsciiIgnoreCase(raw_name, "latin1_swedish_ci"))
    {
        return lookupMySqlCollation(8);
    }
    if (eqlAsciiIgnoreCase(raw_name, "latin1_bin")) {
        return lookupMySqlCollation(47);
    }
    if (eqlAsciiIgnoreCase(raw_name, "gbk") ||
        eqlAsciiIgnoreCase(raw_name, "gbk_chinese_ci"))
    {
        return lookupMySqlCollation(28);
    }
    if (eqlAsciiIgnoreCase(raw_name, "gb18030") ||
        eqlAsciiIgnoreCase(raw_name, "gb18030_chinese_ci"))
    {
        return lookupMySqlCollation(248);
    }
    if (eqlAsciiIgnoreCase(raw_name, "utf16") ||
        eqlAsciiIgnoreCase(raw_name, "utf16_general_ci"))
    {
        return lookupMySqlCollation(54);
    }
    if (eqlAsciiIgnoreCase(raw_name, "utf16le") ||
        eqlAsciiIgnoreCase(raw_name, "utf16le_general_ci"))
    {
        return lookupMySqlCollation(56);
    }
    if (eqlAsciiIgnoreCase(raw_name, "utf32") ||
        eqlAsciiIgnoreCase(raw_name, "utf32_general_ci"))
    {
        return lookupMySqlCollation(60);
    }
    return null;
}

/// 判定 MySQL `ColumnDefinition41.character_set` 是否为原始二进制字节列 (`binary = 63`)
pub fn isMySqlBinaryColumn(charset_id: u16): bool {
    return charset_id == MYSQL_CHARSET_BINARY_ID;
}

/// 返回指定 MySQL Collation ID 的单字符最大字节数（未知 ID 安全回退为 1）
pub fn mysqlMaxCharLen(charset_id: u16): u8 {
    return lookupMySqlCollation(charset_id).maxBytesPerChar();
}
