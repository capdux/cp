import std from "std";
import { test, expect_eq } from "cap:test";
import {
    CharsetKind,
    resolveCharsetLabel,
    canonicalCharsetName,
    isUtf8Compatible,
    isAsciiSuperset,
    sniffBomCharset,
} from "cap:charset_label";

test("1. resolveCharsetLabel maps WHATWG aliases and trims ASCII whitespace", () => {
    expect_eq((resolveCharsetLabel("  UTF8  ") ?? unreachable) == CharsetKind.utf8, true);
    expect_eq((resolveCharsetLabel("unicode-1-1-utf-8") ?? unreachable) == CharsetKind.utf8, true);
    expect_eq((resolveCharsetLabel("latin1") ?? unreachable) == CharsetKind.windows1252, true);
    expect_eq((resolveCharsetLabel("ISO-8859-1") ?? unreachable) == CharsetKind.windows1252, true);
    expect_eq((resolveCharsetLabel("us-ascii") ?? unreachable) == CharsetKind.windows1252, true);
    expect_eq((resolveCharsetLabel("cp1252") ?? unreachable) == CharsetKind.windows1252, true);
    expect_eq((resolveCharsetLabel("GB2312") ?? unreachable) == CharsetKind.gbk, true);
    expect_eq((resolveCharsetLabel("chinese") ?? unreachable) == CharsetKind.gbk, true);
    expect_eq((resolveCharsetLabel("sjis") ?? unreachable) == CharsetKind.shift_jis, true);
    expect_eq((resolveCharsetLabel("ms932") ?? unreachable) == CharsetKind.shift_jis, true);
    expect_eq((resolveCharsetLabel("ucs-2") ?? unreachable) == CharsetKind.utf16le, true);
    expect_eq((resolveCharsetLabel("UTF-16") ?? unreachable) == CharsetKind.utf16le, true);
    expect_eq((resolveCharsetLabel("UTF-16BE") ?? unreachable) == CharsetKind.utf16be, true);

    expect_eq(resolveCharsetLabel("") == null, true);
    expect_eq(resolveCharsetLabel("   ") == null, true);
    expect_eq(resolveCharsetLabel("unknown-charset-999") == null, true);
});

test("2. canonicalCharsetName round-trips all 14 CharsetKind variants", () => {
    const all: [14]CharsetKind = [
        CharsetKind.utf8,
        CharsetKind.utf16le,
        CharsetKind.utf16be,
        CharsetKind.windows1252,
        CharsetKind.iso8859_2,
        CharsetKind.gbk,
        CharsetKind.gb18030,
        CharsetKind.big5,
        CharsetKind.shift_jis,
        CharsetKind.euc_jp,
        CharsetKind.euc_kr,
        CharsetKind.koi8_r,
        CharsetKind.windows1251,
        CharsetKind.ibm866,
    ];
    for (k of all) {
        const name = canonicalCharsetName(k);
        expect_eq((resolveCharsetLabel(name) ?? unreachable) == k, true);
    }
    expect_eq(std.mem.eql(u8, canonicalCharsetName(CharsetKind.utf8), "utf-8"), true);
    expect_eq(std.mem.eql(u8, canonicalCharsetName(CharsetKind.windows1252), "windows-1252"), true);
    expect_eq(std.mem.eql(u8, canonicalCharsetName(CharsetKind.shift_jis), "shift_jis"), true);
});

test("3. isUtf8Compatible and isAsciiSuperset predicates", () => {
    expect_eq(isUtf8Compatible("UTF-8"), true);
    expect_eq(isUtf8Compatible("utf8"), true);
    expect_eq(isUtf8Compatible("US-ASCII"), true);
    expect_eq(isUtf8Compatible("ascii"), true);
    expect_eq(isUtf8Compatible("latin1"), false);
    expect_eq(isUtf8Compatible("gbk"), false);
    expect_eq(isUtf8Compatible("utf-16le"), false);

    expect_eq(isAsciiSuperset(CharsetKind.utf8), true);
    expect_eq(isAsciiSuperset(CharsetKind.gbk), true);
    expect_eq(isAsciiSuperset(CharsetKind.shift_jis), true);
    expect_eq(isAsciiSuperset(CharsetKind.utf16le), false);
    expect_eq(isAsciiSuperset(CharsetKind.utf16be), false);
});

test("4. sniffBomCharset detects UTF-8, UTF-16BE, and UTF-16LE BOMs", () => {
    const bom8: [5]u8 = [0xEF, 0xBB, 0xBF, 'h', 'i'];
    const bom16be: [4]u8 = [0xFE, 0xFF, 0x00, 'A'];
    const bom16le: [4]u8 = [0xFF, 0xFE, 'A', 0x00];
    const plain: [4]u8 = ['h', 'e', 'l', 'o'];
    const short_one: [1]u8 = [0xEF];

    expect_eq((sniffBomCharset(&bom8) ?? unreachable) == CharsetKind.utf8, true);
    expect_eq((sniffBomCharset(&bom16be) ?? unreachable) == CharsetKind.utf16be, true);
    expect_eq((sniffBomCharset(&bom16le) ?? unreachable) == CharsetKind.utf16le, true);
    expect_eq(sniffBomCharset(&plain) == null, true);
    expect_eq(sniffBomCharset(&short_one) == null, true);
});

test("5. Cyrillic, Korean, Japanese, and Traditional Chinese aliases", () => {
    expect_eq((resolveCharsetLabel("cp1251") ?? unreachable) == CharsetKind.windows1251, true);
    expect_eq((resolveCharsetLabel("koi8_r") ?? unreachable) == CharsetKind.koi8_r, true);
    expect_eq((resolveCharsetLabel("cp866") ?? unreachable) == CharsetKind.ibm866, true);
    expect_eq((resolveCharsetLabel("windows-949") ?? unreachable) == CharsetKind.euc_kr, true);
    expect_eq((resolveCharsetLabel("x-euc-jp") ?? unreachable) == CharsetKind.euc_jp, true);
    expect_eq((resolveCharsetLabel("big5-hkscs") ?? unreachable) == CharsetKind.big5, true);
    expect_eq((resolveCharsetLabel("latin2") ?? unreachable) == CharsetKind.iso8859_2, true);
});
