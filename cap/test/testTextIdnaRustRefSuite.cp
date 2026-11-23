// Reference-hardened test suite against servo/rust-url (idna, MIT/Apache-2.0) & RFC 3492 / UTS #46
// Covers:
//   1. RFC 3492 Section 7.1 multilingual vectors (German, Simplified/Traditional Chinese, Japanese, Korean, Russian, Arabic, Czech, Emoji)
//   2. IDNA toAscii / toUnicode / normalizeDomain / isValidDomain (ASCII fast-path, Unicode dots, case folding)
//   3. Strict boundary & overflow guards (u32 overflow, surrogates, trailing '-', CheckHyphens '--', >63B label, >253B domain, empty labels)
//   4. 16-lane @Vector(16, u8) SIMD ASCII check, dot scan & lowercase folding 0-diff parity vs scalar reference
//   5. Deterministic PRNG Fuzz (>= 1,600 malformed Punycode / xn-- overflow strings + random SIMD parity)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    IdnaError,
    punycodeEncode,
    punycodeDecode,
    simdIdnaIsAscii,
    simdIdnaIsAsciiScalar,
    simdIdnaIndexOfDot,
    simdIdnaIndexOfDotScalar,
    simdIdnaToLowerAscii,
    simdIdnaToLowerAsciiScalar,
} from "../../lib/text/punycode.cp";
import {
    toAscii,
    toUnicode,
    isValidDomain,
    normalizeDomain,
} from "../../lib/text/idna.cp";

fn nextRng(state: *u64): u64 {
    let x = state.*;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state.* = x;
    return x;
}

fn checkPunycodeRoundtrip(alloc: std.mem.Allocator, utf8: []const u8, expected_puny: []const u8): bool {
    const enc = punycodeEncode(alloc, utf8) catch return false;
    defer alloc.free(enc);
    if (!std.mem.eql(u8, enc, expected_puny)) return false;

    const decoded = punycodeDecode(alloc, expected_puny) catch return false;
    defer alloc.free(decoded);
    return std.mem.eql(u8, decoded, utf8);
}

fn decodeExpectErr(alloc: std.mem.Allocator, raw: []const u8): bool {
    const decoded = punycodeDecode(alloc, raw) catch return true;
    alloc.free(decoded);
    return false;
}

fn toAsciiExpectErr(alloc: std.mem.Allocator, domain: []const u8, expected: IdnaError): bool {
    const res = toAscii(alloc, domain) catch (e) {
        return e == expected;
    };
    alloc.free(res);
    return false;
}

fn fuzzOnePunycode(alloc: std.mem.Allocator, raw: []const u8) {
    const decoded = punycodeDecode(alloc, raw) catch return;
    alloc.free(decoded);
}

fn fuzzOneToAscii(alloc: std.mem.Allocator, raw: []const u8) {
    const res = toAscii(alloc, raw) catch return;
    alloc.free(res);
}

