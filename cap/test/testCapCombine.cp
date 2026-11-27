// cap/test/testCapCombine.cp
// Comprehensive unit tests for @cap/combine

import { test, expect_eq } from "cap:test";
import {
    CharPred,
    ParseCursor,
    tag,
    charByte,
    takeWhile,
    takeWhile1,
    takeUntil,
    skipSpaces,
    parseU64,
    parseI64,
    parseQuotedString,
    delimited,
    separatedList,
} from "../../packages/combine/src/index.cp";

fn strEq(a: string, b: string): bool {
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
    test("1. @cap/combine — ParseCursor line/col tracking & save/restore backtracking", () => {
        let c = ParseCursor.new("ab\ncd");
        expect_eq(c.isEof(), false);
        expect_eq(c.line, 1 as usize);
        expect_eq(c.col, 1 as usize);
        expect_eq(c.peek().?, 'a' as u8);

        const mark = c.save();
        expect_eq(c.advance().?, 'a' as u8);
        expect_eq(c.advance().?, 'b' as u8);
        expect_eq(c.advance().?, '\n' as u8);
        expect_eq(c.line, 2 as usize);
        expect_eq(c.col, 1 as usize);
        expect_eq(strEq(c.remaining(), "cd"), true);

        c.restore(mark);
        expect_eq(c.pos, 0 as usize);
        expect_eq(c.line, 1 as usize);
        expect_eq(c.col, 1 as usize);
        expect_eq(strEq(c.remaining(), "ab\ncd"), true);
    });

    test("2. @cap/combine — tag, charByte, takeWhile, takeWhile1 & skipSpaces", () => {
        let c = ParseCursor.new("  let count_1 = 42;");
        expect_eq(skipSpaces(*c), 2 as usize);
        expect_eq(tag(*c, "const"), false);
        expect_eq(tag(*c, "let"), true);
        expect_eq(skipSpaces(*c), 1 as usize);

        const identOpt = takeWhile1(*c, CharPred.Ident);
        expect_eq(identOpt != null, true);
        expect_eq(strEq(identOpt.?, "count_1"), true);

        * = skipSpaces(*c);
        expect_eq(charByte(*c, '=' as u8), true);
        * = skipSpaces(*c);
        const numStr = takeWhile(*c, CharPred.Digit);
        expect_eq(strEq(numStr, "42"), true);
        expect_eq(charByte(*c, ';' as u8), true);
        expect_eq(c.isEof(), true);
    });

    test("3. @cap/combine — parseU64 & parseI64 valid values, signs & overflow rejection", () => {
        let c1 = ParseCursor.new("18446744073709551615 rest");
        const uMax = parseU64(*c1);
        expect_eq(uMax != null, true);
        expect_eq(uMax.?, ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64));
        expect_eq(strEq(c1.remaining(), " rest"), true);

        // Overflow u64 (> 2^64 - 1) restores cursor and returns null
        let cOver = ParseCursor.new("18446744073709551616");
        expect_eq(parseU64(*cOver) == null, true);
        expect_eq(cOver.pos, 0 as usize);

        let cNeg = ParseCursor.new("-12345,+678");
        expect_eq(parseI64(*cNeg).?, -12345 as i64);
        expect_eq(charByte(*cNeg, ',' as u8), true);
        expect_eq(parseI64(*cNeg).?, 678 as i64);
    });

    test("4. @cap/combine — parseQuotedString with escaped quotes & unterminated rollback", () => {
        let c = ParseCursor.new("\"hello \\\"cp\\\" world\" tail");
        const sOpt = parseQuotedString(*c);
        expect_eq(sOpt != null, true);
        expect_eq(strEq(sOpt.?, "hello \\\"cp\\\" world"), true);
        expect_eq(strEq(c.remaining(), " tail"), true);

        let bad = ParseCursor.new("\"unterminated");
        expect_eq(parseQuotedString(*bad) == null, true);
        expect_eq(bad.pos, 0 as usize);
    });

    test("5. @cap/combine — takeUntil & delimited bracket extraction", () => {
        let c = ParseCursor.new("key:value; [a, b, c]");
        const kOpt = takeUntil(*c, ":");
        expect_eq(kOpt != null, true);
        expect_eq(strEq(kOpt.?, "key"), true);
        expect_eq(tag(*c, ":"), true);

        const vOpt = takeUntil(*c, ";");
        expect_eq(strEq(vOpt.?, "value"), true);
        expect_eq(tag(*c, ";"), true);
        * = skipSpaces(*c);

        const innerOpt = delimited(*c, "[", "]");
        expect_eq(innerOpt != null, true);
        expect_eq(strEq(innerOpt.?, "a, b, c"), true);
        expect_eq(c.isEof(), true);

        // Missing closing delimiter restores cursor
        let bad = ParseCursor.new("(unclosed");
        expect_eq(delimited(*bad, "(", ")") == null, true);
        expect_eq(bad.pos, 0 as usize);
    });

    test("6. @cap/combine — separatedList CSV & key-value pipeline", () => {
        let c = ParseCursor.new("  alpha , beta , gamma , delta ");
        let items: [8]string = undefined;
        const n = separatedList(*c, ",", items[0..8]);
        expect_eq(n, 4 as usize);
        expect_eq(strEq(items[0], "alpha"), true);
        expect_eq(strEq(items[1], "beta"), true);
        expect_eq(strEq(items[2], "gamma"), true);
        expect_eq(strEq(items[3], "delta"), true);
        expect_eq(c.isEof(), true);
    });
}
