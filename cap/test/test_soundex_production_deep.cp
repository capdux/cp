// Source: cp/cap/test/test_soundex_production_deep.cp (Production deep tests for cap:soundex)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    SOUNDEX_LEN,
    MAX_PHONETIC_WORD_LEN,
    soundex,
    soundexEquals,
    soundexDifference,
    metaphone,
    nysiis,
    colognePhonetic,
} from "cap:soundex";

fn bytesEq(a: []const u8, b: []const u8): bool {
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

test("1. NARA American Soundex classic boundary vectors, equality & similarity score", () => {
    expect_eq(SOUNDEX_LEN, 4 as usize);
    expect_eq(MAX_PHONETIC_WORD_LEN, 256 as usize);

    const s_robert = soundex("Robert") ?? unreachable;
    const s_rupert = soundex("Rupert") ?? unreachable;
    const s_rubin = soundex("Rubin") ?? unreachable;
    const s_ashcraft = soundex("Ashcraft") ?? unreachable;
    const s_ashcroft = soundex("Ashcroft") ?? unreachable;
    const s_tymczak = soundex("Tymczak") ?? unreachable;
    const s_pfister = soundex("Pfister") ?? unreachable;
    const s_honeyman = soundex("Honeyman") ?? unreachable;

    expect_eq(bytesEq(s_robert[0..], "R163"), true);
    expect_eq(bytesEq(s_rupert[0..], "R163"), true);
    expect_eq(bytesEq(s_rubin[0..], "R150"), true);
    expect_eq(bytesEq(s_ashcraft[0..], "A261"), true);
    expect_eq(bytesEq(s_ashcroft[0..], "A261"), true);
    expect_eq(bytesEq(s_tymczak[0..], "T522"), true);
    expect_eq(bytesEq(s_pfister[0..], "P236"), true);
    expect_eq(bytesEq(s_honeyman[0..], "H555"), true);

    expect_eq(soundexEquals("Robert", "Rupert"), true);
    expect_eq(soundexEquals("Ashcraft", "Ashcroft"), true);
    expect_eq(soundexEquals("Robert", "Rubin"), false);

    expect_eq(soundexDifference("Robert", "Rupert"), 4 as u8);
    expect_eq(soundexDifference("Robert", "Rubin"), 2 as u8);
    expect_eq(soundexDifference("Ashcraft", "Tymczak"), 0 as u8);
});

test("2. Metaphone (1990) classic phonetic pairs & English pronunciation rules", () => {
    let a: [32]u8 = std.mem.zeroes([32]u8);
    let b: [32]u8 = std.mem.zeroes([32]u8);

    // Knight == Night -> "NT"
    const na = metaphone("Knight", a[0..]) ?? unreachable;
    const nb = metaphone("Night", b[0..]) ?? unreachable;
    expect_eq(bytesEq(a[0..na], "NT"), true);
    expect_eq(bytesEq(b[0..nb], "NT"), true);

    // Phone == Fone -> "FN"
    const pa = metaphone("Phone", a[0..]) ?? unreachable;
    const pb = metaphone("Fone", b[0..]) ?? unreachable;
    expect_eq(bytesEq(a[0..pa], "FN"), true);
    expect_eq(bytesEq(b[0..pb], "FN"), true);

    // School -> "SKL", Smith -> "SM0", Dumb -> "TM", Box -> "BKS", Watch -> "WX"
    const n_school = metaphone("School", a[0..]) ?? unreachable;
    expect_eq(bytesEq(a[0..n_school], "SKL"), true);

    const n_smith = metaphone("Smith", a[0..]) ?? unreachable;
    expect_eq(bytesEq(a[0..n_smith], "SM0"), true);

    const n_dumb = metaphone("Dumb", a[0..]) ?? unreachable;
    expect_eq(bytesEq(a[0..n_dumb], "TM"), true);

    const n_box = metaphone("Box", a[0..]) ?? unreachable;
    expect_eq(bytesEq(a[0..n_box], "BKS"), true);

    const n_watch = metaphone("Watch", a[0..]) ?? unreachable;
    expect_eq(bytesEq(a[0..n_watch], "WX"), true);

    const n_white = metaphone("White", a[0..]) ?? unreachable;
    expect_eq(bytesEq(a[0..n_white], "WT"), true);
});

test("3. NYSIIS classic surname pairs, prefix/suffix transforms & deduplication", () => {
    let a: [32]u8 = std.mem.zeroes([32]u8);
    let b: [32]u8 = std.mem.zeroes([32]u8);

    // MacIntosh == McIntosh -> "MCANTA"
    const ma = nysiis("MacIntosh", a[0..]) ?? unreachable;
    const mb = nysiis("McIntosh", b[0..]) ?? unreachable;
    expect_eq(bytesEq(a[0..ma], "MCANTA"), true);
    expect_eq(bytesEq(b[0..mb], "MCANTA"), true);

    // Knight == Night -> "NAGT"
    const ka = nysiis("Knight", a[0..]) ?? unreachable;
    const kb = nysiis("Night", b[0..]) ?? unreachable;
    expect_eq(bytesEq(a[0..ka], "NAGT"), true);
    expect_eq(bytesEq(b[0..kb], "NAGT"), true);

    // Bishop -> "BASAP", Schmidt -> "SNAD", Philip -> "FALAP"
    const n_bishop = nysiis("Bishop", a[0..]) ?? unreachable;
    expect_eq(bytesEq(a[0..n_bishop], "BASAP"), true);

    const n_schmidt = nysiis("Schmidt", a[0..]) ?? unreachable;
    expect_eq(bytesEq(a[0..n_schmidt], "SNAD"), true);

    const n_philip = nysiis("Philip", a[0..]) ?? unreachable;
    expect_eq(bytesEq(a[0..n_philip], "FALAP"), true);
});

test("4. Cologne Phonetics (Koelner Phonetik) standard vectors & German surname equivalence", () => {
    let buf: [32]u8 = std.mem.zeroes([32]u8);

    const n_wiki = colognePhonetic("Wikipedia", buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..n_wiki], "3412"), true);

    const n_bresch = colognePhonetic("Breschnew", buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..n_bresch], "17863"), true);

    const n_mueller = colognePhonetic("Mueller", buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..n_mueller], "657"), true);

    // Meyer == Meier == Mayr -> "67"
    const n_meyer = colognePhonetic("Meyer", buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..n_meyer], "67"), true);
    const n_meier = colognePhonetic("Meier", buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..n_meier], "67"), true);
    const n_mayr = colognePhonetic("Mayr", buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..n_mayr], "67"), true);

    // Leading vowel '0' preserved: Aachen -> "046", Heinz -> "068"
    const n_aachen = colognePhonetic("Aachen", buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..n_aachen], "046"), true);
    const n_heinz = colognePhonetic("Heinz", buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..n_heinz], "068"), true);
});

