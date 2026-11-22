import { test, expect, expect_eq } from "cap:test";
import {
    MYSQL_CHARSET_DEFAULT_ID,
    MYSQL_CHARSET_UTF8MB4_0900_AI_CI_ID,
    MYSQL_CHARSET_UTF8MB4_BIN_ID,
    MYSQL_CHARSET_UTF8MB4_UNICODE_CI_ID,
    MYSQL_CHARSET_UTF8MB3_GENERAL_CI_ID,
    MYSQL_CHARSET_BINARY_ID,
    MYSQL_CHARSET_LATIN1_SWEDISH_CI_ID,
    MYSQL_CHARSET_ASCII_GENERAL_CI_ID,
    MYSQL_CHARSET_GBK_CHINESE_CI_ID,
    MYSQL_CHARSET_GB18030_CHINESE_CI_ID,
    MySqlCharsetFamily,
    lookupMySqlCollation,
    lookupMySqlCollationByName,
    isMySqlBinaryColumn,
    mysqlMaxCharLen,
} from "cap:sql";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("sql mysql_charset: core utf8mb4 / utf8mb3 / binary / latin1 lookups", fn() {
    const def = lookupMySqlCollation(MYSQL_CHARSET_DEFAULT_ID);
    expect(def.isKnown());
    expect_eq(def.id, 45);
    expect(strEq(def.collation_name, "utf8mb4_general_ci"));
    expect(strEq(def.charsetName(), "utf8mb4"));
    expect(def.family == MySqlCharsetFamily.Utf8mb4);
    expect_eq(def.maxBytesPerChar(), 4);
    expect(def.isUtf8Compatible());
    expect(!def.is_binary_collation);
    expect(!def.is_case_sensitive);
    expect(!def.isRawBinary());
    expect_eq(def.handshakeByte(), 45);

    const mb4_bin = lookupMySqlCollation(MYSQL_CHARSET_UTF8MB4_BIN_ID);
    expect(mb4_bin.is_binary_collation);
    expect(mb4_bin.is_case_sensitive);
    expect(mb4_bin.isUtf8Compatible());
    // utf8mb4_bin 是文本列二进制排序，不是原始二进制列 (binary=63)
    expect(!mb4_bin.isRawBinary());
    expect(!isMySqlBinaryColumn(MYSQL_CHARSET_UTF8MB4_BIN_ID));

    const bin = lookupMySqlCollation(MYSQL_CHARSET_BINARY_ID);
    expect(bin.isRawBinary());
    expect(bin.family.isBinary());
    expect(!bin.isUtf8Compatible());
    expect_eq(bin.maxBytesPerChar(), 1);
    expect(isMySqlBinaryColumn(MYSQL_CHARSET_BINARY_ID));

    const mb3 = lookupMySqlCollation(MYSQL_CHARSET_UTF8MB3_GENERAL_CI_ID);
    expect_eq(mb3.maxBytesPerChar(), 3);
    expect(mb3.isUtf8Compatible());

    const lat1 = lookupMySqlCollation(MYSQL_CHARSET_LATIN1_SWEDISH_CI_ID);
    expect_eq(lat1.maxBytesPerChar(), 1);
    expect(!lat1.isUtf8Compatible());
});

test("sql mysql_charset: CJK, UTF-16/UTF-32 and case-sensitive collations", fn() {
    const gbk = lookupMySqlCollation(MYSQL_CHARSET_GBK_CHINESE_CI_ID);
    expect(gbk.family == MySqlCharsetFamily.Gbk);
    expect_eq(gbk.maxBytesPerChar(), 2);
    expect(strEq(gbk.collation_name, "gbk_chinese_ci"));

    const gb18030 = lookupMySqlCollation(MYSQL_CHARSET_GB18030_CHINESE_CI_ID);
    expect(gb18030.family == MySqlCharsetFamily.Gb18030);
    expect_eq(gb18030.maxBytesPerChar(), 4);

    const utf16 = lookupMySqlCollation(101);
    expect(utf16.family == MySqlCharsetFamily.Utf16);
    expect(strEq(utf16.collation_name, "utf16_unicode_ci"));
    expect_eq(utf16.maxBytesPerChar(), 4);

    const utf32 = lookupMySqlCollation(160);
    expect(utf32.family == MySqlCharsetFamily.Utf32);
    expect_eq(utf32.maxBytesPerChar(), 4);

    const lat1_cs = lookupMySqlCollation(49);
    expect(strEq(lat1_cs.collation_name, "latin1_general_cs"));
    expect(lat1_cs.is_case_sensitive);
    expect(!lat1_cs.is_binary_collation);
});

