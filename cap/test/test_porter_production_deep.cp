// Source: cp/cap/test/test_porter_production_deep.cp (Production deep tests for cap:porter)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    PORTER_MAX_WORD_LEN,
    porterMeasure,
    porterStem,
    porterStemEquals,
} from "cap:porter";

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

fn expectStem(word: []const u8, expected: []const u8) {
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    const n = porterStem(word, buf[0..]) ?? unreachable;
    expect_eq(bytesEq(buf[0..n], expected), true);
}

test("1. Porter measure m ([C](VC)^m[V]) canonical paper vectors & contextual 'y'", () => {
    expect_eq(PORTER_MAX_WORD_LEN, 256 as usize);

    // m == 0: tr, ee, tree, y, by
    expect_eq(porterMeasure("tr"), 0 as usize);
    expect_eq(porterMeasure("ee"), 0 as usize);
    expect_eq(porterMeasure("tree"), 0 as usize);
    expect_eq(porterMeasure("y"), 0 as usize);
    expect_eq(porterMeasure("by"), 0 as usize);

    // m == 1: trouble, oats, trees, ivy
    expect_eq(porterMeasure("trouble"), 1 as usize);
    expect_eq(porterMeasure("oats"), 1 as usize);
    expect_eq(porterMeasure("trees"), 1 as usize);
    expect_eq(porterMeasure("ivy"), 1 as usize);

    // m == 2: troubles, private, oaten, orrery
    expect_eq(porterMeasure("troubles"), 2 as usize);
    expect_eq(porterMeasure("private"), 2 as usize);
    expect_eq(porterMeasure("oaten"), 2 as usize);
    expect_eq(porterMeasure("orrery"), 2 as usize);
});

test("2. Step 1a, Step 1b & Step 1c canonical Porter 1980 vectors", () => {
    // Step 1a
    expectStem("caresses", "caress");
    expectStem("ponies", "poni");
    expectStem("ties", "ti");
    expectStem("caress", "caress");
    expectStem("cats", "cat");

    // Step 1b (including Step 1b extra + Step 5a interaction)
    expectStem("feed", "feed");
    expectStem("agreed", "agre");
    expectStem("plastered", "plaster");
    expectStem("bled", "bled");
    expectStem("motoring", "motor");
    expectStem("sing", "sing");
    expectStem("conflated", "conflat");
    expectStem("troubled", "troubl");
    expectStem("sized", "size");
    expectStem("hopping", "hop");
    expectStem("tanned", "tan");
    expectStem("falling", "fall");
    expectStem("hissing", "hiss");
    expectStem("fizzed", "fizz");
    expectStem("failing", "fail");
    expectStem("filing", "file");

    // Step 1c
    expectStem("happy", "happi");
    expectStem("sky", "sky");
});

test("3. Step 2 (20 rules) & Step 3 (7 rules) canonical Porter 1980 vectors", () => {
    // Step 2
    expectStem("relational", "relat");
    expectStem("conditional", "condit");
    expectStem("rational", "ration");
    expectStem("valenci", "valenc");
    expectStem("hesitanci", "hesit");
    expectStem("digitizer", "digit");
    expectStem("conformabli", "conform");
    expectStem("radicalli", "radic");
    expectStem("differentli", "differ");
    expectStem("vileli", "vile");
    expectStem("analogousli", "analog");
    expectStem("vietnamization", "vietnam");
    expectStem("predication", "predic");
    expectStem("operator", "oper");
    expectStem("feudalism", "feudal");
    expectStem("decisiveness", "decis");
    expectStem("hopefulness", "hope");
    expectStem("callousness", "callous");
    expectStem("formaliti", "formal");
    expectStem("sensitiviti", "sensit");
    expectStem("sensibiliti", "sensibl");

    // Step 3
    expectStem("triplicate", "triplic");
    expectStem("formative", "form");
    expectStem("formalize", "formal");
    expectStem("electriciti", "electr");
    expectStem("electrical", "electr");
    expectStem("hopeful", "hope");
    expectStem("goodness", "good");
});

test("4. Step 4 (19 rules incl. *s/*t ion) & Step 5a / Step 5b canonical Porter 1980 vectors", () => {
    // Step 4
    expectStem("revival", "reviv");
    expectStem("allowance", "allow");
    expectStem("inference", "infer");
    expectStem("airliner", "airlin");
    expectStem("gyroscopic", "gyroscop");
    expectStem("adjustable", "adjust");
    expectStem("defensible", "defens");
    expectStem("irritant", "irrit");
    expectStem("replacement", "replac");
    expectStem("adjustment", "adjust");
    expectStem("dependent", "depend");
    expectStem("adoption", "adopt");
    expectStem("homologou", "homolog");
    expectStem("communism", "commun");
    expectStem("activate", "activ");
    expectStem("angulariti", "angular");
    expectStem("homologous", "homolog");
    expectStem("effective", "effect");
    expectStem("bowdlerize", "bowdler");

    // Step 5a & Step 5b
    expectStem("probate", "probat");
    expectStem("rate", "rate");
    expectStem("cease", "ceas");
    expectStem("controll", "control");
    expectStem("roll", "roll");
});

test("5. Case insensitivity, short words (<=2) & porterStemEquals word family equivalence", () => {
    expectStem("A", "a");
    expectStem("IS", "is");
    expectStem("To", "to");
    expectStem("CARESSES", "caress");
    expectStem("Relational", "relat");

    expect_eq(porterStemEquals("connect", "connected"), true);
    expect_eq(porterStemEquals("connecting", "connection"), true);
    expect_eq(porterStemEquals("connections", "CONNECT"), true);
    expect_eq(porterStemEquals("caresses", "CARESS"), true);
    expect_eq(porterStemEquals("happy", "sky"), false);
    expect_eq(porterStemEquals("123", "abc"), false);
});

test("6. Strict rejection of empty input, non-ASCII letters, oversized words & buffer overflows", () => {
    let buf: [32]u8 = std.mem.zeroes([32]u8);

    // Empty & non-letter characters -> null
    expect_eq(porterStem("", buf[0..]) == null, true);
    expect_eq(porterStem("hello123", buf[0..]) == null, true);
    expect_eq(porterStem("co-op", buf[0..]) == null, true);
    expect_eq(porterStem("hello world", buf[0..]) == null, true);
    expect_eq(porterStem("caf\xC3\xA9", buf[0..]) == null, true);

    // Oversized word (> 256 bytes) -> null
    let huge: [260]u8 = [0x61; 260];
    expect_eq(porterStem(huge[0..257], buf[0..]) == null, true);

    // Output buffer overflow: "caresses" -> "caress" (6 bytes)
    expect_eq(porterStem("caresses", buf[0..5]) == null, true);
    expect_eq(porterStem("caresses", buf[0..6]) ?? unreachable, 6 as usize);
});
