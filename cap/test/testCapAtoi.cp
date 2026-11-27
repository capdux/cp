import { test, expect_eq } from "cap:test";
import {
    fromRadixU64, fromRadix10U64, fromRadix16U64,
    fromRadixI64, fromRadix10I64, atoiU64, atoiI64
} from "../../packages/atoi/src/index.cp";

fn main() {
    test("fromRadix10U64 parses prefix and stops at non-digit", () => {
        let r = fromRadix10U64("12345abc");
        expect_eq(r.ok, true);
        expect_eq(r.value, 12345 as u64);
        expect_eq(r.used, 5 as usize);
        expect_eq(atoiU64("12345abc").ok, false);
        expect_eq(atoiU64("12345").value, 12345 as u64);
    });

    test("fromRadix16U64 & arbitrary radix 2/36", () => {
        let h = fromRadix16U64("deadBEEF!");
        expect_eq(h.ok, true);
        expect_eq(h.value, 0xdeadbeef as u64);
        expect_eq(h.used, 8 as usize);
        let b = fromRadixU64("101101", 2);
        expect_eq(b.ok, true);
        expect_eq(b.value, 45 as u64);
        let r36 = fromRadixU64("zz", 36);
        expect_eq(r36.ok, true);
        expect_eq(r36.value, 1295 as u64);
    });

    test("fromRadix10I64 signed values & exact i64::MIN", () => {
        let p = fromRadix10I64("+42ms");
        expect_eq(p.ok, true);
        expect_eq(p.value, 42 as i64);
        expect_eq(p.used, 3 as usize);
        let n = atoiI64("-9223372036854775808");
        expect_eq(n.ok, true);
        let expected_min: i64 = -9223372036854775807 - 1;
        expect_eq(n.value, expected_min);
    });

    test("u64 & i64 overflow rejection", () => {
        expect_eq(atoiU64("18446744073709551615").ok, true);
        expect_eq(atoiU64("18446744073709551616").ok, false);
        expect_eq(atoiI64("9223372036854775807").ok, true);
        expect_eq(atoiI64("9223372036854775808").ok, false);
        expect_eq(atoiI64("-9223372036854775809").ok, false);
    });

    test("rejects empty, sign-only & invalid radix", () => {
        expect_eq(fromRadix10U64("").ok, false);
        expect_eq(fromRadix10I64("-").ok, false);
        expect_eq(fromRadix10I64("+").ok, false);
        expect_eq(fromRadixU64("123", 1).ok, false);
        expect_eq(fromRadixU64("123", 37).ok, false);
    });

    test("fromRadixI64 negative hex", () => {
        let r = fromRadixI64("-ff", 16);
        expect_eq(r.ok, true);
        expect_eq(r.value, -255 as i64);
        expect_eq(r.used, 3 as usize);
    });
}
