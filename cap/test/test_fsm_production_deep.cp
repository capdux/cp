// Source: cp/cap/test/test_fsm_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:fsm & cap.fsm

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    FsmError,
    TransitionRecord,
    GuardFn,
    ActionFn,
    StateMachine,
} from "cap:fsm";

struct ActionTrace {
    logs: [16][]const u8 = undefined,
    len: usize = 0,

    fn push(*_, tag: []const u8) {
        if (_.len < 16) {
            _.logs[_.len] = tag;
            _.len += 1;
        }
    }
}

fn onExitCreated(ctx: (*anyopaque)?, from_state: []const u8, event: []const u8, to_state: []const u8) {
    * = from_state;
    * = event;
    * = to_state;
    if (p := ctx) {
        let trace: *ActionTrace = unsafe { @ptrCast(@alignCast(p)) };
        trace.push("exit:created");
    }
}

fn onEnterPaid(ctx: (*anyopaque)?, from_state: []const u8, event: []const u8, to_state: []const u8) {
    * = from_state;
    * = event;
    * = to_state;
    if (p := ctx) {
        let trace: *ActionTrace = unsafe { @ptrCast(@alignCast(p)) };
        trace.push("enter:paid");
    }
}

fn onTransitionGlobal(ctx: (*anyopaque)?, from_state: []const u8, event: []const u8, to_state: []const u8) {
    * = from_state;
    * = event;
    * = to_state;
    if (p := ctx) {
        let trace: *ActionTrace = unsafe { @ptrCast(@alignCast(p)) };
        trace.push("transition");
    }
}

fn onExitPaid(ctx: (*anyopaque)?, from_state: []const u8, event: []const u8, to_state: []const u8) {
    * = from_state;
    * = event;
    * = to_state;
    if (p := ctx) {
        let trace: *ActionTrace = unsafe { @ptrCast(@alignCast(p)) };
        trace.push("exit:paid");
    }
}

fn onEnterShipped(ctx: (*anyopaque)?, from_state: []const u8, event: []const u8, to_state: []const u8) {
    * = from_state;
    * = event;
    * = to_state;
    if (p := ctx) {
        let trace: *ActionTrace = unsafe { @ptrCast(@alignCast(p)) };
        trace.push("enter:shipped");
    }
}

struct PaymentCtx {
    amount_cents: i64 = 0,
    min_cents: i64 = 100,
}

fn guardMinPayment(ctx: (*anyopaque)?, from_state: []const u8, event: []const u8, to_state: []const u8): bool {
    * = from_state;
    * = event;
    * = to_state;
    if (p := ctx) {
        const pc: *const PaymentCtx = unsafe { @ptrCast(@alignCast(p)) };
        return pc.amount_cents >= pc.min_cents;
    }
    return false;
}

fn testOrderWorkflowCallbacksAndHistory(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();
    {
        let sm = StateMachine.init(alloc, "created");
        defer sm.deinit();

    sm.addState("paid")?;
    sm.addTerminalState("shipped")?;

    sm.addTransition("created", "pay", "paid")?;
    sm.addTransition("paid", "ship", "shipped")?;

    sm.onExit("created", onExitCreated)?;
    sm.onEnter("paid", onEnterPaid)?;
    sm.onExit("paid", onExitPaid)?;
    sm.onEnter("shipped", onEnterShipped)?;
    sm.onTransition(onTransitionGlobal);

    let trace = ActionTrace{};
    expect_eq(sm.currentState(), "created");
    expect_eq(sm.isTerminal(), false);

    expect_eq(sm.trigger("pay", &trace)?, "paid");
    expect_eq(sm.isTerminal(), false);

    expect_eq(sm.trigger("ship", &trace)?, "shipped");
    expect_eq(sm.currentState(), "shipped");
    expect_eq(sm.isTerminal(), true);

    // Verify exact callback execution order: onExit -> onTransition -> onEnter
    expect_eq(trace.len, 6 as usize);
    expect_eq(trace.logs[0], "exit:created");
    expect_eq(trace.logs[1], "transition");
    expect_eq(trace.logs[2], "enter:paid");
    expect_eq(trace.logs[3], "exit:paid");
    expect_eq(trace.logs[4], "transition");
    expect_eq(trace.logs[5], "enter:shipped");

    // Verify transition audit history
    const hist = sm.history();
    expect_eq(hist.len, 2 as usize);
    expect_eq(hist[0].from, "created");
    expect_eq(hist[0].event, "pay");
    expect_eq(hist[0].to, "paid");
    expect_eq(hist[0].accepted, true);
    expect_eq(hist[1].from, "paid");
    expect_eq(hist[1].event, "ship");
    expect_eq(hist[1].to, "shipped");
    expect_eq(hist[1].accepted, true);

    // Reset restores initial state and clears history
    sm.reset();
    expect_eq(sm.currentState(), "created");
    expect_eq(sm.isTerminal(), false);
    expect_eq(sm.history().len, 0 as usize);
    }
    expect_eq(gpa.deinit() == .ok, true);
}

