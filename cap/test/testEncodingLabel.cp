// Origin: Pure-CP Stdlib Test Suite (lib/encoding/encodingLabel.cp) | Created: 2026-11-22
// Production Deep Verification Suite for WHATWG Encoding Label Catalog & BOM Detector:
//   1. UTF-8 / UTF-16LE / UTF-16BE alias resolution & ASCII whitespace trimming
//   2. windows-1252 / latin1 / us-ascii / iso-8859-1 WHATWG equivalence & canonical names
//   3. CJK encodings (GBK/gb2312, GB18030, Big5, Shift_JIS/ms932, EUC-JP, ISO-2022-JP, EUC-KR/windows-949)
//   4. Legacy single-byte encodings (IBM866, KOI8-U, ISO-8859-3/6/7/8/8-I, windows-874/1253/1255/1257)
//   5. Encoding classification predicates (isUnicode, isCjk, isSingleByte, isAsciiCompatible)
//   6. Unicode BOM detection (UTF-8, UTF-16BE, UTF-16LE, short/non-BOM) & invalid label rejection

import { test, expect_true } from "cap:test";
import {
    EncodingLabel,
    lookupEncodingLabel,
    canonicalEncodingName,
    detectBomEncoding,
} from "../../lib/encoding/encodingLabel.cp";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

// ============================================================================
// Test 1: Unicode label aliases & WHATWG whitespace trimming
// ============================================================================
test("encodingLabel: Unicode aliases and WHATWG ASCII whitespace trimming", () => {
    expect_true(EncodingLabel.which("utf-8") == EncodingLabel.Utf8);
    expect_true(EncodingLabel.which("UTF8") == EncodingLabel.Utf8);
    expect_true(EncodingLabel.which(" \t\r\n Unicode-1-1-UTF-8 \x0C") == EncodingLabel.Utf8);
    expect_true(EncodingLabel.which("unicode20utf8") == EncodingLabel.Utf8);

    expect_true(EncodingLabel.which("utf-16") == EncodingLabel.Utf16Le);
    expect_true(EncodingLabel.which("UCS-2") == EncodingLabel.Utf16Le);
    expect_true(EncodingLabel.which("unicodeFEFF") == EncodingLabel.Utf16Le);
    expect_true(EncodingLabel.which("UTF-16BE") == EncodingLabel.Utf16Be);

    expect_true(strEq(EncodingLabel.Utf8.getLabel(), "utf-8"));
    expect_true(strEq(EncodingLabel.Utf16Le.getLabel(), "utf-16le"));
    expect_true(strEq(EncodingLabel.Utf16Be.getLabel(), "utf-16be"));
});

// ============================================================================
// Test 2: windows-1252 / latin1 / ascii WHATWG canonicalization
// ============================================================================
test("encodingLabel: latin1, us-ascii, and iso-8859-1 map to windows-1252", () => {
    expect_true(lookupEncodingLabel("latin1") == EncodingLabel.Windows1252);
    expect_true(lookupEncodingLabel("ASCII") == EncodingLabel.Windows1252);
    expect_true(lookupEncodingLabel("US-ASCII") == EncodingLabel.Windows1252);
    expect_true(lookupEncodingLabel("ISO-8859-1") == EncodingLabel.Windows1252);
    expect_true(lookupEncodingLabel("cp1252") == EncodingLabel.Windows1252);
    expect_true(lookupEncodingLabel("ANSI_X3.4-1968") == EncodingLabel.Windows1252);

    if (canon := canonicalEncodingName("ISO_8859-1:1987")) {
        expect_true(strEq(canon, "windows-1252"));
    } else {
        expect_true(false);
    }
});

// ============================================================================
// Test 3: CJK encoding alias resolution
// ============================================================================
test("encodingLabel: CJK aliases (GBK, GB18030, Big5, Shift_JIS, EUC-JP, EUC-KR)", () => {
    expect_true(lookupEncodingLabel("gb2312") == EncodingLabel.Gbk);
    expect_true(lookupEncodingLabel("chinese") == EncodingLabel.Gbk);
    expect_true(lookupEncodingLabel("GB18030") == EncodingLabel.Gb18030);
    expect_true(lookupEncodingLabel("big5-hkscs") == EncodingLabel.Big5);
    expect_true(lookupEncodingLabel("ms932") == EncodingLabel.ShiftJis);
    expect_true(lookupEncodingLabel("windows-31j") == EncodingLabel.ShiftJis);
    expect_true(lookupEncodingLabel("x-euc-jp") == EncodingLabel.EucJp);
    expect_true(lookupEncodingLabel("csISO2022JP") == EncodingLabel.Iso2022Jp);
    expect_true(lookupEncodingLabel("windows-949") == EncodingLabel.EucKr);
    expect_true(lookupEncodingLabel("korean") == EncodingLabel.EucKr);

    expect_true(strEq(EncodingLabel.ShiftJis.getLabel(), "shift_jis"));
    expect_true(strEq(EncodingLabel.Gbk.getLabel(), "gbk"));
    expect_true(strEq(EncodingLabel.Gb18030.getLabel(), "gb18030"));
});

