import { test, expect_eq } from "cap:test";
import {
    RangedI64,
    RangedU8,
    RangedU16,
    RangedU32,
    RangedU64,
    RangedI8,
    RangedI16,
    RangedI32,
} from "../../packages/deranged/src/index.cp";

fn main() {
    test("new valid & out-of-bounds rejection", () => {
        let m = RangedI64.new(6, 1, 12);
        expect_eq(m.ok, true);
        expect_eq(m.isValid(), true);
        expect_eq(m.get(), 6 as i64);
        expect_eq(m.min(), 1 as i64);
        expect_eq(m.max(), 12 as i64);
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

    test("wrappingAdd & wrappingSub cyclic clock arithmetic", () => {
        let hr = RangedI64.new(22, 0, 23);
        expect_eq(hr.wrappingAdd(5).get(), 3 as i64);
        expect_eq(hr.wrappingAdd(-25).get(), 21 as i64);
        expect_eq(hr.wrappingSub(25).get(), 21 as i64);
    });

    test("expand & narrow range bounds", () => {
        let d = RangedI64.new(15, 1, 31);
        let wide = d.expand(0, 366);
        expect_eq(wide.min_v, 0 as i64);
        expect_eq(wide.max_v, 366 as i64);
        expect_eq(wide.narrow(1, 20).ok, true);
        expect_eq(wide.narrow(1, 10).ok, false);
    });

    test("RangedU8 & RangedU16 overflow-safe checked & saturating ops", () => {
        const day = RangedU8.new(28, 1, 31);
        expect_eq(day.ok, true);
        expect_eq(day.min(), 1 as u8);
        expect_eq(day.max(), 31 as u8);
        expect_eq(day.checkedAdd(3).get(), 31 as u8);
        expect_eq(day.checkedAdd(4).ok, false);
        expect_eq(day.checkedSub(28).ok, false);
        expect_eq(day.saturatingAdd(250).get(), 31 as u8);
        expect_eq(day.saturatingSub(200).get(), 1 as u8);

        const port = RangedU16.newSaturating(70000 as u16, 1024, 65535);
        expect_eq(port.inBounds(8080), true);
        const p2 = RangedU16.new(65530, 1024, 65535);
        expect_eq(p2.checkedAdd(5).get(), 65535 as u16);
        expect_eq(p2.checkedAdd(10).ok, false);
        expect_eq(p2.saturatingAdd(100).get(), 65535 as u16);
    });

    test("RangedU32, RangedU64, RangedI8, RangedI16 & RangedI32", () => {
        const u32v = RangedU32.new(100, 10, 200);
        expect_eq(u32v.checkedSub(90).get(), 10 as u32);
        expect_eq(u32v.checkedSub(91).ok, false);
        expect_eq(u32v.saturatingAdd(500).get(), 200 as u32);

        const u64v = RangedU64.new(1000, 500, 2000);
        expect_eq(u64v.checkedAdd(1000).get(), 2000 as u64);
        expect_eq(u64v.checkedAdd(1001).ok, false);
        expect_eq(u64v.saturatingSub(9999).get(), 500 as u64);

        const i8v = RangedI8.new(-10, -20, 20);
        expect_eq(i8v.checkedSub(10).get(), -20 as i8);
        expect_eq(i8v.checkedSub(11).ok, false);
        expect_eq(i8v.saturatingAdd(100).get(), 20 as i8);

        const i16v = RangedI16.new(500, -1000, 1000);
        expect_eq(i16v.narrow(0, 600).ok, true);
        expect_eq(i16v.narrow(0, 400).ok, false);

        const i32v = RangedI32.new(350, 0, 359);
        expect_eq(i32v.wrappingAdd(20).get(), 10 as i32);
        expect_eq(i32v.wrappingSub(360).get(), 350 as i32);
        expect_eq(i32v.checkedAdd(10).ok, false);
    });
}
