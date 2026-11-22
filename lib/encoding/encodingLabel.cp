// Source: Absorbed from /data/dev/bun/src/runtime/webcore/EncodingLabel.cp | Synced: 2026-11-22T16:41:27Z
// Pure-CP zero-allocation WHATWG Encoding Standard label catalog & BOM detector.
// Maps 135+ case-insensitive encoding aliases (after trimming ASCII whitespace)
// to their canonical WHATWG EncodingLabel.

pub enum EncodingLabel {
    Utf8,
    Ibm866,
    Iso8859_3,
    Iso8859_6,
    Iso8859_7,
    Iso8859_8,
    Iso8859_8I,
    Koi8U,
    Windows874,
    Windows1252,
    Windows1253,
    Windows1255,
    Windows1257,
    Big5,
    EucJp,
    Iso2022Jp,
    ShiftJis,
    EucKr,
    Utf16Be,
    Utf16Le,
    XUserDefined,
    Replacement,
    Gbk,
    Gb18030,

    fn getLabel(): []const u8 {
        if (_ == EncodingLabel.Utf8) return "utf-8";
        if (_ == EncodingLabel.Utf16Le) return "utf-16le";
        if (_ == EncodingLabel.Utf16Be) return "utf-16be";
        if (_ == EncodingLabel.Windows1252) return "windows-1252";
        if (_ == EncodingLabel.Ibm866) return "ibm866";
        if (_ == EncodingLabel.Iso8859_3) return "iso-8859-3";
        if (_ == EncodingLabel.Iso8859_6) return "iso-8859-6";
        if (_ == EncodingLabel.Iso8859_7) return "iso-8859-7";
        if (_ == EncodingLabel.Iso8859_8) return "iso-8859-8";
        if (_ == EncodingLabel.Iso8859_8I) return "iso-8859-8-i";
        if (_ == EncodingLabel.Koi8U) return "koi8-u";
        if (_ == EncodingLabel.Windows874) return "windows-874";
        if (_ == EncodingLabel.Windows1253) return "windows-1253";
        if (_ == EncodingLabel.Windows1255) return "windows-1255";
        if (_ == EncodingLabel.Windows1257) return "windows-1257";
        if (_ == EncodingLabel.Big5) return "big5";
        if (_ == EncodingLabel.EucJp) return "euc-jp";
        if (_ == EncodingLabel.Iso2022Jp) return "iso-2022-jp";
        if (_ == EncodingLabel.ShiftJis) return "shift_jis";
        if (_ == EncodingLabel.EucKr) return "euc-kr";
        if (_ == EncodingLabel.XUserDefined) return "x-user-defined";
        if (_ == EncodingLabel.Replacement) return "replacement";
        if (_ == EncodingLabel.Gbk) return "gbk";
        return "gb18030";
    }

    fn isUnicode(): bool {
        return _ == EncodingLabel.Utf8 || _ == EncodingLabel.Utf16Le || _ == EncodingLabel.Utf16Be;
    }

    fn isCjk(): bool {
        return _ == EncodingLabel.Big5 ||
            _ == EncodingLabel.EucJp ||
            _ == EncodingLabel.Iso2022Jp ||
            _ == EncodingLabel.ShiftJis ||
            _ == EncodingLabel.EucKr ||
            _ == EncodingLabel.Gbk ||
            _ == EncodingLabel.Gb18030;
    }

    fn isSingleByte(): bool {
        return _ == EncodingLabel.Windows1252 ||
            _ == EncodingLabel.Ibm866 ||
            _ == EncodingLabel.Iso8859_3 ||
            _ == EncodingLabel.Iso8859_6 ||
            _ == EncodingLabel.Iso8859_7 ||
            _ == EncodingLabel.Iso8859_8 ||
            _ == EncodingLabel.Iso8859_8I ||
            _ == EncodingLabel.Koi8U ||
            _ == EncodingLabel.Windows874 ||
            _ == EncodingLabel.Windows1253 ||
            _ == EncodingLabel.Windows1255 ||
            _ == EncodingLabel.Windows1257 ||
            _ == EncodingLabel.XUserDefined;
    }

    fn isAsciiCompatible(): bool {
        return _ != EncodingLabel.Utf16Le &&
            _ != EncodingLabel.Utf16Be &&
            _ != EncodingLabel.Iso2022Jp &&
            _ != EncodingLabel.Replacement;
    }

    /// Resolves a WHATWG encoding label (case-insensitive, trims leading/trailing ASCII whitespace).
    static fn which(input: []const u8): EncodingLabel? {
        return lookupEncodingLabel(input);
    }
}

fn isWhatwgWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == 0x0C;
}

fn trimWhatwgWs(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isWhatwgWs(s[start])) start += 1;
    while (end > start && isWhatwgWs(s[end - 1])) end -= 1;
    return s[start..end];
}

fn eqlLowerAscii(raw: []const u8, lower_lit: []const u8): bool {
    if (raw.len != lower_lit.len) return false;
    let i: usize = 0;
    while (i < raw.len) {
        let c = raw[i];
        if (c >= 'A' && c <= 'Z') c += 32;
        if (c != lower_lit[i]) return false;
        i += 1;
    }
    return true;
}

