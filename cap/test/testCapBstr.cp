// cap/test/testCapBstr.cp
// Comprehensive 6-test suite for @cap/bstr (zero-allocation byte strings, Rust bstr v1 parity)

import { test, expect_eq } from "cap:test";
import {
    SliceRange,
    bstrFind,
    bstrRfind,
    bstrFindByte,
    bstrRfindByte,
    bstrContains,
    bstrStartsWith,
    bstrEndsWith,
    bstrTrimStart,
    bstrTrimEnd,
    bstrTrim,
    bstrLineCount,
    bstrLineAt,
    bstrReplaceAll,
    bstrIsUtf8,
    bstrToUtf8Lossy,
    bstrIsAscii,
    bstrToLowerAscii,
    bstrToUpperAscii,
    bstrEqIgnoreAsciiCase
} from "../../packages/@cap/bstr/src/index.cp";

fn eqBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/bstr — bstrFind/bstrRfind/bstrContains/bstrStartsWith/bstrEndsWith reusing @cap/memchr", () => {
        let s = "ripgrep/gitoxide/regex/ripgrep";
        expect_eq(bstrFind(s, "ripgrep"), 0 as i64);
        expect_eq(bstrRfind(s, "ripgrep"), 23 as i64);
        expect_eq(bstrFind(s, "gitoxide"), 8 as i64);
        expect_eq(bstrFind(s, "missing"), -1 as i64);
        expect_eq(bstrRfind(s, "missing"), -1 as i64);

        expect_eq(bstrFindByte(s, 0x2F), 7 as i64);
        expect_eq(bstrRfindByte(s, 0x2F), 22 as i64);

        expect_eq(bstrContains(s, "regex"), true);
        expect_eq(bstrContains(s, "perl"), false);
        expect_eq(bstrContains(s, ""), true);

        expect_eq(bstrStartsWith(s, "ripgrep/"), true);
        expect_eq(bstrStartsWith(s, "gitoxide"), false);
        expect_eq(bstrStartsWith(s, ""), true);
        expect_eq(bstrStartsWith("hi", "hello"), false);

        expect_eq(bstrEndsWith(s, "/ripgrep"), true);
        expect_eq(bstrEndsWith(s, "regex"), false);
        expect_eq(bstrEndsWith(s, ""), true);
        expect_eq(bstrEndsWith("hi", "hello"), false);
    });

    test("2. @cap/bstr — bstrTrim / bstrTrimStart / bstrTrimEnd zero-copy ASCII whitespace stripping", () => {
        let s = " \t\r\n  hello bstr \x0B\x0C\t\n ";
        let r = bstrTrim(s);
        expect_eq(r.start, 6 as usize);
        expect_eq(r.end, 16 as usize);
        expect_eq(r.len(), 10 as usize);
        expect_eq(r.isEmpty(), false);
        expect_eq(eqBytes(s[r.start..r.end], "hello bstr"), true);

        let rs = bstrTrimStart("   abc  ");
        expect_eq(eqBytes("   abc  "[rs.start..rs.end], "abc  "), true);

        let re = bstrTrimEnd("   abc  ");
        expect_eq(eqBytes("   abc  "[re.start..re.end], "   abc"), true);

        let allWs = bstrTrim(" \t\r\n ");
        expect_eq(allWs.isEmpty(), true);
        expect_eq(allWs.len(), 0 as usize);

        let empty = bstrTrim("");
        expect_eq(empty.isEmpty(), true);
    });

    test("3. @cap/bstr — bstrLineCount & bstrLineAt with LF and CRLF", () => {
        let lf = "alpha\nbeta\ngamma\n";
        expect_eq(bstrLineCount(lf), 3 as usize);
        let l0 = bstrLineAt(lf, 0);
        let l1 = bstrLineAt(lf, 1);
        let l2 = bstrLineAt(lf, 2);
        let l3 = bstrLineAt(lf, 3);
        expect_eq(eqBytes(lf[l0.start..l0.end], "alpha"), true);
        expect_eq(eqBytes(lf[l1.start..l1.end], "beta"), true);
        expect_eq(eqBytes(lf[l2.start..l2.end], "gamma"), true);
        expect_eq(l3.isEmpty(), true);

        let crlf = "first\r\n\r\nthird\r\nfourth";
        expect_eq(bstrLineCount(crlf), 4 as usize);
        let c0 = bstrLineAt(crlf, 0);
        let c1 = bstrLineAt(crlf, 1);
        let c2 = bstrLineAt(crlf, 2);
        let c3 = bstrLineAt(crlf, 3);
        expect_eq(eqBytes(crlf[c0.start..c0.end], "first"), true);
        expect_eq(c1.isEmpty(), true);
        expect_eq(eqBytes(crlf[c2.start..c2.end], "third"), true);
        expect_eq(eqBytes(crlf[c3.start..c3.end], "fourth"), true);

        expect_eq(bstrLineCount(""), 0 as usize);
        expect_eq(bstrLineCount("\r\n"), 1 as usize);
        let singleEmpty = bstrLineAt("\r\n", 0);
        expect_eq(singleEmpty.isEmpty(), true);
    });

    test("4. @cap/bstr — bstrReplaceAll multi-occurrence & empty-needle guard", () => {
        let out: [64]u8 = [0; 64];
        let src = "foo-bar-foo-baz-foo";
        let n1 = bstrReplaceAll(src, "foo", "QUX", out[0..64]);
        expect_eq(n1, 19 as usize);
        expect_eq(eqBytes(out[0..n1], "QUX-bar-QUX-baz-QUX"), true);

        // Expansion replacement
        let n2 = bstrReplaceAll("a,b,c", ",", ":::", out[0..64]);
        expect_eq(n2, 9 as usize);
        expect_eq(eqBytes(out[0..n2], "a:::b:::c"), true);

        // Deletion replacement (empty `to`)
        let n3 = bstrReplaceAll("xx1xx2xx", "xx", "", out[0..64]);
        expect_eq(n3, 2 as usize);
        expect_eq(eqBytes(out[0..n3], "12"), true);

        // Empty `from` guard: safely copies `src` unchanged without infinite loop
        let n4 = bstrReplaceAll("keep-me", "", "X", out[0..64]);
        expect_eq(n4, 7 as usize);
        expect_eq(eqBytes(out[0..n4], "keep-me"), true);
    });

    test("5. @cap/bstr — bstrToUtf8Lossy valid UTF-8 passthrough & invalid byte replacement (U+FFFD)", () => {
        let out: [64]u8 = [0; 64];

        // Valid UTF-8 passthrough: ASCII + 2-byte ('é' C3 A9) + 3-byte ('中' E4 B8 AD) + 4-byte (F0 9F 9A 80)
        let valid: [11]u8 = [
            0x41, 0xC3, 0xA9, 0xE4, 0xB8, 0xAD, 0xF0, 0x9F, 0x9A, 0x80, 0x21
        ];
        expect_eq(bstrIsUtf8(valid[0..11]), true);
        let nv = bstrToUtf8Lossy(valid[0..11], out[0..64]);
        expect_eq(nv, 11 as usize);
        expect_eq(eqBytes(out[0..nv], valid[0..11]), true);

        // Invalid 0xFF between 'a' and 'b' -> "a\u{FFFD}b"
        let bad1: [3]u8 = [0x61, 0xFF, 0x62];
        let exp1: [5]u8 = [0x61, 0xEF, 0xBF, 0xBD, 0x62];
        expect_eq(bstrIsUtf8(bad1[0..3]), false);
        let n1 = bstrToUtf8Lossy(bad1[0..3], out[0..64]);
        expect_eq(n1, 5 as usize);
        expect_eq(eqBytes(out[0..n1], exp1[0..5]), true);

        // Overlong NUL encoding 0xC0 0x80 -> two U+FFFD
        let overlong: [2]u8 = [0xC0, 0x80];
        let exp2: [6]u8 = [0xEF, 0xBF, 0xBD, 0xEF, 0xBF, 0xBD];
        expect_eq(bstrIsUtf8(overlong[0..2]), false);
        let n2 = bstrToUtf8Lossy(overlong[0..2], out[0..64]);
        expect_eq(n2, 6 as usize);
        expect_eq(eqBytes(out[0..n2], exp2[0..6]), true);

        // UTF-16 surrogate U+D800 (0xED 0xA0 0x80) -> three U+FFFD
        let surrogate: [3]u8 = [0xED, 0xA0, 0x80];
        expect_eq(bstrIsUtf8(surrogate[0..3]), false);
        let n3 = bstrToUtf8Lossy(surrogate[0..3], out[0..64]);
        expect_eq(n3, 9 as usize);
        expect_eq(bstrIsUtf8(out[0..n3]), true);

        // Truncated 3-byte sequence (0xE2 0x82 + '!') -> single U+FFFD + '!'
        let trunc: [3]u8 = [0xE2, 0x82, 0x21];
        let exp4: [4]u8 = [0xEF, 0xBF, 0xBD, 0x21];
        let n4 = bstrToUtf8Lossy(trunc[0..3], out[0..64]);
        expect_eq(n4, 4 as usize);
        expect_eq(eqBytes(out[0..n4], exp4[0..4]), true);
    });

    test("6. @cap/bstr — bstrIsAscii / bstrToLowerAscii / bstrToUpperAscii / bstrEqIgnoreAsciiCase", () => {
        let out: [32]u8 = [0; 32];
        let mixed = "Hello, Bstr-123!";
        expect_eq(bstrIsAscii(mixed), true);
        expect_eq(bstrIsAscii(""), true);

        let nonAscii: [3]u8 = [0x61, 0x80, 0x62];
        expect_eq(bstrIsAscii(nonAscii[0..3]), false);

        let nl = bstrToLowerAscii(mixed, out[0..32]);
        expect_eq(nl, 16 as usize);
        expect_eq(eqBytes(out[0..nl], "hello, bstr-123!"), true);

        let nu = bstrToUpperAscii(mixed, out[0..32]);
        expect_eq(nu, 16 as usize);
        expect_eq(eqBytes(out[0..nu], "HELLO, BSTR-123!"), true);

        // Non-ASCII bytes preserved verbatim during ASCII case conversion
        let mixedNonAscii: [4]u8 = [0x41, 0xC3, 0xA9, 0x7A];
        let nKeep = bstrToUpperAscii(mixedNonAscii[0..4], out[0..32]);
        expect_eq(nKeep, 4 as usize);
        expect_eq(out[0], 0x41 as u8);
        expect_eq(out[1], 0xC3 as u8);
        expect_eq(out[2], 0xA9 as u8);
        expect_eq(out[3], 0x5A as u8);

        expect_eq(bstrEqIgnoreAsciiCase("RipGrep", "ripgrep"), true);
        expect_eq(bstrEqIgnoreAsciiCase("RipGrep", "gitoxide"), false);
    });
}
