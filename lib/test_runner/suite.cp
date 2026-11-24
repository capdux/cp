// Source: Absorbed from /data/dev/bun/src/test_runner/Collection.cp | Synced: 2026-11-22T12:49:17Z
// ============================================================================
// lib/test_runner/suite.cp — Pure CP Test Suite Collection, Hooks & Runner
// ============================================================================

const std = @import("std");
import { monotonicNs } from "../sys/process";
import { TestOrderMode, TestOrder, Prng64 } from "./order";

pub enum TestStatus: u8 {
    Pending = 0,
    Passed = 1,
    Failed = 2,
    Skipped = 3,
    Todo = 4,
    TimedOut = 5,

    fn isPass(): bool {
        return _ == .Passed || _ == .Skipped || _ == .Todo;
    }

    fn isFail(): bool {
        return _ == .Failed || _ == .TimedOut;
    }

    fn label(): []const u8 {
        return switch (_) {
            .Pending => "pending",
            .Passed => "passed",
            .Failed => "failed",
            .Skipped => "skipped",
            .Todo => "todo",
            .TimedOut => "timed_out",
        };
    }
}

pub enum TestMode: u8 {
    Normal = 0,
    Skip = 1,
    Todo = 2,
    Only = 3,
}

pub enum HookKind: u8 {
    BeforeAll = 0,
    BeforeEach = 1,
    AfterEach = 2,
    AfterAll = 3,
}

pub struct TestFilter {
    name_pattern: []const u8 = "",
    tag_pattern: []const u8 = "",
    has_only: bool = false,

    pub fn matches(full_name: []const u8, tags: []const []const u8, is_only: bool): bool {
        if (_.has_only && !is_only) return false;
        if (_.name_pattern.len > 0) {
            if (std.mem.indexOf(u8, full_name, _.name_pattern) == null) {
                return false;
            }
        }
        if (_.tag_pattern.len > 0) {
            let found_tag = false;
            for (t of tags) {
                if (std.mem.eql(u8, t, _.tag_pattern)) {
                    found_tag = true;
                    break;
                }
            }
            if (!found_tag) return false;
        }
        return true;
    }
}

/// Execution context passed to test cases and lifecycle hooks.
pub struct TestContext {
    allocator: std.mem.Allocator,
    test_name: []const u8 = "",
    failed: bool = false,
    timed_out: bool = false,
    failure_message: []const u8 = "",
    expected: []const u8 = "",
    received: []const u8 = "",
    assertions: u32 = 0,
    user_data: *!? = null,

    pub fn getUserData(comptime T: type): (*T)? {
        zig {
            if (self.user_data) |p| {
                return @ptrCast(@alignCast(p));
            }
            return null;
        }
    }

    pub fn fail(*_, msg: []const u8) {
        _.failed = true;
        _.failure_message = _.allocator.dupe(u8, msg) catch msg;
    }

    pub fn failDiff(*_, msg: []const u8, expected: []const u8, received: []const u8) {
        _.failed = true;
        _.failure_message = _.allocator.dupe(u8, msg) catch msg;
        _.expected = _.allocator.dupe(u8, expected) catch expected;
        _.received = _.allocator.dupe(u8, received) catch received;
    }

    pub fn timeout(*_, msg: []const u8) {
        _.timed_out = true;
        _.failed = true;
        _.failure_message = _.allocator.dupe(u8, msg) catch msg;
    }

    pub fn expectTrue(*_, cond: bool, msg: []const u8): bool {
        _.assertions += 1;
        if (!cond && !_.failed) {
            _.fail(msg);
        }
        return cond;
    }

    pub fn expectStrEq(*_, actual: []const u8, expected_val: []const u8): bool {
        _.assertions += 1;
        if (std.mem.eql(u8, actual, expected_val)) return true;
        if (!_.failed) {
            const msg = std.fmt.allocPrint(_.allocator, "Expected \"{s}\", received \"{s}\"", .{ expected_val, actual }) catch "String assertion failed";
            _.failDiff(msg, expected_val, actual);
        }
        return false;
    }