test("sql mysql_charset: MySQL 8.0 utf8mb4_0900_* & >255 handshakeByte fallback", fn() {
    const c255 = lookupMySqlCollation(MYSQL_CHARSET_UTF8MB4_0900_AI_CI_ID);
    expect(c255.isKnown());
    expect(strEq(c255.collation_name, "utf8mb4_0900_ai_ci"));
    expect_eq(c255.handshakeByte(), 255);

    const c278 = lookupMySqlCollation(278);
    expect(c278.isKnown());
    expect(c278.is_case_sensitive);
    expect(!c278.is_binary_collation);
    expect_eq(c278.handshakeByte(), 255);

    const c309 = lookupMySqlCollation(309);
    expect(c309.isKnown());
    expect(c309.is_binary_collation);
    expect(c309.isUtf8Compatible());
    expect_eq(c309.handshakeByte(), 255);
});

test("sql mysql_charset: case-insensitive name & alias resolution", fn() {
    if (a := lookupMySqlCollationByName("UTF8MB4")) {
        expect_eq(a.id, 45);
    } else {
        expect(false);
    }

    if (b := lookupMySqlCollationByName("utf8mb4_0900_ai_ci")) {
        expect_eq(b.id, 255);
    } else {
        expect(false);
    }

    if (c := lookupMySqlCollationByName("UTF8")) {
        expect_eq(c.id, 33);
        expect_eq(c.maxBytesPerChar(), 3);
    } else {
        expect(false);
    }

    if (d := lookupMySqlCollationByName("Binary")) {
        expect_eq(d.id, 63);
        expect(d.isRawBinary());
    } else {
        expect(false);
    }

    if (e := lookupMySqlCollationByName("GB18030")) {
        expect_eq(e.id, 248);
        expect_eq(e.maxBytesPerChar(), 4);
    } else {
        expect(false);
    }

    expect(lookupMySqlCollationByName("nonexistent_charset") == null);
    expect(lookupMySqlCollationByName("") == null);
});

test("sql mysql_charset: family defaults and maxCharLen helper", fn() {
    expect_eq(MySqlCharsetFamily.Utf8mb4.defaultCollationId(), MYSQL_CHARSET_DEFAULT_ID);
    expect_eq(MySqlCharsetFamily.Binary.defaultCollationId(), MYSQL_CHARSET_BINARY_ID);
    expect_eq(MySqlCharsetFamily.Ascii.defaultCollationId(), MYSQL_CHARSET_ASCII_GENERAL_CI_ID);
    expect_eq(MySqlCharsetFamily.Latin1.defaultCollationId(), MYSQL_CHARSET_LATIN1_SWEDISH_CI_ID);
    expect_eq(MySqlCharsetFamily.Gbk.defaultCollationId(), MYSQL_CHARSET_GBK_CHINESE_CI_ID);
    expect_eq(MySqlCharsetFamily.Gb18030.defaultCollationId(), MYSQL_CHARSET_GB18030_CHINESE_CI_ID);

    expect_eq(mysqlMaxCharLen(MYSQL_CHARSET_UTF8MB4_UNICODE_CI_ID), 4);
    expect_eq(mysqlMaxCharLen(MYSQL_CHARSET_UTF8MB3_GENERAL_CI_ID), 3);
    expect_eq(mysqlMaxCharLen(MYSQL_CHARSET_GBK_CHINESE_CI_ID), 2);
    expect_eq(mysqlMaxCharLen(MYSQL_CHARSET_ASCII_GENERAL_CI_ID), 1);
});

test("sql mysql_charset: unknown/reserved IDs safe fallback", fn() {
    const z = lookupMySqlCollation(0);
    expect(!z.isKnown());
    expect(strEq(z.collation_name, "(unknown)"));
    expect(z.family == MySqlCharsetFamily.Unknown);
    expect_eq(z.maxBytesPerChar(), 1);
    expect_eq(z.handshakeByte(), 45);

    const gap17 = lookupMySqlCollation(17);
    expect(!gap17.isKnown());
    expect_eq(mysqlMaxCharLen(17), 1);

    const high = lookupMySqlCollation(65535);
    expect(!high.isKnown());
    expect(!high.isRawBinary());
    expect_eq(high.handshakeByte(), 45);
});
