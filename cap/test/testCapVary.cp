// cap/test/testCapVary.cp
// Comprehensive unit tests for @cap/vary (npm vary / RFC 9110 parity)

import { test, expect_eq } from "cap:test";
import {
    isValidFieldName,
    hasField,
    append,
    appendMany,
} from "../../packages/@cap/vary/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
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

fn main() {
    test("1. @cap/vary — append single field to empty header", () => {
        let dst: [64]u8 = undefined;
        const n: i64 = append("", "Accept-Encoding", dst[0..64]);
        expect_eq(n > 0, true);
        expect_eq(sliceEq(dst[0..(n as usize)], "Accept-Encoding"), true);
    });

    test("2. @cap/vary — comma-list dedup & case-insensitive first-seen casing preservation", () => {
        let dst: [128]u8 = undefined;
        const n: i64 = append(
            "Accept-Encoding, Origin",
            "accept-encoding, User-Agent, ORIGIN, Accept-Language",
            dst[0..128],
        );
        expect_eq(n > 0, true);
        expect_eq(
            sliceEq(dst[0..(n as usize)], "Accept-Encoding, Origin, User-Agent, Accept-Language"),
            true,
        );
    });

    test("3. @cap/vary — wildcard '*' collapses all existing & appended fields", () => {
        let dst: [64]u8 = undefined;
        const n1: i64 = append("Accept-Encoding, Origin", "*", dst[0..64]);
        expect_eq(n1, 1 as i64);
        expect_eq(sliceEq(dst[0..1], "*"), true);

        const n2: i64 = append("*", "Accept-Encoding", dst[0..64]);
        expect_eq(n2, 1 as i64);
        expect_eq(sliceEq(dst[0..1], "*"), true);

        const n3: i64 = append("Origin", "Accept, *, User-Agent", dst[0..64]);
        expect_eq(n3, 1 as i64);
        expect_eq(sliceEq(dst[0..1], "*"), true);
    });

    test("4. @cap/vary — hasField exact, case-insensitive & wildcard lookup", () => {
        expect_eq(hasField("Accept-Encoding, Origin", "origin"), true);
        expect_eq(hasField("Accept-Encoding, Origin", "ACCEPT-ENCODING"), true);
        expect_eq(hasField("Accept-Encoding, Origin", "Cookie"), false);
        expect_eq(hasField("*", "Anything-At-All"), true);
        expect_eq(hasField("", "Origin"), false);
    });

    test("5. @cap/vary — rejects invalid field names (spaces, CRLF, control chars)", () => {
        expect_eq(isValidFieldName("Accept-Encoding"), true);
        expect_eq(isValidFieldName("*"), true);
        expect_eq(isValidFieldName("X-Custom_Header.1"), true);
        expect_eq(isValidFieldName(""), false);
        expect_eq(isValidFieldName("Accept Encoding"), false);
        expect_eq(isValidFieldName("Bad\r\nHeader"), false);
        expect_eq(isValidFieldName("Bad:Header"), false);

        let dst: [64]u8 = undefined;
        expect_eq(append("Origin", "Bad Header", dst[0..64]), -1 as i64);
        expect_eq(append("Origin", "X\nY", dst[0..64]), -1 as i64);
        expect_eq(append("*", "Bad Header", dst[0..64]), -1 as i64);
        expect_eq(append("Origin", "", dst[0..64]), -1 as i64);
    });

    test("6. @cap/vary — appendMany batch append & tiny buffer overflow returns -1", () => {
        const fields: [3][]const u8 = ["Origin", "accept-encoding", "X-Api-Version"];
        let dst: [128]u8 = undefined;
        const n: i64 = appendMany("Accept-Encoding", fields[0..3], dst[0..128]);
        expect_eq(n > 0, true);
        expect_eq(
            sliceEq(dst[0..(n as usize)], "Accept-Encoding, Origin, X-Api-Version"),
            true,
        );

        let tiny: [10]u8 = undefined;
        expect_eq(append("Accept-Encoding", "Origin", tiny[0..10]), -1 as i64);
        expect_eq(appendMany("Accept-Encoding", fields[0..3], tiny[0..10]), -1 as i64);
    });
}