    pub fn expectEq(*_, actual: auto, expected_val: auto): bool {
        _.assertions += 1;
        if (actual == expected_val) return true;
        if (!_.failed) {
            const msg = std.fmt.allocPrint(_.allocator, "Expected {any}, received {any}", .{ expected_val, actual }) catch "Equality assertion failed";
            _.fail(msg);
        }
        return false;
    }
}

pub type TestCallback = fn (*TestContext) void;
pub type HookCallback = fn (*TestContext) void;

pub struct TestCase {
    id: usize = 0,
    group_id: usize = 0,
    name: []const u8 = "",
    full_name: []const u8 = "",
    suite_name: []const u8 = "",
    mode: TestMode = .Normal,
    tags: []const []const u8 = [],
    timeout_ms: u64 = 0,
    callback: TestCallback? = null,
    status: TestStatus = .Pending,
    duration_ns: u64 = 0,
    failure_message: []const u8 = "",
    expected: []const u8 = "",
    received: []const u8 = "",
    assertions: u32 = 0,
}

pub struct GroupItem {
    is_group: bool = false,
    index: usize = 0,
}

/// Tree-nested `describe` test group owning hooks, test cases, and child groups.
pub struct TestGroup {
    id: usize = 0,
    parent_id: usize? = null,
    name: []const u8 = "",
    full_name: []const u8 = "",
    mode: TestMode = .Normal,
    before_all: std.ArrayList(HookCallback) = .empty,
    before_each: std.ArrayList(HookCallback) = .empty,
    after_each: std.ArrayList(HookCallback) = .empty,
    after_all: std.ArrayList(HookCallback) = .empty,
    items: std.ArrayList(GroupItem) = .empty,

    fn deinit(*_, allocator: std.mem.Allocator) {
        _.before_all.deinit(allocator);
        _.before_each.deinit(allocator);
        _.after_each.deinit(allocator);
        _.after_all.deinit(allocator);
        _.items.deinit(allocator);
    }
}

pub struct TestRunSummary {
    total: usize = 0,
    passed: usize = 0,
    failed: usize = 0,
    skipped: usize = 0,
    todo: usize = 0,
    timed_out: usize = 0,
    duration_ns: u64 = 0,
    seed: u64 = 0,
    order_mode: TestOrderMode = .Declaration,
    results: []TestCase = [],

    pub fn isSuccess(): bool {
        return _.failed == 0 && _.timed_out == 0;
    }
}

/// Pure CP test suite collector and execution engine supporting nested `describe`,
/// `beforeAll`/`beforeEach`/`afterEach`/`afterAll` lifecycle hooks, `.only`/`.skip`/`.todo`,
/// name/tag filtering, and deterministic declaration/random/reverse ordering.
pub struct TestRunner {
    allocator: std.mem.Allocator,
    groups: std.ArrayList(TestGroup) = .empty,
    tests: std.ArrayList(TestCase) = .empty,
    current_group_id: usize = 0,
    order_mode: TestOrderMode = .Declaration,
    seed: u64 = 0,
    filter: TestFilter = TestFilter{},
    has_any_only: bool = false,
    user_data: *!? = null,

    pub static fn init(allocator: std.mem.Allocator): TestRunner {
        let runner = TestRunner{
            allocator: allocator,
        };
        const root = TestGroup{
            id: 0,
            parent_id: null,
            name: "",
            full_name: "",
            mode: .Normal,
        };
        runner.groups.append(allocator, root) catch {};
        return runner;
    }

    pub fn deinit(*_) {
        for (*g of _.groups.items) {
            if (g.full_name.len > 0) _.allocator.free(g.full_name);
            g.deinit(_.allocator);
        }
        _.groups.deinit(_.allocator);
        for (t of _.tests.items) {
            if (t.full_name.len > 0) _.allocator.free(t.full_name);
        }
        _.tests.deinit(_.allocator);
    }

    pub fn setOrderMode(*_, mode: TestOrderMode, seed: u64) {
        _.order_mode = mode;
        _.seed = seed;
    }

    pub fn setFilter(*_, filter: TestFilter) {
        _.filter = filter;
    }

    pub fn setUserData(*_, ptr: *!?) {
        _.user_data = ptr;
    }

