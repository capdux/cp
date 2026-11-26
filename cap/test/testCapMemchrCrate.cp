import { test, expect_eq } from "cap:test";
import {
    memchr, memchr2, memchr3, memrchr, memrchr2, memrchr3,
    countByte, memmem, memrmem, memmemStr
} from "../../packages/@cap/memchr-crate/src/index.cp";

fn main() {
    test("1. @cap/memchr-crate — memchr, memchr2 & memchr3 forward byte search", () => {
        let buf: [8]u8 = [97, 98, 99, 100, 101, 102, 99, 103]; // "abcdefcg"
        expect_eq(memchr(99, buf[0..8]) ?? 99, 2);
        expect_eq(memchr(122, buf[0..8]) == null, true);
        expect_eq(memchr2(101, 98, buf[0..8]) ?? 99, 1);
        expect_eq(memchr3(120, 103, 100, buf[0..8]) ?? 99, 3);
    });

    test("2. @cap/memchr-crate — memrchr, memrchr2 & memrchr3 reverse byte search", () => {
        let buf: [8]u8 = [97, 98, 99, 100, 101, 102, 99, 103]; // "abcdefcg"
        expect_eq(memrchr(99, buf[0..8]) ?? 99, 6);
        expect_eq(memrchr(122, buf[0..8]) == null, true);
        expect_eq(memrchr2(97, 101, buf[0..8]) ?? 99, 4);
        expect_eq(memrchr3(97, 98, 103, buf[0..8]) ?? 99, 7);
    });

    test("3. @cap/memchr-crate — countByte counts exact occurrences", () => {
        let buf: [6]u8 = [10, 97, 10, 98, 10, 10];
        expect_eq(countByte(10, buf[0..6]), 4);
        expect_eq(countByte(99, buf[0..6]), 0);
    });

    test("4. @cap/memchr-crate — Boyer-Moore-Horspool memmem finds first occurrence", () => {
        let hay: [11]u8 = [104, 101, 108, 108, 111, 32, 119, 111, 114, 108, 100]; // "hello world"
        let ndl: [5]u8 = [119, 111, 114, 108, 100]; // "world"
        let miss: [3]u8 = [120, 121, 122];
        expect_eq(memmem(hay[0..11], ndl[0..5]) ?? 99, 6);
        expect_eq(memmem(hay[0..11], miss[0..3]) == null, true);
        expect_eq(memmem(hay[0..11], ndl[0..0]) ?? 99, 0);
    });

    test("5. @cap/memchr-crate — memrmem finds last occurrence", () => {
        let hay: [9]u8 = [97, 98, 97, 98, 99, 97, 98, 97, 98]; // "ababcabab"
        let ndl: [2]u8 = [97, 98]; // "ab"
        expect_eq(memmem(hay[0..9], ndl[0..2]) ?? 99, 0);
        expect_eq(memrmem(hay[0..9], ndl[0..2]) ?? 99, 7);
        expect_eq(memrmem(hay[0..9], ndl[0..0]) ?? 99, 9);
    });

    test("6. @cap/memchr-crate — memmemStr handles string search & boundary cases", () => {
        expect_eq(memmemStr("the quick brown fox", "brown") ?? 99, 10);
        expect_eq(memmemStr("short", "muchlonger") == null, true);
        expect_eq(memmemStr("abc", "") ?? 99, 0);
    });
}
