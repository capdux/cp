import { test, expect_eq } from "cap:test";
import { PLimit } from "../../packages/@cap/p-limit-core/src/index.cp";

fn main() {
    test("1. @cap/p-limit-core — PLimit.new rejects zero concurrency and initializes valid limit", () => {
        expect_eq(PLimit.new(0) == null, true);

        let opt: PLimit? = PLimit.new(2);
        expect_eq(opt != null, true);
        let lim: PLimit = opt ?? PLimit.single();
        expect_eq(lim.concurrency(), 2);
        expect_eq(lim.activeCount(), 0);
        expect_eq(lim.pendingCount(), 0);
    });

    test("2. @cap/p-limit-core — enforces FIFO order and concurrency cap", () => {
        let lim: PLimit = PLimit.new(2) ?? PLimit.single();
        expect_eq(lim.submit(101), true);
        expect_eq(lim.submit(102), true);
        expect_eq(lim.submit(103), true);
        expect_eq(lim.pendingCount(), 3);

        expect_eq(lim.startNext() ?? 0, 101);
        expect_eq(lim.startNext() ?? 0, 102);
        expect_eq(lim.startNext() == null, true);
        expect_eq(lim.activeCount(), 2);
        expect_eq(lim.pendingCount(), 1);
    });

    test("3. @cap/p-limit-core — finishOne releases slot and allows queued tasks to start", () => {
        let lim: PLimit = PLimit.new(1) ?? PLimit.single();
        expect_eq(lim.finishOne(), false);
        expect_eq(lim.submit(10), true);
        expect_eq(lim.submit(20), true);

        expect_eq(lim.startNext() ?? 0, 10);
        expect_eq(lim.startNext() == null, true);

        expect_eq(lim.finishOne(), true);
        expect_eq(lim.activeCount(), 0);

        expect_eq(lim.startNext() ?? 0, 20);
        expect_eq(lim.activeCount(), 1);
        expect_eq(lim.pendingCount(), 0);
        expect_eq(lim.finishOne(), true);
        expect_eq(lim.finishOne(), false);
    });

    test("4. @cap/p-limit-core — clearQueue drops pending tasks without affecting active ones", () => {
        let lim: PLimit = PLimit.new(2) ?? PLimit.single();
        expect_eq(lim.submit(1), true);
        expect_eq(lim.submit(2), true);
        expect_eq(lim.submit(3), true);
        expect_eq(lim.submit(4), true);

        expect_eq(lim.startNext() ?? 0, 1);
        expect_eq(lim.activeCount(), 1);
        expect_eq(lim.pendingCount(), 3);

        let dropped: usize = lim.clearQueue();
        expect_eq(dropped, 3);
        expect_eq(lim.pendingCount(), 0);
        expect_eq(lim.activeCount(), 1);
        expect_eq(lim.startNext() == null, true);
    });

    test("5. @cap/p-limit-core — setConcurrency dynamically scales concurrency and rejects zero", () => {
        let lim: PLimit = PLimit.new(1) ?? PLimit.single();
        expect_eq(lim.submit(7), true);
        expect_eq(lim.submit(8), true);
        expect_eq(lim.submit(9), true);

        expect_eq(lim.startNext() ?? 0, 7);
        expect_eq(lim.startNext() == null, true);

        expect_eq(lim.setConcurrency(0), false);
        expect_eq(lim.concurrency(), 1);

        expect_eq(lim.setConcurrency(2), true);
        expect_eq(lim.startNext() ?? 0, 8);
        expect_eq(lim.activeCount(), 2);

        expect_eq(lim.setConcurrency(1), true);
        expect_eq(lim.startNext() == null, true);
        expect_eq(lim.finishOne(), true);
        expect_eq(lim.startNext() == null, true);
        expect_eq(lim.finishOne(), true);
        expect_eq(lim.startNext() ?? 0, 9);
    });

    test("6. @cap/p-limit-core — respects fixed 64-task zero-heap queue capacity", () => {
        let lim: PLimit = PLimit.new(4) ?? PLimit.single();
        let i: u32 = 0;
        while (i < 64) {
            expect_eq(lim.submit(i + 1), true);
            i += 1;
        }
        expect_eq(lim.pendingCount(), 64);
        expect_eq(lim.submit(999), false);

        expect_eq(lim.clearQueue(), 64);
        expect_eq(lim.submit(999), true);
        expect_eq(lim.startNext() ?? 0, 999);
    });
}
