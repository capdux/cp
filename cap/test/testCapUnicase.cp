// cap/test/testCapUnicase.cp
// Comprehensive unit tests for @cap/unicase

import { test, expect_eq } from "cap:test";
import {
    UniCase,
    eqIgnoreAsciiCase,
    cmpIgnoreAsciiCase,
    startsWithIgnoreCase,
    endsWithIgnoreCase,
    indexOfIgnoreCase,
    containsIgnoreCase,
    hashIgnoreCase64,
    toLowerAscii,
    toUpperAscii,
} from "../../packages/@cap/unicase/src/index.cp";

fn main() {
    test("1. @cap/unicase — UniCase equality & FNV-1a 64-bit hash consistency", () => {
        const a = UniCase.new("Content-Type");
        const b = UniCase.new("content-type");
        const c = UniCase.new("CONTENT-TYPE");
        const d = UniCase.new("Accept-Encoding");

        expect_eq(a.asStr(), "Content-Type");
        expect_eq(a.equals(b), true);
        expect_eq(a.equals(c), true);
        expect_eq(a.equals(d), false);
        expect_eq(a.hash64(), b.hash64());
        expect_eq(a.hash64(), c.hash64());
        expect_eq(a.hash64() != d.hash64(), true);
        expect_eq(hashIgnoreCase64("X-Request-ID"), hashIgnoreCase64("x-request-id"));
    });

    test("2. @cap/unicase — Three-way cmpIgnoreAsciiCase ordering", () => {
        expect_eq(cmpIgnoreAsciiCase("alpha", "ALPHA"), 0 as i32);
        expect_eq(cmpIgnoreAsciiCase("apple", "Banana"), -1 as i32);
        expect_eq(cmpIgnoreAsciiCase("Zebra", "apple"), 1 as i32);
        expect_eq(cmpIgnoreAsciiCase("ab", "abc"), -1 as i32);
        expect_eq(cmpIgnoreAsciiCase("abcd", "ABC"), 1 as i32);

        const u1 = UniCase.new("Beta");
        const u2 = UniCase.new("alpha");
        expect_eq(u1.cmp(u2), 1 as i32);
        expect_eq(u2.cmp(u1), -1 as i32);
    });

    test("3. @cap/unicase — startsWithIgnoreCase & endsWithIgnoreCase", () => {
        expect_eq(startsWithIgnoreCase("Bearer token123", "bearer "), true);
        expect_eq(startsWithIgnoreCase("HTTPS://example.com", "https://"), true);
        expect_eq(startsWithIgnoreCase("http", "https"), false);

        expect_eq(endsWithIgnoreCase("index.HTML", ".html"), true);
        expect_eq(endsWithIgnoreCase("README.md", ".MD"), true);
        expect_eq(endsWithIgnoreCase("a.cp", ".zig"), false);
    });

    test("4. @cap/unicase — indexOfIgnoreCase & containsIgnoreCase", () => {
        expect_eq(indexOfIgnoreCase("Hello World", "WORLD") ?? (999 as usize), 6 as usize);
        expect_eq(indexOfIgnoreCase("Authorization: Basic xyz", "basic") ?? (999 as usize), 15 as usize);
        expect_eq(indexOfIgnoreCase("abc", "") ?? (999 as usize), 0 as usize);
        expect_eq(indexOfIgnoreCase("abc", "abcd") == null, true);
        expect_eq(containsIgnoreCase("text/html; charset=UTF-8", "utf-8"), true);
        expect_eq(containsIgnoreCase("application/json", "xml"), false);
    });

    test("5. @cap/unicase — toLowerAscii & toUpperAscii with UTF-8 preservation", () => {
        let b1: [64]u8 = undefined;
        expect_eq(toLowerAscii("Hello 世界 123!", b1[0..64]), "hello 世界 123!");

        let b2: [64]u8 = undefined;
        expect_eq(toUpperAscii("Hello 世界 123!", b2[0..64]), "HELLO 世界 123!");
    });

    test("6. @cap/unicase — eqIgnoreAsciiCase with UTF-8 & empty strings", () => {
        expect_eq(eqIgnoreAsciiCase("", ""), true);
        expect_eq(eqIgnoreAsciiCase("CP语言-V1", "cp语言-v1"), true);
        expect_eq(eqIgnoreAsciiCase("CP语言", "CP語"), false);
    });
}
