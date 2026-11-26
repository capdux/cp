import { test, expect_eq } from "cap:test";
import { parseNth, nthCheck } from "../../packages/@cap/nth-check/src/index.cp";

fn main() {
    test("1. @cap/nth-check — parses even and odd keywords", () => {
        const even = parseNth("even");
        expect_eq(even.valid, true);
        expect_eq(even.a, 2 as i32);
        expect_eq(even.b, 0 as i32);
        expect_eq(even.matches(0), false); // 1st child
        expect_eq(even.matches(1), true);  // 2nd child
        expect_eq(even.matches(3), true);  // 4th child

        const odd = parseNth("  ODD ");
        expect_eq(odd.valid, true);
        expect_eq(odd.matches(0), true);
        expect_eq(odd.matches(1), false);
        expect_eq(odd.matches(2), true);
    });

    test("2. @cap/nth-check — handles positive An+B with spaces", () => {
        const r = parseNth("3n + 2");
        expect_eq(r.valid, true);
        expect_eq(r.a, 3 as i32);
        expect_eq(r.b, 2 as i32);
        expect_eq(r.matches(0), false); // pos 1
        expect_eq(r.matches(1), true);  // pos 2
        expect_eq(r.matches(4), true);  // pos 5
        expect_eq(r.matches(5), false); // pos 6
    });

    test("3. @cap/nth-check — handles negative A (-n+3 selects first 3 children)", () => {
        const r = parseNth("-n+3");
        expect_eq(r.valid, true);
        expect_eq(r.a, -1 as i32);
        expect_eq(r.b, 3 as i32);
        expect_eq(r.matches(0), true);  // pos 1
        expect_eq(r.matches(1), true);  // pos 2
        expect_eq(r.matches(2), true);  // pos 3
        expect_eq(r.matches(3), false); // pos 4
    });

    test("4. @cap/nth-check — handles constant B (a=0) and bare n", () => {
        expect_eq(nthCheck("5", 4), true);
        expect_eq(nthCheck("5", 3), false);
        expect_eq(nthCheck("n", 0), true);
        expect_eq(nthCheck("n", 99), true);
        expect_eq(nthCheck("+n", 0), true);
    });

    test("5. @cap/nth-check — handles negative B offset (2n-1 matches odd)", () => {
        const r = parseNth("2n - 1");
        expect_eq(r.valid, true);
        expect_eq(r.a, 2 as i32);
        expect_eq(r.b, -1 as i32);
        expect_eq(r.matches(0), true);  // pos 1 = 2(1)-1
        expect_eq(r.matches(1), false); // pos 2
        expect_eq(r.matches(2), true);  // pos 3 = 2(2)-1
    });

    test("6. @cap/nth-check — rejects malformed formulas", () => {
        expect_eq(parseNth("").valid, false);
        expect_eq(parseNth("2nn+1").valid, false);
        expect_eq(parseNth("2n+").valid, false);
        expect_eq(parseNth("foo").valid, false);
        expect_eq(nthCheck("invalid", 0), false);
    });
}
