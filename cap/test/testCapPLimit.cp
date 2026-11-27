import { test, expect_eq } from "cap:test";
import {
    SubmitResult,
    CompleteResult,
    PLimit,
} from "../../packages/p-limit/src/index.cp";

fn main() {
    test("1. @cap/p-limit — Strict concurrency cap and FIFO queueing", () => {
        let lim: PLimit = PLimit.new(2);
        expect_eq(lim.concurrency(), 2);
        expect_eq(lim.activeCount(), 0);
        expect_eq(lim.pendingCount(), 0);

        const s1: SubmitResult = lim.submit(101);
        const s2: SubmitResult = lim.submit(102);
        const s3: SubmitResult = lim.submit(103);
        const s4: SubmitResult = lim.submit(104);

        expect_eq(s1.started_immediately, true);
        expect_eq(s2.started_immediately, true);
        expect_eq(s3.started_immediately, false);
        expect_eq(s3.queued, true);
        expect_eq(s4.queued, true);
        expect_eq(lim.activeCount(), 2);
        expect_eq(lim.pendingCount(), 2);
    });

    test("2. @cap/p-limit — complete() auto-wakes next queued task in FIFO order", () => {
        let lim: PLimit = PLimit.new(1);
        * = lim.submit(10);
        * = lim.submit(20);
        * = lim.submit(30);

        const c1: CompleteResult = lim.complete();
        expect_eq(c1.has_next, true);
        expect_eq(c1.next_task_id, 20);
        expect_eq(lim.activeCount(), 1);
        expect_eq(lim.pendingCount(), 1);

        const c2: CompleteResult = lim.complete();
        expect_eq(c2.has_next, true);
        expect_eq(c2.next_task_id, 30);
        expect_eq(lim.activeCount(), 1);
        expect_eq(lim.pendingCount(), 0);

        const c3: CompleteResult = lim.complete();
        expect_eq(c3.has_next, false);
        expect_eq(lim.activeCount(), 0);
    });

    test("3. @cap/p-limit — clearQueue() clears pending tasks without affecting active tasks", () => {
        let lim: PLimit = PLimit.new(2);
        * = lim.submit(1);
        * = lim.submit(2);
        * = lim.submit(3);
        * = lim.submit(4);
        * = lim.submit(5);

        expect_eq(lim.activeCount(), 2);
        expect_eq(lim.pendingCount(), 3);

        const dropped: usize = lim.clearQueue();
        expect_eq(dropped, 3);
        expect_eq(lim.pendingCount(), 0);
        expect_eq(lim.activeCount(), 2);

        const c: CompleteResult = lim.complete();
        expect_eq(c.has_next, false);
        expect_eq(lim.activeCount(), 1);
    });

    test("4. @cap/p-limit — Dynamic scale-up with setConcurrency() and drainReady()", () => {
        let lim: PLimit = PLimit.new(1);
        * = lim.submit(100);
        * = lim.submit(200);
        * = lim.submit(300);
        * = lim.submit(400);

        expect_eq(lim.activeCount(), 1);
        expect_eq(lim.pendingCount(), 3);

        expect_eq(lim.setConcurrency(3), true);
        expect_eq(lim.concurrency(), 3);

        let woken: [4]u64 = [0, 0, 0, 0];
        const count: usize = lim.drainReady(&woken);
        expect_eq(count, 2);
        expect_eq(woken[0], 200);
        expect_eq(woken[1], 300);
        expect_eq(lim.activeCount(), 3);
        expect_eq(lim.pendingCount(), 1);
    });

    test("5. @cap/p-limit — Dynamic scale-down delays wake-up until active drops below limit", () => {
        let lim: PLimit = PLimit.new(3);
        * = lim.submit(1);
        * = lim.submit(2);
        * = lim.submit(3);
        * = lim.submit(4); // queued

        // Shrink concurrency from 3 to 1 while 3 tasks are active
        expect_eq(lim.setConcurrency(1), true);
        expect_eq(lim.activeCount(), 3);

        // First two completions only reduce active count (3 -> 2 -> 1), no wake-up yet
        const c1: CompleteResult = lim.complete();
        expect_eq(c1.has_next, false);
        expect_eq(lim.activeCount(), 2);

        const c2: CompleteResult = lim.complete();
        expect_eq(c2.has_next, false);
        expect_eq(lim.activeCount(), 1);

        // Third completion drops active to 0 < 1, waking task 4
        const c3: CompleteResult = lim.complete();
        expect_eq(c3.has_next, true);
        expect_eq(c3.next_task_id, 4);
        expect_eq(lim.activeCount(), 1);
        expect_eq(lim.pendingCount(), 0);
    });

    test("6. @cap/p-limit — Invalid concurrency=0 clamping and queue-full rejection", () => {
        let lim: PLimit = PLimit.new(0);
        expect_eq(lim.concurrency(), 1);
        expect_eq(lim.setConcurrency(0), false);
        expect_eq(lim.concurrency(), 1);

        // Fill active slot (1) + entire YoctoQueueU64 capacity (256)
        const s0: SubmitResult = lim.submit(0);
        expect_eq(s0.started_immediately, true);

        const cap: usize = lim.queueCapacity();
        let i: usize = 0;
        while (i < cap) {
            const sr: SubmitResult = lim.submit((i + 1) as u64);
            expect_eq(sr.queued, true);
            i += 1;
        }
        expect_eq(lim.pendingCount(), cap);

        // Next submission must be rejected cleanly
        const overflow: SubmitResult = lim.submit(9999);
        expect_eq(overflow.started_immediately, false);
        expect_eq(overflow.queued, false);
        expect_eq(overflow.rejected, true);
    });
}
