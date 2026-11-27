// ============================================================================
// cap/test/testCapTestSdk.cp — Production Tests for Upgraded cap:test SDK
// ============================================================================
// Covers:
//   1. Backwards-compatible `test()` & `expect_eq()`
//   2. Bun/Jest-style `describe`, `it`, `beforeAll`, `beforeEach`, `afterEach`, `afterAll`
//   3. Chainable `expect(actual)` & `.not` negation across all core matchers
//   4. `mock()` / `MockSpy` call & return assertions
//   5. Re-exported `FakeTimerClock`, `SnapshotStore`, `TestRunner`, `TestReporter`
//   6. Zero memory leaks (GPA leak check)

const std = @import("std");
import {
    test,
    expect_eq,
    expect,
    describe,
    describeSkip,
    it,
    itSkip,
    itTodo,
    beforeAll,
    beforeEach,
    afterEach,
    afterAll,
    mock,
    expect_true,
    expectEq,
    assert,
    assertEq,
    MockSpy,
    FakeTimerClock,
    FakeTimerFired,
    SnapshotStore,
    SnapshotUpdateMode,
    TestRunner,
    TestContext,
    TestReporter,
    ReportFormat,
    ExpectNumMatcher,
    ExpectSeqMatcher,
    ExpectKeysMatcher,
    ExpectKvEntry,
} from "cap:test";

struct Point {
    x: i32,
    y: i32,
}

fn mayThrow(should_fail: bool): i32! {
    if (should_fail) {
        return error.Boom;
    }
    return 42;
}

fn alwaysThrows(): void! {
    return error.Boom;
}

fn neverThrows(): void! {}

// Shared hook trace state for verifying exact Bun/Jest lifecycle ordering
let hook_log: [16]u8 = [0; 16];
let hook_len: usize = 0;

fn pushHook(ch: u8) {
    if (hook_len < 16) {
        hook_log[hook_len] = ch;
        hook_len += 1;
    }
}

fn hookBeforeAll() {
    pushHook('A');
}

fn hookBeforeEach() {
    pushHook('b');
}

fn hookAfterEach() {
    pushHook('e');
}

fn hookAfterAll() {
    pushHook('Z');
}

fn innerHookBeforeEach() {
    pushHook('i');
}

fn innerHookAfterEach() {
    pushHook('o');
}

test("cap:test legacy test() and expect_eq() remain 100% compatible", () => {
    expect_eq(2 + 2, 4);
    expect_eq("cp-lang", "cp-lang");
    expect_true(true);
    expectEq(10, 10);
    assert(1 < 2);
    assertEq(99, 99);
});

test("cap:test chainable expect() core matchers and .not negation", () => {
    // 1. toBe / not.toBe
    expect(42).toBe(42);
    expect(42).not.toBe(43);
    expect("hello").toBe("hello");
    expect("hello").not.toBe("world");
    expect(true).toBe(true);
    expect(true).not.toBe(false);

    // 2. toEqual / toStrictEqual (structs, slices, arrays, optionals)
    const p1 = Point{ x: 3, y: 4 };
    const p2 = Point{ x: 3, y: 4 };
    const p3 = Point{ x: 5, y: 6 };
    expect(p1).toEqual(p2);
    expect(p1).not.toEqual(p3);
    expect(p1).toStrictEqual(p2);

    const arr_a: [3]i64 = [10, 20, 30];
    const arr_b: [3]i64 = [10, 20, 30];
    const arr_c: [3]i64 = [10, 20, 99];
    expect(arr_a).toEqual(arr_b);
    expect(arr_a).not.toEqual(arr_c);

    // 3. toContain (strings & arrays/slices)
    expect("hello world").toContain("world");
    expect("hello world").not.toContain("zig");
    expect(arr_a[0..]).toContain(@as(i64, 20));
    expect(arr_a[0..]).not.toContain(@as(i64, 77));

    // 4. Truthiness / Nullability / Defined
    expect(true).toBeTruthy();
    expect(1).toBeTruthy();
    expect("non-empty").toBeTruthy();
    expect(false).toBeFalsy();
    expect(0).toBeFalsy();
    expect("").toBeFalsy();
    expect(0).not.toBeTruthy();
    expect(1).not.toBeFalsy();

    const opt_null: i32? = null;
    const opt_some: i32? = 123;
    expect(opt_null).toBeNull();
    expect(opt_null).toBeUndefined();
    expect(opt_null).not.toBeDefined();
    expect(opt_some).toBeDefined();
    expect(opt_some).not.toBeNull();
    expect(null).toBeNull();

    // 5. Numeric comparisons & approximations
    expect(10).toBeGreaterThan(5);
    expect(10).not.toBeGreaterThan(20);
    expect(10).toBeGreaterThanOrEqual(10);
    expect(5).toBeLessThan(10);
    expect(5).not.toBeLessThan(2);
    expect(5).toBeLessThanOrEqual(5);
    expect(3.14159).toBeCloseTo(3.14, 2);
    expect(3.14159).not.toBeCloseTo(3.5, 2);
    expect(42.0).toBeFinite();
    expect(42.0).toBePositive();
    expect(-7.5).toBeNegative();
    expect(std.math.nan(f64)).toBeNaN();

    // 6. Length & String pattern matchers
    expect("abcdef").toHaveLength(6);
    expect("abcdef").not.toHaveLength(5);
    expect(arr_a[0..]).toHaveLength(3);
    expect("bun-style-runner").toMatch("style");
    expect("bun-style-runner").not.toMatch("missing");
    expect("bun-style-runner").toStartWith("bun-");
    expect("bun-style-runner").toEndWith("-runner");

    // 7. toThrow (error unions & functions)
    expect(alwaysThrows).toThrow();
    expect(neverThrows).not.toThrow();
    expect(mayThrow(true)).toThrow();
    expect(mayThrow(false)).not.toThrow();
});

