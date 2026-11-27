import { test, expect_eq } from "cap:test";
import {
    TASK_PENDING, TASK_RUNNING, TASK_COMPLETED, TASK_SKIPPED, TASK_FAILED,
    ListrOptions, ListrRunner
} from "../../packages/listr2/src/index.cp";

fn main() {
    test("1. @cap/listr2 — addTask, startTask & completeTask lifecycle", () => {
        let r = ListrRunner.new(ListrOptions.defaults());
        let id0 = r.addTask("Install deps", 0) ?? 99;
        let id1 = r.addTask("Build bundle", 0) ?? 99;
        expect_eq(id0, 0);
        expect_eq(id1, 1);
        expect_eq(r.countByState(TASK_PENDING), 2);
        expect_eq(r.startTask(0), true);
        expect_eq(r.tasks[0].state, TASK_RUNNING);
        expect_eq(r.completeTask(0), true);
        expect_eq(r.countByState(TASK_COMPLETED), 1);
    });

    test("2. @cap/listr2 — retry budget re-queues task before terminal failure", () => {
        let r = ListrRunner.new(ListrOptions.defaults());
        * = r.addTask("Flaky fetch", 1);
        expect_eq(r.startTask(0), true);
        // First failure consumes retry and returns false (not terminal)
        expect_eq(r.failTask(0, "timeout"), false);
        expect_eq(r.tasks[0].state, TASK_PENDING);
        expect_eq(r.halted, false);
        // Second attempt succeeds
        expect_eq(r.startTask(0), true);
        expect_eq(r.tasks[0].attempts, 2);
        expect_eq(r.completeTask(0), true);
        expect_eq(r.countByState(TASK_COMPLETED), 1);
    });

    test("3. @cap/listr2 — exit_on_error halts runner on terminal failure", () => {
        let r = ListrRunner.new(ListrOptions.defaults());
        * = r.addTask("Step 1", 0);
        * = r.addTask("Step 2", 0);
        * = r.startTask(0);
        expect_eq(r.failTask(0, "boom"), true);
        expect_eq(r.halted, true);
        expect_eq(r.countByState(TASK_FAILED), 1);
        expect_eq(r.startTask(1), false);
    });

    test("4. @cap/listr2 — exit_on_error=false allows subsequent tasks to run", () => {
        let opts = ListrOptions.defaults();
        opts.exit_on_error = false;
        let r = ListrRunner.new(opts);
        * = r.addTask("Task A", 0);
        * = r.addTask("Task B", 0);
        * = r.startTask(0);
        expect_eq(r.failTask(0, "nonfatal"), true);
        expect_eq(r.halted, false);
        expect_eq(r.startTask(1), true);
        expect_eq(r.completeTask(1), true);
    });

    test("5. @cap/listr2 — skipTask & plain-text renderTaskLine", () => {
        let opts = ListrOptions.defaults();
        opts.use_color = false;
        let r = ListrRunner.new(opts);
        * = r.addTask("Deploy", 0);
        * = r.skipTask(0, "CI only");
        let buf: [128]u8 = [0; 128];
        let n = r.renderTaskLine(0, buf[0..128]);
        expect_eq(buf[0..n] as string, "[SKIP] Deploy (CI only)");
        expect_eq(r.countByState(TASK_SKIPPED), 1);
    });

    test("6. @cap/listr2 — ANSI colored renderTaskLine via @cap/picocolors", () => {
        let opts = ListrOptions.defaults();
        opts.use_color = true;
        let r = ListrRunner.new(opts);
        * = r.addTask("Compile", 0);
        * = r.completeTask(0);
        let buf: [128]u8 = [0; 128];
        let n = r.renderTaskLine(0, buf[0..128]);
        // Contains ESC (0x1b) from @cap/picocolors green("[OK]")
        expect_eq(n > 12, true);
        expect_eq(buf[0], 27);
    });
}