fn testGuardAcceptAndReject(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();
    {
        let sm = StateMachine.init(alloc, "pending");
        defer sm.deinit();

    sm.addTerminalState("confirmed")?;
    sm.addGuardedTransition("pending", "confirm", "confirmed", guardMinPayment)?;

    // Insufficient payment -> canTrigger false, trigger returns GuardRejected & state stays pending
    let low_ctx = PaymentCtx{ amount_cents: 50, min_cents: 100 };
    expect_eq(sm.canTrigger("confirm", &low_ctx), false);
    expect_eq(sm.canTrigger("confirm", null), false);

    const rej_res = sm.trigger("confirm", &low_ctx);
    const rej_tag = switch (rej_res) {
        FsmError.GuardRejected => "guard_rejected",
        else => "other",
        ok => "ok",
    };
    expect_eq(rej_tag, "guard_rejected");
    expect_eq(sm.currentState(), "pending");
    expect_eq(sm.isTerminal(), false);

    // Rejected attempt is recorded with accepted = false
    {
        const hist1 = sm.history();
        expect_eq(hist1.len, 1 as usize);
        expect_eq(hist1[0].from, "pending");
        expect_eq(hist1[0].event, "confirm");
        expect_eq(hist1[0].to, "confirmed");
        expect_eq(hist1[0].accepted, false);
    }

    // Sufficient payment -> canTrigger true, trigger succeeds
    let ok_ctx = PaymentCtx{ amount_cents: 150, min_cents: 100 };
    expect_eq(sm.canTrigger("confirm", &ok_ctx), true);
    expect_eq(sm.trigger("confirm", &ok_ctx)?, "confirmed");
    expect_eq(sm.currentState(), "confirmed");
    expect_eq(sm.isTerminal(), true);

    {
        const hist2 = sm.history();
        expect_eq(hist2.len, 2 as usize);
        expect_eq(hist2[1].accepted, true);
    }
    }
    expect_eq(gpa.deinit() == .ok, true);
}

fn testErrorHandlingAndAvailableEvents(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();
    {
        let sm = StateMachine.init(alloc, "idle");
        defer sm.deinit();

    sm.addState("running")?;
    sm.addTerminalState("stopped")?;

    // UnknownState on unregistered from/to
    const unk_res = sm.addTransition("idle", "go", "ghost");
    const unk_tag = switch (unk_res) {
        FsmError.UnknownState => "unknown",
        else => "other",
        ok => "ok",
    };
    expect_eq(unk_tag, "unknown");

    sm.addTransition("idle", "start", "running")?;
    sm.addTransition("idle", "abort", "stopped")?;
    sm.addTransition("running", "stop", "stopped")?;

    // DuplicateTransition on same (from, event)
    const dup_res = sm.addTransition("idle", "start", "stopped");
    const dup_tag = switch (dup_res) {
        FsmError.DuplicateTransition => "dup",
        else => "other",
        ok => "ok",
    };
    expect_eq(dup_tag, "dup");

    // availableEvents at "idle"
    const idle_events = sm.availableEvents(alloc)?;
    defer alloc.free(idle_events);
    expect_eq(idle_events.len, 2 as usize);
    expect_eq(idle_events[0], "start");
    expect_eq(idle_events[1], "abort");

    // InvalidTransition when triggering event not valid for current state
    const inv_res = sm.trigger("stop", null);
    const inv_tag = switch (inv_res) {
        FsmError.InvalidTransition => "invalid",
        else => "other",
        ok => "ok",
    };
    expect_eq(inv_tag, "invalid");
    expect_eq(sm.currentState(), "idle");

    // EmptyMachine check
    let empty_sm = StateMachine.init(alloc, "");
    defer empty_sm.deinit();
    const emp_res = empty_sm.trigger("anything", null);
    const emp_tag = switch (emp_res) {
        FsmError.EmptyMachine => "empty",
        else => "other",
        ok => "ok",
    };
    expect_eq(emp_tag, "empty");
    }
    expect_eq(gpa.deinit() == .ok, true);
}