test("cap:test describe / it / lifecycle hooks ordering and zero leaks", () => {
    hook_len = 0;

    describe("outer suite", () => {
        beforeAll(hookBeforeAll);
        beforeEach(hookBeforeEach);
        afterEach(hookAfterEach);
        afterAll(hookAfterAll);

        it("first case", () => {
            pushHook('1');
            expect(1 + 1).toBe(2);
        });

        describe("inner suite", () => {
            beforeEach(innerHookBeforeEach);
            afterEach(innerHookAfterEach);

            it("nested case", () => {
                pushHook('2');
                expect("nested").toContain("est");
            });
        });

        itSkip("skipped case", () => {
            expect(true).toBe(false);
        });

        itTodo("pending todo case");
    });

    describeSkip("skipped suite", () => {
        it("never runs", () => {
            expect(1).toBe(2);
        });
    });

    // Expected order: A (beforeAll), b 1 e (first), b i 2 o e (nested), Z (afterAll)
    const trace = hook_log[0..hook_len];
    expect(trace).toBe("Ab1ebi2oeZ");
});

test("cap:test mock/spy, FakeTimers, SnapshotStore, and test_runner matchers", () => {
    // 1. MockSpy via mock() + expect(spy)
    let spy = mock(false);
    expect(spy).not.toHaveBeenCalled();

    * = spy.recordReturn(1, "alpha", 200, "ok") catch unreachable;
    * = spy.recordReturn(2, "beta", 201, "created") catch unreachable;

    expect(spy).toHaveBeenCalled();
    expect(spy).toHaveBeenCalledTimes(2);
    expect(spy).toHaveBeenCalledWith(1, "alpha");
    expect(spy).not.toHaveBeenCalledWith(9, "gamma");
    expect(spy).toHaveReturned();
    expect(spy).toHaveReturnedTimes(2);
    expect(spy.toHaveLastReturnedWith(201, "created")).toBe(true);

    spy.mockClear();
    expect(spy).not.toHaveBeenCalled();
    spy.mockReset();
    spy.mockRestore();
    expect(spy.restored).toBe(true);

    // 2. FakeTimerClock
    let clock = FakeTimerClock.init();
    clock.useFakeTimers(1000);
    const t1 = clock.setTimeout(50, 101) catch unreachable;
    * = clock.setTimeout(150, 102) catch unreachable;
    expect(t1).toBeGreaterThan(0);

    let fired_buf: [8]FakeTimerFired = [FakeTimerFired{}; 8];
    const adv = clock.advanceTimersByTime(60, fired_buf[0..]) catch unreachable;
    expect(adv.fired_count).toBe(@as(usize, 1));
    expect(fired_buf[0].tag).toBe(@as(u64, 101));
    expect(clock.now()).toBe(@as(i64, 1060));
    clock.useRealTimers();
    expect(clock.isFakeTimers()).toBe(false);

    // 3. SnapshotStore + TestReporter (GPA leak check)
    const Gpa = std.heap.GeneralPurposeAllocator(.{});
    let gpa = Gpa{};
    defer {
        const leak = gpa.deinit();
        expect(leak == .ok).toBe(true);
    }
    const alloc = gpa.allocator();

    let store = SnapshotStore.init(alloc);
    defer store.deinit();
    store.setColor(false);
    const snap_res = store.matchSnapshot("greeting", "hello cap", .CreateNew) catch unreachable;
    expect(snap_res.matched).toBe(true);

    // 4. ExpectNumMatcher / ExpectSeqMatcher / ExpectKeysMatcher
    let num_m = ExpectNumMatcher.init();
    expect(num_m.toBeGreaterThan(10.0, 2.0)).toBe(true);
    num_m.negate();
    expect(num_m.toBeNaN(5.0)).toBe(true);

    let seq_m = ExpectSeqMatcher.init();
    const nums: [4]i64 = [1, 2, 3, 4];
    expect(seq_m.checkSortedAscI64(nums[0..])).toBe(true);

    let keys_m = ExpectKeysMatcher.init();
    const kvs: [2]ExpectKvEntry = [
        ExpectKvEntry{ key: "host", value: "localhost" },
        ExpectKvEntry{ key: "port", value: "8080" },
    ];
    expect(keys_m.checkContainKey(kvs[0..], "host")).toBe(true);

    // 5. TestReporter formatting
    let runner = TestRunner.init(alloc);
    defer runner.deinit();
    * = runner.addTest("direct runner case", fn (ctx: *TestContext) {
        * = ctx.expectEq(3 * 3, 9);
    }) catch unreachable;
    const summary = runner.run() catch unreachable;
    defer alloc.free(summary.results);
    expect(summary.isSuccess()).toBe(true);
    expect(summary.passed).toBe(@as(usize, 1));

    let reporter = TestReporter.init(alloc, .Json);
    defer reporter.deinit();
    const json_out = reporter.formatSummary(summary) catch unreachable;
    defer alloc.free(json_out);
    expect(json_out).toContain("direct runner case");
});
