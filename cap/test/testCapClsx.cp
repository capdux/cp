// cap/test/testCapClsx.cp
// Comprehensive unit tests for @cap/clsx (npm clsx / classnames parity)

import { test, expect_eq } from "cap:test";
import {
    CondClass,
    ClassBuilder,
    joinClasses,
    joinCondClasses,
} from "../../packages/clsx/src/index.cp";

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
    test("1. @cap/clsx — empty strings & extra whitespace normalization", () => {
        let buf: [128]u8 = undefined;
        let b: ClassBuilder = ClassBuilder.init(buf[0..128]);
        expect_eq(b.add(""), true);
        expect_eq(b.add("   \t\n  "), true);
        expect_eq(b.add("  px-4   py-2\t"), true);
        expect_eq(b.add("  rounded\nshadow  "), true);

        expect_eq(sliceEq(b.build(), "px-4 py-2 rounded shadow"), true);
        expect_eq(b.len(), 24 as usize);

        b.reset();
        expect_eq(b.len(), 0 as usize);
        expect_eq(b.isOverflow(), false);
    });

    test("2. @cap/clsx — addIf conditional toggles", () => {
        let buf: [128]u8 = undefined;
        let b: ClassBuilder = ClassBuilder.init(buf[0..128]);
        expect_eq(b.add("btn"), true);
        expect_eq(b.addIf(true, "btn-primary"), true);
        expect_eq(b.addIf(false, "btn-disabled"), true);
        expect_eq(b.addIf(true, "  focus:ring  "), true);

        expect_eq(sliceEq(b.build(), "btn btn-primary focus:ring"), true);
    });

    test("3. @cap/clsx — addConds batch conditional toggles", () => {
        const items: [4]CondClass = [
            CondClass { name: "card", enabled: true },
            CondClass { name: "card-dark", enabled: false },
            CondClass { name: "  p-4  m-2 ", enabled: true },
            CondClass { name: "hidden", enabled: false },
        ];
        let buf: [128]u8 = undefined;
        let b: ClassBuilder = ClassBuilder.init(buf[0..128]);
        expect_eq(b.addConds(items[0..4]), true);
        expect_eq(sliceEq(b.build(), "card p-4 m-2"), true);
    });

    test("4. @cap/clsx — addUnique deduplicates without substring false positives", () => {
        let buf: [128]u8 = undefined;
        let b: ClassBuilder = ClassBuilder.init(buf[0..128]);
        expect_eq(b.addUnique("btn-primary"), true);
        // "btn" is a substring of "btn-primary", NOT the same token — must be added!
        expect_eq(b.addUnique("btn btn-primary active btn"), true);

        expect_eq(sliceEq(b.build(), "btn-primary btn active"), true);
    });

    test("5. @cap/clsx — has exact word-boundary check", () => {
        let buf: [128]u8 = undefined;
        let b: ClassBuilder = ClassBuilder.init(buf[0..128]);
        expect_eq(b.add("btn-primary flex-col items-center"), true);

        expect_eq(b.has("btn-primary"), true);
        expect_eq(b.has("flex-col"), true);
        expect_eq(b.has("items-center"), true);
        expect_eq(b.has("btn"), false);
        expect_eq(b.has("primary"), false);
        expect_eq(b.has("flex"), false);
        expect_eq(b.has(""), false);
    });

    test("6. @cap/clsx — joinClasses, joinCondClasses & buffer overflow protection", () => {
        const list: [3][]const u8 = ["  a  ", "", "b   c"];
        let dst: [32]u8 = undefined;
        const n1: i64 = joinClasses(list[0..3], dst[0..32]);
        expect_eq(n1, 5 as i64);
        expect_eq(sliceEq(dst[0..5], "a b c"), true);

        const conds: [2]CondClass = [
            CondClass { name: "alpha", enabled: true },
            CondClass { name: "beta", enabled: true },
        ];
        const n2: i64 = joinCondClasses(conds[0..2], dst[0..32]);
        expect_eq(n2, 10 as i64);
        expect_eq(sliceEq(dst[0..10], "alpha beta"), true);

        // Overflow protection
        let tiny: [6]u8 = undefined;
        expect_eq(joinCondClasses(conds[0..2], tiny[0..6]), -1 as i64);
        let b: ClassBuilder = ClassBuilder.init(tiny[0..6]);
        expect_eq(b.add("alpha"), true);
        expect_eq(b.isOverflow(), false);
        expect_eq(b.add("beta"), false);
        expect_eq(b.isOverflow(), true);
        expect_eq(sliceEq(b.build(), "alpha"), true);
    });
}
