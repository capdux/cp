# CP 标准库：Bun/Jest 风格测试框架与断言 SDK 手册 (`cap:test`)

> **模块路径**: `lib/test.cp` + `lib/test_runner/{expect,sdk,mockSpy,fakeTimers,snapshot,...}.cp`  
> **导入协议**: `import { test, expect_eq, describe, describeSkip, it, itSkip, itTodo, beforeAll, beforeEach, afterEach, afterAll, expect, mock, MockSpy, FakeTimerClock, SnapshotStore } from "cap:test";`

---

## 一、 架构总览与核心分层

`cap:test` 提供零开销原生测试运行器入口与完整的 Bun/Jest 风格测试 DSL，底层复用 `cap:test_runner` 套件编排、虚拟时钟、Mock/Spy 与 `.snap` 快照引擎：

1. **向后兼容内置入口 (`lib/test.cp` & `lib/test_runner/sdk.cp`)**：
   - `test(name: string, callback: auto)`、`expect_eq(actual: auto, expected: auto)`
   - 便捷断言：`expect_true(cond: bool)`、`assert(cond: bool)`、`expectEq(actual, expected)`、`expectEqual(actual, expected)`、`assertEq(actual, expected)`
2. **Bun/Jest 风格套件与生命周期钩子 (`lib/test_runner/sdk.cp`)**：
   - 套件分组：`describe(name, builder)`（支持嵌套）、`describeSkip(name, builder)`
   - 用例注册：`it(name, cb)`（支持在 `describe` 内注册或独立执行）、`itSkip(name, cb)`、`itTodo(name)`
   - 生命周期钩子：`beforeAll(cb)`、`beforeEach(cb)`、`afterEach(cb)`、`afterAll(cb)`（支持 0 参函数、`*TestContext` / `*TestRunner` 回调或箭头 Lambda）
3. **可链式断言与 `.not` 否定 (`lib/test_runner/expect.cp`)**：
   - `expect(actual)` 返回 `Expectation(NormalizeExpectType(@TypeOf(actual)))`，内置 `.not`（`NegatedExpectation`）支持全部匹配器取反：
   - **相等与包含**：`toBe(expected)`、`toEqual(expected)`、`toStrictEqual(expected)`、`toContain(item)`
   - **真假与空值**：`toBeTruthy()`、`toBeFalsy()`、`toBeNull()`、`toBeDefined()`、`toBeUndefined()`
   - **数值与浮点**：`toBeGreaterThan(expected)`、`toBeGreaterThanOrEqual(expected)`、`toBeLessThan(expected)`、`toBeLessThanOrEqual(expected)`、`toBeCloseTo(expected, num_digits = 2)`、`toBeNaN()`、`toBeFinite()`、`toBePositive()`、`toBeNegative()`
   - **长度与字符串**：`toHaveLength(expected_len)`、`toMatch(pattern)`、`toStartWith(prefix)`、`toEndWith(suffix)`
   - **错误与异常**：`toThrow()`（校验返回错误联合 `T!` 的函数/值是否返回错误）
   - **Mock/Spy 断言**：`toHaveBeenCalled()`、`toHaveBeenCalledOnce()`、`toHaveBeenCalledTimes(expected)`、`toHaveBeenCalledWith(arg_tag, arg_str)`、`toHaveReturned()`、`toHaveReturnedTimes(expected)`
4. **零堆 `MockSpy` (`lib/test_runner/mockSpy.cp`)**：
   - `mock(has_custom_impl: bool = false): MockSpy`：创建固定容量（`MOCK_SPY_MAX_CALLS = 64`）零堆调用记录器；
   - 记录与重置：`recordReturn`、`recordThrow`、`mockClear`、`mockReset`、`mockRestore`、`callCount`、`getCall(nth_1based)`；
   - 历史匹配：`toHaveBeenCalled`、`toHaveBeenCalledOnce`、`toHaveBeenCalledTimes`、`toHaveBeenCalledWith`、`toHaveBeenLastCalledWith`、`toHaveBeenNthCalledWith`、`toHaveReturned`、`toHaveReturnedTimes`、`toHaveReturnedWith`、`toHaveLastReturnedWith`、`toHaveNthReturnedWith`。
