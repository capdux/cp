// cap/test/testCapPTry.cp
// Comprehensive unit tests for @cap/p-try

import { test, expect_eq } from "cap:test";
import {
    TryOutcome,
    tryOkInt,
    tryOkStr,
    tryErr,
    unwrapOrInt,
    unwrapOrStr,
    tryParseI64,
    tryDivI64,
    tryChain,
} from "../../packages/p-try/src/index.cp";

fn main() {
    test("1. @cap/p-try — tryOkInt, tryOkStr, tryErr & TryOutcome predicates", () => {
        let oi = tryOkInt(42);
        expect_eq(oi.ok, true);
        expect_eq(oi.isOk(), true);
        expect_eq(oi.isErr(), false);
        expect_eq(oi.value_i64, 42 as i64);
        expect_eq(oi.err_code, 0 as i32);

        let os = tryOkStr("hello");
        expect_eq(os.ok, true);
        expect_eq(os.isOk(), true);
        expect_eq(os.value_str, "hello");
        expect_eq(os.err_code, 0 as i32);

        let oe = tryErr(500, "internal failure");
        expect_eq(oe.ok, false);
        expect_eq(oe.isOk(), false);
        expect_eq(oe.isErr(), true);
        expect_eq(oe.err_code, 500 as i32);
        expect_eq(oe.err_msg, "internal failure");

        let sf = TryOutcome.okInt(-7);
        expect_eq(sf.ok, true);
        expect_eq(sf.value_i64, -7 as i64);
    });

    test("2. @cap/p-try — unwrapOrInt & unwrapOrStr fallback behavior", () => {
        expect_eq(unwrapOrInt(tryOkInt(12345), -1), 12345 as i64);
        expect_eq(unwrapOrInt(tryErr(10, "bad"), -99), -99 as i64);

        expect_eq(unwrapOrStr(tryOkStr("ready"), "default"), "ready");
        expect_eq(unwrapOrStr(tryErr(11, "missing"), "default"), "default");
    });

    test("3. @cap/p-try — tryParseI64 valid inputs with sign and whitespace", () => {
        let p1 = tryParseI64("0");
        expect_eq(p1.ok, true);
        expect_eq(p1.value_i64, 0 as i64);

        let p2 = tryParseI64("  +123456  ");
        expect_eq(p2.ok, true);
        expect_eq(p2.value_i64, 123456 as i64);

        let p3 = tryParseI64("\t-0042\n");
        expect_eq(p3.ok, true);
        expect_eq(p3.value_i64, -42 as i64);

        let p4 = tryParseI64("9876543210");
        expect_eq(p4.ok, true);
        expect_eq(p4.value_i64, 9876543210 as i64);
    });

    test("4. @cap/p-try — tryParseI64 i64 limits, overflow & invalid inputs", () => {
        let pmax = tryParseI64("9223372036854775807");
        expect_eq(pmax.ok, true);
        expect_eq(pmax.value_i64, 9223372036854775807 as i64);

        let pmin = tryParseI64("-9223372036854775808");
        expect_eq(pmin.ok, true);
        expect_eq(pmin.value_i64, (-9223372036854775807 as i64) - (1 as i64));

        let ovf_pos = tryParseI64("9223372036854775808");
        expect_eq(ovf_pos.ok, false);
        expect_eq(ovf_pos.err_code, 3 as i32);

        let ovf_neg = tryParseI64("-9223372036854775809");
        expect_eq(ovf_neg.ok, false);
        expect_eq(ovf_neg.err_code, 3 as i32);

        let empty = tryParseI64("   ");
        expect_eq(empty.ok, false);
        expect_eq(empty.err_code, 1 as i32);

        let bad_sign = tryParseI64("-");
        expect_eq(bad_sign.ok, false);
        expect_eq(bad_sign.err_code, 2 as i32);

        let bad_chars = tryParseI64("12a3");
        expect_eq(bad_chars.ok, false);
        expect_eq(bad_chars.err_code, 2 as i32);
    });

    test("5. @cap/p-try — tryDivI64 normal division, div-by-zero & MIN/-1 overflow", () => {
        let d1 = tryDivI64(100, 4);
        expect_eq(d1.ok, true);
        expect_eq(d1.value_i64, 25 as i64);

        let d2 = tryDivI64(-45, 9);
        expect_eq(d2.ok, true);
        expect_eq(d2.value_i64, -5 as i64);

        let d_zero = tryDivI64(10, 0);
        expect_eq(d_zero.ok, false);
        expect_eq(d_zero.err_code, 4 as i32);
        expect_eq(d_zero.err_msg, "division by zero");

        let min_i64: i64 = (-9223372036854775807 as i64) - (1 as i64);
        let d_ovf = tryDivI64(min_i64, -1);
        expect_eq(d_ovf.ok, false);
        expect_eq(d_ovf.err_code, 3 as i32);
        expect_eq(d_ovf.err_msg, "integer overflow");
    });

    test("6. @cap/p-try — tryChain propagation & short-circuit error preservation", () => {
        let step1 = tryParseI64("84");
        let half = tryDivI64(step1.value_i64, 2);
        let step2 = tryChain(step1, half.ok, half.value_i64, half.err_code, half.err_msg);
        expect_eq(step2.ok, true);
        expect_eq(step2.value_i64, 42 as i64);

        let step3 = tryChain(step2, false, 0, 400, "downstream rejected");
        expect_eq(step3.ok, false);
        expect_eq(step3.err_code, 400 as i32);
        expect_eq(step3.err_msg, "downstream rejected");

        // Short-circuit: prior error is preserved regardless of next step flags
        let step4 = tryChain(step3, true, 999, 500, "ignored");
        expect_eq(step4.ok, false);
        expect_eq(step4.err_code, 400 as i32);
        expect_eq(step4.err_msg, "downstream rejected");

        // Chain starting from a parse error
        let bad = tryParseI64("not-a-num");
        let chained = tryChain(bad, true, 1, 0, "");
        expect_eq(chained.ok, false);
        expect_eq(chained.err_code, 2 as i32);
        expect_eq(unwrapOrInt(chained, -1), -1 as i64);
    });
}
