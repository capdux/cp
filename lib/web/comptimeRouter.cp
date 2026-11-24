// Origin: Pure-CP Stdlib (Comptime Static Router + Elysia-Style Lifecycle Trace: cap:web) | Created: 2026-11-24
// ============================================================================
// Pure-CP Zero-Heap Compile-Time Static Route Dispatcher & Lifecycle Tracer
//   - NestJS-style declarative `StaticRouteSpec` route tables
//   - Zig/CP `comptime`-evaluable FNV-1a hash table + 64-bit length bitmask
//     (zero runtime initialization, O(1) fast-path dispatch, HEAD->GET fallback)
//   - One-call `mountInto(app)` preserving full 8-stage Elysia lifecycle & OpenAPI 3.1.0
//   - Elysia-style `TraceSpan` & `LifecycleTrace` with nanosecond monotonic clock
//     and W3C `Server-Timing` HTTP header formatting (`routing;dur=0.001, handler;dur=0.002`)
// ============================================================================

const std = @import("std");
const builtin = @import("builtin");

import {
    App,
    AppMethod,
    Context,
    Response,
    HandlerFn,
} from "./app.cp";

pub const COMPTIME_MAX_ROUTES: usize = 64;
pub const COMPTIME_HASH_BUCKETS: usize = 128;
pub const COMPTIME_METHOD_SLOTS: usize = 7;
pub const COMPTIME_LEN_BUCKETS: usize = 64;

// ============================================================================
// Internal Zero-Alloc Helpers (Comptime & Runtime Safe)
// ============================================================================

fn ctBytesEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/// Deterministic 64-bit FNV-1a hash (safe for both `comptime` and runtime evaluation).
pub fn fnv1a64(s: string): u64 {
    let h: u64 = ((0xcbf29ce4 as u64) << 32) | (0x84222325 as u64);
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        h = h ^ (s[i] as u64);
        h = h *% 1099511628211;
    }
    return h;
}

/// Monotonic nanosecond clock (zero-alloc vDSO on Linux, POSIX clock_gettime elsewhere).
pub fn monotonicNowNs(): u64 {
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

fn ctWriteByte(out_buf: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out_buf.len) return false;
    out_buf[pos.*] = b;
    pos.* += 1;
    return true;
}

