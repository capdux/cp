import { test, expect, expect_eq } from "cap:test";
import {
    COORDINATOR_MAX_WORKERS,
    COORDINATOR_MAX_CRASHED_FILES,
    COORDINATOR_DEFAULT_SCALE_UP_MS,
    CoordinatorError,
    WorkerSlotState,
    WorkerSlot,
    CoordinatorConfig,
    DispatchDecisionKind,
    DispatchDecision,
    ParallelCoordinator,
} from "cap:test_runner";

test("test_runner coordinator: lazy scale-up delays until all live workers exceed threshold", fn()! {
    expect_eq(COORDINATOR_MAX_WORKERS, 64);
    expect_eq(COORDINATOR_MAX_CRASHED_FILES, 64);
    expect_eq(COORDINATOR_DEFAULT_SCALE_UP_MS, 150);

    let coord = ParallelCoordinator.init(CoordinatorConfig{
        file_count: 10,
        parallel_limit: 4,
        scale_up_after_ms: 150,
    });
    expect(!coord.isDone());
    expect_eq(coord.effective_limit, 4);
    expect_eq(coord.shouldScaleUp(1000), 0); // no worker spawned yet

    const w0 = coord.spawnNextWorker() ?? return err.TestFailed;
    expect_eq(w0, 0);
    expect_eq(coord.live_workers, 1);
    expect_eq(coord.shouldScaleUp(1000), 0); // w0 is still idle

    const d0 = coord.assignWork(w0, 1000)?;
    expect(d0.kind == .DispatchFile);
    expect_eq(d0.file_idx, 0);
    expect(!d0.stolen);

    // Before threshold (1000 + 149): do not scale up yet
    expect_eq(coord.shouldScaleUp(1149), 0);

    // At threshold (1000 + 150): scale up remaining 3 workers
    expect_eq(coord.shouldScaleUp(1150), 3);
    const added = coord.applyScaleUp(1150);
    expect_eq(added, 3);
    expect_eq(coord.spawned_count, 4);
    expect_eq(coord.live_workers, 4);
    expect_eq(coord.shouldScaleUp(2000), 0); // already at parallel_limit
});

test("test_runner coordinator: work-stealing drains unspawned slots and reaches isDone", fn()! {
    // 6 files partitioned across 3 slots ([0..2), [2..4), [4..6)),
    // but only worker 0 is spawned (simulating a fast suite with no scale-up).
    let coord = ParallelCoordinator.init(CoordinatorConfig{
        file_count: 6,
        parallel_limit: 3,
        scale_up_after_ms: 500,
    });
    const w0 = coord.spawnNextWorker() ?? return err.TestFailed;

    let completed: u32 = 0;
    let saw_steal = false;
    let t: i64 = 100;
    while (true) {
        const decision = coord.assignWork(w0, t)?;
        if (decision.kind == .ShutdownWorker) {
            break;
        }
        if (decision.stolen) {
            saw_steal = true;
        }
        const bailed = coord.onTestResult(w0, decision.file_idx, true, false)?;
        expect(!bailed);
        coord.onFileDone(w0, decision.file_idx)?;
        completed += 1;
        t += 10;
    }

    expect_eq(completed, 6);
    expect(saw_steal);
    expect_eq(coord.files_done, 6);
    expect_eq(coord.total_passed, 6);
    expect(!coord.isDone()); // w0 still alive until exited

    const crashed = coord.onWorkerExited(w0, false)?;
    expect(crashed == null);
    expect(coord.isDone());
});

