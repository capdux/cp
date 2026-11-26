import { test, expect_eq } from "cap:test";
import {
    BalancedResult,
    balanced,
    balancedRange,
    balancedAll,
} from "../../packages/@cap/balanced-match/src/index.cp";

fn main() {
    test("1. @cap/balanced-match — Nested braces match outermost balanced pair", () => {
        const res: BalancedResult = balanced("{", "}", "pre{in{nested}}post");
        expect_eq(res.found, true);
        expect_eq(res.start, 3);
        expect_eq(res.end, 14);
        expect_eq(res.pre, "pre");
        expect_eq(res.body, "in{nested}");
        expect_eq(res.post, "post");
    });

    test("2. @cap/balanced-match — Unclosed outer opener backtracks to inner valid pair", () => {
        const res: BalancedResult = balanced("{", "}", "a { b { c } d");
        expect_eq(res.found, true);
        expect_eq(res.start, 6);
        expect_eq(res.end, 10);
        expect_eq(res.pre, "a { b ");
        expect_eq(res.body, " c ");
        expect_eq(res.post, " d");
    });

    test("3. @cap/balanced-match — Multi-byte delimiters and nested HTML comments", () => {
        const res: BalancedResult = balanced("<!--", "-->", "x<!--a<!--b-->c-->y");
        expect_eq(res.found, true);
        expect_eq(res.start, 1);
        expect_eq(res.end, 15);
        expect_eq(res.pre, "x");
        expect_eq(res.body, "a<!--b-->c");
        expect_eq(res.post, "y");
    });

    test("4. @cap/balanced-match — Identical open/close delimiters (quotes)", () => {
        const res: BalancedResult = balanced("\"", "\"", "say \"hello\" world");
        expect_eq(res.found, true);
        expect_eq(res.start, 4);
        expect_eq(res.end, 10);
        expect_eq(res.pre, "say ");
        expect_eq(res.body, "hello");
        expect_eq(res.post, " world");
    });

    test("5. @cap/balanced-match — No match and empty delimiter edge cases", () => {
        expect_eq(balanced("{", "}", "no braces here").found, false);
        expect_eq(balanced("{", "}", "unclosed { only").found, false);
        expect_eq(balanced("", "}", "a}b").found, false);
        expect_eq(balanced("{", "", "a{b").found, false);
        expect_eq(balancedRange("(", ")", ")(unmatched(") == null, true);

        const rng = balancedRange("(", ")", "f(a+(b*c))+1").?;
        expect_eq(@field(rng, "0"), 1);
        expect_eq(@field(rng, "1"), 9);
    });

    test("6. @cap/balanced-match — balancedAll collects multiple sequential top-level pairs", () => {
        let out: [4]BalancedResult = undefined;
        const n: usize = balancedAll("{", "}", "x{1}y{2{3}}z{4}!", &out);
        expect_eq(n, 3);

        expect_eq(out[0].start, 1);
        expect_eq(out[0].end, 3);
        expect_eq(out[0].body, "1");

        expect_eq(out[1].start, 5);
        expect_eq(out[1].end, 10);
        expect_eq(out[1].body, "2{3}");

        expect_eq(out[2].start, 12);
        expect_eq(out[2].end, 14);
        expect_eq(out[2].body, "4");
    });
}
