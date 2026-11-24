// Source: Absorbed from /data/dev/bun/src/perf/tracy.cp | Synced: 2026-11-23T22:05:02Z
// lib/bench/trace.cp
// Zero-Allocation Nanosecond Perf Trace Ring Buffer & Chrome Trace Event JSON Exporter

const std = @import("std");
import { HwTimer } from "./timer.cp";

pub enum TracePhase: u8 {
    Begin = 'B',
    End = 'E',
    Instant = 'i',
    Complete = 'X',

    pub static fn asChar(p: TracePhase): u8 {
        return p as u8;
    }
}

pub struct TraceEvent {
    pub name: []const u8 = "";
    pub category: []const u8 = "perf";
    pub phase: TracePhase = TracePhase.Instant;
    pub ts_ns: u64 = 0;
    pub dur_ns: u64 = 0;
    pub pid: u32 = 1;
    pub tid: u32 = 1;
}

pub struct TraceSpan {
    pub name: []const u8 = "";
    pub category: []const u8 = "perf";
    pub start_ns: u64 = 0;
    pub pid: u32 = 1;
    pub tid: u32 = 1;
}

/// Zero-heap-allocation fixed-capacity ring buffer for nanosecond performance trace events.
pub struct TraceRingBuffer<const CAP: usize> {
    pub events: [CAP]TraceEvent = undefined;
    pub write_idx: usize = 0;
    pub count: usize = 0;
    pub total_recorded: u64 = 0;
    pub dropped: u64 = 0;
    pub epoch_ns: u64 = 0;
    pub pid: u32 = 1;
    pub tid: u32 = 1;

    pub static fn init(): TraceRingBuffer<CAP> {
        return TraceRingBuffer<CAP>.initWithIds(1, 1);
    }

    pub static fn initWithIds(pid: u32, tid: u32): TraceRingBuffer<CAP> {
        return TraceRingBuffer<CAP> {
            events: undefined,
            write_idx: 0,
            count: 0,
            total_recorded: 0,
            dropped: 0,
            epoch_ns: HwTimer.nowNs(),
            pid: pid,
            tid: tid,
        };
    }

    fn pushEvent(*_, ev: TraceEvent) {
        if (CAP == 0) {
            _.dropped += 1;
            return;
        }
        if (_.count >= CAP) {
            _.dropped += 1;
        } else {
            _.count += 1;
        }
        _.events[_.write_idx] = ev;
        _.write_idx = (_.write_idx + 1) % CAP;
        _.total_recorded += 1;
    }

    /// Records a `'B'` (Begin) trace event and returns a `TraceSpan` token for pairing with `endSpan`.
    pub fn begin(*_, name: []const u8, category: []const u8 = "perf"): TraceSpan {
        const now = HwTimer.nowNs();
        const rel = if (now >= _.epoch_ns) (now - _.epoch_ns) else 0;
        _.pushEvent(TraceEvent {
            name: name,
            category: category,
            phase: TracePhase.Begin,
            ts_ns: rel,
            dur_ns: 0,
            pid: _.pid,
            tid: _.tid,
        });
        return TraceSpan {
            name: name,
            category: category,
            start_ns: rel,
            pid: _.pid,
            tid: _.tid,
        };
    }

    /// Records an `'E'` (End) trace event matching `name` and `category`.
    pub fn end(*_, name: []const u8, category: []const u8 = "perf") {
        const now = HwTimer.nowNs();
        const rel = if (now >= _.epoch_ns) (now - _.epoch_ns) else 0;
        _.pushEvent(TraceEvent {
            name: name,
            category: category,
            phase: TracePhase.End,
            ts_ns: rel,
            dur_ns: 0,
            pid: _.pid,
            tid: _.tid,
        });
    }

    /// Records an `'E'` (End) event paired with a `TraceSpan` returned from `begin()`.
    pub fn endSpan(*_, span: TraceSpan): u64 {
        const now = HwTimer.nowNs();
        const rel = if (now >= _.epoch_ns) (now - _.epoch_ns) else 0;
        const dur = if (rel >= span.start_ns) (rel - span.start_ns) else 0;
        _.pushEvent(TraceEvent {
            name: span.name,
            category: span.category,
            phase: TracePhase.End,
            ts_ns: rel,
            dur_ns: dur,
            pid: span.pid,
            tid: span.tid,
        });
        return dur;
    }

    /// Records an `'i'` (Instant) point-in-time marker event.
    pub fn instant(*_, name: []const u8, category: []const u8 = "perf") {
        const now = HwTimer.nowNs();
        const rel = if (now >= _.epoch_ns) (now - _.epoch_ns) else 0;
        _.pushEvent(TraceEvent {
            name: name,
            category: category,
            phase: TracePhase.Instant,
            ts_ns: rel,
            dur_ns: 0,
            pid: _.pid,
            tid: _.tid,
        });
    }

