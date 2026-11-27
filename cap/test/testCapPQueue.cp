import { test, expect_eq } from "cap:test";
import { TaskQueue, QueueTask, QueueOptions, PQ_MAX_TASKS } from "../../packages/p-queue/src/index.cp";

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
        expect_eq(q.pending(), 2);
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
        expect_eq(q.onEmpty(), true);
        expect_eq(q.onIdle(), true);
        q.enqueue(7, 0);
        expect_eq(q.isIdle(), false);
        expect_eq(q.onEmpty(), false);
        q.dispatchNext();
        expect_eq(q.onEmpty(), true);
        expect_eq(q.isIdle(), false); // still 1 active
        q.markCompleted();
        expect_eq(q.isIdle(), true);
        expect_eq(q.onIdle(), true);
    });

    test("5. @cap/p-queue — sizeByPriority & clear", () => {
        let q = TaskQueue.new(2);
        q.enqueue(1, 1);
        q.enqueue(2, 5);
        q.enqueue(3, 10);
        expect_eq(q.sizeByPriority(5), 2);
        expect_eq(q.sizeByPriority(10), 1);
        expect_eq(q.sizeByExactPriority(5), 1);
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

    test("7. @cap/p-queue — withOptions, concurrency, setConcurrency, add, addBatch", () => {
        let opts = QueueOptions.default();
        opts.concurrency = 2;
        opts.auto_start = false;
        let q = TaskQueue.withOptions(opts);
        expect_eq(q.isPaused(), true);
        expect_eq(q.concurrency(), 2 as usize);

        const ids: [3]u64 = [11, 12, 13];
        expect_eq(q.addBatch(ids[0..3], 2), 3 as usize);
        expect_eq(q.add(14, 5), true);
        expect_eq(q.canDispatch(), false);
        expect_eq(q.onSizeLessThan(5), true);
        expect_eq(q.onSizeLessThan(4), false);

        q.start();
        q.setConcurrency(3);
        expect_eq(q.concurrency(), 3 as usize);
        expect_eq(q.canDispatch(), true);

        let batch: [4]QueueTask = undefined;
        const dispatched = q.drainBatch(batch[0..4]);
        expect_eq(dispatched, 3 as usize);
        expect_eq(batch[0].id, 14 as u64);
        expect_eq(q.pending(), 3 as usize);
        expect_eq(q.size(), 1 as usize);
    });

    test("8. @cap/p-queue — peek, remove, setPriority, resetStats", () => {
        let q = TaskQueue.new(2);
        let z = QueueTask { id: 0, priority: 0, seq: 0 };
        q.add(1, 1);
        q.add(2, 2);
        q.add(3, 3);
        expect_eq((q.peek() ?? z).id, 3 as u64);

        expect_eq(q.setPriority(1, 10), true);
        expect_eq(q.setPriority(999, 10), false);
        expect_eq((q.peek() ?? z).id, 1 as u64);

        expect_eq(q.remove(1), true);
        expect_eq(q.remove(999), false);
        expect_eq((q.peek() ?? z).id, 3 as u64);

        q.dispatchNext();
        q.markCompleted();
        expect_eq(q.completedCount(), 1 as u64);
        q.resetStats();
        expect_eq(q.completedCount(), 0 as u64);
    });
}
