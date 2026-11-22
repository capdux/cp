import { test, expect, expect_eq } from "cap:test";
import {
    DEFERRED_BATCH_MAX_TASKS,
    DeferredBatchState,
    DeferredTaskOutcome,
    DeferredPluginTask,
    DeferredBatchCoordinator,
} from "cap:bundler";

test("bundler deferred_batch: full Idle -> Scheduled -> Running -> Idle lifecycle", fn() {
    let coord = DeferredBatchCoordinator.init();
    expect(coord.isIdle());
    expect(!coord.hasPending());

    // 空队列 schedule 返回 false 且保持 Idle
    const empty_sched = coord.schedule() catch {
        expect(false);
        return;
    };
    expect(!empty_sched);
    expect(coord.isIdle());

    * = coord.enqueueDeferred(10, 1) catch {
        expect(false);
        return;
    };
    * = coord.enqueueDeferred(20, 2) catch {
        expect(false);
        return;
    };
    expect_eq(coord.pendingCount(), 2);
    expect(coord.hasDeferredSource(10));
    expect(!coord.hasDeferredSource(99));

    const sched_ok = coord.schedule() catch {
        expect(false);
        return;
    };
    expect(sched_ok);
    expect(coord.isScheduled());

    const n_run = coord.beginRun() catch {
        expect(false);
        return;
    };
    expect_eq(n_run, 2);
    expect(coord.isRunning());

    let out: [8]DeferredPluginTask = [DeferredPluginTask{}; 8];
    const summary = coord.drainDeferred(false, out[0..]) catch {
        expect(false);
        return;
    };
    expect_eq(summary.wave, 0);
    expect_eq(summary.drained_count, 2);
    expect_eq(summary.resumed_count, 2);
    expect_eq(summary.rejected_count, 0);
    expect(!summary.had_build_error);
    expect(out[0].outcome == DeferredTaskOutcome.Resumed);
    expect_eq(out[0].source_index, 10);
    expect_eq(out[1].source_index, 20);
    expect(coord.isIdle());
    expect_eq(coord.pendingCount(), 0);
    expect_eq(coord.current_wave, 1);
    expect_eq(coord.total_drained, 2);
});

test("bundler deferred_batch: build error fast-fail rejects all pending tasks", fn() {
    let coord = DeferredBatchCoordinator.init();
    * = coord.enqueueDeferred(101, 3) catch {
        expect(false);
        return;
    };
    * = coord.enqueueDeferred(102, 4) catch {
        expect(false);
        return;
    };

    let out: [4]DeferredPluginTask = [DeferredPluginTask{}; 4];
    const summary = coord.runBatchNow(true, out[0..]) catch {
        expect(false);
        return;
    };
    expect(summary.had_build_error);
    expect_eq(summary.drained_count, 2);
    expect_eq(summary.resumed_count, 0);
    expect_eq(summary.rejected_count, 2);
    expect(out[0].outcome == DeferredTaskOutcome.Rejected);
    expect(out[1].outcome == DeferredTaskOutcome.Rejected);
    expect(coord.isIdle());
});

test("bundler deferred_batch: multi-wave cascade draining & cross-wave duplicate guard", fn() {
    let coord = DeferredBatchCoordinator.init();
    * = coord.enqueueDeferred(1, 0) catch {
        expect(false);
        return;
    };
    let out: [4]DeferredPluginTask = [DeferredPluginTask{}; 4];
    const w0 = coord.runBatchNow(false, out[0..]) catch {
        expect(false);
        return;
    };
    expect_eq(w0.wave, 0);
    expect_eq(w0.drained_count, 1);

    // 同一 source_index 跨波次严禁再次 .defer()
    let dup_ok = true;
    * = coord.enqueueDeferred(1, 0) catch {
        dup_ok = false;
    };
    expect(!dup_ok);

    // 新模块进入第 2 波次 (wave == 1)
    * = coord.enqueueDeferred(2, 1) catch {
        expect(false);
        return;
    };
    const w1 = coord.runBatchNow(false, out[0..]) catch {
        expect(false);
        return;
    };
    expect_eq(w1.wave, 1);
    expect_eq(out[0].wave, 1);
    expect_eq(out[0].source_index, 2);
    expect_eq(coord.total_drained, 2);

    // reset 后允许全新构建复用
    coord.reset();
    expect_eq(coord.current_wave, 0);
    expect_eq(coord.total_drained, 0);
    expect(!coord.hasDeferredSource(1));
});

test("bundler deferred_batch: state machine non-reentrancy guards", fn() {
    let coord = DeferredBatchCoordinator.init();
    expect(coord.state == DeferredBatchState.Idle);

    // Idle 下不可 beginRun 或 drainDeferred
    let ok1 = true;
    * = coord.beginRun() catch {
        ok1 = false;
    };
    expect(!ok1);

    let out: [4]DeferredPluginTask = [DeferredPluginTask{}; 4];
    let ok2 = true;
    * = coord.drainDeferred(false, out[0..]) catch {
        ok2 = false;
    };
    expect(!ok2);

    * = coord.enqueueDeferred(5, 1) catch {
        expect(false);
        return;
    };
    * = coord.schedule() catch {
        expect(false);
        return;
    };

    // Scheduled 下禁止重复 schedule
    let ok3 = true;
    * = coord.schedule() catch {
        ok3 = false;
    };
    expect(!ok3);

    // Scheduled 下未 beginRun 禁止 drainDeferred
    let ok4 = true;
    * = coord.drainDeferred(false, out[0..]) catch {
        ok4 = false;
    };
    expect(!ok4);
});

test("bundler deferred_batch: buffer-too-small guard preserves pending state", fn() {
    let coord = DeferredBatchCoordinator.init();
    * = coord.enqueueDeferred(10, 1) catch {
        expect(false);
        return;
    };
    * = coord.enqueueDeferred(11, 1) catch {
        expect(false);
        return;
    };
    * = coord.schedule() catch {
        expect(false);
        return;
    };
    * = coord.beginRun() catch {
        expect(false);
        return;
    };

    let tiny: [1]DeferredPluginTask = [DeferredPluginTask{}; 1];
    let ok_small = true;
    * = coord.drainDeferred(false, tiny[0..]) catch {
        ok_small = false;
    };
    expect(!ok_small);
    // 缓冲区不足报错后，原挂起队列和 Running 状态保持无损，换大缓冲可正常重试
    expect(coord.isRunning());
    expect_eq(coord.pendingCount(), 2);

    let ok_buf: [2]DeferredPluginTask = [DeferredPluginTask{}; 2];
    const s = coord.drainDeferred(false, ok_buf[0..]) catch {
        expect(false);
        return;
    };
    expect_eq(s.drained_count, 2);
    expect(coord.isIdle());
});

test("bundler deferred_batch: queue capacity limit guard", fn() {
    expect_eq(DEFERRED_BATCH_MAX_TASKS, 64);

    let coord = DeferredBatchCoordinator.init();
    let i: u32 = 0;
    while (i < DEFERRED_BATCH_MAX_TASKS) {
        * = coord.enqueueDeferred(i, 0) catch {
            expect(false);
            return;
        };
        i += 1;
    }
    expect_eq(coord.pendingCount(), DEFERRED_BATCH_MAX_TASKS);

    let ok_cap = true;
    * = coord.enqueueDeferred(999, 0) catch {
        ok_cap = false;
    };
    expect(!ok_cap);
});
