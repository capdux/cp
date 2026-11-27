// ============================================================================
// lib/test_runner/sdk.cp — Bun/Jest-Style Suite & Hook DSL over TestRunner
// ============================================================================

const std = @import("std");
import { TestRunner, TestContext, TestMode } from "./suite";
import { MockSpy } from "./mockSpy";
import { reportExpectFailure } from "./expect";

/// Global active runner pointer used during `describe` collection so zero-arg
/// Bun/Jest callbacks (`describe`, `it`, `beforeAll`, `beforeEach`, `afterEach`,
/// `afterAll`) seamlessly register into the enclosing suite.
let active_runner: (*TestRunner)? = null;
let active_depth: usize = 0;

/// Invokes either a 0-arg CP function, a `(*TestContext) void` / `(*TestRunner) void`
/// function, or an arrow lambda (`*lambda_Context`).
fn invokeZeroOrCtxCallback(cb: auto, runner_ptr: (*TestRunner)?, ctx_ptr: (*TestContext)?) {
    zig {
        const F = @TypeOf(cb);
        const ti = @typeInfo(F);
        const FnT = if (ti == .pointer) ti.pointer.child else F;
        const fn_info = @typeInfo(FnT).@"fn";
        if (fn_info.params.len == 0) {
            const res = cb();
            if (@typeInfo(@TypeOf(res)) == .error_union) {
                res catch |e| {
                    std.debug.print("\n    ❌ Callback error: {any}\n", .{e});
                    if (@hasDecl(__cp_rt, "__cap_test_failed")) {
                        __cp_rt.__cap_test_failed += 1;
                    }
                    if (ctx_ptr) |cp| cp.fail("Callback returned error");
                };
            }
            return;
        }
        if (fn_info.params.len == 1) {
            const P0 = fn_info.params[0].type orelse @compileError("Unsupported callback param");
            if (P0 == *TestRunner) {
                if (runner_ptr) |rp| {
                    cb(rp);
                }
                return;
            }
            if (P0 == *TestContext) {
                if (ctx_ptr) |cp| {
                    cb(cp);
                }
                return;
            }
            const p0_ti = @typeInfo(P0);
            if (p0_ti == .pointer and p0_ti.pointer.size == .one) {
                const Ctx = p0_ti.pointer.child;
                if (@typeInfo(Ctx) == .@"struct" and @hasField(Ctx, "pc") and @hasField(Ctx, "arena")) {
                    var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
                    defer arena.deinit();
                    var lctx = std.mem.zeroes(Ctx);
                    lctx.arena = &arena;
                    const res = cb(&lctx);
                    if (@typeInfo(@TypeOf(res)) == .error_union) {
                        res catch |e| {
                            if (e != error.TestFailed) {
                                std.debug.print("\n    ❌ Panic: {any}\n", .{e});
                            }
                            if (@hasDecl(__cp_rt, "__cap_test_failed")) {
                                __cp_rt.__cap_test_failed += 1;
                            }
                            if (ctx_ptr) |cp| cp.fail("Lambda failed");
                        };
                    }
                    return;
                }
            }
        }
        @compileError("Callback must take 0 arguments, *TestContext, or *TestRunner");
    }
}

/// Converts any zero-arg callback or arrow lambda into a `fn (*TestContext) void`
/// thunk via comptime monomorphization (zero heap allocation).
fn makeContextThunk(comptime cb: auto): fn (*TestContext) void {
    zig {
        return struct {
            fn thunk(ctx: *TestContext) void {
                const start_fails: usize = if (@hasDecl(__cp_rt, "__cap_test_failed")) __cp_rt.__cap_test_failed else 0;
                invokeZeroOrCtxCallback(cb, null, ctx);
                const end_fails: usize = if (@hasDecl(__cp_rt, "__cap_test_failed")) __cp_rt.__cap_test_failed else 0;
                if (end_fails > start_fails and !ctx.failed) {
                    ctx.fail("Assertion failed");
                }
            }
        }.thunk;
    }
}

fn readGlobalFails(): usize {
    zig {
        return if (@hasDecl(__cp_rt, "__cap_test_failed")) __cp_rt.__cap_test_failed else 0;
    }
}

fn bumpGlobalTotalPassed(passed: bool) {
    zig {
        if (@hasDecl(__cp_rt, "__cap_test_total")) __cp_rt.__cap_test_total += 1;
        if (passed) {
            if (@hasDecl(__cp_rt, "__cap_test_passed")) __cp_rt.__cap_test_passed += 1;
        }
    }
}

