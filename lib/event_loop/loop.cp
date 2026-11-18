// Source: bun/src/event_loop/MiniEventLoop.cp
import std from "std";
import { Timer, TimerHeap, TimerState, getNowMs } from "./timer.cp";
import { ConcurrentTask, ConcurrentTaskQueue } from "./concurrent_task.cp";
import { DeferredTaskQueue } from "./deferred_task_queue.cp";
import { AnyTask, ManagedTask } from "./task.cp";
import { AutoFlusher } from "./auto_flusher.cp";

pub const MAX_LOOP_TIMERS: usize = 64;
pub const MAX_LOOP_TASKS: usize = 64;

pub struct EventLoop {
    pub timers: TimerHeap = TimerHeap{},
    pub deferred_tasks: DeferredTaskQueue = DeferredTaskQueue{},
    pub concurrent_tasks: ConcurrentTaskQueue = ConcurrentTaskQueue{},
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
        return _.timers.count() > 0 or _.deferred_tasks.count() > 0 or !_.concurrent_tasks.isEmpty();
    }

    pub fn stop(*_) {
        _.stopped = true;
    }

    pub fn tick(*_): usize {
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

        // 3. Fire expired timers
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
