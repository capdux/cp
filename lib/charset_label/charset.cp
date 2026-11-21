// Source: Absorbed from /data/dev/bun/src/bun.js/webcore/encoding.zig | Synced: 2026-11-21T15:29:38Z (WHATWG Charset Label Canonicalizer)

/// WHATWG Encoding Standard 核心字符集枚举
export enum CharsetKind {
    utf8,
    utf16le,
    utf16be,
    windows1252,
    iso8859_2,
    gbk,
    gb18030,
    big5,
    shift_jis,
    euc_jp,
    euc_kr,
    koi8_r,
    windows1251,
    ibm866,
}

fn isAsciiWhitespace(c: u8): bool {
    return c == 0x09 || c == 0x0A || c == 0x0C || c == 0x0D || c == 0x20;
}

fn trimAsciiWhitespace(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isAsciiWhitespace(s[start])) {
        start += 1;
    }
    while (end > start && isAsciiWhitespace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn asciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
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

/// 按 WHATWG Encoding Standard §4.2 将字符集标签（含大小写与别名）解析为规范 CharsetKind
export fn resolveCharsetLabel(label: []const u8): CharsetKind? {
    const s = trimAsciiWhitespace(label);
    if (s.len == 0) {
        return null;
    }

    // UTF-8
    if (asciiEqlIgnoreCase(s, "utf-8")
        || asciiEqlIgnoreCase(s, "utf8")
        || asciiEqlIgnoreCase(s, "unicode-1-1-utf-8")
        || asciiEqlIgnoreCase(s, "unicode11utf8")
        || asciiEqlIgnoreCase(s, "unicode20utf8")
        || asciiEqlIgnoreCase(s, "x-unicode20utf8"))
    {
        return CharsetKind.utf8;
    }

    // UTF-16LE
    if (asciiEqlIgnoreCase(s, "utf-16")
        || asciiEqlIgnoreCase(s, "utf-16le")
        || asciiEqlIgnoreCase(s, "utf16")
        || asciiEqlIgnoreCase(s, "utf16le")
        || asciiEqlIgnoreCase(s, "ucs-2")
        || asciiEqlIgnoreCase(s, "unicode")
        || asciiEqlIgnoreCase(s, "unicodefeff")
        || asciiEqlIgnoreCase(s, "iso-10646-ucs-2")
        || asciiEqlIgnoreCase(s, "csunicode"))
    {
        return CharsetKind.utf16le;
    }

    // UTF-16BE
    if (asciiEqlIgnoreCase(s, "utf-16be")
        || asciiEqlIgnoreCase(s, "utf16be")
        || asciiEqlIgnoreCase(s, "unicodefffe"))
    {
        return CharsetKind.utf16be;
    }

    // windows-1252 (含 latin1 / iso-8859-1 / ascii / us-ascii)
    if (asciiEqlIgnoreCase(s, "windows-1252")
        || asciiEqlIgnoreCase(s, "cp1252")
        || asciiEqlIgnoreCase(s, "x-cp1252")
        || asciiEqlIgnoreCase(s, "latin1")
        || asciiEqlIgnoreCase(s, "l1")
        || asciiEqlIgnoreCase(s, "iso-8859-1")
        || asciiEqlIgnoreCase(s, "iso8859-1")
        || asciiEqlIgnoreCase(s, "iso88591")
        || asciiEqlIgnoreCase(s, "iso_8859-1")
        || asciiEqlIgnoreCase(s, "iso_8859-1:1987")
        || asciiEqlIgnoreCase(s, "ascii")
        || asciiEqlIgnoreCase(s, "us-ascii")
        || asciiEqlIgnoreCase(s, "ansi_x3.4-1968")
        || asciiEqlIgnoreCase(s, "cp819")
        || asciiEqlIgnoreCase(s, "ibm819")
        || asciiEqlIgnoreCase(s, "csisolatin1"))
    {
        return CharsetKind.windows1252;
    }

    // ISO-8859-2
    if (asciiEqlIgnoreCase(s, "iso-8859-2")
        || asciiEqlIgnoreCase(s, "iso8859-2")
        || asciiEqlIgnoreCase(s, "iso88592")
        || asciiEqlIgnoreCase(s, "iso_8859-2")
        || asciiEqlIgnoreCase(s, "iso_8859-2:1987")
        || asciiEqlIgnoreCase(s, "latin2")
        || asciiEqlIgnoreCase(s, "l2")
        || asciiEqlIgnoreCase(s, "csisolatin2"))
    {
        return CharsetKind.iso8859_2;
    }

    // GBK (含 gb2312 / chinese)
    if (asciiEqlIgnoreCase(s, "gbk")
        || asciiEqlIgnoreCase(s, "gb2312")
        || asciiEqlIgnoreCase(s, "gb_2312")
        || asciiEqlIgnoreCase(s, "gb_2312-80")
        || asciiEqlIgnoreCase(s, "csgb2312")
        || asciiEqlIgnoreCase(s, "chinese")
        || asciiEqlIgnoreCase(s, "iso-ir-58")
        || asciiEqlIgnoreCase(s, "x-gbk")
        || asciiEqlIgnoreCase(s, "csiso58gb231280"))
    {
        return CharsetKind.gbk;
    }

    // GB18030
    if (asciiEqlIgnoreCase(s, "gb18030")) {
        return CharsetKind.gb18030;
    }

    // Big5
    if (asciiEqlIgnoreCase(s, "big5")
        || asciiEqlIgnoreCase(s, "big5-hkscs")
        || asciiEqlIgnoreCase(s, "cn-big5")
        || asciiEqlIgnoreCase(s, "csbig5")
        || asciiEqlIgnoreCase(s, "x-x-big5"))
    {
        return CharsetKind.big5;
    }

    // Shift_JIS
    if (asciiEqlIgnoreCase(s, "shift_jis")
        || asciiEqlIgnoreCase(s, "shift-jis")
        || asciiEqlIgnoreCase(s, "sjis")
        || asciiEqlIgnoreCase(s, "ms932")
        || asciiEqlIgnoreCase(s, "ms_kanji")
        || asciiEqlIgnoreCase(s, "csshiftjis")
        || asciiEqlIgnoreCase(s, "windows-31j")
        || asciiEqlIgnoreCase(s, "x-sjis"))
    {
        return CharsetKind.shift_jis;
    }

    // EUC-JP
    if (asciiEqlIgnoreCase(s, "euc-jp")
        || asciiEqlIgnoreCase(s, "cseucpkdfmtjapanese")
        || asciiEqlIgnoreCase(s, "x-euc-jp"))
    {
        return CharsetKind.euc_jp;
    }

    // EUC-KR
    if (asciiEqlIgnoreCase(s, "euc-kr")
        || asciiEqlIgnoreCase(s, "cseuckr")
        || asciiEqlIgnoreCase(s, "csksc56011987")
        || asciiEqlIgnoreCase(s, "iso-ir-149")
        || asciiEqlIgnoreCase(s, "korean")
        || asciiEqlIgnoreCase(s, "ks_c_5601-1987")
        || asciiEqlIgnoreCase(s, "ks_c_5601-1989")
        || asciiEqlIgnoreCase(s, "ksc5601")
        || asciiEqlIgnoreCase(s, "ksc_5601")
        || asciiEqlIgnoreCase(s, "windows-949"))
    {
        return CharsetKind.euc_kr;
    }

    // KOI8-R
    if (asciiEqlIgnoreCase(s, "koi8-r")
        || asciiEqlIgnoreCase(s, "koi8")
        || asciiEqlIgnoreCase(s, "koi8_r")
        || asciiEqlIgnoreCase(s, "koi")
        || asciiEqlIgnoreCase(s, "cskoi8r"))
    {
        return CharsetKind.koi8_r;
    }

    // windows-1251
    if (asciiEqlIgnoreCase(s, "windows-1251")
        || asciiEqlIgnoreCase(s, "cp1251")
        || asciiEqlIgnoreCase(s, "x-cp1251"))
    {
        return CharsetKind.windows1251;
    }

    // IBM866
    if (asciiEqlIgnoreCase(s, "ibm866")
        || asciiEqlIgnoreCase(s, "866")
        || asciiEqlIgnoreCase(s, "cp866")
        || asciiEqlIgnoreCase(s, "csibm866"))
    {
        return CharsetKind.ibm866;
    }

    return null;
}

/// 返回标准 WHATWG Encoding 规范名称
export fn canonicalCharsetName(kind: CharsetKind): []const u8 {
    return switch (kind) {
        .utf8 => "utf-8",
        .utf16le => "utf-16le",
        .utf16be => "utf-16be",
        .windows1252 => "windows-1252",
        .iso8859_2 => "iso-8859-2",
        .gbk => "gbk",
        .gb18030 => "gb18030",
        .big5 => "big5",
        .shift_jis => "shift_jis",
        .euc_jp => "euc-jp",
        .euc_kr => "euc-kr",
        .koi8_r => "koi8-r",
        .windows1251 => "windows-1251",
        .ibm866 => "ibm866",
    };
}

/// 判断给定字符集标签是否与 UTF-8 兼容（UTF-8 及其别名，或纯 7-bit ASCII 别名）
export fn isUtf8Compatible(label: []const u8): bool {
    const s = trimAsciiWhitespace(label);
    if (s.len == 0) {
        return false;
    }
    if (kind := resolveCharsetLabel(s)) {
        if (kind == CharsetKind.utf8) {
            return true;
        }
    }
    return asciiEqlIgnoreCase(s, "ascii")
        || asciiEqlIgnoreCase(s, "us-ascii")
        || asciiEqlIgnoreCase(s, "ansi_x3.4-1968");
}

/// 判断字符集是否为 ASCII 超集（UTF-16LE / UTF-16BE 返回 false，其余返回 true）
export fn isAsciiSuperset(kind: CharsetKind): bool {
    return switch (kind) {
        .utf16le, .utf16be => false,
        else => true,
    };
}

/// 根据字节流前导 BOM 嗅探字符集（UTF-8 `EF BB BF`、UTF-16BE `FE FF`、UTF-16LE `FF FE`）
export fn sniffBomCharset(bytes: []const u8): CharsetKind? {
    if (bytes.len >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF) {
        return CharsetKind.utf8;
    }
    if (bytes.len >= 2 && bytes[0] == 0xFE && bytes[1] == 0xFF) {
        return CharsetKind.utf16be;
    }
    if (bytes.len >= 2 && bytes[0] == 0xFF && bytes[1] == 0xFE) {
        return CharsetKind.utf16le;
    }
    return null;
}
