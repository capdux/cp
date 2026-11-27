import { test, expect_eq } from "cap:test";
import { RangedI64 } from "../../packages/deranged/src/index.cp";

fn main() {
    test("new valid & out-of-bounds rejection", () => {
        let m = RangedI64.new(6, 1, 12);
        expect_eq(m.ok, true);
        expect_eq(m.get(), 6 as i64);
        let bad = RangedI64.new(13, 1, 12);
        expect_eq(bad.ok, false);
        let rev = RangedI64.new(5, 10, 1);
        expect_eq(rev.ok, false);
    });

    test("newSaturating clamps to bounds", () => {
        let lo = RangedI64.newSaturating(-5, 0, 59);
        let hi = RangedI64.newSaturating(99, 0, 59);
        expect_eq(lo.get(), 0 as i64);
        expect_eq(hi.get(), 59 as i64);
    });

    test("checkedAdd & checkedSub", () => {
        let h = RangedI64.new(20, 0, 23);
        let h2 = h.checkedAdd(3);
        expect_eq(h2.ok, true);
        expect_eq(h2.get(), 23 as i64);
        let h3 = h.checkedAdd(4);
        expect_eq(h3.ok, false);
        let h4 = h.checkedSub(21);
        expect_eq(h4.ok, false);
    });

    test("saturatingAdd & saturatingSub", () => {
        let pct = RangedI64.new(80, 0, 100);
        expect_eq(pct.saturatingAdd(50).get(), 100 as i64);
        expect_eq(pct.saturatingSub(120).get(), 0 as i64);
    });

    test("wrappingAdd cyclic clock arithmetic", () => {
        let hr = RangedI64.new(22, 0, 23);
        expect_eq(hr.wrappingAdd(5).get(), 3 as i64);
        expect_eq(hr.wrappingAdd(-25).get(), 21 as i64);
    });

    test("expand & narrow range bounds", () => {
        let d = RangedI64.new(15, 1, 31);
        let wide = d.expand(0, 366);
        expect_eq(wide.min_v, 0 as i64);
        expect_eq(wide.max_v, 366 as i64);
        expect_eq(wide.narrow(1, 20).ok, true);
        expect_eq(wide.narrow(1, 10).ok, false);
    });
}
