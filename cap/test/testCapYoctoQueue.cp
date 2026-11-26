import { test, expect_eq } from "cap:test";
import { YoctoQueueU64, YoctoQueueI64, YOCTO_QUEUE_CAP } from "../../packages/@cap/yocto-queue/src/index.cp";

fn main() {
    test("1. @cap/yocto-queue — Basic FIFO enqueue/dequeue/peek", () => {
        let q = YoctoQueueU64.new();
        expect_eq(q.isEmpty(), true);
        expect_eq(q.size(), 0);

        expect_eq(q.enqueue(10), true);
        expect_eq(q.enqueue(20), true);
        expect_eq(q.enqueue(30), true);
        expect_eq(q.size(), 3);

        let p = q.peek();
        expect_eq(p.ok, true);
        expect_eq(p.value, 10);

        let r1 = q.dequeue();
        expect_eq(r1.ok, true);
        expect_eq(r1.value, 10);

        let r2 = q.dequeue();
        expect_eq(r2.value, 20);
        expect_eq(q.size(), 1);
    });

    test("2. @cap/yocto-queue — Wraparound across ring boundary", () => {
        let q = YoctoQueueU64.new();
        let round: u64 = 0;
        while (round < 500) {
            expect_eq(q.enqueue(round * 3 + 1), true);
            let r = q.dequeue();
            expect_eq(r.ok, true);
            expect_eq(r.value, round * 3 + 1);
            round = round + 1;
        }
        expect_eq(q.isEmpty(), true);
    });

    test("3. @cap/yocto-queue — Full capacity rejection and recovery", () => {
        let q = YoctoQueueU64.new();
        let i: usize = 0;
        while (i < YOCTO_QUEUE_CAP) {
            expect_eq(q.enqueue(i as u64), true);
            i = i + 1;
        }
        expect_eq(q.isFull(), true);
        expect_eq(q.enqueue(999), false);

        let r = q.dequeue();
        expect_eq(r.ok, true);
        expect_eq(r.value, 0);
        expect_eq(q.isFull(), false);
        expect_eq(q.enqueue(999), true);
        expect_eq(q.size(), YOCTO_QUEUE_CAP);
    });

    test("4. @cap/yocto-queue — peekAt and drain preserve FIFO order", () => {
        let q = YoctoQueueU64.new();
        * = q.enqueue(100);
        * = q.enqueue(200);
        * = q.enqueue(300);
        * = q.enqueue(400);

        expect_eq(q.peekAt(0).value, 100);
        expect_eq(q.peekAt(2).value, 300);
        expect_eq(q.peekAt(4).ok, false);

        let out: [2]u64 = undefined;
        let drained = q.drain(out[0..2]);
        expect_eq(drained, 2);
        expect_eq(out[0], 100);
        expect_eq(out[1], 200);
        expect_eq(q.size(), 2);
    });

    test("5. @cap/yocto-queue — Clear resets queue state cleanly", () => {
        let q = YoctoQueueU64.new();
        * = q.enqueue(1);
        * = q.enqueue(2);
        q.clear();
        expect_eq(q.isEmpty(), true);
        expect_eq(q.size(), 0);
        expect_eq(q.dequeue().ok, false);
        expect_eq(q.peek().ok, false);
    });

    test("6. @cap/yocto-queue — Signed YoctoQueueI64 handles negative values", () => {
        let q = YoctoQueueI64.new();
        expect_eq(q.enqueue(-42), true);
        expect_eq(q.enqueue(0), true);
        expect_eq(q.enqueue(999999), true);
        expect_eq(q.peek().value, -42);
        expect_eq(q.dequeue().value, -42);
        expect_eq(q.dequeue().value, 0);
        expect_eq(q.dequeue().value, 999999);
        expect_eq(q.isEmpty(), true);
    });
}
