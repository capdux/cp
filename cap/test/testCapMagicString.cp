import { test, expect_eq } from "cap:test";
import { MagicString } from "../../packages/magic-string/src/index.cp";

fn main() {
    test("1. @cap/magic-string — Classic Rollup overwrite + prepend + append", () => {
        let s: MagicString = MagicString.new("problems = 99");
        expect_eq(s.hasChanged(), false);

        expect_eq(s.overwrite(0, 8, "answer"), true);
        expect_eq(s.overwrite(11, 13, "42"), true);
        expect_eq(s.prepend("var "), true);
        expect_eq(s.append(";"), true);
        expect_eq(s.hasChanged(), true);

        let buf: [64]u8 = undefined;
        const rendered: string = s.toString(&buf);
        expect_eq(rendered, "var answer = 42;");
    });

    test("2. @cap/magic-string — remove() deletes original ranges cleanly", () => {
        let s: MagicString = MagicString.new("let unused = 1; let ok = 2;");
        expect_eq(s.remove(0, 16), true);

        let buf: [64]u8 = undefined;
        const n: usize = s.writeTo(&buf);
        expect_eq(buf[0..n], "let ok = 2;");
    });

    test("3. @cap/magic-string — prependLeft() and appendRight() ordering at same offset", () => {
        let s: MagicString = MagicString.new("abcdef");
        expect_eq(s.prependLeft(3, "L1"), true);
        expect_eq(s.prependLeft(3, "L2"), true);
        expect_eq(s.appendRight(3, "R1"), true);
        expect_eq(s.appendRight(3, "R2"), true);

        let buf: [64]u8 = undefined;
        const rendered: string = s.toString(&buf);
        expect_eq(rendered, "abcL2L1R1R2def");

        // Verify left inserts stay with left slice and right inserts stay with right slice
        let left_buf: [32]u8 = undefined;
        const ln: usize = s.slice(0, 3, &left_buf);
        expect_eq(left_buf[0..ln], "abcL2L1");

        let right_buf: [32]u8 = undefined;
        const rn: usize = s.slice(3, 6, &right_buf);
        expect_eq(right_buf[0..rn], "R1R2def");
    });

    test("4. @cap/magic-string — Multiple prepend() and append() order", () => {
        let s: MagicString = MagicString.new("CORE");
        expect_eq(s.prepend("B"), true);
        expect_eq(s.prepend("A"), true);
        expect_eq(s.append("C"), true);
        expect_eq(s.append("D"), true);

        let buf: [32]u8 = undefined;
        const n: usize = s.writeTo(&buf);
        expect_eq(buf[0..n], "ABCORECD");
    });

    test("5. @cap/magic-string — slice() across edited and unedited chunks", () => {
        let s: MagicString = MagicString.new("hello world!");
        expect_eq(s.overwrite(6, 11, "CP"), true);

        let b1: [32]u8 = undefined;
        const n1: usize = s.slice(0, 5, &b1);
        expect_eq(b1[0..n1], "hello");

        let b2: [32]u8 = undefined;
        const n2: usize = s.slice(6, 12, &b2);
        expect_eq(b2[0..n2], "CP!");

        let b3: [32]u8 = undefined;
        const n3: usize = s.slice(2, 11, &b3);
        expect_eq(b3[0..n3], "llo CP");
    });

    test("6. @cap/magic-string — Invalid ranges and splitting edited chunks rejected", () => {
        let s: MagicString = MagicString.new("0123456789");
        expect_eq(s.overwrite(5, 5, "x"), false);
        expect_eq(s.overwrite(6, 4, "x"), false);
        expect_eq(s.overwrite(0, 20, "x"), false);
        expect_eq(s.prependLeft(11, "x"), false);
        expect_eq(s.appendRight(11, "x"), false);

        // Overwrite [2, 6) then attempt to split inside edited range [2, 6)
        expect_eq(s.overwrite(2, 6, "ABCD"), true);
        expect_eq(s.overwrite(4, 8, "ZZ"), false);

        let buf: [32]u8 = undefined;
        const n: usize = s.writeTo(&buf);
        expect_eq(buf[0..n], "01ABCD6789");
    });
}
