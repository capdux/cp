import { test, expect_eq } from "cap:test";
import { stem, measure, sameStem } from "../../packages/stemmer/src/index.cp";

fn testEq(a: []const u8, b: []const u8) -> bool {
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
    test("1. @cap/porter-stem — Step 1 plurals, past tense & gerunds", () => {
        let buf: [64]u8 = undefined;
        expect_eq(testEq(buf[0..stem("caresses", buf[0..64])], "caress"), true);
        expect_eq(testEq(buf[0..stem("ponies", buf[0..64])], "poni"), true);
        expect_eq(testEq(buf[0..stem("cats", buf[0..64])], "cat"), true);
        expect_eq(testEq(buf[0..stem("running", buf[0..64])], "run"), true);
        expect_eq(testEq(buf[0..stem("agreed", buf[0..64])], "agre"), true);
    });

    test("2. @cap/porter-stem — Step 2 & Step 3 derivational suffixes", () => {
        let buf: [64]u8 = undefined;
        expect_eq(testEq(buf[0..stem("relational", buf[0..64])], "relat"), true);
        expect_eq(testEq(buf[0..stem("conditional", buf[0..64])], "condit"), true);
        expect_eq(testEq(buf[0..stem("triplicate", buf[0..64])], "triplic"), true);
    });

    test("3. @cap/porter-stem — Step 4 & Step 5 trailing e/l trimming", () => {
        let buf: [64]u8 = undefined;
        expect_eq(testEq(buf[0..stem("probate", buf[0..64])], "probat"), true);
        expect_eq(testEq(buf[0..stem("controlling", buf[0..64])], "control"), true);
    });

    test("4. @cap/porter-stem — sameStem equivalence across inflections", () => {
        expect_eq(sameStem("connected", "connecting"), true);
        expect_eq(sameStem("connections", "connect"), true);
        expect_eq(sameStem("CONNECTED", "connect"), true);
        expect_eq(sameStem("running", "jumping"), false);
    });

    test("5. @cap/porter-stem — measure() VC sequence count", () => {
        expect_eq(measure("tr"), 0 as usize);
        expect_eq(measure("tree"), 0 as usize);
        expect_eq(measure("trouble"), 1 as usize);
        expect_eq(measure("oats"), 1 as usize);
        expect_eq(measure("troubles"), 2 as usize);
        expect_eq(measure("private"), 2 as usize);
    });

    test("6. @cap/porter-stem — Short words, empty string & undersized buffer protection", () => {
        let buf: [64]u8 = undefined;
        expect_eq(testEq(buf[0..stem("GO", buf[0..64])], "go"), true);
        expect_eq(stem("", buf[0..64]), 0 as usize);
        expect_eq(sameStem("", ""), false);

        let tiny: [2]u8 = undefined;
        expect_eq(stem("running", tiny[0..2]), 0 as usize);
    });
}
