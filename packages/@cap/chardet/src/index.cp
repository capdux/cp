// @cap/chardet — Zero-allocation character encoding detector in pure CP
// Clean-room MIT implementation of npm `chardet` (#641, 90.3M/wk).

pub enum DetectedCharset {
    Utf8,
    Utf8Bom,
    Utf16Le,
    Utf16Be,
    Utf32Le,
    Utf32Be,
    Ascii,
    Windows1252,
    Iso8859_1,
    ShiftJis,
    Gbk,
    Unknown,
}

pub struct CharsetMatch {
    pub charset: DetectedCharset,
    pub name: string,
    pub confidence: u8, // 0..100
}

pub fn charsetName(cs: DetectedCharset): string {
    if (cs == .Utf8 || cs == .Utf8Bom) { return "UTF-8"; }
    if (cs == .Utf16Le) { return "UTF-16LE"; }
    if (cs == .Utf16Be) { return "UTF-16BE"; }
    if (cs == .Utf32Le) { return "UTF-32LE"; }
    if (cs == .Utf32Be) { return "UTF-32BE"; }
    if (cs == .Ascii) { return "ASCII"; }
    if (cs == .Windows1252) { return "windows-1252"; }
    if (cs == .Iso8859_1) { return "ISO-8859-1"; }
    if (cs == .ShiftJis) { return "Shift_JIS"; }
    if (cs == .Gbk) { return "GB18030"; }
    return "unknown";
}

/// Checks strict UTF-8 validity and returns whether any multi-byte sequence was seen.
/// Returns: 0 = invalid UTF-8, 1 = pure ASCII, 2 = valid multi-byte UTF-8.
fn chdCheckUtf8(data: []const u8): u8 {
    let i: usize = 0;
    let has_multibyte = false;
    while (i < data.len) {
        const b0 = data[i];
        if (b0 < 0x80) {
            i += 1;
            continue;
        }
        has_multibyte = true;
        if (b0 >= 0xC2 && b0 <= 0xDF) {
            if (i + 1 >= data.len) { return 0; }
            const b1 = data[i + 1];
            if ((b1 & 0xC0) != 0x80) { return 0; }
            i += 2;
        } else if (b0 >= 0xE0 && b0 <= 0xEF) {
            if (i + 2 >= data.len) { return 0; }
            const b1 = data[i + 1];
            const b2 = data[i + 2];
            if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) { return 0; }
            if (b0 == 0xE0 && b1 < 0xA0) { return 0; } // overlong
            if (b0 == 0xED && b1 >= 0xA0) { return 0; } // surrogate
            i += 3;
        } else if (b0 >= 0xF0 && b0 <= 0xF4) {
            if (i + 3 >= data.len) { return 0; }
            const b1 = data[i + 1];
            const b2 = data[i + 2];
            const b3 = data[i + 3];
            if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) { return 0; }
            if (b0 == 0xF0 && b1 < 0x90) { return 0; } // overlong
            if (b0 == 0xF4 && b1 > 0x8F) { return 0; } // > U+10FFFF
            i += 4;
        } else {
            return 0;
        }
    }
    if (has_multibyte) {
        return 2;
    }
    return 1;
}

/// Scores how well `data` fits Shift-JIS double-byte lead/trail byte rules (0..100).
fn chdScoreShiftJis(data: []const u8): u8 {
    let i: usize = 0;
    let double_pairs: usize = 0;
    let half_kana: usize = 0;
    let invalid: usize = 0;
    while (i < data.len) {
        const b = data[i];
        if (b < 0x80) {
            i += 1;
            continue;
        }
        if (b >= 0xA1 && b <= 0xDF) {
            // Half-width Katakana
            half_kana += 1;
            i += 1;
            continue;
        }
        if ((b >= 0x81 && b <= 0x9F) || (b >= 0xE0 && b <= 0xEF)) {
            if (i + 1 < data.len) {
                const b2 = data[i + 1];
                if ((b2 >= 0x40 && b2 <= 0x7E) || (b2 >= 0x80 && b2 <= 0xFC)) {
                    double_pairs += 1;
                    i += 2;
                    continue;
                }
            }
        }
        invalid += 1;
        i += 1;
    }
    if ((double_pairs == 0 && half_kana == 0) || invalid > 0) {
        return 0;
    }
    if (double_pairs > 0) {
        return 80;
    }
    return 65; // half-width Katakana only: lower priority than GBK double-byte Hanzi
}

