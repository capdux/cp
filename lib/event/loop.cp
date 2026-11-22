// Source: Absorbed from /data/dev/bun/src/event_loop/MiniEventLoop.cp | Synced: 2026-11-22T12:49:17Z
// lib/event/loop.cp
// Unified EventLoop integrating intrusive pairing-heap timers, deferred microtasks,
// cross-thread concurrent tasks, AutoFlusher, and lazy POSIX I/O polling for cap:event

import std from "std";
import { Timer, TimerHeap, TimerState, getNowMs } from "./timer.cp";
import { ConcurrentTask, ConcurrentTaskQueue } from "./concurrentTask.cp";
import { DeferredTaskQueue } from "./deferredTask.cp";
import { AnyTask, ManagedTask } from "./task.cp";
import { AutoFlusher } from "./autoFlusher.cp";
import { PollEngine, PollEvents, PollCallback } from "./filePoll.cp";

pub const MAX_LOOP_TIMERS: usize = 64;
pub const MAX_LOOP_TASKS: usize = 64;

pub struct EventLoop {
    pub timers: TimerHeap = TimerHeap{},
    pub deferred_tasks: DeferredTaskQueue = DeferredTaskQueue{},
    pub concurrent_tasks: ConcurrentTaskQueue = ConcurrentTaskQueue{},
    pub poll_engine: (*PollEngine)? = null,
    pub iteration: u64 = 0,
    pub stopped: bool = false,

    internal_timers: [MAX_LOOP_TIMERS]Timer = undefined,
    internal_timers_used: [MAX_LOOP_TIMERS]bool = [false; MAX_LOOP_TIMERS],

    internal_tasks: [MAX_LOOP_TASKS]ConcurrentTask = undefined,
    internal_tasks_used: [MAX_LOOP_TASKS]bool = [false; MAX_LOOP_TASKS],

    pub static fn init(): EventLoop {
        return EventLoop{
            timers: TimerHeap.init(),
            deferred_tasks: DeferredTaskQueue.init(),
            concurrent_tasks: ConcurrentTaskQueue.init(),
            poll_engine: null,
            iteration: 0,
            stopped: false,
            internal_timers: undefined,
            internal_timers_used: [false; MAX_LOOP_TIMERS],
            internal_tasks: undefined,
            internal_tasks_used: [false; MAX_LOOP_TASKS],
        };
    }

    pub fn now(): i64 {
        return getNowMs();
    }

    pub fn isAlive(): bool {
        let has_io = false;
        if (pe := _.poll_engine) {
            has_io = pe.count() > 0;
        }
        return _.timers.count() > 0 or _.deferred_tasks.count() > 0 or !_.concurrent_tasks.isEmpty() or has_io;
    }

    pub fn hasActiveHandles(): bool {
        return _.isAlive();
    }

    pub fn stop(*_) {
        _.stopped = true;
    }

    pub fn watch(*_, fd: i32, events: u32, cb: PollCallback, ctx: *!?): bool {
        if (_.poll_engine == null) {
            _.poll_engine = PollEngine.init();
        }
        return _.poll_engine.?.register(fd, events, cb, ctx);
    }

    pub fn unwatch(*_, fd: i32): bool {
        if (pe := _.poll_engine) {
            return pe.unregister(fd);
        }
        return false;
    }

    pub fn tick(*_): usize {
        return _.tickWithTimeout(0);
    }

    pub fn tickWithTimeout(*_, poll_timeout_ms: i32): usize {
        let total_processed: usize = 0;

        // 1. Drain and run cross-thread / concurrent tasks
        let concurrent_count = _.concurrent_tasks.drainAll();
        total_processed += concurrent_count;
        if (_.concurrent_tasks.isEmpty()) {
            _.resetInternalTasks();
        }

        // 2. Run deferred microtasks (e.g. socket/buffer auto-flushes)
        let deferred_count = _.deferred_tasks.run();
        total_processed += deferred_count;

        // 3. Poll active I/O file descriptors if registered
        if (pe := _.poll_engine) {
            if (pe.count() > 0) {
                total_processed += pe.poll(poll_timeout_ms);
            }
        }

        // 4. Fire expired timers
        let now_ms = getNowMs();
        let fired_timers = _.timers.fireExpired(now_ms);
        total_processed += fired_timers;

        _.iteration += 1;
        return total_processed;
    }

