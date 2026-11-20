// Source: cp/lib/perf/recorder.cp (PerfRecorder with begin/end span pairing, complete/instant/counter events)

import std from "std";
import { PerfClock } from "./clock.cp";
import { PerfError, PerfEvent, PerfPhase, PerfRecorderOptions, PerfSpan } from "./types.cp";

pub struct PerfRecorder {
    alloc: std.mem.Allocator,
    clock: PerfClock,
    events: std.ArrayList(PerfEvent),
    open_spans: std.ArrayList(PerfSpan),
    options: PerfRecorderOptions,

    pub static fn init(alloc: std.mem.Allocator, clock: PerfClock, options: PerfRecorderOptions): PerfRecorder {
        return PerfRecorder{
            alloc: alloc,
            clock: clock,
            events: .empty,
            open_spans: .empty,
            options: options,
        };
    }

    pub fn deinit(*_) {
        _.clear();
        _.events.deinit(_.alloc);
        _.open_spans.deinit(_.alloc);
    }

    pub fn clear(*_) {
        for (ev of _.events.items) {
            _.alloc.free(ev.name);
            _.alloc.free(ev.category);
        }
        _.events.clearRetainingCapacity();
        _.open_spans.clearRetainingCapacity();
    }

    pub fn eventCount(): usize {
        return _.events.items.len;
    }

    pub fn openSpanCount(): usize {
        return _.open_spans.items.len;
    }

    pub fn items(): []const PerfEvent {
        return _.events.items;
    }

    fn checkEventCapacity(): PerfError!void {
        if (_.options.max_events > 0 && _.events.items.len >= _.options.max_events) {
            return PerfError.CapacityExceeded;
        }
    }

    fn resolveCategory(category: []const u8): []const u8 {
        if (category.len == 0) {
            return _.options.default_category;
        }
        return category;
    }

    fn pushEvent(
        *_,
        name: []const u8,
        category: []const u8,
        phase: PerfPhase,
        ts_ns: u64,
        dur_ns: u64,
        pid: u32,
        tid: u32,
        value: i64,
    ): PerfError![]const u8 {
        try _.checkEventCapacity();
        const name_copy = _.alloc.dupe(u8, name) catch return PerfError.OutOfMemory;
        errdefer _.alloc.free(name_copy);
        const cat_src = _.resolveCategory(category);
        const cat_copy = _.alloc.dupe(u8, cat_src) catch return PerfError.OutOfMemory;
        errdefer _.alloc.free(cat_copy);

        _.events.append(_.alloc, PerfEvent{
            name: name_copy,
            category: cat_copy,
            phase: phase,
            ts_ns: ts_ns,
            dur_ns: dur_ns,
            pid: pid,
            tid: tid,
            value: value,
        }) catch return PerfError.OutOfMemory;

        return name_copy;
    }

    pub fn begin(*_, name: []const u8, category: []const u8): PerfError!PerfSpan {
        if (name.len == 0) {
            return PerfError.EmptyName;
        }
        if (_.open_spans.items.len >= _.options.max_open_spans) {
            return PerfError.CapacityExceeded;
        }
        const ts_ns = _.clock.nowNs();
        const pid = _.options.default_pid;
        const tid = _.options.default_tid;
        const owned_name = try _.pushEvent(name, category, .Begin, ts_ns, 0, pid, tid, 0);
        const last_ev = _.events.items[_.events.items.len - 1];
        const depth: u32 = @intCast(_.open_spans.items.len);

        const span = PerfSpan{
            name: owned_name,
            category: last_ev.category,
            start_ns: ts_ns,
            pid: pid,
            tid: tid,
            depth: depth,
            active: true,
        };
        _.open_spans.append(_.alloc, span) catch return PerfError.OutOfMemory;
        return span;
    }

    pub fn end(*_, span: *PerfSpan): PerfError!u64 {
        if (!span.active) {
            return PerfError.InvalidSpan;
        }
        if (_.open_spans.items.len == 0) {
            return PerfError.UnpairedEnd;
        }
        const top = _.open_spans.items[_.open_spans.items.len - 1];
        if (!std.mem.eql(u8, top.name, span.name) || top.start_ns != span.start_ns) {
            return PerfError.UnpairedEnd;
        }
        const end_ns = _.clock.nowNs();
        const dur_ns = if (end_ns >= span.start_ns) end_ns - span.start_ns else 0;
        * = try _.pushEvent(span.name, span.category, .End, end_ns, dur_ns, span.pid, span.tid, 0);
        * = _.open_spans.pop();
        span.active = false;
        return dur_ns;
    }

    pub fn endByName(*_, name: []const u8): PerfError!u64 {
        if (name.len == 0) {
            return PerfError.EmptyName;
        }
        if (_.open_spans.items.len == 0) {
            return PerfError.UnpairedEnd;
        }
        const top = _.open_spans.items[_.open_spans.items.len - 1];
        if (!std.mem.eql(u8, top.name, name)) {
            return PerfError.UnpairedEnd;
        }
        const end_ns = _.clock.nowNs();
        const dur_ns = if (end_ns >= top.start_ns) end_ns - top.start_ns else 0;
        * = try _.pushEvent(top.name, top.category, .End, end_ns, dur_ns, top.pid, top.tid, 0);
        * = _.open_spans.pop();
        return dur_ns;
    }

    pub fn recordComplete(*_, name: []const u8, category: []const u8, start_ns: u64, dur_ns: u64): PerfError!void {
        if (name.len == 0) {
            return PerfError.EmptyName;
        }
        * = try _.pushEvent(name, category, .Complete, start_ns, dur_ns, _.options.default_pid, _.options.default_tid, 0);
    }

    pub fn recordInstant(*_, name: []const u8, category: []const u8): PerfError!void {
        if (name.len == 0) {
            return PerfError.EmptyName;
        }
        const ts_ns = _.clock.nowNs();
        * = try _.pushEvent(name, category, .Instant, ts_ns, 0, _.options.default_pid, _.options.default_tid, 0);
    }

    pub fn recordCounter(*_, name: []const u8, category: []const u8, value: i64): PerfError!void {
        if (name.len == 0) {
            return PerfError.EmptyName;
        }
        const ts_ns = _.clock.nowNs();
        * = try _.pushEvent(name, category, .Counter, ts_ns, 0, _.options.default_pid, _.options.default_tid, value);
    }
}
