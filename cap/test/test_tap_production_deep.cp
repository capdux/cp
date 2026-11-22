// Source: cp/cap/test/test_tap_production_deep.cp (Production deep tests for cap:tap)

import { test, expect_eq } from "cap:test";
import {
    TapEventKind,
    TapDirective,
    TapEvent,
    TapSummary,
    TapIter,
    summarizeTap,
} from "cap:obs";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("1. TAP version 14 + 1..5 plan + ok/not ok + # SKIP/# TODO + indented YAML diagnostic block", () => {
    const tap =
        "TAP version 14\r\n" +
        "1..5\r\n" +
        "ok 1 - initialize core\r\n" +
        "not ok 2 - flaky network # TODO retry not implemented\r\n" +
        "  ---\r\n" +
        "  message: timeout after 100ms\r\n" +
        "  severity: todo\r\n" +
        "  ...\r\n" +
        "ok 3 - windows only # skip running on linux\r\n" +
        "ok 4 - bonus fix # todo unexpectedly fixed\r\n" +
        "ok 5 - shutdown cleanly\r\n";

    let it = TapIter.init(tap);

    const e0 = it.next() ?? unreachable;
    expect_eq(e0.kind == .version, true);
    expect_eq(e0.version, 14);

    const e1 = it.next() ?? unreachable;
    expect_eq(e1.kind == .plan, true);
    expect_eq(e1.plan_start, 1);
    expect_eq(e1.plan_end, 5);

    const e2 = it.next() ?? unreachable;
    expect_eq(e2.kind == .test_point, true);
    expect_eq(e2.ok, true);
    expect_eq(e2.has_number, true);
    expect_eq(e2.test_number, 1);
    expect_eq(bytesEq(e2.description, "initialize core"), true);
    expect_eq(e2.directive == .none, true);

    const e3 = it.next() ?? unreachable;
    expect_eq(e3.kind == .test_point, true);
    expect_eq(e3.ok, false);
    expect_eq(e3.test_number, 2);
    expect_eq(e3.directive == .todo, true);
    expect_eq(bytesEq(e3.reason, "retry not implemented"), true);

    const e4 = it.next() ?? unreachable;
    expect_eq(e4.kind == .yaml_block, true);
    expect_eq(e4.raw_text.len > 20, true);

    const e5 = it.next() ?? unreachable;
    expect_eq(e5.kind == .test_point, true);
    expect_eq(e5.directive == .skip, true);
    expect_eq(bytesEq(e5.reason, "running on linux"), true);

    const e6 = it.next() ?? unreachable;
    expect_eq(e6.ok, true);
    expect_eq(e6.directive == .todo, true);

    const e7 = it.next() ?? unreachable;
    expect_eq(e7.ok, true);
    expect_eq(e7.test_number, 5);

    expect_eq(it.next() == null, true);

    // 汇总：not ok # TODO 不计硬失败，5/5 满足计划 -> allPassed() == true
    const sum = summarizeTap(tap);
    expect_eq(sum.version, 14);
    expect_eq(sum.has_plan, true);
    expect_eq(sum.planned, 5);
    expect_eq(sum.total, 5);
    expect_eq(sum.passed, 2);
    expect_eq(sum.failed, 0);
    expect_eq(sum.skipped, 1);
    expect_eq(sum.todo_passed, 1);
    expect_eq(sum.todo_failed, 1);
    expect_eq(sum.bailed_out, false);
    expect_eq(sum.allPassed(), true);
});

test("2. pragma +strict and Bail out! early abort handling", () => {
    const tap =
        "TAP version 13\n" +
        "pragma +strict\n" +
        "# connecting to database\n" +
        "ok 1 - ping\n" +
        "Bail out! Database connection refused\n";

    let it = TapIter.init(tap);
    const ev0 = it.next() ?? unreachable;
    expect_eq(ev0.kind == .version, true);
    expect_eq(ev0.version, 13);

    const ev1 = it.next() ?? unreachable;
    expect_eq(ev1.kind == .pragma, true);
    expect_eq(bytesEq(ev1.raw_text, "+strict"), true);

    const ev2 = it.next() ?? unreachable;
    expect_eq(ev2.kind == .comment, true);
    expect_eq(bytesEq(ev2.raw_text, "connecting to database"), true);

    const ev3 = it.next() ?? unreachable;
    expect_eq(ev3.kind == .test_point, true);

    const ev4 = it.next() ?? unreachable;
    expect_eq(ev4.kind == .bail_out, true);
    expect_eq(bytesEq(ev4.raw_text, "Database connection refused"), true);

    const sum = summarizeTap(tap);
    expect_eq(sum.bailed_out, true);
    expect_eq(sum.allPassed(), false);
});

test("3. Trailing plan (plan at end) and 1..0 # SKIP full-skip plan", () => {
    const trailing =
        "ok 1 - first\n" +
        "ok 2 - second\n" +
        "1..2\n";
    const s1 = summarizeTap(trailing);
    expect_eq(s1.has_plan, true);
    expect_eq(s1.planned, 2);
    expect_eq(s1.total, 2);
    expect_eq(s1.passed, 2);
    expect_eq(s1.allPassed(), true);

    const skip_all = "TAP version 14\n1..0 # SKIP no GPU available\n";
    let it = TapIter.init(skip_all);
    * = it.next();
    const plan_ev = it.next() ?? unreachable;
    expect_eq(plan_ev.kind == .plan, true);
    expect_eq(plan_ev.plan_start, 1);
    expect_eq(plan_ev.plan_end, 0);
    expect_eq(plan_ev.directive == .skip, true);
    expect_eq(bytesEq(plan_ev.reason, "no GPU available"), true);

    const s2 = summarizeTap(skip_all);
    expect_eq(s2.has_plan, true);
    expect_eq(s2.planned, 0);
    expect_eq(s2.total, 0);
    expect_eq(s2.allPassed(), true);
});

test("4. Escaped \\# inside description must not trigger SKIP/TODO directive", () => {
    const tap =
        "1..2\n" +
        "ok 1 - issue \\# SKIP fake directive in title\n" +
        "ok 2 - item \\#42 real directive # SKIP actual reason\n";

    let it = TapIter.init(tap);
    * = it.next(); // plan

    const t1 = it.next() ?? unreachable;
    expect_eq(t1.directive == .none, true);
    expect_eq(bytesEq(t1.description, "issue \\# SKIP fake directive in title"), true);

    const t2 = it.next() ?? unreachable;
    expect_eq(t2.directive == .skip, true);
    expect_eq(bytesEq(t2.description, "item \\#42 real directive"), true);
    expect_eq(bytesEq(t2.reason, "actual reason"), true);
});

test("5. Plan count mismatch and hard failure force allPassed() == false", () => {
    // 计划 3 个但只运行了 2 个 -> allPassed() == false
    const short_run =
        "1..3\n" +
        "ok 1 - a\n" +
        "ok 2 - b\n";
    const s1 = summarizeTap(short_run);
    expect_eq(s1.planned, 3);
    expect_eq(s1.total, 2);
    expect_eq(s1.failed, 0);
    expect_eq(s1.allPassed(), false);

    // 存在非 TODO 的 not ok -> failed == 1 -> allPassed() == false
    const with_fail =
        "1..2\n" +
        "ok 1 - a\n" +
        "not ok 2 - b\n";
    const s2 = summarizeTap(with_fail);
    expect_eq(s2.passed, 1);
    expect_eq(s2.failed, 1);
    expect_eq(s2.allPassed(), false);
});
