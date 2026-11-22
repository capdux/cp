// Origin: Pure-CP Stdlib Test Suite (lib/ws/hmrSocket.cp) | Created: 2026-11-22
// Production Deep Verification Suite for DevServer HMR WebSocket Protocol & Session:
//   1. isHmrLocalhostIp & requireLocalhost privilege checks (IPv4/IPv6 loopback vs remote)
//   2. 17-byte HMR Version frame encode/decode & malformed frame guards
//   3. 9-byte HMR Init frame encode/decode, source_map_key derivation & invalid hex guards
//   4. HmrTopicBits & applySubscribePayload added/removed diffing + TooManyTopics guard
//   5. Active route switching & deduplicated SourceMap tracking/untracking
//   6. closeAndDrain leak-free teardown & SourceMapCapacityExceeded (32 slots) guard

import { test, expect_eq, expect_true } from "cap:test";
import {
    HMR_VERSION_FRAME_LEN,
    HMR_INIT_FRAME_LEN,
    HMR_MAX_REFERENCED_SOURCEMAPS,
    HmrSocketError,
    HmrIncomingOpcode,
    HmrTopicBits,
    isHmrLocalhostIp,
    encodeHmrVersionFrame,
    decodeHmrVersionFrame,
    encodeHmrInitFrame,
    decodeHmrInitFrame,
    HmrSocketSession,
} from "cap:ws";

test("HmrSocket: localhost IP check & privilege enforcement", fn() {
    expect_true(isHmrLocalhostIp("127.0.0.1", false));
    expect_true(isHmrLocalhostIp("::1", true));
    expect_true(!isHmrLocalhostIp("127.0.0.1", true));
    expect_true(!isHmrLocalhostIp("::1", false));
    expect_true(!isHmrLocalhostIp("192.168.1.5", false));

    const local_sess = HmrSocketSession.init("127.0.0.1", false);
    expect_true(local_sess.requireLocalhost() catch false);

    const remote_sess = HmrSocketSession.init("10.0.0.2", false);
    let unauth = false;
    remote_sess.requireLocalhost() catch |e| {
        if (e == HmrSocketError.UnauthorizedNonLocalhost) unauth = true;
    };
    expect_true(unauth);
});

test("HmrSocket: 17-byte Version frame roundtrip & error guards", fn() {
    let hash: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i < 16) {
        hash[i] = @as(u8, i + 1);
        i += 1;
    }

    let wire: [32]u8 = [0; 32];
    const encoded = encodeHmrVersionFrame(hash, wire[0..]) catch "";
    expect_eq(encoded.len, HMR_VERSION_FRAME_LEN);
    expect_eq(encoded[0], 'V');

    const decoded = decodeHmrVersionFrame(encoded) catch unreachable;
    expect_eq(decoded[0], 1);
    expect_eq(decoded[15], 16);

    // Malformed frames: empty, wrong opcode, wrong length
    let empty_err = false;
    decodeHmrVersionFrame("") catch |e| {
        if (e == HmrSocketError.EmptyMessage) empty_err = true;
    };
    expect_true(empty_err);

    let op_err = false;
    decodeHmrVersionFrame("X0123456789abcdef") catch |e| {
        if (e == HmrSocketError.InvalidOpcode) op_err = true;
    };
    expect_true(op_err);

    let len_err = false;
    decodeHmrVersionFrame("Vshort") catch |e| {
        if (e == HmrSocketError.InvalidFrameLength) len_err = true;
    };
    expect_true(len_err);
});

test("HmrSocket: 9-byte Init frame roundtrip & invalid hex guards", fn() {
    let wire: [16]u8 = [0; 16];
    const encoded = encodeHmrInitFrame(0x1a2b3c4d, wire[0..]) catch "";
    expect_eq(encoded.len, HMR_INIT_FRAME_LEN);
    expect_eq(encoded, "i1a2b3c4d");

    const parsed = decodeHmrInitFrame(encoded) catch unreachable;
    expect_eq(parsed.generation_id, 0x1a2b3c4d);
    expect_eq(parsed.source_map_key, @as(u64, 0x1a2b3c4d) << 32);

    // Uppercase hex also valid
    const parsed_up = decodeHmrInitFrame("iDEADBEEF") catch unreachable;
    expect_eq(parsed_up.generation_id, 0xdeadbeef);

    // Invalid hex character
    let hex_err = false;
    decodeHmrInitFrame("i1234567z") catch |e| {
        if (e == HmrSocketError.InvalidHexEncoding) hex_err = true;
    };
    expect_true(hex_err);

    if (op := HmrIncomingOpcode.fromByte('s')) {
        expect_eq(op, HmrIncomingOpcode.Subscribe);
    } else {
        expect_true(false);
    }
    expect_true(HmrIncomingOpcode.fromByte('?') == null);
});

