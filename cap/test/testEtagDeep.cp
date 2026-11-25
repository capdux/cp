// cap/test/testEtagDeep.cp
// Deep unit tests for cap:etag (Rust `etag` crate parity + RFC 9110 §8.8.3 / §13.1 / §13.2.2).

import { test, expect_eq } from "cap:test";
import {
    EntityTag,
    ParsedETag,
    PreconditionResult,
    ETagListIter,
    isValidEtagChar,
    isValidOpaqueTag,
    isWildcardHeader,
    parseHeaderList,
    ifNoneMatchTag,
    ifMatchTag,
    ifNoneMatch,
    ifMatch,
    ifRangeMatch,
    evaluatePreconditions,
    generateStrong,
    generateWeak,
    generateContentStrong,
    generateContentWeak,
    generateSha256Strong,
    generateSha256Weak,
    fromStat,
    fromStatStrong,
} from "cap:etag";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn main() {
    test("1. cap:etag — RFC 9110 §8.8.3 etagc validation & EntityTag constructors", () => {
        // Boundary checks for etagc = %x21 / %x23-7E / %x80-FF
        expect_eq(isValidEtagChar(0x00), false);
        expect_eq(isValidEtagChar(0x09), false); // HTAB
        expect_eq(isValidEtagChar(0x0A), false); // LF
        expect_eq(isValidEtagChar(0x0D), false); // CR
        expect_eq(isValidEtagChar(0x1F), false);
        expect_eq(isValidEtagChar(0x20), false); // SP
        expect_eq(isValidEtagChar(0x21), true);  // '!'
        expect_eq(isValidEtagChar(0x22), false); // '"'
        expect_eq(isValidEtagChar(0x23), true);  // '#'
        expect_eq(isValidEtagChar(0x7E), true);  // '~'
        expect_eq(isValidEtagChar(0x7F), false); // DEL
        expect_eq(isValidEtagChar(0x80), true);  // obs-text start
        expect_eq(isValidEtagChar(0xFF), true);  // obs-text end

        expect_eq(isValidOpaqueTag(""), true);
        expect_eq(isValidOpaqueTag("!#v1.2.3~xyz"), true);
        expect_eq(isValidOpaqueTag("bad space"), false);
        expect_eq(isValidOpaqueTag("bad\"quote"), false);

        const s = EntityTag.strong("v1-abc").?;
        expect_eq(s.isStrong(), true);
        expect_eq(s.isWeak(), false);
        expect_eq(bytesEq(s.opaqueTag(), "v1-abc"), true);
        expect_eq(s.formattedLen(), 8 as usize);

        const w = EntityTag.weak("v1-abc").?;
        expect_eq(w.isWeak(), true);
        expect_eq(w.isStrong(), false);
        expect_eq(w.formattedLen(), 10 as usize);

        // Invalid opaque-tag rejected by constructors
        expect_eq(EntityTag.strong("has space") == null, true);
        expect_eq(EntityTag.weak("has\"quote") == null, true);

        // Empty opaque-tag is valid per RFC 9110 (*etagc)
        const empty_s = EntityTag.strong("").?;
        const empty_w = EntityTag.weak("").?;
        let buf: [16]u8 = [0; 16];
        expect_eq(bytesEq(empty_s.format(buf[0..]).?, "\"\""), true);
        expect_eq(bytesEq(empty_w.format(buf[0..]).?, "W/\"\""), true);

        // Conversions and buffer-too-small guard
        expect_eq(s.toWeak().isWeak(), true);
        expect_eq(w.toStrong().isStrong(), true);
        let tiny: [5]u8 = [0; 5];
        expect_eq(s.format(tiny[0..]) == null, true);
        let exact: [8]u8 = [0; 8];
        expect_eq(bytesEq(s.format(exact[0..]).?, "\"v1-abc\""), true);
    });

    test("2. cap:etag — parseStrict vs parse & RFC 9110 §8.8.3.2 comparison matrix", () => {
        const ps1 = EntityTag.parseStrict("\"xyzzy\"").?;
        expect_eq(ps1.isStrong(), true);
        expect_eq(bytesEq(ps1.opaqueTag(), "xyzzy"), true);

        const pw1 = EntityTag.parseStrict("W/\"xyzzy\"").?;
        expect_eq(pw1.isWeak(), true);
        expect_eq(bytesEq(pw1.opaqueTag(), "xyzzy"), true);

        expect_eq(EntityTag.parseStrict("\"\"") != null, true);
        expect_eq(EntityTag.parseStrict("W/\"\"") != null, true);

        // Strict rejects OWS anywhere (leading, trailing, or between W/ and ")
        expect_eq(EntityTag.parseStrict(" \"xyzzy\"") == null, true);
        expect_eq(EntityTag.parseStrict("\"xyzzy\" ") == null, true);
        expect_eq(EntityTag.parseStrict("W/ \"xyzzy\"") == null, true);
        expect_eq(EntityTag.parseStrict("w/\"xyzzy\"") == null, true);
        expect_eq(EntityTag.parseStrict("\"a\"b\"") == null, true);
        expect_eq(EntityTag.parseStrict("\"a b\"") == null, true);
        expect_eq(EntityTag.parseStrict("xyzzy") == null, true);

        // Lenient parse trims OWS
        expect_eq(EntityTag.parse("  \"xyzzy\"\t") != null, true);
        expect_eq(EntityTag.parse(" W/ \"xyzzy\" ") != null, true);
        expect_eq(EntityTag.parse("w/\"xyzzy\"") == null, true);

        // obs-text bytes (0x80..0xFF) allowed inside opaque-tag
        const obs_raw: [5]u8 = ['"', 0x80, 'a', 0xFF, '"'];
        const obs_tag = EntityTag.parseStrict(obs_raw[0..]).?;
        expect_eq(obs_tag.opaqueTag().len, 3 as usize);

        // RFC 9110 §8.8.3.2 comparison truth table
        const s_a = EntityTag.parseStrict("\"1\"").?;
        const s_a2 = EntityTag.parseStrict("\"1\"").?;
        const s_b = EntityTag.parseStrict("\"2\"").?;
        const w_a = EntityTag.parseStrict("W/\"1\"").?;
        const w_a2 = EntityTag.parseStrict("W/\"1\"").?;

        // Strong comparison
        expect_eq(s_a.strongEq(s_a2), true);
        expect_eq(s_a.strongEq(s_b), false);
        expect_eq(s_a.strongEq(w_a), false);
        expect_eq(w_a.strongEq(s_a), false);
        expect_eq(w_a.strongEq(w_a2), false);

        // Weak comparison
        expect_eq(s_a.weakEq(s_a2), true);
        expect_eq(s_a.weakEq(s_b), false);
        expect_eq(s_a.weakEq(w_a), true);
        expect_eq(w_a.weakEq(s_a), true);
        expect_eq(w_a.weakEq(w_a2), true);

        // Backward-compatible ParsedETag alias & legacy methods
        let buf: [32]u8 = [0; 32];
        const compat_s = ParsedETag.parse("\"abc123\"").?;
        const compat_w = ParsedETag.parse("W/\"abc123\"").?;
        expect_eq(!compat_s.is_weak && compat_w.is_weak, true);
        expect_eq(bytesEq(compat_s.tag, "abc123"), true);
        expect_eq(compat_s.weakMatch(compat_w), true);
        expect_eq(compat_s.strongMatch(compat_w), false);
        expect_eq(compat_s.strongMatch(compat_s), true);
        expect_eq(bytesEq(compat_w.toHeader(buf[0..]).?, "W/\"abc123\""), true);
    });

    test("3. cap:etag — Generators (FNV-1a, 128-bit+len, SHA-256 FIPS vectors, fromStat)", () => {
        let b1: [80]u8 = [0; 80];
        let b2: [80]u8 = [0; 80];

        // 1. FNV-1a 64-bit
        const f_s = generateStrong("hello world", b1[0..]).?;
        const f_w = generateWeak("hello world", b2[0..]).?;
        expect_eq(f_s.len, 18 as usize);
        expect_eq(f_w.len, 20 as usize);
        expect_eq(EntityTag.parseStrict(f_s).?.isStrong(), true);
        expect_eq(EntityTag.parseStrict(f_w).?.isWeak(), true);
        expect_eq(EntityTag.parseStrict(f_s).?.weakEq(EntityTag.parseStrict(f_w).?), true);
        expect_eq(ifNoneMatch(f_s, f_w), true);
        expect_eq(ifMatch(f_s, f_w), false);
        expect_eq(ifMatch(f_s, f_s), true);

        let small17: [17]u8 = [0; 17];
        expect_eq(generateStrong("hello", small17[0..]) == null, true);
        let small19: [19]u8 = [0; 19];
        expect_eq(generateWeak("hello", small19[0..]) == null, true);

        // 2. 128-bit dual hash + length encoding
        const c_empty = generateContentStrong("", b1[0..]).?;
        expect_eq(c_empty.len, 36 as usize);
        expect_eq(EntityTag.parseStrict(c_empty).?.isStrong(), true);

        const c_s = generateContentStrong("payload-data-1234", b1[0..]).?;
        const c_w = generateContentWeak("payload-data-1234", b2[0..]).?;
        expect_eq(EntityTag.parseStrict(c_s).?.weakEq(EntityTag.parseStrict(c_w).?), true);
        expect_eq(EntityTag.parseStrict(c_s).?.strongEq(EntityTag.parseStrict(c_w).?), false);

        let b3: [80]u8 = [0; 80];
        const c_other = generateContentStrong("payload-data-1235", b3[0..]).?;
        expect_eq(bytesEq(c_s, c_other), false);

        // 3. SHA-256 FIPS 180-4 known test vectors
        const sha_empty = generateSha256Strong("", b1[0..]).?;
        expect_eq(
            bytesEq(
                sha_empty,
                "\"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855\"",
            ),
            true,
        );

        const sha_abc = generateSha256Strong("abc", b1[0..]).?;
        expect_eq(
            bytesEq(
                sha_abc,
                "\"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad\"",
            ),
            true,
        );

        const sha_abc_w = generateSha256Weak("abc", b2[0..]).?;
        expect_eq(
            bytesEq(
                sha_abc_w,
                "W/\"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad\"",
            ),
            true,
        );

        // Multi-block SHA-256 (56 bytes triggers second padding block)
        const sha_long = generateSha256Strong(
            "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
            b1[0..],
        ).?;
        expect_eq(
            bytesEq(
                sha_long,
                "\"248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1\"",
            ),
            true,
        );

        let small65: [65]u8 = [0; 65];
        expect_eq(generateSha256Strong("abc", small65[0..]) == null, true);
        let small67: [67]u8 = [0; 67];
        expect_eq(generateSha256Weak("abc", small67[0..]) == null, true);

        // 4. File metadata fromStat / fromStatStrong
        const st_zero = fromStat(0, 0, b1[0..]).?;
        expect_eq(bytesEq(st_zero, "W/\"0-0\""), true);
        expect_eq(EntityTag.parseStrict(st_zero).?.isWeak(), true);

        const st_pos = fromStat(4096, 0x1a2b3c4d, b1[0..]).?;
        expect_eq(bytesEq(st_pos, "W/\"1000-1a2b3c4d\""), true);
        expect_eq(EntityTag.parseStrict(st_pos).?.isWeak(), true);

        const st_strong = fromStatStrong(4096, 0x1a2b3c4d, b2[0..]).?;
        expect_eq(bytesEq(st_strong, "\"1000-1a2b3c4d\""), true);
        expect_eq(EntityTag.parseStrict(st_strong).?.isStrong(), true);

        const st_neg = fromStat(16, -255, b1[0..]).?;
        expect_eq(bytesEq(st_neg, "W/\"10--ff\""), true);
        expect_eq(EntityTag.parseStrict(st_neg).?.isWeak(), true);

        let tiny4: [4]u8 = [0; 4];
        expect_eq(fromStat(1024, 123456, tiny4[0..]) == null, true);
    });

    test("4. cap:etag — Header list parsing, quoted commas, wildcards, & If-Range", () => {
        expect_eq(isWildcardHeader("*"), true);
        expect_eq(isWildcardHeader("  *\t "), true);
        expect_eq(isWildcardHeader("\"*\""), false);
        expect_eq(isWildcardHeader("*, \"a\""), false);

        // ETagListIter with quoted commas, OWS, empty elements, and malformed entries
        let iter = ETagListIter.init(" , \"a,b\" , invalid_tok , W/\"c,d,e\" , , \"z\" ");
        expect_eq(iter.isWildcard(), false);
        const t1 = iter.next().?;
        expect_eq(t1.isStrong() && bytesEq(t1.opaqueTag(), "a,b"), true);
        const t2 = iter.next().?;
        expect_eq(t2.isWeak() && bytesEq(t2.opaqueTag(), "c,d,e"), true);
        const t3 = iter.next().?;
        expect_eq(t3.isStrong() && bytesEq(t3.opaqueTag(), "z"), true);
        expect_eq(iter.next() == null, true);

        // parseHeaderList into fixed buffer
        let tags: [2]EntityTag = [
            EntityTag{ tag: "", is_weak: false },
            EntityTag{ tag: "", is_weak: false },
        ];
        const n = parseHeaderList("\"one\", W/\"two,2\", \"three\"", tags[0..]);
        expect_eq(n, 2 as usize);
        expect_eq(bytesEq(tags[0].opaqueTag(), "one"), true);
        expect_eq(bytesEq(tags[1].opaqueTag(), "two,2") && tags[1].isWeak(), true);

        // parseHeaderList on wildcard returns 0
        expect_eq(parseHeaderList(" * ", tags[0..]), 0 as usize);

        // ifMatchTag / ifNoneMatchTag + quoted comma protection
        const cur = EntityTag.parseStrict("\"a,b\"").?;
        expect_eq(ifNoneMatchTag(cur, "\"x\", W/\"a,b\", \"y\""), true);
        expect_eq(ifMatchTag(cur, "\"x\", W/\"a,b\", \"y\""), false);
        expect_eq(ifMatchTag(cur, "\"x\", \"a,b\", \"y\""), true);
        expect_eq(ifMatchTag(cur, " * "), true);

        expect_eq(ifNoneMatch("\"a,b\"", "\"x\", W/\"a,b\", \"z\""), true);
        expect_eq(ifMatch("\"a,b\"", "\"x\", W/\"a,b\", \"z\""), false);
        expect_eq(ifMatch("\"a,b\"", "\"x\", \"a,b\", \"z\""), true);
        expect_eq(ifNoneMatch("\"anything\"", " * "), true);
        expect_eq(ifNoneMatch("\"abc\"", "\"def\", \"ghi\""), false);

        // If-Range: strong comparison, single tag only, no wildcard
        expect_eq(ifRangeMatch("\"abc\"", "\"abc\""), true);
        expect_eq(ifRangeMatch("W/\"abc\"", "\"abc\""), false);
        expect_eq(ifRangeMatch("\"abc\"", "W/\"abc\""), false);
        expect_eq(ifRangeMatch("\"abc\"", "*"), false);
        expect_eq(ifRangeMatch("\"abc\"", "\"abc\", \"def\""), false);
    });

    test("5. cap:etag — RFC 9110 §13.2.2 evaluatePreconditions decision machine", () => {
        // 1. No preconditions -> Proceed
        expect_eq(evaluatePreconditions("GET", "\"v1\"", null, null) == PreconditionResult.Proceed, true);
        expect_eq(evaluatePreconditions("PUT", null, null, null) == PreconditionResult.Proceed, true);

        // 2. If-None-Match on GET / HEAD -> 304 Not Modified on match, Proceed on miss
        expect_eq(evaluatePreconditions("GET", "\"v1\"", null, "\"v1\"") == PreconditionResult.NotModified304, true);
        expect_eq(evaluatePreconditions("HEAD", "\"v1\"", null, "W/\"v1\"") == PreconditionResult.NotModified304, true);
        expect_eq(evaluatePreconditions("GET", "W/\"v1\"", null, "\"v0\", \"v1\"") == PreconditionResult.NotModified304, true);
        expect_eq(evaluatePreconditions("GET", "\"v1\"", null, "*") == PreconditionResult.NotModified304, true);
        expect_eq(evaluatePreconditions("GET", "\"v1\"", null, "\"v2\"") == PreconditionResult.Proceed, true);
        expect_eq(evaluatePreconditions("GET", null, null, "*") == PreconditionResult.Proceed, true);

        // 3. If-None-Match on unsafe methods (PUT / POST / DELETE / PATCH) -> 412 on match
        expect_eq(evaluatePreconditions("PUT", "\"v1\"", null, "\"v1\"") == PreconditionResult.PreconditionFailed412, true);
        expect_eq(evaluatePreconditions("DELETE", "\"v1\"", null, "W/\"v1\"") == PreconditionResult.PreconditionFailed412, true);
        expect_eq(evaluatePreconditions("POST", "\"v1\"", null, "*") == PreconditionResult.PreconditionFailed412, true);

        // Create-if-not-exists pattern: PUT with If-None-Match: *
        expect_eq(evaluatePreconditions("PUT", null, null, "*") == PreconditionResult.Proceed, true);
        expect_eq(evaluatePreconditions("PUT", "\"existing\"", null, "*") == PreconditionResult.PreconditionFailed412, true);

        // 4. If-Match (strong comparison) -> 412 on failure, Proceed on match
        expect_eq(evaluatePreconditions("PUT", "\"v1\"", "\"v1\"", null) == PreconditionResult.Proceed, true);
        expect_eq(evaluatePreconditions("PUT", "\"v1\"", "\"v0\", \"v1\"", null) == PreconditionResult.Proceed, true);
        expect_eq(evaluatePreconditions("PUT", "\"v1\"", "*", null) == PreconditionResult.Proceed, true);
        expect_eq(evaluatePreconditions("PUT", "\"v1\"", "W/\"v1\"", null) == PreconditionResult.PreconditionFailed412, true);
        expect_eq(evaluatePreconditions("PUT", "W/\"v1\"", "\"v1\"", null) == PreconditionResult.PreconditionFailed412, true);
        expect_eq(evaluatePreconditions("PUT", "\"v1\"", "\"v2\"", null) == PreconditionResult.PreconditionFailed412, true);
        expect_eq(evaluatePreconditions("PUT", null, "*", null) == PreconditionResult.PreconditionFailed412, true);
        expect_eq(evaluatePreconditions("PUT", null, "\"v1\"", null) == PreconditionResult.PreconditionFailed412, true);

        // 5. Precedence (RFC 9110 §13.2.2): Step 1 (If-Match) is evaluated BEFORE Step 3 (If-None-Match)
        expect_eq(evaluatePreconditions("GET", "\"v1\"", "\"stale\"", "\"v1\"") == PreconditionResult.PreconditionFailed412, true);
        expect_eq(evaluatePreconditions("GET", "\"v1\"", "\"v1\"", "\"v1\"") == PreconditionResult.NotModified304, true);
        expect_eq(evaluatePreconditions("GET", "\"v1\"", "\"v1\"", "\"v0\"") == PreconditionResult.Proceed, true);
    });
}
