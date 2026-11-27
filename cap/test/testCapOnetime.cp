import { test, expect_eq } from "cap:test";
import { OnceGuard, OnceRegistry, ONCE_OK, ONCE_ALREADY_CALLED } from "../../packages/onetime/src/index.cp";

fn main() {
    test("1. @cap/onetime — Executes once and caches first value", () => {
        let g = OnceGuard.new();
        expect_eq(g.isCalled(), false);
        expect_eq(g.callCount(), 0);

        let r1 = g.runWithValue(42);
        expect_eq(r1.status, ONCE_OK);
        expect_eq(r1.executed_now, true);
        expect_eq(r1.value, 42);
        expect_eq(r1.call_count, 1);

        let r2 = g.runWithValue(999);
        expect_eq(r2.status, ONCE_OK);
        expect_eq(r2.executed_now, false);
        expect_eq(r2.value, 42);
        expect_eq(r2.call_count, 2);
        expect_eq(g.callCount(), 2);
    });

    test("2. @cap/onetime — Strict mode rejects repeated invocation", () => {
        let g = OnceGuard.newStrict();
        let r1 = g.runWithValue(7);
        expect_eq(r1.status, ONCE_OK);
        expect_eq(r1.executed_now, true);

        let r2 = g.runWithValue(8);
        expect_eq(r2.status, ONCE_ALREADY_CALLED);
        expect_eq(r2.executed_now, false);
        expect_eq(r2.value, 7);
        expect_eq(g.callCount(), 2);
    });

    test("3. @cap/onetime — shouldRun boolean gate", () => {
        let g = OnceGuard.new();
        expect_eq(g.shouldRun(), true);
        expect_eq(g.shouldRun(), false);
        expect_eq(g.shouldRun(), false);
        expect_eq(g.callCount(), 3);
    });

    test("4. @cap/onetime — reset allows clean re-initialization", () => {
        let g = OnceGuard.new();
        * = g.runWithValue(10);
        expect_eq(g.isCalled(), true);
        g.reset();
        expect_eq(g.isCalled(), false);
        expect_eq(g.callCount(), 0);
        let r = g.runWithValue(20);
        expect_eq(r.executed_now, true);
        expect_eq(r.value, 20);
    });

    test("5. @cap/onetime — OnceRegistry tracks multiple independent keys", () => {
        let reg = OnceRegistry.new();
        let a1 = reg.runForKey(101, 1000);
        let b1 = reg.runForKey(202, 2000);
        let a2 = reg.runForKey(101, 9999);

        expect_eq(a1.executed_now, true);
        expect_eq(a1.value, 1000);
        expect_eq(b1.executed_now, true);
        expect_eq(b1.value, 2000);
        expect_eq(a2.executed_now, false);
        expect_eq(a2.value, 1000);
        expect_eq(reg.size(), 2);
        expect_eq(reg.callCountOf(101), 2);
        expect_eq(reg.callCountOf(202), 1);
        expect_eq(reg.has(303), false);
    });

    test("6. @cap/onetime — OnceRegistry capacity boundary check", () => {
        let reg = OnceRegistry.new();
        let i: u64 = 0;
        while (i < 64) {
            let r = reg.runForKey(i, (i * 2) as i64);
            expect_eq(r.executed_now, true);
            i = i + 1;
        }
        expect_eq(reg.size(), 64);
        let overflow = reg.runForKey(999, 1);
        expect_eq(overflow.status, ONCE_ALREADY_CALLED);
        expect_eq(overflow.executed_now, false);
    });
}
