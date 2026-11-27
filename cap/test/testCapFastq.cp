import { test, expect_eq } from "cap:test";
import {
    FASTQ_CAP,
    FastQueue,
} from "../../packages/fastq/src/index.cp";

fn main() {
    test("1. @cap/fastq — FIFO push and concurrency cap enforcement", () => {
        let q = FastQueue.new(2);
        expect_eq(q.idle(), true);
        expect_eq(q.push(10), true);
        expect_eq(q.push(20), true);
        expect_eq(q.push(30), true);
        expect_eq(q.length(), 3 as usize);

        expect_eq(q.nextTask() ?? 0, 10 as u64);
        expect_eq(q.nextTask() ?? 0, 20 as u64);
        expect_eq(q.nextTask() == null, true); // concurrency cap = 2
        expect_eq(q.running(), 2 as usize);
        expect_eq(q.length(), 1 as usize);
    });

    test("2. @cap/fastq — unshift prepends high-priority tasks", () => {
        let q = FastQueue.new(4);
        * = q.push(1);
        * = q.push(2);
        * = q.unshift(99);
        * = q.unshift(100);
        expect_eq(q.nextTask() ?? 0, 100 as u64);
        expect_eq(q.nextTask() ?? 0, 99 as u64);
        expect_eq(q.nextTask() ?? 0, 1 as u64);
        expect_eq(q.nextTask() ?? 0, 2 as u64);
    });

    test("3. @cap/fastq — pause blocks dispatch and unpause resumes", () => {
        let q = FastQueue.new(2);
        * = q.push(7);
        q.pause();
        expect_eq(q.isPaused(), true);
        expect_eq(q.nextTask() == null, true);
        q.unpause();
        expect_eq(q.isPaused(), false);
        expect_eq(q.nextTask() ?? 0, 7 as u64);
    });

    test("4. @cap/fastq — completeTask reports drain when last task finishes", () => {
        let q = FastQueue.new(1);
        * = q.push(1);
        * = q.push(2);
        * = q.nextTask();
        expect_eq(q.completeTask(), false); // still 1 queued
        * = q.nextTask();
        expect_eq(q.completeTask(), true);  // drained!
        expect_eq(q.idle(), true);
    });

    test("5. @cap/fastq — kill drops pending tasks without affecting running", () => {
        let q = FastQueue.new(1);
        * = q.push(1);
        * = q.push(2);
        * = q.push(3);
        * = q.nextTask();
        expect_eq(q.kill(), 2 as usize);
        expect_eq(q.length(), 0 as usize);
        expect_eq(q.running(), 1 as usize);
        expect_eq(q.completeTask(), true);
    });

    test("6. @cap/fastq — dynamic setConcurrency and capacity guard", () => {
        let q = FastQueue.new(0); // clamped to 1
        let i: usize = 0;
        while (i < FASTQ_CAP) {
            expect_eq(q.push(i as u64), true);
            i += 1;
        }
        expect_eq(q.push(999), false); // full
        q.setConcurrency(3);
        expect_eq(q.nextTask() ?? 999, 0 as u64);
        expect_eq(q.nextTask() ?? 999, 1 as u64);
        expect_eq(q.nextTask() ?? 999, 2 as u64);
        expect_eq(q.running(), 3 as usize);
    });
}
