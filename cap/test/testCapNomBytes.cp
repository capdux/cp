// cap/test/testCapNomBytes.cp
// Comprehensive unit tests for @cap/nom-bytes

import { test, expect_eq } from "cap:test";
import {
    Cursor,
    tag,
    tagNoCase,
    charLit,
    take,
    takeUntil,
    takeTillByte,
    space0,
    multispace0,
    digit1,
    alpha1,
    alphanumeric1,
    hexDigit1,
    parseU64,
    parseI64,
    delimited,
} from "../../packages/winnow/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
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
    test("1. @cap/nom-bytes — tag & tagNoCase matching with rollback on failure", () => {
        let cur = Cursor.new("HTTP/1.1 200 OK");
        expect_eq(tag(&cur, "HTTPS"), false);
        expect_eq(cur.pos, 0 as usize); // strictly unchanged on failure

        expect_eq(tag(&cur, "HTTP/"), true);
        expect_eq(cur.pos, 5 as usize);
        expect_eq(tagNoCase(&cur, "1.1 "), true);
        expect_eq(sliceEq(cur.rest(), "200 OK"), true);
    });

    test("2. @cap/nom-bytes — take, takeUntil & takeTillByte", () => {
        let cur = Cursor.new("hello::world;end");
        const s1 = takeUntil(&cur, "::");
        expect_eq(s1.ok, true);
        expect_eq(sliceEq(cur.data[s1.start..(s1.start + s1.len)], "hello"), true);

        expect_eq(tag(&cur, "::"), true);
        const s2 = takeTillByte(&cur, ';' as u8);
        expect_eq(s2.ok, true);
        expect_eq(sliceEq(cur.data[s2.start..(s2.start + s2.len)], "world"), true);

        expect_eq(charLit(&cur, ';' as u8), true);
        const s3 = take(&cur, 3);
        expect_eq(s3.ok, true);
        expect_eq(sliceEq(cur.data[s3.start..(s3.start + s3.len)], "end"), true);
        expect_eq(cur.isEmpty(), true);
    });

    test("3. @cap/nom-bytes — digit1, alpha1, alphanumeric1 & hexDigit1", () => {
        let cur = Cursor.new("abc123 0xDeadBeef 99");
        const a = alpha1(&cur);
        expect_eq(a.ok && sliceEq(cur.data[a.start..(a.start + a.len)], "abc"), true);
        const d = digit1(&cur);
        expect_eq(d.ok && sliceEq(cur.data[d.start..(d.start + d.len)], "123"), true);

        * = space0(&cur);
        expect_eq(tag(&cur, "0x"), true);
        const hx = hexDigit1(&cur);
        expect_eq(hx.ok && sliceEq(cur.data[hx.start..(hx.start + hx.len)], "DeadBeef"), true);

        let cur2 = Cursor.new("var42!");
        const an = alphanumeric1(&cur2);
        expect_eq(an.ok && sliceEq(cur2.data[an.start..(an.start + an.len)], "var42"), true);
    });

    test("4. @cap/nom-bytes — parseU64 & parseI64 with negatives & overflow rollback", () => {
        let cur = Cursor.new("12345 -98765 99999999999999999999999");
        const u = parseU64(&cur);
        expect_eq(u.ok, true);
        expect_eq(u.value, 12345 as u64);

        * = space0(&cur);
        const neg = parseI64(&cur);
        expect_eq(neg.ok, true);
        expect_eq(neg.value, -98765 as i64);

        * = space0(&cur);
        const posBeforeOverflow = cur.pos;
        const ovf = parseU64(&cur);
        expect_eq(ovf.ok, false);
        expect_eq(cur.pos, posBeforeOverflow); // rolled back on overflow
    });

    test("5. @cap/nom-bytes — Composed config parser: '  key = (42)  '", () => {
        let cur = Cursor.new("  \n key = (42)  ");
        * = multispace0(&cur);
        const k = alpha1(&cur);
        expect_eq(k.ok && sliceEq(cur.data[k.start..(k.start + k.len)], "key"), true);

        * = multispace0(&cur);
        expect_eq(charLit(&cur, '=' as u8), true);
        * = multispace0(&cur);

        const inner = delimited(&cur, '(' as u8, ')' as u8);
        expect_eq(inner.ok, true);
        let innerCur = Cursor.new(cur.data[inner.start..(inner.start + inner.len)]);
        const num = parseI64(&innerCur);
        expect_eq(num.ok, true);
        expect_eq(num.value, 42 as i64);
    });

    test("6. @cap/nom-bytes — Empty input, EOF peek & unclosed delimiter rollback", () => {
        let empty = Cursor.new("");
        expect_eq(empty.isEmpty(), true);
        expect_eq(empty.remaining(), 0 as usize);
        expect_eq(empty.peek(), -1 as i32);
        expect_eq(digit1(&empty).ok, false);
        expect_eq(take(&empty, 1).ok, false);

        let badDelim = Cursor.new("(unclosed");
        expect_eq(delimited(&badDelim, '(' as u8, ')' as u8).ok, false);
        expect_eq(badDelim.pos, 0 as usize);
    });
}
