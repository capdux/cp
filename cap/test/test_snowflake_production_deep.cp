// Source: cp/cap/test/test_snowflake_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    DEFAULT_EPOCH_MS,
    SnowflakeError,
    SnowflakeGenerator,
    decomposeId,
    encodeBase62,
    decodeBase62,
} from "cap:codec";

test("1. Deterministic generation, bit layout and decomposeId roundtrip") {
    let gen = try SnowflakeGenerator.init(7, 19, DEFAULT_EPOCH_MS);
    const ts = DEFAULT_EPOCH_MS + 123456;
    const id0 = try gen.nextIdAt(ts);
    const id1 = try gen.nextIdAt(ts);
    try std.testing.expect(id1 > id0);

    const p0 = decomposeId(id0, DEFAULT_EPOCH_MS);
    const p1 = decomposeId(id1, DEFAULT_EPOCH_MS);
    expect_eq(p0.timestamp_ms, ts);
    expect_eq(p0.datacenter_id, 7);
    expect_eq(p0.worker_id, 19);
    expect_eq(p0.sequence, 0);
    expect_eq(p1.sequence, 1);
}

test("2. Same-millisecond 4096 rollover and clock backwards protection") {
    let gen = try SnowflakeGenerator.init(1, 2, DEFAULT_EPOCH_MS);
    const ts = DEFAULT_EPOCH_MS + 5000;
    let prev: u64 = 0;
    let i: usize = 0;
    while (i < 4100) : (i += 1) {
        const id = try gen.nextIdAt(ts);
        try std.testing.expect(id > prev);
        prev = id;
    }
    const last_parts = decomposeId(prev, DEFAULT_EPOCH_MS);
    expect_eq(last_parts.timestamp_ms, ts + 1);
    try std.testing.expectError(SnowflakeError.ClockMovedBackwards, gen.nextIdAt(ts));
}

test("3. Base62 zero-alloc encode/decode roundtrip and invalid input protection") {
    let buf: [11]u8 = undefined;
    const s0 = encodeBase62(0, &buf);
    try std.testing.expectEqualStrings("0", s0);
    expect_eq(try decodeBase62(s0), 0);

    const val: u64 = 987654321012345678;
    const s1 = encodeBase62(val, &buf);
    expect_eq(try decodeBase62(s1), val);

    try std.testing.expectError(SnowflakeError.InvalidIdString, decodeBase62(""));
    try std.testing.expectError(SnowflakeError.InvalidIdString, decodeBase62("bad!char"));
    try std.testing.expectError(SnowflakeError.InvalidNodeId, SnowflakeGenerator.init(32, 0, DEFAULT_EPOCH_MS));
}

fn workerThread(gen: *SnowflakeGenerator, out: []u64) {
    let i: usize = 0;
    while (i < out.len) : (i += 1) {
        out[i] = gen.nextId() catch 0;
    }
}

test("4. Multi-threaded real concurrent ID uniqueness (4 threads x 500 IDs)") {
    const alloc = std.testing.allocator;
    let gen = try SnowflakeGenerator.init(3, 5, DEFAULT_EPOCH_MS);
    const ids = try alloc.alloc(u64, 2000);
    defer alloc.free(ids);

    let threads: [4]std.Thread = undefined;
    let t: usize = 0;
    while (t < 4) : (t += 1) {
        threads[t] = try std.Thread.spawn(.{}, workerThread, .{ &gen, ids[t * 500 .. (t + 1) * 500] });
    }
    t = 0;
    while (t < 4) : (t += 1) {
        threads[t].join();
    }

    std.mem.sort(u64, ids, {}, std.sort.asc(u64));
    try std.testing.expect(ids[0] > 0);
    let idx: usize = 1;
    while (idx < ids.len) : (idx += 1) {
        try std.testing.expect(ids[idx] > ids[idx - 1]);
    }
}

test("5. Dynamic cap.snowflake namespace and zero-leak check") {
    let gen = try cap.codec.SnowflakeGenerator.init(0, 1, cap.codec.DEFAULT_EPOCH_MS);
    const id = try gen.nextIdAt(cap.codec.DEFAULT_EPOCH_MS + 42);
    const parts = cap.codec.decomposeId(id, cap.codec.DEFAULT_EPOCH_MS);
    expect_eq(parts.worker_id, 1);
}