test("HmrSocketSession: topic subscription bitmask diffing & TooManyTopics guard", fn() {
    let sess = HmrSocketSession.init("::1", true);
    expect_true(sess.subscriptions.isEmpty());

    // Subscribe to hot_update ('h') + errors ('e')
    const d1 = sess.applySubscribePayload("he") catch unreachable;
    expect_true(d1.added.hot_update && d1.added.errors);
    expect_true(!d1.added.incremental_visualizer);
    expect_true(d1.removed.isEmpty());

    // Change to errors ('e') + memory_visualizer ('m') -> added: m, removed: h
    const d2 = sess.applySubscribePayload("em") catch unreachable;
    expect_true(d2.added.memory_visualizer);
    expect_true(!d2.added.errors);
    expect_true(d2.removed.hot_update);
    expect_true(!d2.removed.errors);
    expect_true(sess.subscriptions.errors && sess.subscriptions.memory_visualizer);
    expect_true(!sess.subscriptions.hot_update);

    // Payload exceeding HMR_TOPIC_MAX_COUNT (8) -> TooManyTopics
    let too_many = false;
    sess.applySubscribePayload("hevmhevmx") catch |e| {
        if (e == HmrSocketError.TooManyTopics) too_many = true;
    };
    expect_true(too_many);
});

test("HmrSocketSession: active route switching & SourceMap dedup/untrack", fn() {
    let sess = HmrSocketSession.init("127.0.0.1", false);
    expect_true(sess.setActiveRoute(10) == null);
    if (prev := sess.setActiveRoute(20)) {
        expect_eq(prev, 10);
    } else {
        expect_true(false);
    }

    expect_true(sess.trackSourceMap(0x100) catch false);
    expect_true(!(sess.trackSourceMap(0x100) catch true)); // dedup
    expect_true(sess.trackSourceMap(0x200) catch false);
    expect_eq(sess.referencedSourceMapCount(), 2);

    expect_true(sess.untrackSourceMap(0x100));
    expect_true(!sess.untrackSourceMap(0x100));
    expect_eq(sess.referencedSourceMapCount(), 1);
});

test("HmrSocketSession: closeAndDrain teardown & SourceMapCapacityExceeded guard", fn() {
    let sess = HmrSocketSession.init("127.0.0.1", false);
    sess.inspector_connection_id = 7;
    * = sess.setActiveRoute(3);
    * = sess.applySubscribePayload("hev") catch unreachable;

    // Fill all 32 SourceMap reference slots
    let k: u64 = 1;
    while (k <= @as(u64, HMR_MAX_REFERENCED_SOURCEMAPS)) {
        expect_true(sess.trackSourceMap(k) catch false);
        k += 1;
    }

    // 33rd unique SourceMap key -> SourceMapCapacityExceeded
    let cap_err = false;
    sess.trackSourceMap(999) catch |e| {
        if (e == HmrSocketError.SourceMapCapacityExceeded) cap_err = true;
    };
    expect_true(cap_err);

    // closeAndDrain returns all tracked state and resets session cleanly
    let drained: [32]u64 = [0; 32];
    const summary = sess.closeAndDrain(drained[0..]);
    expect_eq(summary.total_source_maps, HMR_MAX_REFERENCED_SOURCEMAPS);
    expect_eq(summary.drained_source_maps, HMR_MAX_REFERENCED_SOURCEMAPS);
    expect_eq(summary.prev_inspector_id, 7);
    expect_true(summary.removed_topics.hot_update && summary.removed_topics.errors && summary.removed_topics.incremental_visualizer);
    if (r := summary.prev_route_id) {
        expect_eq(r, 3);
    } else {
        expect_true(false);
    }

    // Session is completely clean after closeAndDrain
    expect_eq(sess.referencedSourceMapCount(), 0);
    expect_true(sess.subscriptions.isEmpty());
    expect_true(sess.active_route_id == null);
    expect_eq(sess.inspector_connection_id, -1);
});
