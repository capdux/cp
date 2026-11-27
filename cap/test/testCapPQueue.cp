import { test, expect_eq } from "cap:test";
import { TaskQueue, QueueTask, PQ_MAX_TASKS } from "../../packages/p-queue/src/index.cp";

fn main() {
    test("1. @cap/p-queue — Priority ordering with FIFO tie-breaking", () => {
        let q = TaskQueue.new(4);
        let z = QueueTask { id: 0, priority: 0, seq: 0 };
        q.enqueue(101, 0);
        q.enqueue(102, 5);
        q.enqueue(103, 5);
        q.enqueue(104, 10);
        expect_eq(q.size(), 4);
        expect_eq((q.dispatchNext() ?? z).id, 104);
        expect_eq((q.dispatchNext() ?? z).id, 102);
        expect_eq((q.dispatchNext() ?? z).id, 103);
        expect_eq((q.dispatchNext() ?? z).id, 101);
    });

    test("2. @cap/p-queue — Enforces max_concurrency and frees slot on markCompleted", () => {
        let q = TaskQueue.new(2);
        q.enqueue(1, 0);
        q.enqueue(2, 0);
        q.enqueue(3, 0);
        expect_eq(q.dispatchNext() != null, true);
        expect_eq(q.dispatchNext() != null, true);
        expect_eq(q.activeCount(), 2);
        expect_eq(q.dispatchNext() == null, true); // concurrency cap reached
        expect_eq(q.markCompleted(), true);
        expect_eq(q.activeCount(), 1);
        expect_eq(q.completedCount(), 1);
        expect_eq(q.dispatchNext() != null, true);
    });

    test("3. @cap/p-queue — pause & start control dispatching", () => {
        let q = TaskQueue.new(2);
        q.enqueue(10, 1);
        q.pause();
        expect_eq(q.isPaused(), true);
        expect_eq(q.dispatchNext() == null, true);
        q.start();
        expect_eq(q.isPaused(), false);
        expect_eq(q.dispatchNext() != null, true);
    });

    test("4. @cap/p-queue — isIdle tracks both pending and active tasks", () => {
        let q = TaskQueue.new(1);
        expect_eq(q.isIdle(), true);
        q.enqueue(7, 0);
        expect_eq(q.isIdle(), false);
        q.dispatchNext();
        expect_eq(q.isIdle(), false); // still 1 active
        q.markCompleted();
        expect_eq(q.isIdle(), true);
    });

    test("5. @cap/p-queue — sizeByPriority & clear", () => {
        let q = TaskQueue.new(2);
        q.enqueue(1, 1);
        q.enqueue(2, 5);
        q.enqueue(3, 10);
        expect_eq(q.sizeByPriority(5), 2);
        expect_eq(q.sizeByPriority(10), 1);
        q.clear();
        expect_eq(q.size(), 0);
    });

    test("6. @cap/p-queue — Full queue capacity protection", () => {
        let q = TaskQueue.new(1);
        let i: usize = 0;
        while (i < PQ_MAX_TASKS) {
            expect_eq(q.enqueue(i as u64, 0), true);
            i = i + 1;
        }
        expect_eq(q.enqueue(999, 0), false);
    });
}