    fn makeFullName(parent_full: []const u8, name: []const u8): ([]const u8)! {
        if (parent_full.len == 0) {
            return _.allocator.dupe(u8, name);
        }
        return std.fmt.allocPrint(_.allocator, "{s} > {s}", .{ parent_full, name });
    }

    pub fn beginGroup(*_, name: []const u8): usize! {
        return _.beginGroupWithMode(name, .Normal);
    }

    pub fn beginGroupWithMode(*_, name: []const u8, mode: TestMode): usize! {
        if (mode == .Only) _.has_any_only = true;
        const parent_id = _.current_group_id;
        const parent_full = _.groups.items[parent_id].full_name;
        const full = _.makeFullName(parent_full, name)?;

        const new_id = _.groups.items.len;
        const grp = TestGroup{
            id: new_id,
            parent_id: parent_id,
            name: name,
            full_name: full,
            mode: mode,
        };
        _.groups.append(_.allocator, grp)?;
        _.groups.items[parent_id].items.append(_.allocator, GroupItem{
            is_group: true,
            index: new_id,
        })?;
        _.current_group_id = new_id;
        return new_id;
    }

    pub fn endGroup(*_) {
        if (_.current_group_id == 0) return;
        const pid = _.groups.items[_.current_group_id].parent_id ?? 0;
        _.current_group_id = pid;
    }

    pub fn describe(*_, name: []const u8, builder: fn (*TestRunner) void): ! {
        * = _.beginGroupWithMode(name, .Normal)?;
        defer _.endGroup();
        builder(_);
    }

    pub fn describeSkip(*_, name: []const u8, builder: fn (*TestRunner) void): ! {
        * = _.beginGroupWithMode(name, .Skip)?;
        defer _.endGroup();
        builder(_);
    }

    pub fn describeOnly(*_, name: []const u8, builder: fn (*TestRunner) void): ! {
        * = _.beginGroupWithMode(name, .Only)?;
        defer _.endGroup();
        builder(_);
    }

    pub fn addHook(*_, kind: HookKind, cb: HookCallback): ! {
        const g = &_.groups.items[_.current_group_id];
        switch (kind) {
            .BeforeAll => g.before_all.append(_.allocator, cb)?,
            .BeforeEach => g.before_each.append(_.allocator, cb)?,
            .AfterEach => g.after_each.append(_.allocator, cb)?,
            .AfterAll => g.after_all.append(_.allocator, cb)?,
        }
    }

    pub fn beforeAll(*_, cb: HookCallback): ! {
        return _.addHook(.BeforeAll, cb);
    }

    pub fn beforeEach(*_, cb: HookCallback): ! {
        return _.addHook(.BeforeEach, cb);
    }

    pub fn afterEach(*_, cb: HookCallback): ! {
        return _.addHook(.AfterEach, cb);
    }

    pub fn afterAll(*_, cb: HookCallback): ! {
        return _.addHook(.AfterAll, cb);
    }

    pub fn addTestWithOptions(
        *_,
        name: []const u8,
        mode: TestMode,
        tags: []const []const u8,
        timeout_ms: u64,
        cb: TestCallback?,
    ): usize! {
        if (mode == .Only) _.has_any_only = true;
        const gid = _.current_group_id;
        const grp = &_.groups.items[gid];
        const full = _.makeFullName(grp.full_name, name)?;

        const tid = _.tests.items.len;
        _.tests.append(_.allocator, TestCase{
            id: tid,
            group_id: gid,
            name: name,
            full_name: full,
            suite_name: grp.full_name,
            mode: mode,
            tags: tags,
            timeout_ms: timeout_ms,
            callback: cb,
            status: .Pending,
        })?;
        grp.items.append(_.allocator, GroupItem{
            is_group: false,
            index: tid,
        })?;
        return tid;
    }

    pub fn addTest(*_, name: []const u8, cb: TestCallback): usize! {
        return _.addTestWithOptions(name, .Normal, [], 0, cb);
    }

    pub fn testSkip(*_, name: []const u8, cb: TestCallback?): usize! {
        return _.addTestWithOptions(name, .Skip, [], 0, cb);
    }

    pub fn testTodo(*_, name: []const u8): usize! {
        return _.addTestWithOptions(name, .Todo, [], 0, null);
    }

