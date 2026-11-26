import { test, expect_eq } from "cap:test";
import {
    P_SETTLE_CAP,
    SettleStatus,
    SettledEntry,
    PSettlePlanner,
} from "../../packages/@cap/p-settle/src/index.cp";

fn main() {
    test("1. @cap/p-settle — Invalid concurrency=0 returns null and empty planner isAllSettled", () => {
        const bad: PSettlePlanner? = PSettlePlanner.new(0);
        expect_eq(bad == null, true);

        const opt: PSettlePlanner? = PSettlePlanner.new(3);
        expect_eq(opt != null, true);
        let p: PSettlePlanner = opt.?;
        expect_eq(p.concurrency(), 3);
        expect_eq(p.capacity(), P_SETTLE_CAP);
        expect_eq(p.isAllSettled(), true);
        expect_eq(p.fulfilledCount(), 0);
        expect_eq(p.rejectedCount(), 0);
        expect_eq(p.entryAt(0) == null, true);
    });

    test("2. @cap/p-settle — Mixed fulfilled and rejected tasks never short-circuit", () => {
        let p: PSettlePlanner = PSettlePlanner.new(2).?;
        expect_eq(p.addTask(10), true);
        expect_eq(p.addTask(20), true);
        expect_eq(p.addTask(30), true);
        expect_eq(p.addTask(40), true);
        expect_eq(p.isAllSettled(), false);

        expect_eq(p.dispatchNext().?, 10);
        expect_eq(p.dispatchNext().?, 20);
        expect_eq(p.dispatchNext() == null, true);

        // Reject task 10 first -> remaining tasks 30 & 40 MUST still be dispatched
        expect_eq(p.reportRejected(10, -500), true);
        expect_eq(p.pendingCount(), 2);
        expect_eq(p.isAllSettled(), false);

        expect_eq(p.dispatchNext().?, 30);
        expect_eq(p.reportFulfilled(20, 2000), true);
        expect_eq(p.dispatchNext().?, 40);
        expect_eq(p.reportRejected(30, -404), true);
        expect_eq(p.reportFulfilled(40, 4000), true);

        expect_eq(p.isAllSettled(), true);
        expect_eq(p.fulfilledCount(), 2);
        expect_eq(p.rejectedCount(), 2);
        expect_eq(p.activeCount(), 0);
        expect_eq(p.pendingCount(), 0);
    });

    test("3. @cap/p-settle — entryAt() preserves registration order regardless of completion order", () => {
        let p: PSettlePlanner = PSettlePlanner.new(3).?;
        expect_eq(p.addTask(100), true);
        expect_eq(p.addTask(200), true);
        expect_eq(p.addTask(300), true);

        expect_eq(p.dispatchNext().?, 100);
        expect_eq(p.dispatchNext().?, 200);
        expect_eq(p.dispatchNext().?, 300);

        // Complete in reverse order
        expect_eq(p.reportFulfilled(300, 33), true);
        expect_eq(p.reportRejected(200, -22), true);
        expect_eq(p.reportFulfilled(100, 11), true);

        const e0: SettledEntry = p.entryAt(0).?;
        const e1: SettledEntry = p.entryAt(1).?;
        const e2: SettledEntry = p.entryAt(2).?;
        expect_eq(p.entryAt(3) == null, true);

        expect_eq(e0.id, 100);
        expect_eq(e0.status == SettleStatus.Fulfilled, true);
        expect_eq(e0.value, 11);
        expect_eq(e0.err_code, 0);

        expect_eq(e1.id, 200);
        expect_eq(e1.status == SettleStatus.Rejected, true);
        expect_eq(e1.value, 0);
        expect_eq(e1.err_code, -22);

        expect_eq(e2.id, 300);
        expect_eq(e2.status == SettleStatus.Fulfilled, true);
        expect_eq(e2.value, 33);
        expect_eq(e2.err_code, 0);
    });

    test("4. @cap/p-settle — Pending status visible before settlement", () => {
        let p: PSettlePlanner = PSettlePlanner.new(1).?;
        expect_eq(p.addTask(1), true);
        expect_eq(p.addTask(2), true);

        const pre0: SettledEntry = p.entryAt(0).?;
        expect_eq(pre0.status == SettleStatus.Pending, true);

        expect_eq(p.dispatchNext().?, 1);
        const mid0: SettledEntry = p.entryAt(0).?;
        expect_eq(mid0.status == SettleStatus.Pending, true);

        expect_eq(p.reportRejected(1, -1), true);
        const post0: SettledEntry = p.entryAt(0).?;
        expect_eq(post0.status == SettleStatus.Rejected, true);
        expect_eq(post0.err_code, -1);

        expect_eq(p.dispatchNext().?, 2);
        expect_eq(p.reportFulfilled(2, 42), true);
        expect_eq(p.isAllSettled(), true);
    });

    test("5. @cap/p-settle — Duplicate IDs and invalid state transitions rejected", () => {
        let p: PSettlePlanner = PSettlePlanner.new(2).?;
        expect_eq(p.addTask(55), true);
        expect_eq(p.addTask(55), false);

        // Cannot settle before dispatch
        expect_eq(p.reportFulfilled(55, 1), false);
        expect_eq(p.reportRejected(55, -1), false);
        expect_eq(p.reportFulfilled(999, 1), false);

        expect_eq(p.dispatchNext().?, 55);
        expect_eq(p.reportFulfilled(55, 555), true);
        // Double settlement rejected
        expect_eq(p.reportFulfilled(55, 666), false);
        expect_eq(p.reportRejected(55, -9), false);

        const e: SettledEntry = p.entryOf(55).?;
        expect_eq(e.status == SettleStatus.Fulfilled, true);
        expect_eq(e.value, 555);
    });

    test("6. @cap/p-settle — 32-slot fixed capacity and all-rejected batch", () => {
        let p: PSettlePlanner = PSettlePlanner.new(4).?;
        let i: usize = 0;
        while (i < P_SETTLE_CAP) : (i += 1) {
            expect_eq(p.addTask((i + 1) as u64), true);
        }
        expect_eq(p.totalCount(), P_SETTLE_CAP);
        expect_eq(p.addTask(9999), false);

        let settled: usize = 0;
        while (settled < P_SETTLE_CAP) : (settled += 1) {
            const tid: u64 = p.dispatchNext().?;
            expect_eq(p.reportRejected(tid, -(tid as i32)), true);
        }
        expect_eq(p.isAllSettled(), true);
        expect_eq(p.fulfilledCount(), 0);
        expect_eq(p.rejectedCount(), P_SETTLE_CAP);
        const last: SettledEntry = p.entryAt(31).?;
        expect_eq(last.id, 32);
        expect_eq(last.err_code, -32);
    });
}
