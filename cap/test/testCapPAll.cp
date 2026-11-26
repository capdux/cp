import { test, expect_eq } from "cap:test";
import { P_ALL_CAP, PAllPlanner } from "../../packages/@cap/p-all/src/index.cp";

fn main() {
    test("1. @cap/p-all — Invalid concurrency=0 returns null and empty planner isDone", () => {
        const bad: PAllPlanner? = PAllPlanner.new(0, true);
        expect_eq(bad == null, true);

        const opt: PAllPlanner? = PAllPlanner.new(2, true);
        expect_eq(opt != null, true);
        let p: PAllPlanner = opt.?;
        expect_eq(p.concurrency(), 2);
        expect_eq(p.capacity(), P_ALL_CAP);
        expect_eq(p.isDone(), true);
        expect_eq(p.hasFailed(), false);
        expect_eq(p.firstErrorCode(), 0);
        expect_eq(p.completedCount(), 0);
    });

    test("2. @cap/p-all — Bounded concurrency dispatch and full success collection", () => {
        let p: PAllPlanner = PAllPlanner.new(2, true).?;
        expect_eq(p.addTask(10), true);
        expect_eq(p.addTask(20), true);
        expect_eq(p.addTask(30), true);
        expect_eq(p.isDone(), false);

        // Only 2 tasks can be dispatched concurrently
        expect_eq(p.dispatchNext().?, 10);
        expect_eq(p.dispatchNext().?, 20);
        expect_eq(p.dispatchNext() == null, true);
        expect_eq(p.activeCount(), 2);
        expect_eq(p.pendingCount(), 1);

        // Complete task 20 out-of-order, then dispatch task 30
        expect_eq(p.reportSuccess(20, 200), true);
        expect_eq(p.dispatchNext().?, 30);
        expect_eq(p.reportSuccess(10, 100), true);
        expect_eq(p.reportSuccess(30, 300), true);

        expect_eq(p.isDone(), true);
        expect_eq(p.hasFailed(), false);
        expect_eq(p.completedCount(), 3);
        expect_eq(p.resultOf(10).?, 100);
        expect_eq(p.resultOf(20).?, 200);
        expect_eq(p.resultOf(30).?, 300);
        expect_eq(p.resultOf(999) == null, true);
    });

    test("3. @cap/p-all — stop_on_error=true immediately aborts and clears pending queue", () => {
        let p: PAllPlanner = PAllPlanner.new(2, true).?;
        expect_eq(p.addTask(1), true);
        expect_eq(p.addTask(2), true);
        expect_eq(p.addTask(3), true);
        expect_eq(p.addTask(4), true);

        expect_eq(p.dispatchNext().?, 1);
        expect_eq(p.dispatchNext().?, 2);
        expect_eq(p.pendingCount(), 2);

        // Task 1 fails -> queue cleared, planner marked failed & done, no further dispatch
        expect_eq(p.reportError(1, -42), true);
        expect_eq(p.hasFailed(), true);
        expect_eq(p.firstErrorCode(), -42);
        expect_eq(p.pendingCount(), 0);
        expect_eq(p.isDone(), true);
        expect_eq(p.dispatchNext() == null, true);
        expect_eq(p.addTask(5), false);
        expect_eq(p.resultOf(1) == null, true);

        // In-flight task 2 can still report completion safely without crashing
        expect_eq(p.reportSuccess(2, 22), true);
        expect_eq(p.activeCount(), 0);
        expect_eq(p.completedCount(), 1);
        expect_eq(p.resultOf(2).?, 22);
    });

    test("4. @cap/p-all — stop_on_error=false continues all tasks and retains firstErrorCode", () => {
        let p: PAllPlanner = PAllPlanner.new(2, false).?;
        expect_eq(p.addTask(101), true);
        expect_eq(p.addTask(102), true);
        expect_eq(p.addTask(103), true);
        expect_eq(p.addTask(104), true);

        expect_eq(p.dispatchNext().?, 101);
        expect_eq(p.dispatchNext().?, 102);

        // First failure does NOT clear queue or stop remaining tasks
        expect_eq(p.reportError(101, 500), true);
        expect_eq(p.hasFailed(), true);
        expect_eq(p.firstErrorCode(), 500);
        expect_eq(p.isDone(), false);
        expect_eq(p.pendingCount(), 2);

        expect_eq(p.dispatchNext().?, 103);
        expect_eq(p.reportSuccess(102, 2020), true);
        expect_eq(p.dispatchNext().?, 104);
        expect_eq(p.reportError(103, 404), true);
        expect_eq(p.reportSuccess(104, 4040), true);

        expect_eq(p.isDone(), true);
        expect_eq(p.firstErrorCode(), 500);
        expect_eq(p.completedCount(), 2);
        expect_eq(p.errorCount(), 2);
        expect_eq(p.resultOf(101) == null, true);
        expect_eq(p.resultOf(102).?, 2020);
        expect_eq(p.resultOf(103) == null, true);
        expect_eq(p.resultOf(104).?, 4040);
    });

    test("5. @cap/p-all — Duplicate task IDs and invalid state transitions are rejected", () => {
        let p: PAllPlanner = PAllPlanner.new(1, true).?;
        expect_eq(p.addTask(7), true);
        expect_eq(p.addTask(7), false);

        // Cannot report before dispatch
        expect_eq(p.reportSuccess(7, 1), false);
        expect_eq(p.reportError(7, -1), false);
        expect_eq(p.reportSuccess(99, 1), false);

        expect_eq(p.dispatchNext().?, 7);
        expect_eq(p.reportSuccess(7, 777), true);
        // Double reporting is rejected
        expect_eq(p.reportSuccess(7, 888), false);
        expect_eq(p.reportError(7, -2), false);
        expect_eq(p.resultOf(7).?, 777);
    });

    test("6. @cap/p-all — 32-slot fixed capacity enforcement", () => {
        let p: PAllPlanner = PAllPlanner.new(4, true).?;
        let i: usize = 0;
        while (i < P_ALL_CAP) : (i += 1) {
            expect_eq(p.addTask((i + 1) as u64), true);
        }
        expect_eq(p.totalCount(), P_ALL_CAP);
        // 33rd task must be rejected cleanly
        expect_eq(p.addTask(9999), false);

        let done: usize = 0;
        while (done < P_ALL_CAP) : (done += 1) {
            const tid: u64 = p.dispatchNext().?;
            expect_eq(p.reportSuccess(tid, (tid as i64) * 10), true);
        }
        expect_eq(p.isDone(), true);
        expect_eq(p.completedCount(), P_ALL_CAP);
        expect_eq(p.resultOf(32).?, 320);
    });
}