test("5. Empty inputs, digits/punctuation-only rejection & mixed non-letter stripping", () => {
    let buf: [32]u8 = std.mem.zeroes([32]u8);

    // Empty & non-letter inputs -> null / false / 0
    expect_eq(soundex("") == null, true);
    expect_eq(soundex("12345-!@#") == null, true);
    expect_eq(soundexEquals("", ""), false);
    expect_eq(soundexEquals("123", "Robert"), false);
    expect_eq(soundexDifference("123", "Robert"), 0 as u8);

    expect_eq(metaphone("", buf[0..]) == null, true);
    expect_eq(metaphone("98765!", buf[0..]) == null, true);
    expect_eq(nysiis("", buf[0..]) == null, true);
    expect_eq(nysiis("---", buf[0..]) == null, true);
    expect_eq(colognePhonetic("", buf[0..]) == null, true);
    expect_eq(colognePhonetic("42.0", buf[0..]) == null, true);

    // Punctuation/apostrophes/spaces inside names are cleanly ignored
    expect_eq(soundexEquals("O'Brien", "OBrien"), true);
    const n_ob = colognePhonetic("W-i-k-i-p-e-d-i-a!", buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..n_ob], "3412"), true);
});

test("6. Strict zero-alloc output buffer overflow guards across Metaphone, NYSIIS & Cologne", () => {
    let buf: [16]u8 = std.mem.zeroes([16]u8);

    // Metaphone "School" -> "SKL" (3 bytes)
    expect_eq(metaphone("School", buf[0..2]) == null, true);
    expect_eq(metaphone("School", buf[0..3]) ?? unreachable, 3 as usize);

    // NYSIIS "MacIntosh" -> "MCANTA" (6 bytes)
    expect_eq(nysiis("MacIntosh", buf[0..5]) == null, true);
    expect_eq(nysiis("MacIntosh", buf[0..6]) ?? unreachable, 6 as usize);

    // Cologne "Breschnew" -> "17863" (5 bytes)
    expect_eq(colognePhonetic("Breschnew", buf[0..4]) == null, true);
    expect_eq(colognePhonetic("Breschnew", buf[0..5]) ?? unreachable, 5 as usize);
});
