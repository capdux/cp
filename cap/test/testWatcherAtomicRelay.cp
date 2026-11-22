// Production deep regression tests for lib/watcher/atomicRelay.cp (cap:watcher)

import { test, expect_eq } from "cap:test";
import {
    WATCHER_RELAY_SLOTS,
    NEXT_EVENT_WAITING,
    NEXT_EVENT_DONE,
    WatcherRelayError,
    WatcherSubmitOutcome,
    WatcherAtomicRelay,
} from "cap:watcher";

struct ReloadBatch {
    batch_id: u32 = 0,
    file_count: u32 = 0,
}

test("atomicRelay: constants, idle init & empty submission ignored", () => {
    expect_eq(WATCHER_RELAY_SLOTS, 3);
    expect_eq(NEXT_EVENT_WAITING, 254);
    expect_eq(NEXT_EVENT_DONE, 255);

    let relay = WatcherAtomicRelay<ReloadBatch>.init(ReloadBatch{});
    expect_eq(relay.isIdle(), true);

    const s0 = relay.watcherAcquire() catch 99;
    expect_eq(s0, 0);

    // Submitting with has_changes=false keeps relay idle
    const out = relay.watcherReleaseAndSubmit(s0, false) catch WatcherSubmitOutcome.ScheduledImmediate;
    expect_eq(out == WatcherSubmitOutcome.EmptyIgnored, true);
    expect_eq(relay.isIdle(), true);
    expect_eq(relay.total_submitted, 0);
});

test("atomicRelay: immediate schedule and clean recycle to idle", () => {
    let relay = WatcherAtomicRelay<ReloadBatch>.init(ReloadBatch{});

    const s0 = relay.watcherAcquire() catch 99;
    const b0 = relay.slotMut(s0);
    b0.batch_id = 1;
    b0.file_count = 3;

    const out = relay.watcherReleaseAndSubmit(s0, true) catch WatcherSubmitOutcome.EmptyIgnored;
    expect_eq(out == WatcherSubmitOutcome.ScheduledImmediate, true);
    expect_eq(relay.isIdle(), false);
    expect_eq(relay.server_active_slot ?? 99, s0);

    // Consumer finishes s0 and finds no pending work -> transitions back to idle
    const next = relay.serverRecycle(s0) catch 99;
    expect_eq(next == null, true);
    expect_eq(relay.isIdle(), true);
    expect_eq(relay.total_submitted, 1);
    expect_eq(relay.total_recycled, 1);
});

test("atomicRelay: queued pending slot picked up seamlessly on recycle", () => {
    let relay = WatcherAtomicRelay<ReloadBatch>.init(ReloadBatch{});

    // Producer submits batch 1 -> immediate
    const s0 = relay.watcherAcquire() catch 99;
    relay.slotMut(s0).batch_id = 10;
    const o0 = relay.watcherReleaseAndSubmit(s0, true) catch WatcherSubmitOutcome.EmptyIgnored;
    expect_eq(o0 == WatcherSubmitOutcome.ScheduledImmediate, true);

    // While server is busy with s0, producer acquires a distinct slot s1 and queues batch 2
    const s1 = relay.watcherAcquire() catch 99;
    expect_eq(s1 != s0, true);
    relay.slotMut(s1).batch_id = 20;
    const o1 = relay.watcherReleaseAndSubmit(s1, true) catch WatcherSubmitOutcome.EmptyIgnored;
    expect_eq(o1 == WatcherSubmitOutcome.QueuedBehindActive, true);

    // Server finishes s0 -> immediately receives s1 without going idle
    const next1 = relay.serverRecycle(s0) catch 99;
    expect_eq(next1 ?? 99, s1);
    expect_eq(relay.slotCopy(s1).batch_id, 20);
    expect_eq(relay.isIdle(), false);

    // Server finishes s1 -> now returns to idle
    const next2 = relay.serverRecycle(s1) catch 99;
    expect_eq(next2 == null, true);
    expect_eq(relay.isIdle(), true);
});

test("atomicRelay: triple-buffer coalescing replaces unstarted pending slot", () => {
    let relay = WatcherAtomicRelay<ReloadBatch>.init(ReloadBatch{});

    // Slot A active on server
    const sa = relay.watcherAcquire() catch 99;
    relay.slotMut(sa).batch_id = 100;
    * = relay.watcherReleaseAndSubmit(sa, true) catch WatcherSubmitOutcome.EmptyIgnored;

    // Slot B queued as pending
    const sb = relay.watcherAcquire() catch 99;
    expect_eq(sb != sa, true);
    relay.slotMut(sb).batch_id = 200;
    const ob = relay.watcherReleaseAndSubmit(sb, true) catch WatcherSubmitOutcome.EmptyIgnored;
    expect_eq(ob == WatcherSubmitOutcome.QueuedBehindActive, true);

    // Slot C acquired (third slot!) and replaces pending Slot B before server finishes A
    const sc = relay.watcherAcquire() catch 99;
    expect_eq(sc != sa && sc != sb, true);
    relay.slotMut(sc).batch_id = 300;
    const oc = relay.watcherReleaseAndSubmit(sc, true) catch WatcherSubmitOutcome.EmptyIgnored;
    expect_eq(oc == WatcherSubmitOutcome.ReplacedPending, true);
    expect_eq(relay.total_replaced, 1);

    // Server finishes A -> receives newest Slot C (batch 300), skipping stale B
    const next = relay.serverRecycle(sa) catch 99;
    expect_eq(next ?? 99, sc);
    expect_eq(relay.slotCopy(sc).batch_id, 300);

    * = relay.serverRecycle(sc) catch 99;
    expect_eq(relay.isIdle(), true);
});

test("atomicRelay: protocol misuse error guards", () => {
    let relay = WatcherAtomicRelay<ReloadBatch>.init(ReloadBatch{});

    // Double acquire rejected
    const s0 = relay.watcherAcquire() catch 99;
    let err_dup = false;
    * = relay.watcherAcquire() catch |e| {
        err_dup = (e == WatcherRelayError.AlreadyAcquired);
        0
    };
    expect_eq(err_dup, true);

    // Mismatched slot release rejected
    let err_mismatch = false;
    * = relay.watcherReleaseAndSubmit(2, true) catch |e| {
        err_mismatch = (e == WatcherRelayError.NotAcquired);
        WatcherSubmitOutcome.EmptyIgnored
    };
    expect_eq(err_mismatch, true);

    // Valid release
    * = relay.watcherReleaseAndSubmit(s0, false) catch WatcherSubmitOutcome.EmptyIgnored;

    // Releasing without acquire rejected
    let err_none = false;
    * = relay.watcherReleaseAndSubmit(s0, true) catch |e| {
        err_none = (e == WatcherRelayError.NotAcquired);
        WatcherSubmitOutcome.EmptyIgnored
    };
    expect_eq(err_none, true);

    // Recycling when server has no active event rejected
    let err_srv = false;
    * = relay.serverRecycle(0) catch |e| {
        err_srv = (e == WatcherRelayError.NoActiveServerEvent);
        null
    };
    expect_eq(err_srv, true);
});
