// cap/test/testCapPercentEncoding.cp
// Comprehensive unit tests for @cap/percent-encoding (WHATWG URL Standard)

import { test, expect_eq } from "cap:test";
import {
    AsciiSet,
    controlsSet,
    fragmentSet,
    querySet,
    pathSet,
    userinfoSet,
    componentSet,
    nonAlphanumericSet,
    percentEncodeLen,
    percentEncode,
    percentDecode,
    percentDecodeStrict,
    percentDecodePlus
} from "../../packages/percent-encoding/src/index.cp";

fn peTestSliceEq(a: []const u8, b: []const u8): bool {
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
    test("1. @cap/percent-encoding — AsciiSet bitmap add/remove/unionWith & WHATWG set membership", () => {
        let s = AsciiSet.empty();
        expect_eq(s.shouldPercentEncode(0x41), false); // 'A'
        expect_eq(s.shouldPercentEncode(0x80), true);  // non-ASCII always encoded
        expect_eq(s.shouldPercentEncode(0xFF), true);

        s.add(0x20); // ' '
        s.add(0x3F); // '?'
        expect_eq(s.shouldPercentEncode(0x20), true);
        expect_eq(s.shouldPercentEncode(0x3F), true);
        expect_eq(s.shouldPercentEncode(0x21), false);

        s.remove(0x20);
        expect_eq(s.shouldPercentEncode(0x20), false);
        expect_eq(s.shouldPercentEncode(0x3F), true);

        let s2 = AsciiSet.empty();
        s2.add(0x23); // '#'
        s2.add(0x25); // '%'
        let u = s.unionWith(s2);
        expect_eq(u.shouldPercentEncode(0x3F), true);
        expect_eq(u.shouldPercentEncode(0x23), true);
        expect_eq(u.shouldPercentEncode(0x25), true);
        expect_eq(u.shouldPercentEncode(0x20), false);

        // Verify controlsSet: 0x00..0x1F + 0x7F + >= 0x80
        let ctrl = controlsSet();
        expect_eq(ctrl.shouldPercentEncode(0x00), true);
        expect_eq(ctrl.shouldPercentEncode(0x1F), true);
        expect_eq(ctrl.shouldPercentEncode(0x20), false);
        expect_eq(ctrl.shouldPercentEncode(0x7E), false);
        expect_eq(ctrl.shouldPercentEncode(0x7F), true);
        expect_eq(ctrl.shouldPercentEncode(0x80), true);

        // Verify exact WHATWG set member differences
        let frag = fragmentSet();
        let qry = querySet();
        let pth = pathSet();
        let usr = userinfoSet();
        let cmp = componentSet();

        expect_eq(frag.shouldPercentEncode(0x60), true);  // '`' in fragment
        expect_eq(frag.shouldPercentEncode(0x23), false); // '#' not in fragment
        expect_eq(qry.shouldPercentEncode(0x23), true);   // '#' in query
        expect_eq(qry.shouldPercentEncode(0x60), false);  // '`' not in query
        expect_eq(pth.shouldPercentEncode(0x3F), true);   // '?' in path
        expect_eq(pth.shouldPercentEncode(0x7B), true);   // '{' in path
        expect_eq(pth.shouldPercentEncode(0x7D), true);   // '}' in path
        expect_eq(pth.shouldPercentEncode(0x2F), false);  // '/' not in path
        expect_eq(usr.shouldPercentEncode(0x2F), true);   // '/' in userinfo
        expect_eq(usr.shouldPercentEncode(0x40), true);   // '@' in userinfo
        expect_eq(usr.shouldPercentEncode(0x5C), true);   // '\' in userinfo
        expect_eq(usr.shouldPercentEncode(0x7C), true);   // '|' in userinfo
        expect_eq(usr.shouldPercentEncode(0x26), false);  // '&' not in userinfo
        expect_eq(cmp.shouldPercentEncode(0x24), true);   // '$' in component
        expect_eq(cmp.shouldPercentEncode(0x25), true);   // '%' in component
        expect_eq(cmp.shouldPercentEncode(0x26), true);   // '&' in component
        expect_eq(cmp.shouldPercentEncode(0x2B), true);   // '+' in component
        expect_eq(cmp.shouldPercentEncode(0x2C), true);   // ',' in component
        expect_eq(cmp.shouldPercentEncode(0x2D), false);  // '-' not in component
    });

    test("2. @cap/percent-encoding — Differential encoding across fragment/query/path/userinfo/component", () => {
        let sample = "a b#`?{}/:@$&+,";
        let out: [128]u8 = undefined;

        // fragment: encodes ' ', '`'
        let n_frag = percentEncode(sample, fragmentSet(), out[0..128]);
        expect_eq(peTestSliceEq(out[0..n_frag], "a%20b#%60?{}/:@$&+,"), true);

        // query: encodes ' ', '#'
        let n_qry = percentEncode(sample, querySet(), out[0..128]);
        expect_eq(peTestSliceEq(out[0..n_qry], "a%20b%23`?{}/:@$&+,"), true);

        // path: encodes ' ', '#', '`', '?', '{', '}'
        let n_pth = percentEncode(sample, pathSet(), out[0..128]);
        expect_eq(peTestSliceEq(out[0..n_pth], "a%20b%23%60%3F%7B%7D/:@$&+,"), true);

        // userinfo: path + '/', ':', '@'
        let n_usr = percentEncode(sample, userinfoSet(), out[0..128]);
        expect_eq(peTestSliceEq(out[0..n_usr], "a%20b%23%60%3F%7B%7D%2F%3A%40$&+,"), true);

        // component: userinfo + '$', '&', '+', ','
        let n_cmp = percentEncode(sample, componentSet(), out[0..128]);
        expect_eq(peTestSliceEq(out[0..n_cmp], "a%20b%23%60%3F%7B%7D%2F%3A%40%24%26%2B%2C"), true);
    });

    test("3. @cap/percent-encoding — nonAlphanumericSet & UTF-8 multi-byte Chinese encoding", () => {
        let out: [128]u8 = undefined;
        let non_alnum = nonAlphanumericSet();

        // Only A-Za-z0-9 remain unencoded
        let n1 = percentEncode("AZaz09-_.~!", non_alnum, out[0..128]);
        expect_eq(peTestSliceEq(out[0..n1], "AZaz09%2D%5F%2E%7E%21"), true);

        // UTF-8 Chinese "中文" = E4 B8 AD E6 96 87
        let utf8_zh: [6]u8 = [0xE4, 0xB8, 0xAD, 0xE6, 0x96, 0x87];
        let n2 = percentEncode(utf8_zh[0..6], controlsSet(), out[0..128]);
        expect_eq(n2, 18 as usize);
        expect_eq(peTestSliceEq(out[0..n2], "%E4%B8%AD%E6%96%87"), true);

        // Mixed ASCII + UTF-8 in pathSet
        let mixed = "path/中文 1";
        let n3 = percentEncode(mixed, pathSet(), out[0..128]);
        expect_eq(peTestSliceEq(out[0..n3], "path/%E4%B8%AD%E6%96%87%201"), true);
    });

    test("4. @cap/percent-encoding — percentDecode lenient decoding (preserves lone % and %ZZ)", () => {
        let out: [128]u8 = undefined;

        let n1 = percentDecode("foo%20bar%2f%2Fbaz", out[0..128]);
        expect_eq(peTestSliceEq(out[0..n1], "foo bar//baz"), true);

        // Trailing '%' and incomplete '%2' preserved verbatim
        let n2 = percentDecode("100%", out[0..128]);
        expect_eq(peTestSliceEq(out[0..n2], "100%"), true);

        let n3 = percentDecode("100%2", out[0..128]);
        expect_eq(peTestSliceEq(out[0..n3], "100%2"), true);

        // Invalid hex '%ZZ' and '%%20' (first '%' literal, '%20' decoded)
        let n4 = percentDecode("a%ZZb%%20c", out[0..128]);
        expect_eq(peTestSliceEq(out[0..n4], "a%ZZb% c"), true);

        // UTF-8 round-trip decode
        let n5 = percentDecode("%E4%B8%AD%E6%96%87", out[0..128]);
        expect_eq(peTestSliceEq(out[0..n5], "中文"), true);
    });

    test("5. @cap/percent-encoding — percentDecodeStrict rejects invalid % & percentDecodePlus form decoding", () => {
        let out: [128]u8 = undefined;

        // Valid strict decode (mixed hex case)
        let r1 = percentDecodeStrict("hello%20world%2f%3A", out[0..128]);
        expect_eq(r1, 13 as i64);
        expect_eq(peTestSliceEq(out[0..13], "hello world/:"), true);

        // Strict rejections: trailing %, incomplete %2, non-hex %ZZ, %2G
        expect_eq(percentDecodeStrict("100%", out[0..128]), -1 as i64);
        expect_eq(percentDecodeStrict("100%2", out[0..128]), -1 as i64);
        expect_eq(percentDecodeStrict("%ZZ", out[0..128]), -1 as i64);
        expect_eq(percentDecodeStrict("%2G", out[0..128]), -1 as i64);

        // Strict rejects undersized output buffer
        let tiny: [3]u8 = undefined;
        expect_eq(percentDecodeStrict("abcd", tiny[0..3]), -1 as i64);

        // percentDecodePlus: '+' -> ' ', '%2B' -> '+', lenient on '%ZZ'
        let np = percentDecodePlus("a+b%2Bc+d%20e%ZZ", out[0..128]);
        expect_eq(peTestSliceEq(out[0..np], "a b+c d e%ZZ"), true);
    });

    test("6. @cap/percent-encoding — percentEncodeLen exact prediction & small buffer overflow safety", () => {
        let input = "a b<c>";
        let set = fragmentSet();

        // "a" (1) + "%20" (3) + "b" (1) + "%3C" (3) + "c" (1) + "%3E" (3) = 12
        let exact = percentEncodeLen(input, set);
        expect_eq(exact, 12 as usize);

        let full: [16]u8 = undefined;
        let nf = percentEncode(input, set, full[0..16]);
        expect_eq(nf, exact);
        expect_eq(peTestSliceEq(full[0..nf], "a%20b%3Cc%3E"), true);

        // Small buffer: 4 bytes fits "a%20" cleanly without overflow
        let small: [6]u8 = [0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA];
        let ns = percentEncode(input, set, small[0..4]);
        expect_eq(ns, 4 as usize);
        expect_eq(peTestSliceEq(small[0..4], "a%20"), true);
        expect_eq(small[4], 0xAA as u8); // sentinel untouched

        // Buffer of 2 bytes cannot fit "%20" after "a" -> writes only "a" (no truncated '%')
        let ns2 = percentEncode(input, set, small[0..2]);
        expect_eq(ns2, 1 as usize);
        expect_eq(peTestSliceEq(small[0..1], "a"), true);

        // Empty input & zero-length output buffer
        let empty: [1]u8 = [0];
        expect_eq(percentEncodeLen(empty[0..0], set), 0 as usize);
        expect_eq(percentEncode(empty[0..0], set, full[0..16]), 0 as usize);
        expect_eq(percentEncode(input, set, full[0..0]), 0 as usize);
    });
}