/// Looks up a WHATWG EncodingLabel from any standard alias (case-insensitive).
pub fn lookupEncodingLabel(raw: []const u8): EncodingLabel? {
    const s = trimWhatwgWs(raw);
    if (s.len == 0 || s.len > 24) return null;

    // UTF-8 (hot path first)
    if (eqlLowerAscii(s, "utf-8") ||
        eqlLowerAscii(s, "utf8") ||
        eqlLowerAscii(s, "unicode-1-1-utf-8") ||
        eqlLowerAscii(s, "unicode11utf8") ||
        eqlLowerAscii(s, "unicode20utf8") ||
        eqlLowerAscii(s, "x-unicode20utf8"))
    {
        return EncodingLabel.Utf8;
    }

    // windows-1252 / latin1 / ascii
    if (eqlLowerAscii(s, "windows-1252") ||
        eqlLowerAscii(s, "latin1") ||
        eqlLowerAscii(s, "iso-8859-1") ||
        eqlLowerAscii(s, "iso8859-1") ||
        eqlLowerAscii(s, "iso_8859-1") ||
        eqlLowerAscii(s, "iso88591") ||
        eqlLowerAscii(s, "ascii") ||
        eqlLowerAscii(s, "us-ascii") ||
        eqlLowerAscii(s, "cp1252") ||
        eqlLowerAscii(s, "x-cp1252") ||
        eqlLowerAscii(s, "l1") ||
        eqlLowerAscii(s, "cp819") ||
        eqlLowerAscii(s, "ibm819") ||
        eqlLowerAscii(s, "iso-ir-100") ||
        eqlLowerAscii(s, "csisolatin1") ||
        eqlLowerAscii(s, "ansi_x3.4-1968") ||
        eqlLowerAscii(s, "iso_8859-1:1987"))
    {
        return EncodingLabel.Windows1252;
    }

    // UTF-16LE
    if (eqlLowerAscii(s, "utf-16le") ||
        eqlLowerAscii(s, "utf-16") ||
        eqlLowerAscii(s, "ucs-2") ||
        eqlLowerAscii(s, "unicode") ||
        eqlLowerAscii(s, "csunicode") ||
        eqlLowerAscii(s, "unicodefeff") ||
        eqlLowerAscii(s, "iso-10646-ucs-2"))
    {
        return EncodingLabel.Utf16Le;
    }

    // UTF-16BE
    if (eqlLowerAscii(s, "utf-16be")) {
        return EncodingLabel.Utf16Be;
    }

    // GBK & GB18030
    if (eqlLowerAscii(s, "gbk") ||
        eqlLowerAscii(s, "gb2312") ||
        eqlLowerAscii(s, "chinese") ||
        eqlLowerAscii(s, "csgb2312") ||
        eqlLowerAscii(s, "csiso58gb231280") ||
        eqlLowerAscii(s, "gb_2312") ||
        eqlLowerAscii(s, "gb_2312-80") ||
        eqlLowerAscii(s, "iso-ir-58") ||
        eqlLowerAscii(s, "x-gbk"))
    {
        return EncodingLabel.Gbk;
    }
    if (eqlLowerAscii(s, "gb18030")) {
        return EncodingLabel.Gb18030;
    }

    // Big5
    if (eqlLowerAscii(s, "big5") ||
        eqlLowerAscii(s, "big5-hkscs") ||
        eqlLowerAscii(s, "cn-big5") ||
        eqlLowerAscii(s, "csbig5") ||
        eqlLowerAscii(s, "x-x-big5"))
    {
        return EncodingLabel.Big5;
    }

    // Shift_JIS
    if (eqlLowerAscii(s, "shift_jis") ||
        eqlLowerAscii(s, "shift-jis") ||
        eqlLowerAscii(s, "sjis") ||
        eqlLowerAscii(s, "csshiftjis") ||
        eqlLowerAscii(s, "ms932") ||
        eqlLowerAscii(s, "ms_kanji") ||
        eqlLowerAscii(s, "windows-31j") ||
        eqlLowerAscii(s, "x-sjis"))
    {
        return EncodingLabel.ShiftJis;
    }

    // EUC-JP & ISO-2022-JP
    if (eqlLowerAscii(s, "euc-jp") ||
        eqlLowerAscii(s, "cseucpkdfmtjapanese") ||
        eqlLowerAscii(s, "x-euc-jp"))
    {
        return EncodingLabel.EucJp;
    }
    if (eqlLowerAscii(s, "iso-2022-jp") || eqlLowerAscii(s, "csiso2022jp")) {
        return EncodingLabel.Iso2022Jp;
    }

    // EUC-KR
    if (eqlLowerAscii(s, "euc-kr") ||
        eqlLowerAscii(s, "cseuckr") ||
        eqlLowerAscii(s, "csksc56011987") ||
        eqlLowerAscii(s, "iso-ir-149") ||
        eqlLowerAscii(s, "korean") ||
        eqlLowerAscii(s, "ks_c_5601-1987") ||
        eqlLowerAscii(s, "ks_c_5601-1989") ||
        eqlLowerAscii(s, "ksc5601") ||
        eqlLowerAscii(s, "ksc_5601") ||
        eqlLowerAscii(s, "windows-949"))
    {
        return EncodingLabel.EucKr;
    }

    // IBM866 & KOI8-U
    if (eqlLowerAscii(s, "ibm866") || eqlLowerAscii(s, "cp866") || eqlLowerAscii(s, "866") || eqlLowerAscii(s, "csibm866")) {
        return EncodingLabel.Ibm866;
    }
    if (eqlLowerAscii(s, "koi8-u") || eqlLowerAscii(s, "koi8-ru")) {
        return EncodingLabel.Koi8U;
    }

    // ISO-8859-3 / 6 / 7 / 8 / 8-I
    if (eqlLowerAscii(s, "iso-8859-3") || eqlLowerAscii(s, "iso8859-3") || eqlLowerAscii(s, "iso_8859-3") ||
        eqlLowerAscii(s, "latin3") || eqlLowerAscii(s, "csisolatin3") || eqlLowerAscii(s, "iso-ir-109") || eqlLowerAscii(s, "l3"))
    {
        return EncodingLabel.Iso8859_3;
    }
    if (eqlLowerAscii(s, "iso-8859-6") || eqlLowerAscii(s, "iso8859-6") || eqlLowerAscii(s, "iso_8859-6") ||
        eqlLowerAscii(s, "arabic") || eqlLowerAscii(s, "csisolatinarabic") || eqlLowerAscii(s, "iso-ir-127") ||
        eqlLowerAscii(s, "asmo-708") || eqlLowerAscii(s, "ecma-114"))
    {
        return EncodingLabel.Iso8859_6;
    }
    if (eqlLowerAscii(s, "iso-8859-7") || eqlLowerAscii(s, "iso8859-7") || eqlLowerAscii(s, "iso_8859-7") ||
        eqlLowerAscii(s, "greek") || eqlLowerAscii(s, "greek8") || eqlLowerAscii(s, "csisolatingreek") ||
        eqlLowerAscii(s, "iso-ir-126") || eqlLowerAscii(s, "ecma-118") || eqlLowerAscii(s, "elot_928"))
    {
        return EncodingLabel.Iso8859_7;
    }
    if (eqlLowerAscii(s, "iso-8859-8") || eqlLowerAscii(s, "iso8859-8") || eqlLowerAscii(s, "iso_8859-8") ||
        eqlLowerAscii(s, "hebrew") || eqlLowerAscii(s, "csisolatinhebrew") || eqlLowerAscii(s, "iso-ir-138") || eqlLowerAscii(s, "visual"))
    {
        return EncodingLabel.Iso8859_8;
    }
    if (eqlLowerAscii(s, "iso-8859-8-i") || eqlLowerAscii(s, "logical") || eqlLowerAscii(s, "csiso88598i")) {
        return EncodingLabel.Iso8859_8I;
    }

    // windows-874 / 1253 / 1255 / 1257
    if (eqlLowerAscii(s, "windows-874") || eqlLowerAscii(s, "dos-874") || eqlLowerAscii(s, "iso-8859-11") ||
        eqlLowerAscii(s, "iso8859-11") || eqlLowerAscii(s, "iso885911") || eqlLowerAscii(s, "iso_8859-11") || eqlLowerAscii(s, "tis-620"))
    {
        return EncodingLabel.Windows874;
    }
    if (eqlLowerAscii(s, "windows-1253") || eqlLowerAscii(s, "cp1253") || eqlLowerAscii(s, "x-cp1253")) {
        return EncodingLabel.Windows1253;
    }
    if (eqlLowerAscii(s, "windows-1255") || eqlLowerAscii(s, "cp1255") || eqlLowerAscii(s, "x-cp1255")) {
        return EncodingLabel.Windows1255;
    }
    if (eqlLowerAscii(s, "windows-1257") || eqlLowerAscii(s, "cp1257") || eqlLowerAscii(s, "x-cp1257")) {
        return EncodingLabel.Windows1257;
    }

    // Special encodings
    if (eqlLowerAscii(s, "x-user-defined")) return EncodingLabel.XUserDefined;
    if (eqlLowerAscii(s, "replacement")) return EncodingLabel.Replacement;

    return null;
}

/// Returns the canonical lowercase WHATWG encoding name for an alias, or `null` if unknown.
pub fn canonicalEncodingName(raw: []const u8): ([]const u8)? {
    if (lbl := lookupEncodingLabel(raw)) {
        return lbl.getLabel();
    }
    return null;
}

/// Detects Unicode BOM prefix (`UTF-8`, `UTF-16BE`, `UTF-16LE`).
pub fn detectBomEncoding(bytes: []const u8): EncodingLabel? {
    if (bytes.len >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF) {
        return EncodingLabel.Utf8;
    }
    if (bytes.len >= 2 && bytes[0] == 0xFE && bytes[1] == 0xFF) {
        return EncodingLabel.Utf16Be;
    }
    if (bytes.len >= 2 && bytes[0] == 0xFF && bytes[1] == 0xFE) {
        return EncodingLabel.Utf16Le;
    }
    return null;
}
