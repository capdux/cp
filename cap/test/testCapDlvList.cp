import { test, expect_eq } from "cap:test";
import {
    DLV_NONE,
    DLV_CAP,
    DlvListI64,
} from "../../packages/@cap/dlv-list/src/index.cp";

fn main() {
    test("1. @cap/dlv-list — empty list state and pop/front/back return null", () => {
        let list = DlvListI64.new();
        expect_eq(list.len(), 0 as usize);
        expect_eq(list.isEmpty(), true);
        expect_eq(list.front() == null, true);
        expect_eq(list.back() == null, true);
        expect_eq(list.popFront() == null, true);
        expect_eq(list.popBack() == null, true);
        expect_eq(list.get(0) == null, true);
    });

    test("2. @cap/dlv-list — pushFront and pushBack maintain doubly linked order", () => {
        let list = DlvListI64.new();
        const i1 = list.pushBack(20);
        const i0 = list.pushFront(10);
        const i2 = list.pushBack(30);
        expect_eq(list.len(), 3 as usize);
        expect_eq(list.isEmpty(), false);
        expect_eq(list.front().?, 10 as i64);
        expect_eq(list.back().?, 30 as i64);
        expect_eq(list.get(i0).?, 10 as i64);
        expect_eq(list.get(i1).?, 20 as i64);
        expect_eq(list.get(i2).?, 30 as i64);
    });

    test("3. @cap/dlv-list — popFront and popBack drain in exact order", () => {
        let list = DlvListI64.new();
        * = list.pushBack(100);
        * = list.pushBack(200);
        * = list.pushBack(300);
        expect_eq(list.popFront().?, 100 as i64);
        expect_eq(list.popBack().?, 300 as i64);
        expect_eq(list.len(), 1 as usize);
        expect_eq(list.front().?, 200 as i64);
        expect_eq(list.back().?, 200 as i64);
        expect_eq(list.popFront().?, 200 as i64);
        expect_eq(list.isEmpty(), true);
        expect_eq(list.popBack() == null, true);
    });

    test("4. @cap/dlv-list — remove middle handle preserves neighbors and invalidates handle", () => {
        let list = DlvListI64.new();
        const h1 = list.pushBack(1);
        const h2 = list.pushBack(2);
        const h3 = list.pushBack(3);
        expect_eq(list.remove(h2).?, 2 as i64);
        expect_eq(list.get(h2) == null, true);
        expect_eq(list.remove(h2) == null, true);
        expect_eq(list.len(), 2 as usize);
        expect_eq(list.get(h1).?, 1 as i64);
        expect_eq(list.get(h3).?, 3 as i64);
        expect_eq(list.popFront().?, 1 as i64);
        expect_eq(list.popFront().?, 3 as i64);
    });

    test("5. @cap/dlv-list — removed slots are reused by subsequent pushes", () => {
        let list = DlvListI64.new();
        const h0 = list.pushBack(10);
        const h1 = list.pushBack(20);
        const h2 = list.pushBack(30);
        expect_eq(list.remove(h1).?, 20 as i64);
        const reused = list.pushBack(99);
        expect_eq(reused, h1);
        expect_eq(list.get(reused).?, 99 as i64);
        expect_eq(list.get(h0).?, 10 as i64);
        expect_eq(list.get(h2).?, 30 as i64);
        expect_eq(list.len(), 3 as usize);
    });

    test("6. @cap/dlv-list — capacity boundary returns DLV_NONE and recovers after removal", () => {
        let list = DlvListI64.new();
        let i: usize = 0;
        while (i < DLV_CAP) : (i += 1) {
            const h = list.pushBack(i as i64);
            expect_eq(h != DLV_NONE, true);
        }
        expect_eq(list.len(), DLV_CAP);
        expect_eq(list.pushBack(999), DLV_NONE);
        expect_eq(list.pushFront(999), DLV_NONE);
        expect_eq(list.popFront().?, 0 as i64);
        const recovered = list.pushBack(777);
        expect_eq(recovered != DLV_NONE, true);
        expect_eq(list.back().?, 777 as i64);
        expect_eq(list.len(), DLV_CAP);
    });
}
