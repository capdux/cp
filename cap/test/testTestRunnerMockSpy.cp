// cap/test/testTestRunnerMockSpy.cp
// Production-grade deep tests for cap:test_runner mockSpy.cp (Bun Mock/Spy call matchers)

import { test, assert, assertEq } from "cap:test";
import {
    MOCK_SPY_MAX_CALLS,
    MockSpyError,
    MockCallOutcome,
    MockSpy,
} from "cap:test_runner";

fn eqStr(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("MockSpy: toHaveBeenCalled / Once / Times and 1-based Nth argument matchers", () => {
    let spy = MockSpy.init(true);
    assert(!spy.toHaveBeenCalled());
    assert(!spy.toHaveBeenCalledOnce());
    assert(spy.toHaveBeenCalledTimes(0));
    assert(!spy.toHaveBeenLastCalledWith(0, ""));

    * = spy.recordReturn(10, "fetchUser", 200, "alice") catch unreachable;
    assert(spy.toHaveBeenCalled());
    assert(spy.toHaveBeenCalledOnce());
    assert(spy.toHaveBeenCalledTimes(1));

    * = spy.recordReturn(20, "saveUser", 201, "created") catch unreachable;
    assert(!spy.toHaveBeenCalledOnce());
    assert(spy.toHaveBeenCalledTimes(2));

    assert(spy.toHaveBeenCalledWith(10, "fetchUser"));
    assert(spy.toHaveBeenCalledWith(20, "saveUser"));
    assert(!spy.toHaveBeenCalledWith(10, "saveUser"));

    assert(spy.toHaveBeenLastCalledWith(20, "saveUser"));
    assert(!spy.toHaveBeenLastCalledWith(10, "fetchUser"));

    // 1-based Nth matchers (0 is out of range -> false)
    assert(!spy.toHaveBeenNthCalledWith(0, 10, "fetchUser"));
    assert(spy.toHaveBeenNthCalledWith(1, 10, "fetchUser"));
    assert(spy.toHaveBeenNthCalledWith(2, 20, "saveUser"));
    assert(!spy.toHaveBeenNthCalledWith(3, 20, "saveUser"));
});

test("MockSpy: return vs throw tracking and return-value matchers", () => {
    let spy = MockSpy.init(false);
    * = spy.recordReturn(1, "ok1", 100, "res1") catch unreachable;
    * = spy.recordThrow(2, "boom", 500, "EIO") catch unreachable;
    * = spy.recordReturn(3, "ok2", 200, "res2") catch unreachable;

    assert(spy.toHaveBeenCalledTimes(3));
    assert(spy.toHaveReturned());
    assert(spy.toHaveReturnedTimes(2)); // thrown call does not count as returned

    assert(spy.toHaveReturnedWith(100, "res1"));
    assert(!spy.toHaveReturnedWith(500, "EIO")); // threw, not returned
    assert(spy.toHaveNthReturnedWith(1, 100, "res1"));
    assert(!spy.toHaveNthReturnedWith(2, 500, "EIO"));
    assert(spy.toHaveNthReturnedWith(3, 200, "res2"));
    assert(spy.toHaveLastReturnedWith(200, "res2"));

    if (rec2 := spy.getCall(2)) {
        assert(rec2.outcome == MockCallOutcome.Threw);
        assertEq(rec2.ret_tag, 500);
        assert(eqStr(rec2.ret_str, "EIO"));
    } else {
        assert(false);
    }
    assert(spy.getCall(0) == null);
    assert(spy.getCall(4) == null);
});

test("MockSpy: mockClear vs mockReset vs mockRestore semantics", () => {
    let spy = MockSpy.init(true);
    * = spy.recordReturn(1, "a", 1, "r") catch unreachable;
    assert(spy.has_custom_impl);
    assert(!spy.restored);

    // mockClear clears history, keeps custom impl
    spy.mockClear();
    assert(spy.toHaveBeenCalledTimes(0));
    assert(!spy.toHaveReturned());
    assert(spy.has_custom_impl);
    assert(!spy.restored);

    // mockReset clears history and drops custom impl
    * = spy.recordReturn(2, "b", 2, "r2") catch unreachable;
    spy.mockReset();
    assert(spy.toHaveBeenCalledTimes(0));
    assert(!spy.has_custom_impl);
    assert(!spy.restored);

    // mockRestore marks restored
    * = spy.recordReturn(3, "c", 3, "r3") catch unreachable;
    spy.mockRestore();
    assert(spy.toHaveBeenCalledTimes(0));
    assert(!spy.has_custom_impl);
    assert(spy.restored);
});

test("MockSpy: last call throwing makes toHaveLastReturnedWith false", () => {
    let spy = MockSpy.init(false);
    * = spy.recordReturn(1, "first", 1, "ok") catch unreachable;
    * = spy.recordThrow(2, "second", -1, "fail") catch unreachable;

    assert(spy.toHaveReturned());
    assert(spy.toHaveReturnedTimes(1));
    assert(!spy.toHaveLastReturnedWith(-1, "fail"));
    assert(!spy.toHaveLastReturnedWith(1, "ok"));
});

test("MockSpy: 64-slot history capacity guard still tracks exact total_calls", () => {
    assertEq(MOCK_SPY_MAX_CALLS, 64);
    let spy = MockSpy.init(false);
    let i: usize = 0;
    while (i < MOCK_SPY_MAX_CALLS) {
        * = spy.recordReturn(@as(i64, @intCast(i)), "k", 0, "v") catch unreachable;
        i += 1;
    }
    assertEq(spy.callCount(), 64);

    let full_err = false;
    * = spy.recordReturn(99, "overflow", 0, "v") catch |e| {
        full_err = (e == MockSpyError.HistoryFull);
    };
    assert(full_err);
    assertEq(spy.callCount(), 65);
    assert(spy.toHaveBeenCalledTimes(65));
    assert(spy.toHaveReturnedTimes(65));
});
