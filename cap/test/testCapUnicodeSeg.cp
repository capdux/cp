// cap/test/testCapUnicodeSeg.cp
// Comprehensive unit tests for @cap/unicode-segmentation (UAX #29 graphemes & slicing)

import { test, expect_eq } from "cap:test";
import {
    GraphemeIter,
    countGraphemes,
    nthGrapheme,
    sliceGraphemes,
} from "../../packages/unicode-segmentation/src/index.cp";

fn segEq(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/unicode-segmentation — CRLF, combining marks & countGraphemes", () => {
        expect_eq(countGraphemes("a\r\nb"), 3 as usize);
        const cafe = "cafe\xCC\x81";
        expect_eq(countGraphemes(cafe), 4 as usize);

        const g3 = nthGrapheme(cafe, 3).?;
        expect_eq(segEq(cafe[g3.start..g3.end], "e\xCC\x81"), true);
    });

    test("2. @cap/unicode-segmentation — GraphemeIter & sliceGraphemes", () => {
        const s = "hi\r\n!";
        let it = GraphemeIter.new(s);
        expect_eq(segEq(s[it.next().?.start..1], "h"), true);
        const second = it.next().?;
        expect_eq(segEq(s[second.start..second.end], "i"), true);
        const crlf = it.next().?;
        expect_eq(segEq(s[crlf.start..crlf.end], "\r\n"), true);

        const sl = sliceGraphemes(s, 0 as usize, 3 as usize);
        expect_eq(segEq(s[sl.start..sl.end], "hi\r\n"), true);
    });
}