    pub fn testOnly(*_, name: []const u8, cb: TestCallback): usize! {
        return _.addTestWithOptions(name, .Only, [], 0, cb);
    }

    static fn effectiveTestMode(tc: TestCase, ancestor_mode: TestMode): TestMode {
        if (tc.mode == .Todo) return .Todo;
        if (tc.mode == .Skip || ancestor_mode == .Skip) return .Skip;
        if (tc.mode == .Only || ancestor_mode == .Only) return .Only;
        return .Normal;
    }

    static fn isTestRunnable(tc: TestCase, ancestor_mode: TestMode, eff_filter: TestFilter): bool {
        const eff_mode = TestRunner.effectiveTestMode(tc, ancestor_mode);
        if (eff_mode == .Skip || eff_mode == .Todo) return false;
        return eff_filter.matches(tc.full_name, tc.tags, eff_mode == .Only);
    }

    fn groupHasRunnableTests(group_id: usize, ancestor_mode: TestMode, eff_filter: TestFilter): bool {
        const grp = _.groups.items[group_id];
        const cur_mode = if (ancestor_mode == .Skip || grp.mode == .Skip)
            TestMode.Skip
        else if (grp.mode == .Only || ancestor_mode == .Only)
            TestMode.Only
        else
            TestMode.Normal;

        if (cur_mode == .Skip) return false;

        for (item of grp.items.items) {
            if (item.is_group) {
                if (_.groupHasRunnableTests(item.index, cur_mode, eff_filter)) return true;
            } else {
                const tc = _.tests.items[item.index];
                if (TestRunner.isTestRunnable(tc, cur_mode, eff_filter)) return true;
            }
        }
        return false;
    }

    fn collectAncestorChain(group_id: usize, out_ids: *std.ArrayList(usize)): ! {
        let chain: std.ArrayList(usize) = .empty;
        defer chain.deinit(_.allocator);
        let cur: usize? = group_id;
        while (gid := cur) {
            chain.append(_.allocator, gid)?;
            cur = _.groups.items[gid].parent_id;
        }
        // Reverse so root (outermost) is first, target group (innermost) is last
        let i: usize = chain.items.len;
        while (i > 0) {
            i -= 1;
            out_ids.append(_.allocator, chain.items[i])?;
        }
    }

    fn runHookList(*_, hooks: []const HookCallback, ctx: *TestContext) {
        for (h of hooks) {
            h(ctx);
        }
    }

    fn executeSingleTest(
        *_,
        test_id: usize,
        ancestor_mode: TestMode,
        eff_filter: TestFilter,
        results_out: *std.ArrayList(TestCase),
    ): ! {
        let tc = _.tests.items[test_id];
        const eff_mode = TestRunner.effectiveTestMode(tc, ancestor_mode);

        if (eff_mode == .Todo) {
            tc.status = .Todo;
            _.tests.items[test_id] = tc;
            results_out.append(_.allocator, tc)?;
            return;
        }
        if (eff_mode == .Skip || !eff_filter.matches(tc.full_name, tc.tags, eff_mode == .Only)) {
            tc.status = .Skipped;
            _.tests.items[test_id] = tc;
            results_out.append(_.allocator, tc)?;
            return;
        }

        let ancestors: std.ArrayList(usize) = .empty;
        defer ancestors.deinit(_.allocator);
        _.collectAncestorChain(tc.group_id, &ancestors)?;

        let ctx = TestContext{
            allocator: _.allocator,
            test_name: tc.full_name,
            user_data: _.user_data,
        };

        const t_start = monotonicNs();

        // 1. Run beforeEach outermost -> innermost
        for (gid of ancestors.items) {
            _.runHookList(_.groups.items[gid].before_each.items, &ctx);
        }

        // 2. Run test callback if hooks didn't already fail
        if (!ctx.failed) {
            if (cb := tc.callback) {
                cb(&ctx);
            }
        }

        // 3. Run afterEach innermost -> outermost
        let ai: usize = ancestors.items.len;
        while (ai > 0) {
            ai -= 1;
            const gid = ancestors.items[ai];
            _.runHookList(_.groups.items[gid].after_each.items, &ctx);
        }

        const t_end = monotonicNs();
        const elapsed = if (t_end >= t_start) (t_end - t_start) else 0;
        if (tc.timeout_ms > 0 && elapsed > tc.timeout_ms * 1000000 && !ctx.timed_out) {
            ctx.timeout("Test exceeded configured timeout");
        }

        tc.duration_ns = elapsed;
        tc.assertions = ctx.assertions;
        tc.failure_message = ctx.failure_message;
        tc.expected = ctx.expected;
        tc.received = ctx.received;
        if (ctx.timed_out) {
            tc.status = .TimedOut;
        } else if (ctx.failed) {
            tc.status = .Failed;
        } else {
            tc.status = .Passed;
        }

        _.tests.items[test_id] = tc;
        results_out.append(_.allocator, tc)?;
    }

