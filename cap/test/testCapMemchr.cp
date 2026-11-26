// cap/test/testCapMemchr.cp
// Comprehensive unit tests for @cap/memchr (SWAR-accelerated byte & substring search)

import { test, expect_eq } from "cap:test";
import {
    memchr,
    memchr2,
    memchr3,
    memrchr,
    memrchr2,
    memrchr3,
    memchrCount,
    memchrFindAll,
    memmemFind,
    memmemRfind
} from "../../packages/@cap/memchr/src/index.cp";

fn main() {
    test("1. @cap/memchr — SWAR memchr across 8-byte aligned/unaligned, first/last & 0x00/0x80/0xFF", () => {
        // 35-byte buffer covering multiple 8-byte SWAR chunks + tail
        let buf: [35]u8 = [
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
            0x09, 0x0A, 0x00, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
            0x80, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
            0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0xFF,
            0x21, 0x22, 0x7F
        ];

        // First byte (offset 0), last byte (offset 34), aligned boundary (offset 16), unaligned (offset 10, 31)
        expect_eq(memchr(0x01, buf[0..35]), 0 as i64);
        expect_eq(memchr(0x08, buf[0..35]), 7 as i64);
        expect_eq(memchr(0x00, buf[0..35]), 10 as i64);
        expect_eq(memchr(0x80, buf[0..35]), 16 as i64);
        expect_eq(memchr(0xFF, buf[0..35]), 31 as i64);
        expect_eq(memchr(0x7F, buf[0..35]), 34 as i64);

        // Unaligned subslice starting at offset 3
        expect_eq(memchr(0x00, buf[3..35]), 7 as i64);
        expect_eq(memchr(0x80, buf[3..35]), 13 as i64);
        expect_eq(memchr(0xFF, buf[3..35]), 28 as i64);
    });

    test("2. @cap/memchr — memchr2 & memchr3 multi-needle earliest hit", () => {
        let s = "0123456789abcdefXYZ!@#";
        expect_eq(memchr2(0x5A, 0x58, s), 16 as i64);       // 'Z' vs 'X' -> 'X' at 16
        expect_eq(memchr2(0x23, 0x21, s), 19 as i64);       // '#' vs '!' -> '!' at 19
        expect_eq(memchr3(0x23, 0x59, 0x64, s), 13 as i64); // '#', 'Y', 'd' -> 'd' at 13
        expect_eq(memchr3(0x5A, 0x21, 0x40, s), 18 as i64); // 'Z', '!', '@' -> 'Z' at 18
        expect_eq(memchr2(0x7E, 0x7D, s), -1 as i64);
        expect_eq(memchr3(0x7E, 0x7D, 0x7C, s), -1 as i64);
    });

    test("3. @cap/memchr — memrchr, memrchr2 & memrchr3 reverse SWAR search", () => {
        let s = "alpha/beta/gamma:delta;epsilon/zeta";
        expect_eq(memrchr(0x2F, s), 30 as i64);               // last '/' at 30
        expect_eq(memrchr(0x61, s), 34 as i64);               // last 'a' at 34
        expect_eq(memrchr(0x61, s[0..10]), 9 as i64);         // last 'a' in "alpha/beta" at 9
        expect_eq(memrchr2(0x3A, 0x3B, s), 22 as i64);        // ':' (16) vs ';' (22) -> 22
        expect_eq(memrchr3(0x2F, 0x3A, 0x3B, s), 30 as i64);  // last of '/', ':', ';' -> 30
        expect_eq(memrchr3(0x3A, 0x3B, 0x78, s), 22 as i64);  // ':' (16), ';' (22), 'x' (none) -> 22
        expect_eq(memrchr(0x5A, s), -1 as i64);
    });

    test("4. @cap/memchr — memchrCount & memchrFindAll full offset extraction", () => {
        let s = "a,bb,ccc,dddd,eeeee,ffffff";
        expect_eq(memchrCount(0x2C, s), 5 as usize); // 5 commas
        expect_eq(memchrCount(0x65, s), 5 as usize); // 5 'e's
        expect_eq(memchrCount(0x7A, s), 0 as usize); // 0 'z's

        let offsets: [8]usize = [0; 8];
        let found = memchrFindAll(0x2C, s, offsets[0..8]);
        expect_eq(found, 5 as usize);
        expect_eq(offsets[0], 1 as usize);
        expect_eq(offsets[1], 4 as usize);
        expect_eq(offsets[2], 8 as usize);
        expect_eq(offsets[3], 13 as usize);
        expect_eq(offsets[4], 19 as usize);

        // Truncated output slice stops safely at capacity
        let small: [2]usize = [0; 2];
        let found2 = memchrFindAll(0x2C, s, small[0..2]);
        expect_eq(found2, 2 as usize);
        expect_eq(small[0], 1 as usize);
        expect_eq(small[1], 4 as usize);
    });

    test("5. @cap/memchr — memmemFind & memmemRfind substring search (overlapping 'aaab' & empty needle)", () => {
        let overlap = "aaaaaaaabaaaaab";
        expect_eq(memmemFind(overlap, "aaab"), 5 as i64);
        expect_eq(memmemRfind(overlap, "aaab"), 11 as i64);

        let text = "the quick brown fox jumps over the lazy fox";
        expect_eq(memmemFind(text, "fox"), 16 as i64);
        expect_eq(memmemRfind(text, "fox"), 40 as i64);
        expect_eq(memmemFind(text, "the"), 0 as i64);
        expect_eq(memmemRfind(text, "the"), 31 as i64);
        expect_eq(memmemFind(text, "cat"), -1 as i64);
        expect_eq(memmemRfind(text, "cat"), -1 as i64);

        // Empty needle returns 0 (forward) and haystack.len (reverse)
        expect_eq(memmemFind(text, ""), 0 as i64);
        expect_eq(memmemRfind(text, ""), text.len as i64);

        // Needle longer than haystack
        expect_eq(memmemFind("abc", "abcd"), -1 as i64);
        expect_eq(memmemRfind("abc", "abcd"), -1 as i64);
    });

    test("6. @cap/memchr — Empty haystack & miss boundaries return -1 / 0", () => {
        let empty = "";
        let offsets: [4]usize = [0; 4];

        expect_eq(memchr(0x61, empty), -1 as i64);
        expect_eq(memchr2(0x61, 0x62, empty), -1 as i64);
        expect_eq(memchr3(0x61, 0x62, 0x63, empty), -1 as i64);
        expect_eq(memrchr(0x61, empty), -1 as i64);
        expect_eq(memrchr2(0x61, 0x62, empty), -1 as i64);
        expect_eq(memrchr3(0x61, 0x62, 0x63, empty), -1 as i64);
        expect_eq(memchrCount(0x61, empty), 0 as usize);
        expect_eq(memchrFindAll(0x61, empty, offsets[0..4]), 0 as usize);
        expect_eq(memmemFind(empty, "a"), -1 as i64);
        expect_eq(memmemRfind(empty, "a"), -1 as i64);
        expect_eq(memmemFind(empty, ""), 0 as i64);
        expect_eq(memmemRfind(empty, ""), 0 as i64);
    });
}
