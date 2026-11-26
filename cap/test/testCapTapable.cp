// cap/test/testCapTapable.cp
// Comprehensive unit tests for @cap/tapable

import { test, expect_eq } from "cap:test";
import {
    TapEntry,
    SyncHook,
    SyncBailHook,
    SyncWaterfallHook,
    SyncLoopHook,
} from "../../packages/@cap/tapable/src/index.cp";

let g_sync_trace: [8]u32 = [0, 0, 0, 0, 0, 0, 0, 0];
let g_sync_len: usize = 0;

fn testSyncHandler(hid: u32, arg: i64): i64 {
    if (g_sync_len < 8) {
        g_sync_trace[g_sync_len] = hid;
        g_sync_len += 1;
    }
    return arg + i64(hid);
}

fn testBailHandler(hid: u32, arg: i64): i64? {
    if (hid == 20) return arg * 10;
    if (hid == 30) return arg * 100;
    return null;
}

fn testWaterfallHandler(hid: u32, val: i64): i64 {
    if (hid == 1) return val + 5;
    if (hid == 2) return val * 2;
    if (hid == 3) return val - 3;
    return val;
}

let g_loop_ticks: i64 = 0;

fn testLoopConverge(hid: u32, limit: i64): bool {
    g_loop_ticks += 1;
    if (hid == 2 && g_loop_ticks < limit) {
        return true; // restart loop from beginning
    }
    return false;
}

fn testLoopForever(hid: u32, arg: i64): bool {
    * = hid;
    * = arg;
    return true;
}

fn main() {
    test("1. @cap/tapable — SyncHook stage ordering & stable tie-breaking", () => {
        let hook = SyncHook.init();
        expect_eq(hook.isUsed(), false);

        expect_eq(hook.tap("default_a", 10), true);
        expect_eq(hook.tapStage("late", 10, 30), true);
        expect_eq(hook.tapStage("early", -5, 1), true);
        expect_eq(hook.tap("default_b", 20), true);

        expect_eq(hook.isUsed(), true);
        expect_eq(hook.tapCount(), 4 as usize);

        let order: [8]u32 = undefined;
        const n = hook.getOrderedHandlers(order[0..]);
        expect_eq(n, 4 as usize);
        expect_eq(order[0], 1 as u32);
        expect_eq(order[1], 10 as u32);
        expect_eq(order[2], 20 as u32);
        expect_eq(order[3], 30 as u32);

        g_sync_len = 0;
        const called = hook.call(testSyncHandler, 7);
        expect_eq(called, 4 as usize);
        expect_eq(g_sync_trace[0], 1 as u32);
        expect_eq(g_sync_trace[3], 30 as u32);
    });

    test("2. @cap/tapable — tapBefore insertion & setEnabled toggle", () => {
        let hook = SyncHook.init();
        * = hook.tap("first", 1);
        * = hook.tap("second", 2);
        * = hook.tapBefore("inserted", "second", 99);

        let order: [8]u32 = undefined;
        const n1 = hook.getOrderedHandlers(order[0..]);
        expect_eq(n1, 3 as usize);
        expect_eq(order[0], 1 as u32);
        expect_eq(order[1], 99 as u32);
        expect_eq(order[2], 2 as u32);

        expect_eq(hook.setEnabled("inserted", false), true);
        const n2 = hook.getOrderedHandlers(order[0..]);
        expect_eq(n2, 2 as usize);
        expect_eq(order[0], 1 as u32);
        expect_eq(order[1], 2 as u32);
    });

    test("3. @cap/tapable — SyncBailHook short-circuits on first non-null result", () => {
        let hook = SyncBailHook.init();
        * = hook.tap("pass", 10);
        * = hook.tap("bail_here", 20);
        * = hook.tap("never_reached", 30);

        const res = hook.call(testBailHandler, 4);
        expect_eq(res != null, true);
        if (v := res) {
            expect_eq(v, 40 as i64);
        }

        // Disable bailing taps -> returns null
        * = hook.setEnabled("bail_here", false);
        * = hook.setEnabled("never_reached", false);
        expect_eq(hook.call(testBailHandler, 4) == null, true);
    });

    test("4. @cap/tapable — SyncWaterfallHook chains stage-ordered transforms", () => {
        let hook = SyncWaterfallHook.init();
        * = hook.tapStage("sub3", 10, 3);
        * = hook.tapStage("add5", -10, 1);
        * = hook.tapStage("mul2", 0, 2);

        // ((10 + 5) * 2) - 3 = 27
        const out = hook.call(testWaterfallHandler, 10);
        expect_eq(out, 27 as i64);
    });

    test("5. @cap/tapable — SyncLoopHook restarts until convergence & guards max_rounds", () => {
        let hook = SyncLoopHook.init();
        * = hook.tap("step1", 1);
        * = hook.tap("step2", 2);
        * = hook.tap("step3", 3);

        g_loop_ticks = 0;
        const ok = hook.call(testLoopConverge, 5, 10);
        expect_eq(ok, true);
        // Round 1: h1(1), h2(2 -> restart)
        // Round 2: h1(3), h2(4 -> restart)
        // Round 3: h1(5), h2(6 -> ok), h3(7 -> ok)
        expect_eq(g_loop_ticks, 7 as i64);

        // Non-converging loop stops at max_rounds and returns false
        const stopped = hook.call(testLoopForever, 0, 5);
        expect_eq(stopped, false);
    });

    test("6. @cap/tapable — fixed 32-tap capacity guard", () => {
        let hook = SyncHook.init();
        let i: u32 = 0;
        while (i < 32) : (i += 1) {
            expect_eq(hook.tap("t", i), true);
        }
        expect_eq(hook.tapCount(), 32 as usize);
        expect_eq(hook.tap("overflow", 999), false);
    });
}