/// Scores how well `data` fits GBK / GB18030 double-byte sequences (0..100).
fn chdScoreGbk(data: []const u8): u8 {
    let i: usize = 0;
    let valid_pairs: usize = 0;
    let invalid: usize = 0;
    while (i < data.len) {
        const b = data[i];
        if (b < 0x80) {
            i += 1;
            continue;
        }
        if (b >= 0x81 && b <= 0xFE) {
            if (i + 1 < data.len) {
                const b2 = data[i + 1];
                if ((b2 >= 0x40 && b2 <= 0x7E) || (b2 >= 0x80 && b2 <= 0xFE)) {
                    valid_pairs += 1;
                    i += 2;
                    continue;
                }
            }
        }
        invalid += 1;
        i += 1;
    }
    if (valid_pairs == 0 || invalid > 0) {
        return 0;
    }
    return 75;
}

/// Detects the most likely character encoding of `data` with a confidence score (0..100).
pub fn detect(data: []const u8): CharsetMatch {
    if (data.len == 0) {
        return CharsetMatch { charset: .Ascii, name: "ASCII", confidence: 100 };
    }

    // 1. BOM checks (100% confidence)
    if (data.len >= 4) {
        if (data[0] == 0x00 && data[1] == 0x00 && data[2] == 0xFE && data[3] == 0xFF) {
            return CharsetMatch { charset: .Utf32Be, name: "UTF-32BE", confidence: 100 };
        }
        if (data[0] == 0xFF && data[1] == 0xFE && data[2] == 0x00 && data[3] == 0x00) {
            return CharsetMatch { charset: .Utf32Le, name: "UTF-32LE", confidence: 100 };
        }
    }
    if (data.len >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
        return CharsetMatch { charset: .Utf8Bom, name: "UTF-8", confidence: 100 };
    }
    if (data.len >= 2) {
        if (data[0] == 0xFE && data[1] == 0xFF) {
            return CharsetMatch { charset: .Utf16Be, name: "UTF-16BE", confidence: 100 };
        }
        if (data[0] == 0xFF && data[1] == 0xFE) {
            return CharsetMatch { charset: .Utf16Le, name: "UTF-16LE", confidence: 100 };
        }
    }

    // 2. UTF-16LE / UTF-16BE null-byte cadence heuristic (for BOM-less UTF-16 ASCII text)
    if (data.len >= 4 && (data.len % 2) == 0) {
        let even_zeros: usize = 0;
        let odd_zeros: usize = 0;
        let i: usize = 0;
        while (i < data.len) {
            if (data[i] == 0) { even_zeros += 1; }
            if (data[i + 1] == 0) { odd_zeros += 1; }
            i += 2;
        }
        const pairs = data.len / 2;
        if (odd_zeros * 4 >= pairs * 3 && even_zeros == 0) {
            return CharsetMatch { charset: .Utf16Le, name: "UTF-16LE", confidence: 90 };
        }
        if (even_zeros * 4 >= pairs * 3 && odd_zeros == 0) {
            return CharsetMatch { charset: .Utf16Be, name: "UTF-16BE", confidence: 90 };
        }
    }

    // 3. Strict UTF-8 / ASCII check
    const utf8_res = chdCheckUtf8(data);
    if (utf8_res == 1) {
        return CharsetMatch { charset: .Ascii, name: "ASCII", confidence: 100 };
    }
    if (utf8_res == 2) {
        return CharsetMatch { charset: .Utf8, name: "UTF-8", confidence: 95 };
    }

    // 4. Legacy single-byte vs CJK multi-byte heuristics
    let has_c1_printable = false;
    let has_high_bytes = false;
    let idx: usize = 0;
    while (idx < data.len) {
        const b = data[idx];
        if (b >= 0x80) { has_high_bytes = true; }
        if (b >= 0x80 && b <= 0x9F) { has_c1_printable = true; }
        idx += 1;
    }

    const sjis_score = chdScoreShiftJis(data);
    const gbk_score = chdScoreGbk(data);
    if (sjis_score > 0 && sjis_score >= gbk_score) {
        return CharsetMatch { charset: .ShiftJis, name: "Shift_JIS", confidence: sjis_score };
    }
    if (gbk_score > 0) {
        return CharsetMatch { charset: .Gbk, name: "GB18030", confidence: gbk_score };
    }

    if (has_c1_printable) {
        return CharsetMatch { charset: .Windows1252, name: "windows-1252", confidence: 70 };
    }
    if (has_high_bytes) {
        return CharsetMatch { charset: .Iso8859_1, name: "ISO-8859-1", confidence: 60 };
    }
    return CharsetMatch { charset: .Unknown, name: "unknown", confidence: 0 };
}

/// Convenience helper returning just the detected encoding name string.
pub fn detectName(data: []const u8): string {
    return detect(data).name;
}

export {
    DetectedCharset,
    CharsetMatch,
    charsetName,
    detect,
    detectName,
};
