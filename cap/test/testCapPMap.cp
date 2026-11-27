import { test, expect_eq } from "cap:test";
import {
    BatchSlice, ConcurrencyPool, batchCount, batchRange, workerPartition
} from "../../packages/p-map/src/index.cp";

fn main() {
    test("1. @cap/p-map — batchCount & batchRange pagination", () => {
        let z = BatchSlice { start: 0, end: 0 };
        expect_eq(batchCount(10, 3), 4);
        expect_eq(batchCount(0, 3), 0);
        let b0 = batchRange(10, 3, 0) ?? z;
        let b3 = batchRange(10, 3, 3) ?? z;
        expect_eq(b0.start, 0);
        expect_eq(b0.end, 3);
        expect_eq(b3.start, 9);
        expect_eq(b3.end, 10);
        expect_eq(batchRange(10, 3, 4) == null, true);
    });

    test("2. @cap/p-map — workerPartition balanced sharding (+/- 1 item)", () => {
        let z = BatchSlice { start: 0, end: 0 };
        let w0 = workerPartition(10, 3, 0) ?? z; // 4 items: 0..4
        let w1 = workerPartition(10, 3, 1) ?? z; // 3 items: 4..7
        let w2 = workerPartition(10, 3, 2) ?? z; // 3 items: 7..10
        expect_eq(w0.start, 0); expect_eq(w0.end, 4);
        expect_eq(w1.start, 4); expect_eq(w1.end, 7);
        expect_eq(w2.start, 7); expect_eq(w2.end, 10);
        expect_eq(workerPartition(10, 3, 3) == null, true);
    });

    test("3. @cap/p-map — ConcurrencyPool respects max_concurrency", () => {
        let pool = ConcurrencyPool.new(5, 2, true);
        expect_eq(pool.acquireNext() ?? 99, 0);
        expect_eq(pool.acquireNext() ?? 99, 1);
        expect_eq(pool.inFlight(), 2);
        expect_eq(pool.acquireNext() == null, true);
        expect_eq(pool.reportOk(), true);
        expect_eq(pool.acquireNext() ?? 99, 2);
    });

    test("4. @cap/p-map — stop_on_error halts new dispatches while draining in-flight", () => {
        let pool = ConcurrencyPool.new(5, 2, true);
        pool.acquireNext(); // 0
        pool.acquireNext(); // 1
        pool.reportErr();   // 0 failed -> halts
        expect_eq(pool.isDone(), false); // 1 still in-flight
        expect_eq(pool.acquireNext() == null, true);
        pool.reportOk();    // 1 finished
        expect_eq(pool.isDone(), true);
        expect_eq(pool.failedCount(), 1);
        expect_eq(pool.completedCount(), 1);
    });

    test("5. @cap/p-map — stop_on_error=false continues all remaining items", () => {
        let pool = ConcurrencyPool.new(3, 1, false);
        expect_eq(pool.acquireNext() ?? 99, 0);
        pool.reportErr();
        expect_eq(pool.acquireNext() ?? 99, 1);
        pool.reportOk();
        expect_eq(pool.acquireNext() ?? 99, 2);
        pool.reportOk();
        expect_eq(pool.isDone(), true);
        expect_eq(pool.completedCount(), 2);
        expect_eq(pool.failedCount(), 1);
    });

    test("6. @cap/p-map — Empty pool is immediately done", () => {
        let pool = ConcurrencyPool.new(0, 4, true);
        expect_eq(pool.isDone(), true);
        expect_eq(pool.acquireNext() == null, true);
        expect_eq(pool.reportOk(), false);
    });
}