test("test_runner coordinator: --bail=2 triggers early abort and shuts down idle workers", fn()! {
    let coord = ParallelCoordinator.init(CoordinatorConfig{
        file_count: 8,
        parallel_limit: 3,
        scale_up_after_ms: 100,
        bail_threshold: 2,
    });
    const w0 = coord.spawnNextWorker() ?? return err.TestFailed;
    const w1 = coord.spawnNextWorker() ?? return err.TestFailed;
    const w2 = coord.spawnNextWorker() ?? return err.TestFailed;

    const d0 = coord.assignWork(w0, 1000)?;
    const d1 = coord.assignWork(w1, 1000)?;
    // Leave w2 idle

    // First failure does NOT bail yet (threshold = 2)
    const bail1 = coord.onTestResult(w0, d0.file_idx, false, false)?;
    expect(!bail1);
    expect(!coord.bailed);
    expect(coord.workers[w2].state == .Idle);

    // Second failure hits threshold = 2 -> triggers bailOut()
    const bail2 = coord.onTestResult(w1, d1.file_idx, false, false)?;
    expect(bail2);
    expect(coord.bailed);
    expect(coord.workers[w2].state == .ShuttingDown);

    // Completing w0 transitions it to ShuttingDown and next assignWork returns ShutdownWorker
    coord.onFileDone(w0, d0.file_idx)?;
    expect(coord.workers[w0].state == .ShuttingDown);
    const next_d0 = coord.assignWork(w0, 1100)?;
    expect(next_d0.kind == .ShutdownWorker);

    coord.onFileDone(w1, d1.file_idx)?;
    * = coord.onWorkerExited(w0, false)?;
    * = coord.onWorkerExited(w1, false)?;
    * = coord.onWorkerExited(w2, false)?;
    expect(coord.isDone());
    expect_eq(coord.total_failed, 2);
});

test("test_runner coordinator: abnormal worker exit accounts crashed file and triggers bail", fn()! {
    let coord = ParallelCoordinator.init(CoordinatorConfig{
        file_count: 4,
        parallel_limit: 2,
        bail_threshold: 1,
    });
    const w0 = coord.spawnNextWorker() ?? return err.TestFailed;
    const d0 = coord.assignWork(w0, 500)?;

    const crashed = coord.onWorkerExited(w0, true)?;
    expect_eq(crashed ?? 999, d0.file_idx);
    expect_eq(coord.crashed_count, 1);
    expect_eq(coord.crashedFileAt(0) ?? 999, d0.file_idx);
    expect(coord.crashedFileAt(1) == null);
    expect_eq(coord.files_done, 1);
    expect_eq(coord.total_failed, 1);
    expect(coord.bailed);
    expect(coord.isDone());
});

test("test_runner coordinator: interleaved header deduplication and dots mode", fn()! {
    let normal = ParallelCoordinator.init(CoordinatorConfig{
        file_count: 4,
        parallel_limit: 2,
        dots_mode: false,
    });
    expect(normal.shouldEmitHeader(0));
    expect(!normal.shouldEmitHeader(0)); // same file: deduplicated
    expect(normal.shouldEmitHeader(1));  // switched file: re-emit header
    expect(normal.shouldEmitHeader(0));  // switched back: re-emit header

    let dots = ParallelCoordinator.init(CoordinatorConfig{
        file_count: 4,
        parallel_limit: 2,
        dots_mode: true,
    });
    expect(!dots.shouldEmitHeader(0));
    expect(!dots.shouldEmitHeader(1));
});

test("test_runner coordinator: empty suite and invalid slot/state error guards", fn()! {
    let empty = ParallelCoordinator.init(CoordinatorConfig{ file_count: 0, parallel_limit: 4 });
    expect(empty.isDone());
    expect(empty.spawnNextWorker() == null);

    let coord = ParallelCoordinator.init(CoordinatorConfig{ file_count: 2, parallel_limit: 2 });
    const w0 = coord.spawnNextWorker() ?? return err.TestFailed;

    let errs: usize = 0;
    // Unspawned slot 1 -> InvalidWorkerSlot
    if (_ := coord.assignWork(1, 0)) {} else { errs += 1; }
    // Idle worker has no inflight file -> NoFileInflight
    if (_ := coord.onTestResult(w0, 0, true, false)) {} else { errs += 1; }
    if (_ := coord.onFileDone(w0, 0)) {} else { errs += 1; }

    // File index mismatch -> FileIndexMismatch
    const d0 = coord.assignWork(w0, 0)?;
    if (_ := coord.onFileDone(w0, d0.file_idx + 1)) {} else { errs += 1; }
    if (_ := coord.onTestResult(w0, d0.file_idx + 1, true, false)) {} else { errs += 1; }

    // Exited worker -> WorkerNotAlive
    * = coord.onWorkerExited(w0, false)?;
    if (_ := coord.assignWork(w0, 10)) {} else { errs += 1; }
    if (_ := coord.onWorkerExited(w0, false)) {} else { errs += 1; }

    expect_eq(errs, 7);
});
