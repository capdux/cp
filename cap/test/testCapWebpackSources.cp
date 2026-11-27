import { test, expect_eq } from "cap:test";
import {
    SourceEdit,
    ReplaceSource,
    concatSources,
} from "../../packages/webpack-sources/src/index.cp";

fn main() {
    test("1. @cap/webpack-sources — ReplaceSource with no edits renders original", () => {
        let src = ReplaceSource.init("const x = 1;");
        let buf: [64]u8 = undefined;
        const n = src.render(buf[0..64]);
        expect_eq(buf[0..n], "const x = 1;");
    });

    test("2. @cap/webpack-sources — replace, insert, and remove out of order", () => {
        let src = ReplaceSource.init("hello world!");
        expect_eq(src.replace(6, 11, "cp"), true);
        expect_eq(src.insert(0, "/* hdr */ "), true);
        expect_eq(src.remove(11, 12), true);

        let buf: [128]u8 = undefined;
        const n = src.render(buf[0..128]);
        expect_eq(buf[0..n], "/* hdr */ hello cp");
    });

    test("3. @cap/webpack-sources — multiple inserts at same position preserve order", () => {
        let src = ReplaceSource.init("body");
        expect_eq(src.insert(0, "A:"), true);
        expect_eq(src.insert(0, "B:"), true);
        expect_eq(src.insert(4, ";"), true);

        let buf: [64]u8 = undefined;
        const n = src.render(buf[0..64]);
        expect_eq(buf[0..n], "A:B:body;");
    });

    test("4. @cap/webpack-sources — rejects out-of-bounds ranges", () => {
        let src = ReplaceSource.init("abc");
        expect_eq(src.replace(2, 1, "x"), false);
        expect_eq(src.replace(0, 4, "x"), false);
        expect_eq(src.insert(4, "x"), false);
        expect_eq(src.remove(1, 5), false);
        expect_eq(src.edit_count, 0 as usize);
    });

    test("5. @cap/webpack-sources — concatSources joins with separator", () => {
        let parts: [3]string = undefined;
        parts[0] = "fn a() {}";
        parts[1] = "fn b() {}";
        parts[2] = "fn c() {}";

        let buf: [128]u8 = undefined;
        const n = concatSources(parts[0..3], "\n", buf[0..128]);
        expect_eq(buf[0..n], "fn a() {}\nfn b() {}\nfn c() {}");
    });

    test("6. @cap/webpack-sources — concatSources with empty separator and empty slice", () => {
        let parts: [2]string = undefined;
        parts[0] = "foo";
        parts[1] = "bar";

        let buf: [64]u8 = undefined;
        const n1 = concatSources(parts[0..2], "", buf[0..64]);
        expect_eq(buf[0..n1], "foobar");

        const n2 = concatSources(parts[0..0], "\n", buf[0..64]);
        expect_eq(n2, 0 as usize);
    });
}
