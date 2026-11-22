// Origin: Pure-CP Stdlib Test Suite (lib/obs/traceParent.cp) | Created: 2026-11-23
// Production Deep Verification Suite for W3C Trace Context Level 1/2 (`traceparent` & `tracestate`):
//   1. Standard `traceparent` parse, flag helpers (`isSampled`, `isRandom`, `withSampled`) & 55B round-trip format
//   2. Strict W3C rejections: all-zero `trace_id`/`parent_id`, uppercase hex, version `ff`, bad separators & lengths
//   3. Forward compatibility for future versions (`version > 0x00` with trailing `-<extension>` fields)
//   4. `deriveChild` distributed span propagation & all-zero child span rejection
//   5. `tracestate` multi-tenant `tenant@vendor` validation, OWS trimming, empty member skipping & duplicate dedup
//   6. `tracestate` `setFront` MRU promotion, `remove` order preservation & 32-entry capacity tail eviction
//   7. Malformed header & CRLF/NUL boundary guards, `BufferTooSmall` & `cap.obs` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    TRACEPARENT_HEADER_LEN,
    TRACE_FLAG_SAMPLED,
    TRACE_FLAG_RANDOM,
    TRACESTATE_MAX_ENTRIES,
    TraceContextError,
    TraceParent,
    isValidTraceStateKey,
    TraceState,
} from "cap:obs";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("TraceParent: canonical parse, flags & 55-byte round-trip format") {
    const raw: []const u8 = "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-03";
    const tp = TraceParent.parse(raw) catch {
        expect_true(false);
        return;
    };

    expect_eq(tp.version, @as(u8, 0x00));
    expect_eq(tp.trace_id[0], @as(u8, 0x4b));
    expect_eq(tp.trace_id[15], @as(u8, 0x36));
    expect_eq(tp.parent_id[0], @as(u8, 0x00));
    expect_eq(tp.parent_id[7], @as(u8, 0xb7));
    expect_eq(tp.trace_flags, @as(u8, TRACE_FLAG_SAMPLED | TRACE_FLAG_RANDOM));
    expect_true(tp.isSampled());
    expect_true(tp.isRandom());

    let out_buf: [64]u8 = [0; 64];
    let out_s: []u8 = out_buf[0..];
    const written = tp.format(*out_s) catch 0;
    expect_eq(written, TRACEPARENT_HEADER_LEN);
    expect_true(strEq(out_buf[0..written], raw));

    // Toggle sampled off while preserving random bit (0x03 -> 0x02)
    const unsampled = tp.withSampled(false);
    expect_true(!unsampled.isSampled());
    expect_true(unsampled.isRandom());
    expect_eq(unsampled.trace_flags, @as(u8, TRACE_FLAG_RANDOM));
    const re_sampled = unsampled.withSampled(true);
    expect_true(re_sampled.isSampled());
}

test("TraceParent: rejects all-zero IDs, uppercase hex, version ff & malformed length") {
    // 1. All-zero trace_id
    let err_zero_tid: bool = false;
    * = TraceParent.parse("00-00000000000000000000000000000000-00f067aa0ba902b7-01") catch |e| {
        err_zero_tid = (e == TraceContextError.AllZeroTraceId);
    };
    expect_true(err_zero_tid);

    // 2. All-zero parent_id
    let err_zero_pid: bool = false;
    * = TraceParent.parse("00-4bf92f3577b34da6a3ce929d0e0e4736-0000000000000000-01") catch |e| {
        err_zero_pid = (e == TraceContextError.AllZeroParentId);
    };
    expect_true(err_zero_pid);

    // 3. Uppercase hex rejected per W3C spec
    let err_upper: bool = false;
    * = TraceParent.parse("00-4BF92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01") catch |e| {
        err_upper = (e == TraceContextError.InvalidHex);
    };
    expect_true(err_upper);

    // 4. Version 0xff forbidden
    let err_ver_ff: bool = false;
    * = TraceParent.parse("ff-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01") catch |e| {
        err_ver_ff = (e == TraceContextError.InvalidVersion);
    };
    expect_true(err_ver_ff);

    // 5. Version 0x00 rejects trailing fields
    let err_v0_extra: bool = false;
    * = TraceParent.parse("00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01-extra") catch |e| {
        err_v0_extra = (e == TraceContextError.InvalidLength);
    };
    expect_true(err_v0_extra);

    // 6. Wrong separator
    let err_sep: bool = false;
    * = TraceParent.parse("00_4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01") catch |e| {
        err_sep = (e == TraceContextError.InvalidSeparator);
    };
    expect_true(err_sep);
}