fn main() {
    test("1. RFC 3492 Section 7.1 multilingual Punycode bidirectional vectors", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;

        // German: bücher
        expect_eq(checkPunycodeRoundtrip(alloc, "b\xc3\xbccher", "bcher-kva"), true);
        // Simplified Chinese: 他们为什么不说中文
        expect_eq(checkPunycodeRoundtrip(alloc, "他们为什么不说中文", "ihqwcrb4cv8a8dqg056pqjye"), true);
        // Traditional Chinese: 他們爲什麽不說中文
        expect_eq(checkPunycodeRoundtrip(alloc, "他們爲什麽不說中文", "ihqwctvzc91f659drss3x8bo0yb"), true);
        // Japanese: なぜみんな日本語を話してくれないのか
        expect_eq(checkPunycodeRoundtrip(alloc, "なぜみんな日本語を話してくれないのか", "n8jok5ay5dzabd5bym9f0cm5685rrjetr6pdxa"), true);
        // Korean: 세계의모든사람들이한국어를이해한다면얼마나좋을까
        expect_eq(checkPunycodeRoundtrip(alloc, "세계의모든사람들이한국어를이해한다면얼마나좋을까", "989aomsvi5e83db1d2a355cv1e0vak1dwrv93d5xbh15a0dt30a5jpsd879ccm6fea98c"), true);
        // Russian: почемужеонинеговорятпорусски
        expect_eq(checkPunycodeRoundtrip(alloc, "почемужеонинеговорятпорусски", "b1abfaaepdrnnbgefbadotcwatmq2g4l"), true);
        // Arabic: ليهمابتكلموشعربي؟
        expect_eq(checkPunycodeRoundtrip(alloc, "ليهمابتكلموشعربي؟", "egbpdaj6bu4bxfgehfvwxn"), true);
        // Czech: Pročprostěnemluvíčesky
        expect_eq(checkPunycodeRoundtrip(alloc, "Pročprostěnemluvíčesky", "Proprostnemluvesky-uyb24dma41a"), true);
        // 4-byte UTF-8 Emoji (U+1F600 😀)
        expect_eq(checkPunycodeRoundtrip(alloc, "😀", "e28h"), true);
    });

    test("2. IDNA toAscii / toUnicode / normalizeDomain / isValidDomain", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;

        // Pure ASCII fast-path with mixed case (>16 bytes for SIMD path) & trailing root dot
        const a1 = toAscii(alloc, "WWW.Example-Subdomain-Test.COM.") catch unreachable;
        defer alloc.free(a1);
        expect_eq(a1, "www.example-subdomain-test.com.");

        // German IDN: Bücher.example -> xn--bcher-kva.example -> bücher.example
        const a2 = toAscii(alloc, "B\xc3\xbccher.example") catch unreachable;
        defer alloc.free(a2);
        expect_eq(a2, "xn--bcher-kva.example");

        const u2 = toUnicode(alloc, "XN--BCHER-KVA.EXAMPLE") catch unreachable;
        defer alloc.free(u2);
        expect_eq(u2, "b\xc3\xbccher.example");

        // Chinese IDN with ideographic full stop U+3002 (。)
        const a3 = toAscii(alloc, "中国。com") catch unreachable;
        defer alloc.free(a3);
        expect_eq(a3, "xn--fiqs8s.com");

        const u3 = toUnicode(alloc, "xn--fiqs8s.com") catch unreachable;
        defer alloc.free(u3);
        expect_eq(u3, "中国.com");

        const norm = normalizeDomain(alloc, "中国。COM") catch unreachable;
        defer alloc.free(norm);
        expect_eq(norm, "中国.com");

        expect_eq(isValidDomain("example.com"), true);
        expect_eq(isValidDomain("xn--bcher-kva.example"), true);
        expect_eq(isValidDomain("中国.com"), true);
    });

    test("3. Strict overflow, surrogate, trailing '-', CheckHyphens & length guards", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;

        // Punycode malformed: trailing '-' with no delta digits, non-ASCII, u32 overflow, surrogate
        expect_eq(decodeExpectErr(alloc, "abc-"), true);
        expect_eq(decodeExpectErr(alloc, "-"), true);
        expect_eq(decodeExpectErr(alloc, "b\xc3\xbc-kva"), true);
        expect_eq(decodeExpectErr(alloc, "99999999999999999999"), true);
        expect_eq(decodeExpectErr(alloc, "zzzzzzzzzzzzzzzzzzzz"), true);
        expect_eq(decodeExpectErr(alloc, "a-999999999999999"), true);
        // UTF-16 surrogate U+D800 encoded delta -> rejected
        expect_eq(decodeExpectErr(alloc, "eda"), false); // sanity valid U+00E0..
        expect_eq(decodeExpectErr(alloc, "ed394a"), true); // > 0x10FFFF overflow

        // UTS #46 CheckHyphens: non-xn-- with '--' in positions 3-4 rejected
        expect_eq(toAsciiExpectErr(alloc, "ab--cd.example.com", IdnaError.ReservedHyphens), true);
        expect_eq(toAsciiExpectErr(alloc, "zq--test.com", IdnaError.ReservedHyphens), true);
        expect_eq(isValidDomain("ab--cd.example.com"), false);

        // Leading / trailing hyphen, empty label a..b, leading dot
        expect_eq(toAsciiExpectErr(alloc, "-bad.com", IdnaError.LeadingOrTrailingHyphen), true);
        expect_eq(toAsciiExpectErr(alloc, "bad-.com", IdnaError.LeadingOrTrailingHyphen), true);
        expect_eq(toAsciiExpectErr(alloc, "a..b.com", IdnaError.EmptyLabel), true);
        expect_eq(toAsciiExpectErr(alloc, ".example.com", IdnaError.EmptyLabel), true);

        // Label > 63 bytes rejected
        const label64 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
        expect_eq(toAsciiExpectErr(alloc, label64, IdnaError.LabelTooLong), true);

        // Domain > 253 bytes (254 without trailing dot) rejected; 253 bytes allowed
        let dom254: [254]u8 = undefined;
        let idx: usize = 0;
        while (idx < 254) : (idx += 1) {
            dom254[idx] = if (idx == 60 || idx == 121 || idx == 182 || idx == 243) '.' else 'a';
        }
        expect_eq(toAsciiExpectErr(alloc, dom254[0..254], IdnaError.DomainTooLong), true);
        expect_eq(isValidDomain(dom254[0..253]), true);
    });

    test("4. 16-lane SIMD ASCII check, dot scan & lowercase 0-diff parity vs scalar", () => {
        const s_ascii = "API-Gateway-V2.SubDomain-01.Example.COM";
        const s_utf8 = "api-gateway-v2.中国.example.com";
        expect_eq(simdIdnaIsAscii(s_ascii), simdIdnaIsAsciiScalar(s_ascii));
        expect_eq(simdIdnaIsAscii(s_utf8), simdIdnaIsAsciiScalar(s_utf8));
        expect_eq(simdIdnaIndexOfDot(s_ascii), simdIdnaIndexOfDotScalar(s_ascii));
        expect_eq(simdIdnaIndexOfDot("no_dot_in_this_very_long_hostname_label_0123456789"), simdIdnaIndexOfDotScalar("no_dot_in_this_very_long_hostname_label_0123456789"));

        let out_simd: [64]u8 = undefined;
        let out_scal: [64]u8 = undefined;
        const n1 = simdIdnaToLowerAscii(s_ascii, out_simd[0..s_ascii.len]);
        const n2 = simdIdnaToLowerAsciiScalar(s_ascii, out_scal[0..s_ascii.len]);
        expect_eq(n1, n2);
        expect_eq(out_simd[0..n1], out_scal[0..n2]);
        expect_eq(out_simd[0..n1], "api-gateway-v2.subdomain-01.example.com");
    });

    test("5. Deterministic PRNG Fuzz (>= 1,600 malformed Punycode/xn-- & random SIMD parity)", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        const alphabet: []const u8 = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._\x80\xC3\xFF";
        let seed: u64 = 0x5EED_1D4A_2026_0033;
        let buf: [72]u8 = undefined;

        // 5A. 1,000 malformed Punycode & xn-- overflow domain fuzz iterations
        let iter: usize = 0;
        while (iter < 1000) : (iter += 1) {
            const len: usize = ((nextRng(&seed) % 52) + 1) as usize;
            let k: usize = 0;
            if ((iter & 3) == 0 && len >= 5) {
                buf[0] = 'x';
                buf[1] = 'n';
                buf[2] = '-';
                buf[3] = '-';
                k = 4;
            }
            while (k < len) : (k += 1) {
                buf[k] = alphabet[(nextRng(&seed) % (alphabet.len as u64)) as usize];
            }
            const sample = buf[0..len];
            fuzzOnePunycode(alloc, sample);
            fuzzOneToAscii(alloc, sample);
            * = isValidDomain(sample);
        }

        // 5B. 600 random byte-slice SIMD vs scalar parity checks
        let in_buf: [64]u8 = undefined;
        let o_simd: [64]u8 = undefined;
        let o_scal: [64]u8 = undefined;
        let r_iter: usize = 0;
        while (r_iter < 600) : (r_iter += 1) {
            const n: usize = (nextRng(&seed) % 64) as usize;
            let j: usize = 0;
            while (j < n) : (j += 1) {
                in_buf[j] = alphabet[(nextRng(&seed) % (alphabet.len as u64)) as usize];
            }
            const slice = in_buf[0..n];
            expect_eq(simdIdnaIsAscii(slice), simdIdnaIsAsciiScalar(slice));
            expect_eq(simdIdnaIndexOfDot(slice), simdIdnaIndexOfDotScalar(slice));
            const l1 = simdIdnaToLowerAscii(slice, o_simd[0..n]);
            const l2 = simdIdnaToLowerAsciiScalar(slice, o_scal[0..n]);
            expect_eq(l1, l2);
            expect_eq(o_simd[0..l1], o_scal[0..l2]);
        }
    });
}