// ============================================================================
// Test 4: Legacy single-byte & special encodings
// ============================================================================
test("encodingLabel: IBM866, KOI8-U, ISO-8859-*, windows-* & special labels", () => {
    expect_true(lookupEncodingLabel("cp866") == EncodingLabel.Ibm866);
    expect_true(lookupEncodingLabel("koi8-ru") == EncodingLabel.Koi8U);
    expect_true(lookupEncodingLabel("latin3") == EncodingLabel.Iso8859_3);
    expect_true(lookupEncodingLabel("arabic") == EncodingLabel.Iso8859_6);
    expect_true(lookupEncodingLabel("greek8") == EncodingLabel.Iso8859_7);
    expect_true(lookupEncodingLabel("visual") == EncodingLabel.Iso8859_8);
    expect_true(lookupEncodingLabel("logical") == EncodingLabel.Iso8859_8I);
    expect_true(lookupEncodingLabel("tis-620") == EncodingLabel.Windows874);
    expect_true(lookupEncodingLabel("cp1253") == EncodingLabel.Windows1253);
    expect_true(lookupEncodingLabel("cp1255") == EncodingLabel.Windows1255);
    expect_true(lookupEncodingLabel("cp1257") == EncodingLabel.Windows1257);
    expect_true(lookupEncodingLabel("x-user-defined") == EncodingLabel.XUserDefined);
    expect_true(lookupEncodingLabel("replacement") == EncodingLabel.Replacement);
});

// ============================================================================
// Test 5: Classification predicates
// ============================================================================
test("encodingLabel: isUnicode, isCjk, isSingleByte & isAsciiCompatible predicates", () => {
    expect_true(EncodingLabel.Utf8.isUnicode());
    expect_true(EncodingLabel.Utf8.isAsciiCompatible());
    expect_true(!EncodingLabel.Utf8.isCjk());

    expect_true(EncodingLabel.Utf16Le.isUnicode());
    expect_true(!EncodingLabel.Utf16Le.isAsciiCompatible());

    expect_true(EncodingLabel.Gbk.isCjk());
    expect_true(EncodingLabel.Gbk.isAsciiCompatible());
    expect_true(!EncodingLabel.Iso2022Jp.isAsciiCompatible());

    expect_true(EncodingLabel.Windows1252.isSingleByte());
    expect_true(!EncodingLabel.Big5.isSingleByte());
    expect_true(!EncodingLabel.Replacement.isAsciiCompatible());
});

// ============================================================================
// Test 6: BOM detection & unknown / empty label rejection
// ============================================================================
test("encodingLabel: BOM detection and invalid label rejection", () => {
    const utf8_bom: [4]u8 = [0xEF, 0xBB, 0xBF, 'A'];
    const utf16be_bom: [4]u8 = [0xFE, 0xFF, 0x00, 0x41];
    const utf16le_bom: [4]u8 = [0xFF, 0xFE, 0x41, 0x00];
    const plain: [4]u8 = ['h', 'i', '!', '\n'];
    const too_short: [1]u8 = [0xEF];

    expect_true(detectBomEncoding(utf8_bom[0..]) == EncodingLabel.Utf8);
    expect_true(detectBomEncoding(utf16be_bom[0..]) == EncodingLabel.Utf16Be);
    expect_true(detectBomEncoding(utf16le_bom[0..]) == EncodingLabel.Utf16Le);
    expect_true(detectBomEncoding(plain[0..]) == null);
    expect_true(detectBomEncoding(too_short[0..]) == null);

    expect_true(lookupEncodingLabel("") == null);
    expect_true(lookupEncodingLabel("   \t ") == null);
    expect_true(lookupEncodingLabel("utf-32") == null);
    expect_true(lookupEncodingLabel("unknown-charset-9999") == null);
    expect_true(canonicalEncodingName("not-a-valid-encoding") == null);
});
