// Source: cp/cap/test/test_valkey_production_deep.cp
// Deep Production Test Suite for cap:valkey & cap.valkey (RESP2/RESP3 Codec & Incremental Stream Parser)
// Covers:
//   1. RESP2 & RESP3 all-types lossless round-trip encode/decode + RESP2 downgrade mode
//   2. RespStreamParser 1..7 byte arbitrary chunking (half-packet & sticky-packet) + Incomplete vs ProtocolError
//   3. Nested Map/Set/Push, Attribute skipping, and depth/bulk/collection resource-exhaustion guards
//   4. Command encoding (encodeCommand) and Inline command encoding/parsing
//   5. Dynamic namespace mounting (`import cap from "cap"; cap.valkey`) & `std.testing.allocator` zero leaks

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    ProtocolError,
    RespLimits,
    RespType,
    VerbatimString,
    MapEntry,
    RespValue,
    RespStreamParser,
    parseOne,
    parseOneWithLimits,
    parseInlineCommand,
    encodeValue,
    encodeCommand,
    encodeInlineCommand,
} from "cap:valkey";
import cap from "cap";

fn expectRoundTrip(alloc: std.mem.Allocator, val: RespValue, resp3: bool) {
    const wire = encodeValue(alloc, val, resp3) catch unreachable;
    defer alloc.free(wire);

    const res = parseOne(alloc, wire) catch unreachable;
    defer res.value.deinit(alloc);

    expect_eq(res.consumed, wire.len);
    expect_eq(res.value.eql(val), true);
}

fn expectParseErr(alloc: std.mem.Allocator, wire: []const u8, limits: RespLimits, expected: ProtocolError) {
    const res = parseOneWithLimits(alloc, wire, limits) catch |e| {
        expect_eq(e == expected, true);
        return;
    };
    res.value.deinit(alloc);
    unreachable;
}

test("1. RESP2 & RESP3 All-Types Lossless Round-Trip Encode/Decode") {
    const alloc = std.testing.allocator;

    // Scalar types
    expectRoundTrip(alloc, RespValue{ SimpleString: "OK" }, true);
    expectRoundTrip(alloc, RespValue{ SimpleError: "ERR unknown command" }, true);
    expectRoundTrip(alloc, RespValue{ Integer: 0 }, true);
    expectRoundTrip(alloc, RespValue{ Integer: 42 }, true);
    expectRoundTrip(alloc, RespValue{ Integer: -9876543210 }, true);
    expectRoundTrip(alloc, RespValue{ BulkString: "" }, true);
    expectRoundTrip(alloc, RespValue{ BulkString: "hello\r\n\x00world" }, true);
    expectRoundTrip(alloc, RespValue.Null, true);
    expectRoundTrip(alloc, RespValue{ Boolean: true }, true);
    expectRoundTrip(alloc, RespValue{ Boolean: false }, true);
    expectRoundTrip(alloc, RespValue{ Double: 3.14159 }, true);
    expectRoundTrip(alloc, RespValue{ Double: std.math.inf(f64) }, true);
    expectRoundTrip(alloc, RespValue{ Double: -std.math.inf(f64) }, true);
    expectRoundTrip(alloc, RespValue{ Double: std.math.nan(f64) }, true);
    expectRoundTrip(alloc, RespValue{ BigNumber: "3492890328409238509324850943850943825024385" }, true);
    expectRoundTrip(alloc, RespValue{ BigNumber: "-123456789012345678901234567890" }, true);
    expectRoundTrip(alloc, RespValue{ BulkError: "SYNTAX invalid\r\nline2" }, true);
    expectRoundTrip(alloc, RespValue{ VerbatimString: VerbatimString{ format: "txt", content: "Some string" } }, true);
    expectRoundTrip(alloc, RespValue{ VerbatimString: VerbatimString{ format: "mkd", content: "# Title\n" } }, true);

    // Array
    let arr_items: [3]RespValue = [
        RespValue{ Integer: 1 },
        RespValue{ BulkString: "two" },
        RespValue.Null,
    ];
    expectRoundTrip(alloc, RespValue{ Array: &arr_items }, true);

    // Set
    let set_items: [2]RespValue = [
        RespValue{ SimpleString: "alpha" },
        RespValue{ SimpleString: "beta" },
    ];
    expectRoundTrip(alloc, RespValue{ Set: &set_items }, true);

    // Push
    let push_items: [3]RespValue = [
        RespValue{ BulkString: "message" },
        RespValue{ BulkString: "chan1" },
        RespValue{ BulkString: "payload" },
    ];
    expectRoundTrip(alloc, RespValue{ Push: &push_items }, true);

    // Map
    let map_entries: [2]MapEntry = [
        MapEntry{ key: RespValue{ SimpleString: "first" }, value: RespValue{ Integer: 1 } },
        MapEntry{ key: RespValue{ SimpleString: "second" }, value: RespValue{ Boolean: true } },
    ];
    expectRoundTrip(alloc, RespValue{ Map: &map_entries }, true);

    // RESP2 null & downgrade checks
    const resp2_null_wire = encodeValue(alloc, RespValue.Null, false) catch unreachable;
    defer alloc.free(resp2_null_wire);
    expect_eq(std.mem.eql(u8, resp2_null_wire, "$-1\r\n"), true);

    const resp2_nil_arr = parseOne(alloc, "*-1\r\n") catch unreachable;
    defer resp2_nil_arr.value.deinit(alloc);
    expect_eq(resp2_nil_arr.value.isNull(), true);

    const resp2_bool_wire = encodeValue(alloc, RespValue{ Boolean: true }, false) catch unreachable;
    defer alloc.free(resp2_bool_wire);
    expect_eq(std.mem.eql(u8, resp2_bool_wire, ":1\r\n"), true);
}