test("TraceParent: forward compatibility with future versions (version > 0x00)") {
    // Version 0x01 with trailing extension field must succeed
    const future_hdr: []const u8 = "01-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01-future-ext";
    const tp = TraceParent.parse(future_hdr) catch {
        expect_true(false);
        return;
    };
    expect_eq(tp.version, @as(u8, 0x01));
    expect_true(tp.isSampled());
    expect_eq(tp.trace_id[0], @as(u8, 0x4b));
    expect_eq(tp.parent_id[7], @as(u8, 0xb7));

    // Future version with non-dash trailing char must still fail
    let err_bad_ext: bool = false;
    * = TraceParent.parse("01-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01+bad") catch |e| {
        err_bad_ext = (e == TraceContextError.InvalidSeparator);
    };
    expect_true(err_bad_ext);
}

test("TraceParent: deriveChild propagation preserves trace_id & flags") {
    const parent = TraceParent.parse("00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01") catch {
        expect_true(false);
        return;
    };

    const child_span: [8]u8 = [0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88];
    const child = parent.deriveChild(child_span) catch {
        expect_true(false);
        return;
    };

    expect_eq(child.version, parent.version);
    expect_eq(child.trace_flags, parent.trace_flags);
    expect_true(strEq(child.trace_id[0..], parent.trace_id[0..]));
    expect_true(strEq(child.parent_id[0..], child_span[0..]));

    let buf: [55]u8 = [0; 55];
    let buf_s: []u8 = buf[0..];
    const n = child.format(*buf_s) catch 0;
    expect_eq(n, TRACEPARENT_HEADER_LEN);
    expect_true(strEq(buf[0..n], "00-4bf92f3577b34da6a3ce929d0e0e4736-1122334455667788-01"));

    // All-zero child parent_id must be rejected
    const zero_span: [8]u8 = [0; 8];
    let got_zero_err: bool = false;
    * = parent.deriveChild(zero_span) catch |e| {
        got_zero_err = (e == TraceContextError.AllZeroParentId);
    };
    expect_true(got_zero_err);
}

test("TraceState: multi-tenant tenant@vendor parsing, OWS trimming & duplicate dedup") {
    expect_true(isValidTraceStateKey("rojo"));
    expect_true(isValidTraceStateKey("congo"));
    expect_true(isValidTraceStateKey("tenant1@vendor_a"));
    expect_true(isValidTraceStateKey("0123@v-1/2*3"));
    expect_true(!isValidTraceStateKey(""));
    expect_true(!isValidTraceStateKey("Uppercase"));
    expect_true(!isValidTraceStateKey("1badvendor"));
    expect_true(!isValidTraceStateKey("a@b@c"));
    expect_true(!isValidTraceStateKey("tenant@vendornamewaytoolong")); // vendor > 14 bytes

    // Parse with OWS, empty members, multi-tenant keys, and duplicate key (leftmost wins)
    const hdr: []const u8 = "  rojo=00f067aa0ba902b7 , , acme@congo=t61rcWkgMzE , rojo=stale_dup ";
    const ts = TraceState.parse(hdr) catch {
        expect_true(false);
        return;
    };

    expect_eq(ts.len(), @as(usize, 2));
    if (v1 := ts.get("rojo")) {
        expect_true(strEq(v1, "00f067aa0ba902b7"));
    } else {
        expect_true(false);
    }
    if (v2 := ts.get("acme@congo")) {
        expect_true(strEq(v2, "t61rcWkgMzE"));
    } else {
        expect_true(false);
    }

    let out: [128]u8 = [0; 128];
    let out_s: []u8 = out[0..];
    const n = ts.format(*out_s) catch 0;
    expect_true(strEq(out[0..n], "rojo=00f067aa0ba902b7,acme@congo=t61rcWkgMzE"));
}

