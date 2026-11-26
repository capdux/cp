import { test, expect_eq } from "cap:test";
import {
    MAX_LISTENERS, REASON_END, REASON_FINISH, REASON_CLOSE, REASON_ERROR,
    ListenerSlot, StreamLifecycle, isFinished
} from "../../packages/@cap/on-finished/src/index.cp";

fn main() {
    test("1. @cap/on-finished — initial state is not finished", () => {
        let lc = StreamLifecycle.new();
        expect_eq(isFinished(lc), false);
        expect_eq(lc.isReadableFinished(), false);
        expect_eq(lc.isWritableFinished(), false);
        expect_eq(lc.fired_count, 0);
    });

    test("2. @cap/on-finished — markFinish fires all queued listeners once", () => {
        let lc = StreamLifecycle.new();
        expect_eq(lc.onFinished(10), true);
        expect_eq(lc.onFinished(20), true);
        expect_eq(lc.fired_count, 0);

        let n1 = lc.markFinish();
        expect_eq(n1, 2);
        expect_eq(lc.isWritableFinished(), true);
        expect_eq(lc.fired_count, 2);

        // Idempotent second trigger fires 0 new listeners and preserves first reason
        let n2 = lc.markClose();
        expect_eq(n2, 0);
        let s10 = lc.getListener(10) ?? ListenerSlot { id: 0, fired: false, reason: 0, err_code: 0 };
        expect_eq(s10.reason, REASON_FINISH);
    });

    test("3. @cap/on-finished — listener attached after finish fires immediately", () => {
        let lc = StreamLifecycle.new();
        * = lc.markEnd();
        expect_eq(lc.onFinished(99), true);
        let s99 = lc.getListener(99) ?? ListenerSlot { id: 0, fired: false, reason: 0, err_code: 0 };
        expect_eq(s99.fired, true);
        expect_eq(s99.reason, REASON_END);
        expect_eq(lc.fired_count, 1);
    });

    test("4. @cap/on-finished — markError propagates error code to all listeners", () => {
        let lc = StreamLifecycle.new();
        * = lc.onFinished(1);
        let n = lc.markError(104); // ECONNRESET
        expect_eq(n, 1);
        expect_eq(lc.errored, true);
        let s1 = lc.getListener(1) ?? ListenerSlot { id: 0, fired: false, reason: 0, err_code: 0 };
        expect_eq(s1.reason, REASON_ERROR);
        expect_eq(s1.err_code, 104);
    });

    test("5. @cap/on-finished — markClose marks both readable and writable finished", () => {
        let lc = StreamLifecycle.new();
        * = lc.markClose();
        expect_eq(lc.isReadableFinished(), true);
        expect_eq(lc.isWritableFinished(), true);
        expect_eq(lc.finish_reason, REASON_CLOSE);
    });

    test("6. @cap/on-finished — enforces MAX_LISTENERS capacity bound", () => {
        let lc = StreamLifecycle.new();
        let i: u32 = 0;
        while ((i as usize) < MAX_LISTENERS) {
            expect_eq(lc.onFinished(i + 1), true);
            i = i + 1;
        }
        expect_eq(lc.onFinished(999), false);
        expect_eq(lc.markFinish(), MAX_LISTENERS);
    });
}