test("2. RespStreamParser 1..7 Byte Half-Packet & Sticky-Packet Streaming") {
    const alloc = std.testing.allocator;

    // Build a sticky stream of 6 heterogeneous frames
    const f1 = "+PONG\r\n";
    const f2 = ":1024\r\n";
    const f3 = "$11\r\nhello\r\nvalk\r\n";
    const f4 = "#t\r\n";
    const f5 = "_\r\n";
    const f6 = "*2\r\n$3\r\nfoo\r\n,2.5\r\n";

    let stream_buf: std.ArrayListUnmanaged(u8) = .empty;
    defer stream_buf.deinit(alloc);
    stream_buf.appendSlice(alloc, f1) catch unreachable;
    stream_buf.appendSlice(alloc, f2) catch unreachable;
    stream_buf.appendSlice(alloc, f3) catch unreachable;
    stream_buf.appendSlice(alloc, f4) catch unreachable;
    stream_buf.appendSlice(alloc, f5) catch unreachable;
    stream_buf.appendSlice(alloc, f6) catch unreachable;

    // Test feeding in varying 1..7 byte chunk sizes
    let chunk_size: usize = 1;
    while (chunk_size <= 7) : (chunk_size += 1) {
        let parser = RespStreamParser.init(alloc);
        defer parser.deinit();

        let decoded: std.ArrayListUnmanaged(RespValue) = .empty;
        defer {
            let k: usize = 0;
            while (k < decoded.items.len) : (k += 1) {
                decoded.items[k].deinit(alloc);
            }
            decoded.deinit(alloc);
        }

        let offset: usize = 0;
        while (offset < stream_buf.items.len) {
            const end = @min(offset + chunk_size, stream_buf.items.len);
            parser.feed(stream_buf.items[offset..end]) catch unreachable;
            offset = end;

            while (true) {
                const next_opt = parser.next() catch unreachable;
                if (next_opt == null) break;
                decoded.append(alloc, next_opt.?) catch unreachable;
            }
        }

        expect_eq(parser.bufferedLen(), 0 as usize);
        expect_eq(decoded.items.len, 6 as usize);
        expect_eq(std.mem.eql(u8, decoded.items[0].asStr().?, "PONG"), true);
        expect_eq(decoded.items[1].asInt().?, 1024 as i64);
        expect_eq(std.mem.eql(u8, decoded.items[2].asStr().?, "hello\r\nvalk"), true);
        expect_eq(decoded.items[3].asBool().?, true);
        expect_eq(decoded.items[4].isNull(), true);
        switch (decoded.items[5]) {
            .Array items => {
                expect_eq(items.len, 2 as usize);
                expect_eq(std.mem.eql(u8, items[0].asStr().?, "foo"), true);
                expect_eq(items[1].asDouble().?, 2.5);
            },
            else => unreachable,
        }
    }

    // Verify Incomplete vs real ProtocolError distinction
    expectParseErr(alloc, "$5\r\nhel", RespLimits{}, ProtocolError.Incomplete);
    expectParseErr(alloc, "@invalid\r\n", RespLimits{}, ProtocolError.InvalidTypeByte);
    expectParseErr(alloc, "#x\r\n", RespLimits{}, ProtocolError.InvalidBoolean);
}

