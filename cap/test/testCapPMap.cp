import { test, expect_eq } from "cap:test";
import {
    PMAP_STATUS_OK, PMAP_STATUS_SKIP, PMAP_STATUS_ERR,
    BatchSlice, ConcurrencyPool, PMapOptions, PMapErrorEntry, MapOutcome,
    defaultPMapOptions, pMapSkip, mapOk, mapSkip, mapErr,
    batchCount, batchRange, workerPartition, pMap, pMapCollectErrors,
} from "../../packages/p-map/src/index.cp";

fn doubleOrSkipEven(val: i64, idx: usize): MapOutcome {
    if ((val & 1) == 0) {
        return mapSkip();
    }
    return mapOk(val * 10 + (idx as i64));
}

fn failOnNegative(val: i64, idx: usize): MapOutcome {
    if (val < 0) {
        return mapErr((val as i32) - (idx as i32));
    }
    return mapOk(val + 1);
}

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

    test("6. @cap/p-map — Empty pool is immediately done & reportSkip/reset", () => {
        let pool = ConcurrencyPool.new(0, 4, true);
        expect_eq(pool.isDone(), true);
        expect_eq(pool.acquireNext() == null, true);
        expect_eq(pool.reportOk(), false);

        let pool2 = ConcurrencyPool.new(2, 2, true);
        expect_eq(pool2.remainingCount(), 2 as usize);
        pool2.acquireNext();
        expect_eq(pool2.reportSkip(), true);
        expect_eq(pool2.skippedCount(), 1 as usize);
        pool2.reset();
        expect_eq(pool2.skippedCount(), 0 as usize);
        expect_eq(pool2.remainingCount(), 2 as usize);
    });

    test("7. @cap/p-map — pMap with concurrency waves & pMapSkip filtering", () => {
        const items: [5]i64 = [1, 2, 3, 4, 5];
        let out: [5]i64 = [0; 5];
        let opts = defaultPMapOptions();
        opts.concurrency = 2;

        const skip_sentinel = pMapSkip();
        expect_eq(skip_sentinel.marker, PMAP_STATUS_SKIP);

        const summary = pMap(items[0..5], out[0..5], opts, doubleOrSkipEven);
        expect_eq(summary.total, 5 as usize);
        expect_eq(summary.produced, 3 as usize);
        expect_eq(summary.skipped, 2 as usize);
        expect_eq(summary.failed, 0 as usize);
        expect_eq(summary.batches_run, 3 as usize);
        expect_eq(summary.halted, false);
        expect_eq(out[0], 10 as i64); // 1*10 + 0
        expect_eq(out[1], 32 as i64); // 3*10 + 2
        expect_eq(out[2], 54 as i64); // 5*10 + 4
    });

    test("8. @cap/p-map — pMapCollectErrors with stop_on_error=true vs false", () => {
        const items: [5]i64 = [10, -2, 20, -5, 30];
        let out: [5]i64 = [0; 5];
        let errs: [5]PMapErrorEntry = [
            PMapErrorEntry{ index: 0, err_code: 0 },
            PMapErrorEntry{ index: 0, err_code: 0 },
            PMapErrorEntry{ index: 0, err_code: 0 },
            PMapErrorEntry{ index: 0, err_code: 0 },
            PMapErrorEntry{ index: 0, err_code: 0 },
        ];

        // stop_on_error = true with concurrency = 2 -> wave 1 runs [0, 1], halts before wave 2
        const s_stop = pMapCollectErrors(
            items[0..5],
            out[0..5],
            errs[0..5],
            PMapOptions{ concurrency: 2, stop_on_error: true },
            failOnNegative,
        );
        expect_eq(s_stop.halted, true);
        expect_eq(s_stop.failed, 1 as usize);
        expect_eq(s_stop.produced, 1 as usize);
        expect_eq(s_stop.first_error_index ?? 99, 1 as usize);
        expect_eq(s_stop.first_error_code, -3 as i32);

        // stop_on_error = false -> collects all errors across all waves
        const s_all = pMapCollectErrors(
            items[0..5],
            out[0..5],
            errs[0..5],
            PMapOptions{ concurrency: 2, stop_on_error: false },
            failOnNegative,
        );
        expect_eq(s_all.halted, false);
        expect_eq(s_all.produced, 3 as usize);
        expect_eq(s_all.failed, 2 as usize);
        expect_eq(errs[0].index, 1 as usize);
        expect_eq(errs[1].index, 3 as usize);
        expect_eq(errs[1].err_code, -8 as i32);
    });
}