    pub fn runUntilDone(*_, isDone: fn ((*anyopaque)?) bool, ctx: (*anyopaque)?): usize {
        let total: usize = 0;
        while (!isDone(ctx) and !_.stopped) {
            let processed = _.tick();
            total += processed;

            if (!_.isAlive() and !isDone(ctx)) {
                break;
            }
        }
        return total;
    }

    pub fn run(*_, max_ticks: usize): usize {
        let total: usize = 0;
        let ticks: usize = 0;
        while (ticks < max_ticks and !_.stopped and _.isAlive()) {
            let processed = _.tick();
            total += processed;
            ticks += 1;
        }
        return total;
    }

    pub fn postConcurrent(*_, task: *ConcurrentTask) {
        _.concurrent_tasks.push(task);
    }

    pub fn postTask(*_, func: fn ((*anyopaque)?) void, ctx: (*anyopaque)?): bool {
        let slot = _.allocTaskSlot() orelse return false;
        slot.* = ConcurrentTask.init(func, ctx, false);
        _.concurrent_tasks.push(slot);
        return true;
    }

    pub fn postDeferred(*_, ctx: *anyopaque, task: fn (*anyopaque) bool): bool {
        return _.deferred_tasks.postTask(ctx, task);
    }

    pub fn deferTask(*_, ctx: *anyopaque, task: fn (*anyopaque) bool): bool {
        return _.deferred_tasks.postTask(ctx, task);
    }

    pub fn unregisterTask(*_, ctx: *anyopaque): bool {
        return _.deferred_tasks.unregisterTask(ctx);
    }

    pub fn scheduleTimer(*_, timer: *Timer) {
        _.timers.insert(timer);
    }

    pub fn cancelTimer(*_, timer: *Timer): bool {
        let removed = _.timers.remove(timer);
        _.freeTimerSlot(timer);
        return removed;
    }

    pub fn setTimeout(*_, delay_ms: u64, callback: fn (*Timer) void, ctx: (*anyopaque)?): (*Timer)? {
        let slot = _.allocTimerSlot() orelse return null;
        slot.* = Timer.init(delay_ms, callback, ctx);
        _.timers.insert(slot);
        return slot;
    }

    pub fn setInterval(*_, interval_ms: u64, callback: fn (*Timer) void, ctx: (*anyopaque)?): (*Timer)? {
        let slot = _.allocTimerSlot() orelse return null;
        slot.* = Timer.initInterval(interval_ms, callback, ctx);
        _.timers.insert(slot);
        return slot;
    }

    pub fn clearTimeout(*_, timer: *Timer): bool {
        return _.cancelTimer(timer);
    }

    pub fn clearInterval(*_, timer: *Timer): bool {
        return _.cancelTimer(timer);
    }

    fn allocTimerSlot(*_): (*Timer)? {
        let i: usize = 0;
        while (i < MAX_LOOP_TIMERS) {
            if (!_.internal_timers_used[i]) {
                _.internal_timers_used[i] = true;
                return &_.internal_timers[i];
            }
            i += 1;
        }
        return null;
    }

    fn freeTimerSlot(*_, timer: *Timer) {
        let i: usize = 0;
        while (i < MAX_LOOP_TIMERS) {
            if (&_.internal_timers[i] == timer) {
                _.internal_timers_used[i] = false;
                return;
            }
            i += 1;
        }
    }

    fn allocTaskSlot(*_): (*ConcurrentTask)? {
        let i: usize = 0;
        while (i < MAX_LOOP_TASKS) {
            if (!_.internal_tasks_used[i]) {
                _.internal_tasks_used[i] = true;
                return &_.internal_tasks[i];
            }
            i += 1;
        }
        return null;
    }

    fn resetInternalTasks(*_) {
        let i: usize = 0;
        while (i < MAX_LOOP_TASKS) {
            _.internal_tasks_used[i] = false;
            i += 1;
        }
    }
}