test("3. Nested Map/Set/Push, Attribute Skipping & Depth/Length Guards") {
    const alloc = std.testing.allocator;

    // Nested RESP3 Map -> { "nums": ~[1, 2], "push": >["sub", "ch"] }
    const nested_wire = "%2\r\n+nums\r\n~2\r\n:1\r\n:2\r\n+push\r\n>2\r\n$3\r\nsub\r\n$2\r\nch\r\n";
    const parsed = parseOne(alloc, nested_wire) catch unreachable;
    defer parsed.value.deinit(alloc);
    expect_eq(parsed.consumed, nested_wire.len);
    switch (parsed.value) {
        .Map entries => {
            expect_eq(entries.len, 2 as usize);
            expect_eq(std.mem.eql(u8, entries[0].key.asStr().?, "nums"), true);
            switch (entries[0].value) {
                .Set s => {
                    expect_eq(s.len, 2 as usize);
                    expect_eq(s[0].asInt().?, 1 as i64);
                    expect_eq(s[1].asInt().?, 2 as i64);
                },
                else => unreachable,
            }
            switch (entries[1].value) {
                .Push p => {
                    expect_eq(p.len, 2 as usize);
                    expect_eq(std.mem.eql(u8, p[0].asStr().?, "sub"), true);
                    expect_eq(std.mem.eql(u8, p[1].asStr().?, "ch"), true);
                },
                else => unreachable,
            }
        },
        else => unreachable,
    }

    // Attribute prefix skipped transparently
    const attr_wire = "|1\r\n+ttl\r\n:60\r\n+OK\r\n";
    const attr_parsed = parseOne(alloc, attr_wire) catch unreachable;
    defer attr_parsed.value.deinit(alloc);
    expect_eq(std.mem.eql(u8, attr_parsed.value.asStr().?, "OK"), true);

    // Depth guard (max_depth = 2 -> 3 nested arrays rejected)
    const tight_limits = RespLimits{
        max_depth: 2,
        max_bulk_len: 16,
        max_collection_len: 4,
    };
    expectParseErr(alloc, "*1\r\n*1\r\n*1\r\n:1\r\n", tight_limits, ProtocolError.DepthLimitExceeded);

    // Bulk length guard (rejected immediately before full payload arrives)
    expectParseErr(alloc, "$999999\r\n", tight_limits, ProtocolError.BulkTooLarge);

    // Collection length guard
    expectParseErr(alloc, "*100\r\n", tight_limits, ProtocolError.CollectionTooLarge);
}

test("4. Command Encoding & Inline Command Parsing") {
    const alloc = std.testing.allocator;

    let cmd_args: [3][]const u8 = ["SET", "user:1", "alice\r\nbob"];
    const cmd_wire = encodeCommand(alloc, &cmd_args) catch unreachable;
    defer alloc.free(cmd_wire);

    const expected_wire = "*3\r\n$3\r\nSET\r\n$6\r\nuser:1\r\n$10\r\nalice\r\nbob\r\n";
    expect_eq(std.mem.eql(u8, cmd_wire, expected_wire), true);

    const parsed_cmd = parseOne(alloc, cmd_wire) catch unreachable;
    defer parsed_cmd.value.deinit(alloc);
    switch (parsed_cmd.value) {
        .Array items => {
            expect_eq(items.len, 3 as usize);
            expect_eq(std.mem.eql(u8, items[0].asStr().?, "SET"), true);
            expect_eq(std.mem.eql(u8, items[1].asStr().?, "user:1"), true);
            expect_eq(std.mem.eql(u8, items[2].asStr().?, "alice\r\nbob"), true);
        },
        else => unreachable,
    }

    // Inline command encode & stream parse
    let inline_args: [2][]const u8 = ["PING", "valkey"];
    const inline_wire = encodeInlineCommand(alloc, &inline_args) catch unreachable;
    defer alloc.free(inline_wire);
    expect_eq(std.mem.eql(u8, inline_wire, "PING valkey\r\n"), true);

    let parser = RespStreamParser.init(alloc);
    defer parser.deinit();
    parser.setAllowInline(true);

    // Feed 3 bytes at a time
    parser.feed(inline_wire[0..3]) catch unreachable;
    expect_eq(parser.next() catch unreachable == null, true);
    parser.feed(inline_wire[3..]) catch unreachable;

    if (inl_val := parser.next() catch unreachable) {
        defer inl_val.deinit(alloc);
        switch (inl_val) {
            .Array items => {
                expect_eq(items.len, 2 as usize);
                expect_eq(std.mem.eql(u8, items[0].asStr().?, "PING"), true);
                expect_eq(std.mem.eql(u8, items[1].asStr().?, "valkey"), true);
            },
            else => unreachable,
        }
    } else {
        unreachable;
    }
}

test("5. Dynamic Namespace Mounting (cap.valkey) & Zero Memory Leaks") {
    const alloc = std.testing.allocator;

    let cmd_args: [2][]const u8 = ["GET", "session:42"];
    const wire = cap.valkey.encodeCommand(alloc, &cmd_args) catch unreachable;
    defer alloc.free(wire);

    let parser = cap.valkey.RespStreamParser.init(alloc);
    defer parser.deinit();

    parser.feed(wire) catch unreachable;
    parser.feed("=11\r\nmkd:# Hello\r\n") catch unreachable;

    if (v1 := parser.next() catch unreachable) {
        defer v1.deinit(alloc);
        switch (v1) {
            .Array items => {
                expect_eq(items.len, 2 as usize);
                expect_eq(std.mem.eql(u8, items[0].asStr().?, "GET"), true);
                expect_eq(std.mem.eql(u8, items[1].asStr().?, "session:42"), true);
            },
            else => unreachable,
        }
    } else {
        unreachable;
    }

    if (v2 := parser.next() catch unreachable) {
        defer v2.deinit(alloc);
        switch (v2) {
            .VerbatimString vs => {
                expect_eq(std.mem.eql(u8, vs.format, "mkd"), true);
                expect_eq(std.mem.eql(u8, vs.content, "# Hello"), true);
            },
            else => unreachable,
        }
    } else {
        unreachable;
    }

    expect_eq(parser.next() catch unreachable == null, true);
}