    fn executeGroup(
        *_,
        group_id: usize,
        ancestor_mode: TestMode,
        eff_filter: TestFilter,
        results_out: *std.ArrayList(TestCase),
    ): ! {
        const grp = _.groups.items[group_id];
        const cur_mode = if (ancestor_mode == .Skip || grp.mode == .Skip)
            TestMode.Skip
        else if (grp.mode == .Only || ancestor_mode == .Only)
            TestMode.Only
        else
            TestMode.Normal;

        const has_runnable = _.groupHasRunnableTests(group_id, ancestor_mode, eff_filter);

        let before_all_failed = false;
        let before_all_err: []const u8 = "";

        if (has_runnable && grp.before_all.items.len > 0) {
            let hook_ctx = TestContext{
                allocator: _.allocator,
                test_name: grp.full_name,
                user_data: _.user_data,
            };
            _.runHookList(grp.before_all.items, &hook_ctx);
            if (hook_ctx.failed) {
                before_all_failed = true;
                before_all_err = hook_ctx.failure_message;
            }
        }

        // Order items within this group
        const n = grp.items.items.len;
        const group_seed = Prng64.deriveSeed(_.seed, grp.full_name);
        const item_order = TestOrder.orderIndices(_.allocator, n, _.order_mode, group_seed)?;
        defer _.allocator.free(item_order);

        for (idx of item_order) {
            const item = grp.items.items[idx];
            if (item.is_group) {
                _.executeGroup(item.index, cur_mode, eff_filter, results_out)?;
            } else if (before_all_failed) {
                let tc = _.tests.items[item.index];
                tc.status = .Failed;
                tc.failure_message = if (before_all_err.len > 0) before_all_err else "beforeAll hook failed";
                _.tests.items[item.index] = tc;
                results_out.append(_.allocator, tc)?;
            } else {
                _.executeSingleTest(item.index, cur_mode, eff_filter, results_out)?;
            }
        }

        if (has_runnable && grp.after_all.items.len > 0) {
            let hook_ctx = TestContext{
                allocator: _.allocator,
                test_name: grp.full_name,
                user_data: _.user_data,
            };
            _.runHookList(grp.after_all.items, &hook_ctx);
        }
    }

    /// Executes the collected test tree and returns a complete `TestRunSummary`.
    pub fn run(*_): TestRunSummary! {
        let eff_filter = _.filter;
        if (_.has_any_only) {
            eff_filter.has_only = true;
        }

        let results_list: std.ArrayList(TestCase) = .empty;
        errdefer results_list.deinit(_.allocator);

        const suite_start = monotonicNs();
        _.executeGroup(0, .Normal, eff_filter, &results_list)?;
        const suite_end = monotonicNs();

        let summary = TestRunSummary{
            total: results_list.items.len,
            duration_ns: if (suite_end >= suite_start) (suite_end - suite_start) else 0,
            seed: _.seed,
            order_mode: _.order_mode,
        };

        for (tc of results_list.items) {
            switch (tc.status) {
                .Passed => summary.passed += 1,
                .Failed => summary.failed += 1,
                .Skipped => summary.skipped += 1,
                .Todo => summary.todo += 1,
                .TimedOut => summary.timed_out += 1,
                .Pending => {},
            }
        }
        summary.results = results_list.toOwnedSlice(_.allocator)?;
        return summary;
    }
}