fn runDescribeWithMode(name: []const u8, mode: TestMode, builder: auto) {
    if (runner := active_runner) {
        * = runner.beginGroupWithMode(name, mode) catch {
            reportExpectFailure("describe", "failed to open nested describe group");
            return;
        };
        active_depth += 1;
        defer {
            active_depth -= 1;
            runner.endGroup();
        }
        invokeZeroOrCtxCallback(builder, runner, null);
        return;
    }

    // Root-level `describe`: collect into a scoped TestRunner, execute, and deinit cleanly.
    const Gpa = std.heap.GeneralPurposeAllocator(.{});
    let gpa = Gpa{};
    defer {
        const leak = gpa.deinit();
        if (leak == .leak) {
            reportExpectFailure("describe", "memory leak detected in test suite");
        }
    }
    const alloc = gpa.allocator();
    let runner = TestRunner.init(alloc);
    defer runner.deinit();

    active_runner = &runner;
    active_depth = 1;
    defer {
        active_runner = null;
        active_depth = 0;
    }

    * = runner.beginGroupWithMode(name, mode) catch {
        reportExpectFailure("describe", "failed to open root describe group");
        return;
    };
    invokeZeroOrCtxCallback(builder, &runner, null);
    runner.endGroup();

    const summary = runner.run() catch {
        reportExpectFailure("describe", "suite execution failed");
        return;
    };
    defer alloc.free(summary.results);

    for (tc of summary.results) {
        if (tc.status == .Passed) {
            std.debug.print("  ✔ PASS  {s}\n", .{ tc.full_name });
            bumpGlobalTotalPassed(true);
        } else if (tc.status == .Skipped || tc.status == .Todo) {
            std.debug.print("  ○ SKIP  {s}\n", .{ tc.full_name });
        } else {
            std.debug.print("  ❌ FAIL  {s}: {s}\n", .{ tc.full_name, tc.failure_message });
            zig {
                if (@hasDecl(__cp_rt, "__cap_test_total")) __cp_rt.__cap_test_total += 1;
                if (@hasDecl(__cp_rt, "__cap_test_failed")) __cp_rt.__cap_test_failed += 1;
            }
        }
    }
}

/// Defines a Bun/Jest-style test suite group. Supports nesting and lifecycle hooks.
pub fn describe(name: []const u8, builder: auto) {
    runDescribeWithMode(name, .Normal, builder);
}

/// Defines a skipped `describe` group.
pub fn describeSkip(name: []const u8, builder: auto) {
    runDescribeWithMode(name, .Skip, builder);
}

/// Registers a test case inside the active `describe`, or executes standalone.
pub fn it(name: []const u8, comptime cb: auto) {
    if (runner := active_runner) {
        * = runner.addTest(name, makeContextThunk(cb)) catch {
            reportExpectFailure("it", "failed to register test case");
        };
        return;
    }
    const start_fails = readGlobalFails();
    std.debug.print("\n▶ RUN   {s}\n", .{ name });
    invokeZeroOrCtxCallback(cb, null, null);
    const end_fails = readGlobalFails();
    if (end_fails == start_fails) {
        std.debug.print("✔ PASS  {s}\n", .{ name });
        bumpGlobalTotalPassed(true);
    } else {
        std.debug.print("❌ FAIL  {s}\n", .{ name });
        bumpGlobalTotalPassed(false);
    }
}

/// Registers a skipped test case.
pub fn itSkip(name: []const u8, comptime cb: auto) {
    if (runner := active_runner) {
        * = runner.testSkip(name, makeContextThunk(cb)) catch {};
    }
}

/// Registers a todo test case.
pub fn itTodo(name: []const u8) {
    if (runner := active_runner) {
        * = runner.testTodo(name) catch {};
    }
}

/// Registers a `beforeAll` hook in the active `describe` group.
pub fn beforeAll(comptime cb: auto) {
    if (runner := active_runner) {
        * = runner.beforeAll(makeContextThunk(cb)) catch {
            reportExpectFailure("beforeAll", "failed to register hook");
        };
    }
}

/// Registers a `beforeEach` hook in the active `describe` group.
pub fn beforeEach(comptime cb: auto) {
    if (runner := active_runner) {
        * = runner.beforeEach(makeContextThunk(cb)) catch {
            reportExpectFailure("beforeEach", "failed to register hook");
        };
    }
}

/// Registers an `afterEach` hook in the active `describe` group.
pub fn afterEach(comptime cb: auto) {
    if (runner := active_runner) {
        * = runner.afterEach(makeContextThunk(cb)) catch {
            reportExpectFailure("afterEach", "failed to register hook");
        };
    }
}

/// Registers an `afterAll` hook in the active `describe` group.
pub fn afterAll(comptime cb: auto) {
    if (runner := active_runner) {
        * = runner.afterAll(makeContextThunk(cb)) catch {
            reportExpectFailure("afterAll", "failed to register hook");
        };
    }
}

/// Creates a zero-allocation `MockSpy` instance.
pub fn mock(has_custom_impl: bool = false): MockSpy {
    return MockSpy.init(has_custom_impl);
}

// ============================================================================
// Legacy & Convenience Assertion Helpers
// ============================================================================

pub fn expect_true(cond: bool) {
    if (!cond) {
        reportExpectFailure("expect_true", "expected true, got false");
    }
}

pub fn assert(cond: bool) {
    if (!cond) {
        reportExpectFailure("assert", "assertion failed");
    }
}

pub fn expectEq(actual: auto, expected: auto) {
    zig {
        if (!__cp_rt.cp_eq(actual, expected)) {
            std.debug.print("\n    ❌ Assertion Failed (expectEq):\n    Expected: {any}\n    Got:      {any}\n", .{ expected, actual });
            if (@hasDecl(__cp_rt, "__cap_test_failed")) {
                __cp_rt.__cap_test_failed += 1;
            }
        }
    }
}

pub fn expectEqual(actual: auto, expected: auto) {
    expectEq(actual, expected);
}

pub fn assertEq(actual: auto, expected: auto) {
    expectEq(actual, expected);
}
