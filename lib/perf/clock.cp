// Source: bun/src/perf/hw_timer.cp -> cp/lib/perf/clock.cp (monotonic nanosecond clock + deterministic virtual clock)

import std from "std";
import builtin from "builtin";

pub const ClockReadFn = *const fn (usize) u64;

pub fn osMonotonicNs(): u64 {
    if (comptime builtin.os.tag == .linux) {
        let ts: std.os.linux.timespec = undefined;
        * = std.os.linux.clock_gettime(.MONOTONIC, &ts);
        const sec: u64 = if (ts.sec > 0) @intCast(ts.sec) else 0;
        const nsec: u64 = if (ts.nsec > 0) @intCast(ts.nsec) else 0;
        return sec * std.time.ns_per_s + nsec;
    }
    let ts: std.c.timespec = undefined;
    * = std.c.clock_gettime(.MONOTONIC, &ts);
    const sec: u64 = if (ts.sec > 0) @intCast(ts.sec) else 0;
    const nsec: u64 = if (ts.nsec > 0) @intCast(ts.nsec) else 0;
    return sec * std.time.ns_per_s + nsec;
}

pub enum ClockKind {
    Monotonic,
    Virtual,
    Custom,
}

pub struct PerfClock {
    kind: ClockKind = .Monotonic,
    virtual_ns: u64 = 0,
    custom_ctx: usize = 0,
    custom_fn: ClockReadFn? = null,

    pub static fn monotonic(): PerfClock {
        return PerfClock{
            kind: .Monotonic,
            virtual_ns: 0,
            custom_ctx: 0,
            custom_fn: null,
        };
    }

    pub static fn virtual(initial_ns: u64): PerfClock {
        return PerfClock{
            kind: .Virtual,
            virtual_ns: initial_ns,
            custom_ctx: 0,
            custom_fn: null,
        };
    }

    pub static fn custom(ctx: usize, read_fn: ClockReadFn): PerfClock {
        return PerfClock{
            kind: .Custom,
            virtual_ns: 0,
            custom_ctx: ctx,
            custom_fn: read_fn,
        };
    }

    pub fn nowNs(): u64 {
        switch (_.kind) {
            .Monotonic => return osMonotonicNs(),
            .Virtual => return _.virtual_ns,
            .Custom => {
                if (f := _.custom_fn) {
                    return f(_.custom_ctx);
                }
                return 0;
            },
        }
    }

    pub fn advance(*_, delta_ns: u64) {
        if (_.kind == .Virtual) {
            _.virtual_ns += delta_ns;
        }
    }

    pub fn setNs(*_, ns: u64) {
        if (_.kind == .Virtual) {
            _.virtual_ns = ns;
        }
    }

    pub fn elapsedSince(start_ns: u64): u64 {
        const current = _.nowNs();
        if (current >= start_ns) {
            return current - start_ns;
        }
        return 0;
    }
}

pub struct PerfTimer {
    clock: PerfClock,
    start_ns: u64,

    pub static fn start(): PerfTimer {
        const clock = PerfClock.monotonic();
        const now = clock.nowNs();
        return PerfTimer{
            clock: clock,
            start_ns: now,
        };
    }

    pub static fn startWithClock(clock: PerfClock): PerfTimer {
        const now = clock.nowNs();
        return PerfTimer{
            clock: clock,
            start_ns: now,
        };
    }

    pub fn readNs(): u64 {
        return _.clock.elapsedSince(_.start_ns);
    }

    pub fn lapNs(*_): u64 {
        const now = _.clock.nowNs();
        const elapsed = if (now >= _.start_ns) now - _.start_ns else 0;
        _.start_ns = now;
        return elapsed;
    }

    pub fn reset(*_) {
        _.start_ns = _.clock.nowNs();
    }
}
