import { test, expect_eq } from "cap:test";
import {
    FIFO_CAP,
    newFifoI64,
    newFifoStr,
} from "../../packages/fast-fifo/src/index.cp";

fn main() {
    test("1. @cap/fast-fifo — preserves FIFO order on push, peek, and shift", () => {
        let q = newFifoI64();
        expect_eq(q.isEmpty(), true);
        expect_eq(q.push(10), true);
        expect_eq(q.push(20), true);
        expect_eq(q.push(30), true);
        expect_eq(q.len(), 3 as usize);
        expect_eq(q.peek() ?? -1, 10 as i64);
        expect_eq(q.shift() ?? -1, 10 as i64);
        expect_eq(q.shift() ?? -1, 20 as i64);
        expect_eq(q.shift() ?? -1, 30 as i64);
        expect_eq(q.shift() == null, true);
        expect_eq(q.isEmpty(), true);
    });

    test("2. @cap/fast-fifo — wraps ring buffer indices cleanly across 500 cycles", () => {
        let q = newFifoI64();
        let i: i64 = 0;
        while (i < 500) {
            expect_eq(q.push(i), true);
            expect_eq(q.shift() ?? -1, i);
            i += 1;
        }
        expect_eq(q.isEmpty(), true);
    });

    test("3. @cap/fast-fifo — enforces capacity limit and rejects overflow", () => {
        let q = newFifoI64();
        let i: usize = 0;
        while (i < FIFO_CAP) {
            expect_eq(q.push(i as i64), true);
            i += 1;
        }
        expect_eq(q.isFull(), true);
        expect_eq(q.push(999), false);
        expect_eq(q.shift() ?? -1, 0 as i64);
        expect_eq(q.push(999), true);
    });

    test("4. @cap/fast-fifo — clear resets queue state", () => {
        let q = newFifoI64();
        q.push(1);
        q.push(2);
        q.clear();
        expect_eq(q.isEmpty(), true);
        expect_eq(q.len(), 0 as usize);
        expect_eq(q.peek() == null, true);
    });

    test("5. @cap/fast-fifo — FifoStr queues string slices in FIFO order", () => {
        let q = newFifoStr();
        expect_eq(q.push("alpha"), true);
        expect_eq(q.push("beta"), true);
        expect_eq(q.peek() ?? "", "alpha");
        expect_eq(q.shift() ?? "", "alpha");
        expect_eq(q.shift() ?? "", "beta");
        expect_eq(q.shift() == null, true);
    });

    test("6. @cap/fast-fifo — empty peek and shift return null safely", () => {
        let q1 = newFifoI64();
        expect_eq(q1.peek() == null, true);
        expect_eq(q1.shift() == null, true);
        let q2 = newFifoStr();
        expect_eq(q2.peek() == null, true);
        expect_eq(q2.shift() == null, true);
    });
}
