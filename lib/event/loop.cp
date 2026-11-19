// Source: /data/dev/bun/src/event_loop/MiniEventLoop.cp
// lib/event/loop.cp
// Unified EventLoop integrating intrusive timers, deferred microtasks, cross-thread tasks, and I/O polling

const std = @import("std");
import { Timer, TimerHeap, TimerCallback } from "./timer";
import { ConcurrentTask, ConcurrentQueue, TaskFn } from "./concurrent_task";
import { DeferredTaskQueue, DeferredFn } from "./deferred_task";
import { PollEngine, PollEvents, PollCallback } from "./file_poll";

pub class EventLoop {
    timers: *TimerHeap = undefined;
    deferred: *DeferredTaskQueue = undefined;
    poll_engine: *PollEngine = undefined;
    concurrent: *ConcurrentQueue = undefined;
    next_timer_id: u64 = 1;
    running: bool = false;
    stopped: bool = false;

    pub static fn init(): *EventLoop {
        let el = std.heap.page_allocator.create(EventLoop) catch unreachable;
        el.timers = TimerHeap.init();
        el.deferred = DeferredTaskQueue.init();
        el.poll_engine = PollEngine.init();
        el.concurrent = ConcurrentQueue.init();
        el.next_timer_id = 1;
        el.running = false;
        el.stopped = false;
        return el;
    }

    pub fn setTimeout(*_, cb: TimerCallback, delay_ms: i64, ctx: *!?): *Timer {
        const id = _.next_timer_id;
        _.next_timer_id += 1;
        const timer = Timer.create(id, delay_ms, 0, cb, ctx);
        _.timers.push(timer);
        return timer;
    }

    pub fn setInterval(*_, cb: TimerCallback, interval_ms: i64, ctx: *!?): *Timer {
        const id = _.next_timer_id;
        _.next_timer_id += 1;
        const timer = Timer.create(id, interval_ms, interval_ms, cb, ctx);
        _.timers.push(timer);
        return timer;
    }

    pub fn clearTimeout(*_, timer: *Timer): bool {
        return _.timers.remove(timer);
    }

    pub fn clearInterval(*_, timer: *Timer): bool {
        return _.timers.remove(timer);
    }

    pub fn deferTask(*_, task: DeferredFn, ctx: *!?): bool {
        return _.deferred.postTask(ctx, task);
    }

    pub fn postTask(*_, cb: TaskFn, ctx: *!?) {
        const task = ConcurrentTask.create(cb, ctx, true);
        _.concurrent.push(task);
    }

    pub fn watch(*_, fd: i32, events: u32, cb: PollCallback, ctx: *!?): bool {
        return _.poll_engine.register(fd, events, cb, ctx);
    }

    pub fn unwatch(*_, fd: i32): bool {
        return _.poll_engine.unregister(fd);
    }

    pub fn hasActiveHandles(*_): bool {
        return _.timers.count() > 0 || _.poll_engine.count() > 0 || _.concurrent.len() > 0 || _.deferred.count() > 0;
    }

    pub fn tick(*_, timeout_override: i32?): usize {
        let events_processed: usize = 0;

        // 1. Drain concurrent cross-thread tasks
        events_processed += _.concurrent.drain(64);

        // 2. Run deferred microtasks before sleeping/polling
        events_processed += _.deferred.run();

        // 3. Compute timeout until next timer
        const now = TimerHeap.nowMs();
        let timeout_ms: i32 = 0;
        if (t := timeout_override) {
            timeout_ms = t;
        } else {
            const next_timeout = _.timers.getTimeoutMs(now);
            if (t := next_timeout) {
                timeout_ms = t;
            } else if (_.poll_engine.count() > 0) {
                timeout_ms = 100; // default poll interval when waiting for I/O
            } else {
                timeout_ms = 0;
            }
        }

        // 4. Poll I/O file descriptors
        if (_.poll_engine.count() > 0) {
            events_processed += _.poll_engine.poll(timeout_ms);
        } else if (timeout_ms > 0) {
            // Sleep for timeout if no I/O descriptors
            TimerHeap.sleepMs(timeout_ms as u64);
        }

        // 5. Drain expired timers
        const now_after = TimerHeap.nowMs();
        events_processed += _.timers.drainExpired(now_after, 64);

        // 6. Run any follow-up microtasks triggered by timers or I/O
        events_processed += _.deferred.run();

        return events_processed;
    }

    pub fn run(*_) {
        _.running = true;
        _.stopped = false;
        while (_.hasActiveHandles() && !_.stopped) {
            * = _.tick(null);
        }
        _.running = false;
    }

    pub fn stop(*_) {
        _.stopped = true;
    }
}