fn testTopologyAnalysisAndGraphVizDot(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();
    {
        let sm = StateMachine.init(alloc, "init");
        defer sm.deinit();

    sm.addState("active")?;
    sm.addState("stuck")?;
    sm.addState("orphan")?;
    sm.addTerminalState("done")?;

    sm.addTransition("init", "begin", "active")?;
    sm.addTransition("active", "finish", "done")?;
    sm.addTransition("active", "jam", "stuck")?;

    // Reachable states via BFS from "init": ["init", "active", "done", "stuck"] (excludes "orphan")
    const reachable = sm.reachableStates(alloc)?;
    defer alloc.free(reachable);
    expect_eq(reachable.len, 4 as usize);
    expect_eq(reachable[0], "init");
    expect_eq(reachable[1], "active");
    expect_eq(reachable[2], "done");
    expect_eq(reachable[3], "stuck");

    // Dead-end non-terminal states: ["stuck"] ("done" is terminal so excluded; "orphan" is unreachable)
    const dead_ends = sm.findDeadEndStates(alloc)?;
    defer alloc.free(dead_ends);
    expect_eq(dead_ends.len, 1 as usize);
    expect_eq(dead_ends[0], "stuck");

    // GraphViz DOT output verification
    const dot = sm.toDot(alloc)?;
    defer alloc.free(dot);
    expect_eq(std.mem.indexOf(u8, dot, "digraph fsm {") != null, true);
    expect_eq(std.mem.indexOf(u8, dot, "__start -> \"init\";") != null, true);
    expect_eq(std.mem.indexOf(u8, dot, "\"done\" [shape=doublecircle];") != null, true);
    expect_eq(std.mem.indexOf(u8, dot, "\"init\" [shape=circle];") != null, true);
    expect_eq(std.mem.indexOf(u8, dot, "\"active\" -> \"done\" [label=\"finish\"];") != null, true);
    }
    expect_eq(gpa.deinit() == .ok, true);
}

fn testCapFsmDynamicMountZeroLeak(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();
    {
        let sm = cap.fsm.StateMachine.init(alloc, "draft");
        defer sm.deinit();

    sm.addState("review")?;
    sm.addTerminalState("published")?;
    sm.addTransition("draft", "submit", "review")?;
    sm.addTransition("review", "approve", "published")?;

    expect_eq(sm.canTrigger("submit", null), true);
    expect_eq(sm.trigger("submit", null)?, "review");
    expect_eq(sm.trigger("approve", null)?, "published");
    expect_eq(sm.isTerminal(), true);

    const events = sm.availableEvents(alloc)?;
    defer alloc.free(events);
    expect_eq(events.len, 0 as usize);

    const dot = sm.toDot(alloc)?;
    defer alloc.free(dot);
    expect_eq(std.mem.indexOf(u8, dot, "\"published\" [shape=doublecircle];") != null, true);
    }
    expect_eq(gpa.deinit() == .ok, true);
}

fn main(): !void {
    test("1. Order workflow transitions, onExit/onTransition/onEnter order & history", () => {
        testOrderWorkflowCallbacksAndHistory() catch unreachable;
    });
    test("2. GuardFn predicate accept & GuardRejected state preservation", () => {
        testGuardAcceptAndReject() catch unreachable;
    });
    test("3. InvalidTransition, UnknownState, DuplicateTransition & availableEvents", () => {
        testErrorHandlingAndAvailableEvents() catch unreachable;
    });
    test("4. BFS reachableStates, findDeadEndStates & GraphViz toDot export", () => {
        testTopologyAnalysisAndGraphVizDot() catch unreachable;
    });
    test("5. import cap from \"cap\"; cap.fsm dynamic mount & zero memory leak", () => {
        testCapFsmDynamicMountZeroLeak() catch unreachable;
    });
}