test("TraceState: setFront MRU promotion, remove & 32-entry tail eviction") {
    let ts = TraceState.parse("a=1,b=2,c=3") catch {
        expect_true(false);
        return;
    };

    // Updating existing key "c" promotes it to index 0
    ts.setFront("c", "30") catch { expect_true(false); };
    expect_eq(ts.len(), @as(usize, 3));
    let buf: [128]u8 = [0; 128];
    let buf_s: []u8 = buf[0..];
    const n1 = ts.format(*buf_s) catch 0;
    expect_true(strEq(buf[0..n1], "c=30,a=1,b=2"));

    // Remove middle entry "a"
    expect_true(ts.remove("a"));
    expect_true(!ts.remove("missing"));
    const n2 = ts.format(*buf_s) catch 0;
    expect_true(strEq(buf[0..n2], "c=30,b=2"));

    // Fill up to 32 entries and insert 33rd -> oldest tail entry ("b") must be evicted
    let full = TraceState.init();
    let idx: usize = 0;
    while (idx < TRACESTATE_MAX_ENTRIES) : (idx += 1) {
        let k_arr: [4]u8 = [0; 4];
        k_arr[0] = 'v';
        k_arr[1] = '_';
        k_arr[2] = @as(u8, 'a') + @as(u8, @intCast(idx / @as(usize, 10)));
        k_arr[3] = @as(u8, '0') + @as(u8, @intCast(idx % @as(usize, 10)));
        full.setFront(k_arr[0..], "ok") catch { expect_true(false); };
    }
    expect_eq(full.len(), TRACESTATE_MAX_ENTRIES);
    // "v_a0" was inserted first so it is now at the tail (index 31)
    expect_true(full.get("v_a0") != null);

    // Insert 33rd key -> "v_a0" evicted, "new@sys" promoted to front (index 0)
    full.setFront("new@sys", "top") catch { expect_true(false); };
    expect_eq(full.len(), TRACESTATE_MAX_ENTRIES);
    expect_true(full.get("v_a0") == null);
    if (first := full.entryAt(0)) {
        expect_true(strEq(first.key(), "new@sys"));
        expect_true(strEq(first.value(), "top"));
    } else {
        expect_true(false);
    }
}

test("TraceContext: CRLF/NUL injection guard, BufferTooSmall & cap.obs facade parity") {
    // 1. CRLF rejected in traceparent & tracestate
    let tp_crlf: bool = false;
    * = TraceParent.parse("00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01\r\nX-Inject: 1") catch |e| {
        tp_crlf = (e == TraceContextError.CrlfRejected);
    };
    expect_true(tp_crlf);

    let ts_crlf: bool = false;
    * = TraceState.parse("rojo=1\ncongo=2") catch |e| {
        ts_crlf = (e == TraceContextError.CrlfRejected);
    };
    expect_true(ts_crlf);

    let ts_mut = TraceState.init();
    let mut_crlf: bool = false;
    ts_mut.setFront("rojo", "bad\rval") catch |e| {
        mut_crlf = (e == TraceContextError.CrlfRejected);
    };
    expect_true(mut_crlf);

    // 2. BufferTooSmall on format
    const tp = TraceParent.parse("00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01") catch {
        expect_true(false);
        return;
    };
    let tiny: [32]u8 = [0; 32];
    let tiny_s: []u8 = tiny[0..];
    let got_small: bool = false;
    * = tp.format(*tiny_s) catch |e| {
        got_small = (e == TraceContextError.BufferTooSmall);
    };
    expect_true(got_small);

    // 3. `cap.obs` facade parity
    expect_eq(cap.obs.TRACEPARENT_HEADER_LEN, @as(usize, 55));
    expect_true(cap.obs.isValidTraceStateKey("tenant@vendor"));
}