5. **虚拟时钟 `FakeTimerClock` (`lib/test_runner/fakeTimers.cp`)**：
   - 单调/系统时钟解耦，支持 `FAKE_TIMERS_MAX_SLOTS` 槽位与防死循环步进上限；
   - 控制与查询：`init`、`isFakeTimers`、`useFakeTimers(initial_system_time_ms)`、`useRealTimers`、`setSystemTime(new_system_time_ms)`、`now`、`monotonicNow`、`setRunLimit(limit)`、`getTimerCount`、`peekNextDueMs`、`peekLastDueMs`；
   - 调度与推进：`setTimeout(delay_ms, tag)`、`setInterval(interval_ms, tag)`、`clearTimer(id)`、`clearAllTimers`、`advanceTimersToNextTimer(out_events)`、`advanceTimersByTime(delta_ms, out_events)`、`runOnlyPendingTimers(out_events)`、`runAllTimers(out_events)`。
6. **快照存储 `SnapshotStore` (`lib/test_runner/snapshot.cp`)**：
   - 解析/序列化 Jest/Bun 兼容 `.snap` 文件（`SNAPSHOT_FILE_HEADER`），失配时通过 `cap:patch` `DiffFormatter` 生成字符级彩色 Diff；
   - 方法：`init(allocator)`、`deinit`、`setColor(use_color)`、`count`、`get(name)`、`set(name, value)`、`parseSource(src)`、`load(path)`、`save(path)`、`matchSnapshot(name, actual, update_mode)`。
7. **重导出 `cap:test_runner` 完整组件 (`lib/test.cp`)**：
   - 同步重导出 `TestRunner`、`TestGroup`、`TestCase`、`TestContext`、`TestReporter`（`ReportFormat`）、`DoneCallback`、`ExpectMatcher`、`ExpectNumMatcher`、`ExpectSeqMatcher`、`ExpectKeysMatcher` 等。

---

## 二、 最小可运行示例

### 1. Bun/Jest 风格 `describe` / `it` / 钩子与链式 `expect`

```typescript
import {
    describe,
    it,
    itSkip,
    itTodo,
    beforeAll,
    beforeEach,
    afterEach,
    afterAll,
    expect,
} from "cap:test";

fn mayFail(fail_now: bool): i32! {
    if (fail_now) return error.Boom;
    return 42;
}

let counter: i32 = 0;

describe("math & assertions", () => {
    beforeAll(() => {
        counter = 0;
    });

    beforeEach(() => {
        counter += 1;
    });

    afterEach(() => {});
    afterAll(() => {});

    it("supports toBe, toEqual, toContain, toThrow and .not", () => {
        expect(1 + 2).toBe(3);
        expect(1 + 2).not.toBe(4);
        expect("hello cp").toContain("cp");
        expect("hello cp").toStartWith("hello");
        expect([3]i32{ 10, 20, 30 }).toEqual([3]i32{ 10, 20, 30 });
        expect(mayFail(true)).toThrow();
        expect(mayFail(false)).not.toThrow();
        expect(counter).toBeGreaterThan(0);
    });

    itSkip("skipped case", () => {
        expect(true).toBe(false);
    });

    itTodo("pending case");
});
```

### 2. `mock()` / `MockSpy` 与 `FakeTimerClock` / `SnapshotStore`

```typescript
const std = @import("std");
import {
    test,
    expect,
    mock,
    FakeTimerClock,
    FakeTimerFired,
    SnapshotStore,
    SnapshotUpdateMode,
} from "cap:test";

test("MockSpy, FakeTimerClock and SnapshotStore", () => {
    let spy = mock(false);
    * = spy.recordReturn(1, "user:1", 200, "ok") catch unreachable;
    expect(spy).toHaveBeenCalled();
    expect(spy).toHaveBeenCalledOnce();
    expect(spy).toHaveBeenCalledTimes(1);
    expect(spy).toHaveBeenCalledWith(1, "user:1");
    expect(spy).toHaveReturned();

    let clock = FakeTimerClock.init();
    clock.useFakeTimers(1_700_000_000_000);
    * = clock.setTimeout(50, 101) catch unreachable;
    let events: [4]FakeTimerFired = undefined;
    const res = clock.advanceTimersByTime(50, &events) catch unreachable;
    expect(res.fired_count).toBe(1);
    clock.useRealTimers();

    let store = SnapshotStore.init(std.heap.page_allocator);
    defer store.deinit();
    store.setColor(false);
    * = store.set("greeting", "hello cap:test") catch unreachable;
    const snap = store.matchSnapshot("greeting", "hello cap:test", SnapshotUpdateMode.None) catch unreachable;
    expect(snap.matched).toBe(true);
});
```