fn ctWriteRaw(out_buf: []u8, pos: *usize, s: string): bool {
    if (s.len > out_buf.len || pos.* > out_buf.len - s.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn ctWriteU64Decimal(out_buf: []u8, pos: *usize, val: u64): bool {
    if (val == 0) {
        return ctWriteByte(out_buf, pos, '0');
    }
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    while (v > 0) {
        tmp[n] = (48 + ((v % 10) as u8)) as u8;
        n += 1;
        v = v / 10;
    }
    while (n > 0) {
        n -= 1;
        if (!ctWriteByte(out_buf, pos, tmp[n])) return false;
    }
    return true;
}

fn ctWriteDurationMs3(out_buf: []u8, pos: *usize, duration_ns: u64): bool {
    const ms_whole: u64 = duration_ns / 1000000;
    const ms_frac: u64 = (duration_ns % 1000000) / 1000;
    if (!ctWriteU64Decimal(out_buf, pos, ms_whole)) return false;
    if (!ctWriteByte(out_buf, pos, '.')) return false;
    const d1: u8 = (48 + ((ms_frac / 100) as u8)) as u8;
    const d2: u8 = (48 + (((ms_frac / 10) % 10) as u8)) as u8;
    const d3: u8 = (48 + ((ms_frac % 10) as u8)) as u8;
    if (!ctWriteByte(out_buf, pos, d1)) return false;
    if (!ctWriteByte(out_buf, pos, d2)) return false;
    return ctWriteByte(out_buf, pos, d3);
}

// ============================================================================
// 1. NestJS-Style Declarative Route Specification
// ============================================================================

/// Declarative static route entry (compatible with `comptime` constant tables).
pub struct StaticRouteSpec {
    method: AppMethod,
    path: string,
    handler: HandlerFn,
    summary: string = "",
}

/// Declarative `@Get` helper for building `StaticRouteSpec` entries.
pub fn routeGet(path: string, handler: HandlerFn, summary: string): StaticRouteSpec {
    return StaticRouteSpec{
        method: AppMethod.GET,
        path: path,
        handler: handler,
        summary: summary,
    };
}

/// Declarative `@Post` helper for building `StaticRouteSpec` entries.
pub fn routePost(path: string, handler: HandlerFn, summary: string): StaticRouteSpec {
    return StaticRouteSpec{
        method: AppMethod.POST,
        path: path,
        handler: handler,
        summary: summary,
    };
}

/// Declarative `@Put` helper for building `StaticRouteSpec` entries.
pub fn routePut(path: string, handler: HandlerFn, summary: string): StaticRouteSpec {
    return StaticRouteSpec{
        method: AppMethod.PUT,
        path: path,
        handler: handler,
        summary: summary,
    };
}

/// Declarative `@Delete` helper for building `StaticRouteSpec` entries.
pub fn routeDelete(path: string, handler: HandlerFn, summary: string): StaticRouteSpec {
    return StaticRouteSpec{
        method: AppMethod.DELETE,
        path: path,
        handler: handler,
        summary: summary,
    };
}

/// Declarative `@Patch` helper for building `StaticRouteSpec` entries.
pub fn routePatch(path: string, handler: HandlerFn, summary: string): StaticRouteSpec {
    return StaticRouteSpec{
        method: AppMethod.PATCH,
        path: path,
        handler: handler,
        summary: summary,
    };
}

/// Declarative `@Head` helper for building `StaticRouteSpec` entries.
pub fn routeHead(path: string, handler: HandlerFn, summary: string): StaticRouteSpec {
    return StaticRouteSpec{
        method: AppMethod.HEAD,
        path: path,
        handler: handler,
        summary: summary,
    };
}

/// Declarative `@Options` helper for building `StaticRouteSpec` entries.
pub fn routeOptions(path: string, handler: HandlerFn, summary: string): StaticRouteSpec {
    return StaticRouteSpec{
        method: AppMethod.OPTIONS,
        path: path,
        handler: handler,
        summary: summary,
    };
}

// ============================================================================
// 2. Comptime-Precomputed Static Route Table (Length Bitmask + FNV-1a Buckets)
// ============================================================================

pub struct ComptimeBucket {
    occupied: bool = false,
    hash: u64 = 0,
    path: string = "",
    path_len: usize = 0,
    route_indices: [COMPTIME_METHOD_SLOTS]i16 = [-1; COMPTIME_METHOD_SLOTS],
    allowed_mask: u8 = 0,
}

pub struct StaticLookupResult {
    matched: bool = false,
    route_idx: i16 = -1,
    allowed_mask: u8 = 0,
    head_fallback_to_get: bool = false,
}

pub struct ComptimeRouteTable {
    routes: [COMPTIME_MAX_ROUTES]StaticRouteSpec? = [null; COMPTIME_MAX_ROUTES],
    route_count: usize = 0,
    buckets: [COMPTIME_HASH_BUCKETS]ComptimeBucket = [ComptimeBucket{}; COMPTIME_HASH_BUCKETS],
    len_mask: u64 = 0,
    len_counts: [COMPTIME_LEN_BUCKETS]u8 = [0; COMPTIME_LEN_BUCKETS],
    min_path_len: usize = 0,
    max_path_len: usize = 0,
    unique_paths: usize = 0,
    collision_count: usize = 0,

    pub static fn init(): ComptimeRouteTable {
        return ComptimeRouteTable{};
    }

    /// Compile a slice of `StaticRouteSpec` into a pre-hashed, length-bucketed O(1) table.
    /// Safe to evaluate at `comptime` (`const TABLE = ComptimeRouteTable.compile(SPECS[0..]);`)
    /// so all FNV-1a hashes, length bitmasks, and buckets live in `.rodata` with zero init cost.
    pub static fn compile(specs: []const StaticRouteSpec): ComptimeRouteTable {
        let table = ComptimeRouteTable.init();
        let i: usize = 0;
        while (i < specs.len) : (i += 1) {
            * = table.insert(specs[i]);
        }
        return table;
    }

    /// Insert a single `StaticRouteSpec` into the route table (works at `comptime` and runtime).
    pub fn insert(*_, spec: StaticRouteSpec): bool {
        if (_.route_count >= COMPTIME_MAX_ROUTES) return false;
        const m_idx = spec.method.methodIndex();
        if (m_idx >= COMPTIME_METHOD_SLOTS) return false;

        const r_idx = _.route_count;
        _.routes[r_idx] = spec;
        _.route_count += 1;

        const plen = spec.path.len;
        if (_.unique_paths == 0 && r_idx == 0) {
            _.min_path_len = plen;
            _.max_path_len = plen;
        } else {
            if (plen < _.min_path_len) _.min_path_len = plen;
            if (plen > _.max_path_len) _.max_path_len = plen;
        }
        if (plen < COMPTIME_LEN_BUCKETS) {
            _.len_mask = _.len_mask | ((1 as u64) << (plen as u6));
        }

        const h = fnv1a64(spec.path);
        const mask = COMPTIME_HASH_BUCKETS - 1;
        let slot = (h as usize) & mask;
        let probe: usize = 0;
        while (probe < COMPTIME_HASH_BUCKETS) : (probe += 1) {
            if (!_.buckets[slot].occupied) {
                _.buckets[slot].occupied = true;
                _.buckets[slot].hash = h;
                _.buckets[slot].path = spec.path;
                _.buckets[slot].path_len = plen;
                _.buckets[slot].route_indices[m_idx] = r_idx as i16;
                _.buckets[slot].allowed_mask = _.buckets[slot].allowed_mask | spec.method.bitMask();
                _.unique_paths += 1;
                if (plen < COMPTIME_LEN_BUCKETS && _.len_counts[plen] < 255) {
                    _.len_counts[plen] += 1;
                }
                if (probe > 0) {
                    _.collision_count += 1;
                }
                return true;
            }
            if (_.buckets[slot].hash == h
                && _.buckets[slot].path_len == plen
                && ctBytesEql(_.buckets[slot].path, spec.path))
            {
                _.buckets[slot].route_indices[m_idx] = r_idx as i16;
                _.buckets[slot].allowed_mask = _.buckets[slot].allowed_mask | spec.method.bitMask();
                return true;
            }
            slot = (slot + 1) & mask;
        }
        return false;
    }

    /// Number of registered static routes.
    pub fn count(): usize {
        return _.route_count;
    }

    /// O(1) check whether any static route has length `len`.
    pub fn hasPathLen(len: usize): bool {
        if (_.route_count == 0) return false;
        if (len < _.min_path_len || len > _.max_path_len) return false;
        if (len < COMPTIME_LEN_BUCKETS) {
            return (_.len_mask & ((1 as u64) << (len as u6))) != 0;
        }
        return true;
    }

    /// Number of unique static paths in length bucket `len` (`len < 64`).
    pub fn bucketCountForLen(len: usize): u8 {
        if (len >= COMPTIME_LEN_BUCKETS) return 0;
        return _.len_counts[len];
    }

    /// O(1) lookup combining length-bitmask pre-filter + FNV-1a open-addressed table.
    /// Automatically supports Axum-style `HEAD -> GET` fallback.
    pub fn lookup(method: AppMethod, path: string): StaticLookupResult {
        let res = StaticLookupResult{};
        if (_.route_count == 0) return res;

        const plen = path.len;
        if (plen < _.min_path_len || plen > _.max_path_len) return res;
        if (plen < COMPTIME_LEN_BUCKETS) {
            if ((_.len_mask & ((1 as u64) << (plen as u6))) == 0) return res;
        }

        const m_idx = method.methodIndex();
        const h = fnv1a64(path);
        const mask = COMPTIME_HASH_BUCKETS - 1;
        let slot = (h as usize) & mask;
        let probe: usize = 0;
        while (probe < COMPTIME_HASH_BUCKETS) : (probe += 1) {
            if (!_.buckets[slot].occupied) return res;
            if (_.buckets[slot].hash == h
                && _.buckets[slot].path_len == plen
                && ctBytesEql(_.buckets[slot].path, path))
            {
                res.allowed_mask = _.buckets[slot].allowed_mask;
                const r_idx = _.buckets[slot].route_indices[m_idx];
                if (r_idx >= 0) {
                    res.matched = true;
                    res.route_idx = r_idx;
                    return res;
                }
                // Automatic HEAD -> GET fallback when GET is registered
                if (m_idx == 5 && _.buckets[slot].route_indices[0] >= 0) {
                    res.matched = true;
                    res.route_idx = _.buckets[slot].route_indices[0];
                    res.head_fallback_to_get = true;
                    return res;
                }
                return res;
            }
            slot = (slot + 1) & mask;
        }
        return res;
    }

    /// O(1) zero-branch-heavy fast-path dispatch directly into the matched static handler.
    /// Returns `null` on miss (caller may fall back to `App.dispatch()` for dynamic routes / 404).
    pub fn dispatchStatic(method: AppMethod, path: string, ctx: *Context): Response? {
        const hit = _.lookup(method, path);
        if (!hit.matched || hit.route_idx < 0) return null;
        if (spec := _.routes[hit.route_idx as usize]) {
            ctx.method = method;
            ctx.path = path;
            ctx.matched_path = spec.path;
            let resp = spec.handler(ctx);
            if (hit.head_fallback_to_get) {
                resp.body = "";
            }
            return resp;
        }
        return null;
    }

    /// O(1) static dispatch instrumented with `LifecycleTrace` (`routing` + `handler` spans).
    pub fn dispatchStaticWithTrace(
        method: AppMethod,
        path: string,
        ctx: *Context,
        trace: *LifecycleTrace,
    ): Response? {
        trace.startRouting();
        const hit = _.lookup(method, path);
        trace.finishRouting();
        if (!hit.matched || hit.route_idx < 0) return null;
        if (spec := _.routes[hit.route_idx as usize]) {
            ctx.method = method;
            ctx.path = path;
            ctx.matched_path = spec.path;
            trace.startHandler();
            let resp = spec.handler(ctx);
            trace.finishHandler();
            if (hit.head_fallback_to_get) {
                resp.body = "";
            }
            return resp;
        }
        return null;
    }

    /// Mount all routes from this comptime table into an `App` instance,
    /// preserving full 8-stage Elysia lifecycle hooks and OpenAPI 3.1.0 summaries.
    pub fn mountInto(app: *App): usize {
        let mounted: usize = 0;
        let i: usize = 0;
        while (i < _.route_count) : (i += 1) {
            if (spec := _.routes[i]) {
                * = app.routeDoc(spec.method, spec.path, spec.summary, spec.handler);
                mounted += 1;
            }
        }
        return mounted;
    }

    /// NestJS `@Controller(prefix)` style mount into `App` with shared path prefix.
    pub fn mountIntoWithPrefix(app: *App, prefix: string): usize {
        let mounted: usize = 0;
        let i: usize = 0;
        while (i < _.route_count) : (i += 1) {
            if (spec := _.routes[i]) {
                app.registerWithPrefix(spec.method, prefix, spec.path, spec.summary, spec.handler);
                mounted += 1;
            }
        }
        return mounted;
    }
}

// ============================================================================
// 3. Elysia-Style Lifecycle Tracing (`TraceSpan` & `LifecycleTrace`)
// ============================================================================

/// Single nanosecond-precision lifecycle stage timing span.
pub struct TraceSpan {
    name: string = "";
    start_ns: u64 = 0;
    end_ns: u64 = 0;
    duration_ns: u64 = 0;
    recorded: bool = false;

    pub static fn init(name: string): TraceSpan {
        return TraceSpan{
            name: name,
            start_ns: 0,
            end_ns: 0,
            duration_ns: 0,
            recorded: false,
        };
    }

    pub fn begin(*_, now_ns: u64) {
        _.start_ns = now_ns;
    }

    pub fn end(*_, now_ns: u64) {
        _.end_ns = now_ns;
        _.duration_ns = if (now_ns >= _.start_ns) (now_ns - _.start_ns) else 0;
        _.recorded = true;
    }

    pub fn beginNow(*_) {
        _.start_ns = monotonicNowNs();
    }

    pub fn endNow(*_) {
        const now = monotonicNowNs();
        _.end(now);
    }

    pub fn recordDurationNs(*_, dur_ns: u64) {
        _.duration_ns = dur_ns;
        _.recorded = true;
    }

    pub fn durationUs(): u64 {
        return _.duration_ns / 1000;
    }
}

/// Elysia `app.trace()` equivalent: records nanosecond timings for
/// `onRequest`, `routing`, `derive`, `handler`, and `response` stages
/// and emits a W3C `Server-Timing` HTTP header with zero heap allocations.
pub struct LifecycleTrace {
    on_request: TraceSpan = TraceSpan.init("onRequest"),
    routing: TraceSpan = TraceSpan.init("routing"),
    derive: TraceSpan = TraceSpan.init("derive"),
    handler: TraceSpan = TraceSpan.init("handler"),
    response: TraceSpan = TraceSpan.init("response"),
    total_start_ns: u64 = 0,
    total_end_ns: u64 = 0,
    total_recorded: bool = false,

    pub static fn init(): LifecycleTrace {
        return LifecycleTrace{};
    }

    // --- Deterministic timestamp API (for unit tests & custom clocks) ---

    pub fn beginRequest(*_, now_ns: u64) {
        _.total_start_ns = now_ns;
    }

    pub fn endRequest(*_, now_ns: u64) {
        _.total_end_ns = now_ns;
        _.total_recorded = true;
    }

    pub fn beginOnRequest(*_, now_ns: u64) {
        _.on_request.begin(now_ns);
    }

    pub fn endOnRequest(*_, now_ns: u64) {
        _.on_request.end(now_ns);
    }

    pub fn beginRouting(*_, now_ns: u64) {
        _.routing.begin(now_ns);
    }

    pub fn endRouting(*_, now_ns: u64) {
        _.routing.end(now_ns);
    }

    pub fn beginDerive(*_, now_ns: u64) {
        _.derive.begin(now_ns);
    }

    pub fn endDerive(*_, now_ns: u64) {
        _.derive.end(now_ns);
    }

    pub fn beginHandler(*_, now_ns: u64) {
        _.handler.begin(now_ns);
    }

    pub fn endHandler(*_, now_ns: u64) {
        _.handler.end(now_ns);
    }

    pub fn beginResponse(*_, now_ns: u64) {
        _.response.begin(now_ns);
    }

    pub fn endResponse(*_, now_ns: u64) {
        _.response.end(now_ns);
    }

    // --- Live monotonic nanosecond clock convenience API ---

    pub fn startOnRequest(*_) {
        _.on_request.beginNow();
    }

    pub fn finishOnRequest(*_) {
        _.on_request.endNow();
    }

    pub fn startRouting(*_) {
        _.routing.beginNow();
    }

    pub fn finishRouting(*_) {
        _.routing.endNow();
    }

    pub fn startDerive(*_) {
        _.derive.beginNow();
    }

    pub fn finishDerive(*_) {
        _.derive.endNow();
    }

    pub fn startHandler(*_) {
        _.handler.beginNow();
    }

    pub fn finishHandler(*_) {
        _.handler.endNow();
    }

    pub fn startResponse(*_) {
        _.response.beginNow();
    }

    pub fn finishResponse(*_) {
        _.response.endNow();
    }

    /// Total elapsed nanoseconds (overall request span if recorded, otherwise sum of stages).
    pub fn totalDurationNs(): u64 {
        if (_.total_recorded && _.total_end_ns >= _.total_start_ns) {
            return _.total_end_ns - _.total_start_ns;
        }
        let sum: u64 = 0;
        if (_.on_request.recorded) sum += _.on_request.duration_ns;
        if (_.routing.recorded) sum += _.routing.duration_ns;
        if (_.derive.recorded) sum += _.derive.duration_ns;
        if (_.handler.recorded) sum += _.handler.duration_ns;
        if (_.response.recorded) sum += _.response.duration_ns;
        return sum;
    }

    fn appendSpanTiming(out_buf: []u8, pos: *usize, first: *bool, span: TraceSpan): bool {
        if (!span.recorded) return true;
        if (!first.*) {
            if (!ctWriteRaw(out_buf, pos, ", ")) return false;
        }
        if (!ctWriteRaw(out_buf, pos, span.name)) return false;
        if (!ctWriteRaw(out_buf, pos, ";dur=")) return false;
        if (!ctWriteDurationMs3(out_buf, pos, span.duration_ns)) return false;
        first.* = false;
        return true;
    }

    /// Format recorded lifecycle spans into a W3C `Server-Timing` header value
    /// (e.g. `routing;dur=0.001, handler;dur=0.002`). Zero heap allocation.
    pub fn formatTraceLine(out_buf: []u8): usize? {
        let pos: usize = 0;
        let first: bool = true;
        if (!_.appendSpanTiming(out_buf, *pos, *first, _.on_request)) return null;
        if (!_.appendSpanTiming(out_buf, *pos, *first, _.routing)) return null;
        if (!_.appendSpanTiming(out_buf, *pos, *first, _.derive)) return null;
        if (!_.appendSpanTiming(out_buf, *pos, *first, _.handler)) return null;
        if (!_.appendSpanTiming(out_buf, *pos, *first, _.response)) return null;
        return pos;
    }

    /// Format and attach the `Server-Timing` header directly onto `resp`.
    /// Caller owns backing storage `out_buf` for the lifetime of `resp`.
    pub fn attachServerTiming(resp: *Response, out_buf: []u8): bool {
        if (n := _.formatTraceLine(out_buf)) {
            if (n == 0) return true;
            * = resp.setHeader("Server-Timing", out_buf[0..n]);
            return true;
        }
        return false;
    }
}