    /// Records an `'X'` (Complete) event with explicit start timestamp and duration in nanoseconds.
    pub fn complete(*_, name: []const u8, category: []const u8, start_ns: u64, dur_ns: u64) {
        _.pushEvent(TraceEvent {
            name: name,
            category: category,
            phase: TracePhase.Complete,
            ts_ns: start_ns,
            dur_ns: dur_ns,
            pid: _.pid,
            tid: _.tid,
        });
    }

    pub fn len(): usize {
        return _.count;
    }

    pub fn totalRecorded(): u64 {
        return _.total_recorded;
    }

    pub fn droppedCount(): u64 {
        return _.dropped;
    }

    /// Returns the `index`-th oldest event currently stored in the ring buffer.
    pub fn get(index: usize): TraceEvent? {
        if (index >= _.count || CAP == 0) {
            return null;
        }
        const oldest = if (_.count < CAP) (0 as usize) else _.write_idx;
        const slot = (oldest + index) % CAP;
        return _.events[slot];
    }

    pub fn clear(*_) {
        _.write_idx = 0;
        _.count = 0;
        _.total_recorded = 0;
        _.dropped = 0;
        _.epoch_ns = HwTimer.nowNs();
    }

    static fn writeBytes(out: []u8, pos: usize, s: []const u8): usize {
        if (pos + s.len > out.len) {
            return pos;
        }
        let i: usize = 0;
        while (i < s.len) {
            out[pos + i] = s[i];
            i += 1;
        }
        return pos + s.len;
    }

    static fn writeJsonEscaped(out: []u8, pos: usize, s: []const u8): usize {
        let p = pos;
        p = TraceRingBuffer<CAP>.writeBytes(out, p, "\"");
        let i: usize = 0;
        while (i < s.len && p < out.len) {
            const c = s[i];
            if (c == '"' || c == '\\') {
                if (p + 2 <= out.len) {
                    out[p] = '\\';
                    out[p + 1] = c;
                    p += 2;
                }
            } else if (c == '\n') {
                p = TraceRingBuffer<CAP>.writeBytes(out, p, "\\n");
            } else if (c == '\r') {
                p = TraceRingBuffer<CAP>.writeBytes(out, p, "\\r");
            } else if (c == '\t') {
                p = TraceRingBuffer<CAP>.writeBytes(out, p, "\\t");
            } else {
                out[p] = c;
                p += 1;
            }
            i += 1;
        }
        p = TraceRingBuffer<CAP>.writeBytes(out, p, "\"");
        return p;
    }

    static fn writeMicros(out: []u8, pos: usize, ns: u64): usize {
        const whole = ns / 1000;
        const frac = ns % 1000;
        const written = std.fmt.bufPrint(out[pos..], "{d}.{d:0>3}", .{ whole, frac }) catch return pos;
        return pos + written.len;
    }

    /// Exports all buffered trace events in Chrome Trace Event JSON format (`chrome://tracing` / Perfetto).
    pub fn exportChromeTraceJson(out_buf: []u8): []const u8 {
        let p: usize = 0;
        p = TraceRingBuffer<CAP>.writeBytes(out_buf, p, "{\"traceEvents\":[");
        let i: usize = 0;
        while (i < _.count) {
            if (ev := _.get(i)) {
                if (i > 0) {
                    p = TraceRingBuffer<CAP>.writeBytes(out_buf, p, ",");
                }
                p = TraceRingBuffer<CAP>.writeBytes(out_buf, p, "{\"name\":");
                p = TraceRingBuffer<CAP>.writeJsonEscaped(out_buf, p, ev.name);
                p = TraceRingBuffer<CAP>.writeBytes(out_buf, p, ",\"cat\":");
                p = TraceRingBuffer<CAP>.writeJsonEscaped(out_buf, p, ev.category);
                p = TraceRingBuffer<CAP>.writeBytes(out_buf, p, ",\"ph\":\"");
                if (p < out_buf.len) {
                    out_buf[p] = TracePhase.asChar(ev.phase);
                    p += 1;
                }
                p = TraceRingBuffer<CAP>.writeBytes(out_buf, p, "\",\"ts\":");
                p = TraceRingBuffer<CAP>.writeMicros(out_buf, p, ev.ts_ns);
                if (ev.phase == TracePhase.Complete) {
                    p = TraceRingBuffer<CAP>.writeBytes(out_buf, p, ",\"dur\":");
                    p = TraceRingBuffer<CAP>.writeMicros(out_buf, p, ev.dur_ns);
                }
                const tail = std.fmt.bufPrint(out_buf[p..], ",\"pid\":{d},\"tid\":{d}}}", .{ ev.pid, ev.tid }) catch "";
                p += tail.len;
            }
            i += 1;
        }
        p = TraceRingBuffer<CAP>.writeBytes(out_buf, p, "]}");
        return out_buf[0..p];
    }
}

pub type PerfTrace = TraceRingBuffer<256>;
