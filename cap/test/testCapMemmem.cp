// cap/test/testCapMemmem.cp
// Comprehensive unit tests for @cap/memmem

import { test, expect_eq } from "cap:test";
import {
    Finder,
    find,
    rfind,
    contains,
    count,
} from "../../packages/memmem/src/index.cp";

fn main() {
    test("1. @cap/memmem — Empty, single-byte, exact match & oversized needle boundaries", () => {
        expect_eq(find("hello", ""), 0 as i64);
        expect_eq(rfind("hello", ""), 5 as i64);
        expect_eq(find("", ""), 0 as i64);
        expect_eq(find("", "a"), -1 as i64);
        expect_eq(find("abc", "abcd"), -1 as i64);
        expect_eq(find("abc", "abc"), 0 as i64);
        expect_eq(find("abc", "b"), 1 as i64);
        expect_eq(rfind("ababa", "a"), 4 as i64);
        expect_eq(contains("hello world", "world"), true);
        expect_eq(contains("hello world", "xyz"), false);
    });

    test("2. @cap/memmem — Forward find vs reverse rfind", () => {
        const text = "alpha-beta-gamma-beta-delta";
        expect_eq(find(text, "beta"), 6 as i64);
        expect_eq(rfind(text, "beta"), 17 as i64);
        expect_eq(find(text, "alpha"), 0 as i64);
        expect_eq(rfind(text, "delta"), 22 as i64);
        expect_eq(rfind(text, "missing"), -1 as i64);
    });

    test("3. @cap/memmem — Pathological periodic string O(N+M) KMP accuracy", () => {
        let hay: [120]u8 = undefined;
        let i: usize = 0;
        while (i < 119) {
            hay[i] = 'a';
            i += 1;
        }
        hay[119] = 'b';

        let pat: [20]u8 = undefined;
        let j: usize = 0;
        while (j < 19) {
            pat[j] = 'a';
            j += 1;
        }
        pat[19] = 'b';

        const f = Finder.init(pat[0..20]);
        expect_eq(f.needleLen(), 20 as usize);
        expect_eq(f.findIn(hay[0..120]), 100 as i64);
        expect_eq(find(hay[0..120], pat[0..20]), 100 as i64);
        expect_eq(rfind(hay[0..120], pat[0..20]), 100 as i64);
    });

    test("4. @cap/memmem — Non-overlapping count (including empty needle)", () => {
        expect_eq(count("aaaaa", "aa"), 2 as usize);
        expect_eq(count("aaaaaa", "aa"), 3 as usize);
        expect_eq(count("abcabcabc", "abc"), 3 as usize);
        expect_eq(count("abc", ""), 4 as usize);
        expect_eq(count("hello", "xyz"), 0 as usize);
    });

    test("5. @cap/memmem — Precompiled Finder iterative findFrom scanning", () => {
        const hay = "GET /a HTTP/1.1\r\nHost: x\r\n\r\nGET /b HTTP/1.1\r\n\r\n";
        const f = Finder.init("\r\n\r\n");
        expect_eq(f.countIn(hay), 2 as usize);

        const p1 = f.findFrom(hay, 0 as usize);
        expect_eq(p1, 24 as i64);
        const p2 = f.findFrom(hay, (p1 as usize) + (4 as usize));
        expect_eq(p2, 43 as i64);
        const p3 = f.findFrom(hay, (p2 as usize) + (4 as usize));
        expect_eq(p3, -1 as i64);
    });

    test("6. @cap/memmem — Binary frame search containing embedded NUL (0x00) bytes", () => {
        let frame: [12]u8 = undefined;
        frame[0] = 0x01 as u8;
        frame[1] = 0x00 as u8;
        frame[2] = 0xFF as u8;
        frame[3] = 0x00 as u8;
        frame[4] = 0xAA as u8;
        frame[5] = 0xBB as u8;
        frame[6] = 0x00 as u8;
        frame[7] = 0xCC as u8;
        frame[8] = 0x00 as u8;
        frame[9] = 0xAA as u8;
        frame[10] = 0xBB as u8;
        frame[11] = 0x00 as u8;

        let sig: [4]u8 = undefined;
        sig[0] = 0x00 as u8;
        sig[1] = 0xAA as u8;
        sig[2] = 0xBB as u8;
        sig[3] = 0x00 as u8;

        expect_eq(find(frame[0..12], sig[0..4]), 3 as i64);
        expect_eq(rfind(frame[0..12], sig[0..4]), 8 as i64);
        expect_eq(count(frame[0..12], sig[0..4]), 2 as usize);
    });
}
