// Source: cp/cap/test/test_bson_production_deep.cp
// Deep Production Test Suite for cap:bson (MongoDB BSON 1.1 & 12-Byte ObjectId Codec)
// Covers:
//   1. All scalar types round-trip (double, utf8, boolean, null_val, int32, int64, utc_datetime, timestamp, binary, object_id)
//   2. ObjectId 24-char Hex encode/decode, timestampSec extraction & InvalidObjectId guard
//   3. Nested document & array ("0", "1", ...) complete round-trip
//   4. Security & malformed input guards: InvalidDocumentSize, MissingTerminator, InvalidCString, TrailingBytes, 65-level MaxDepthExceeded
//   5. Dynamic lazy namespace mounting (`import cap from "cap"; cap.bson`) & `std.testing.allocator` zero memory leaks

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    BsonError,
    BsonBinary,
    BsonTimestamp,
    BsonField,
    BsonValue,
    ObjectId,
    deinitDocument,
    getField,
    eqlFields,
    encodeDocument,
    decodeDocument,
    decodeDocumentWithMaxDepth,
} from "cap:bson";
import cap from "cap";

test("1. All BSON 1.1 Scalar Types Encode/Decode Round-Trip") {
    const alloc = std.testing.allocator;

    const oid = ObjectId.generate(1700000000, [0x11, 0x22, 0x33, 0x44, 0x55], 0x010203);
    const bin_payload = "\xDE\xAD\xBE\xEF\x00\x01";

    let fields: [10]BsonField = [
        BsonField.init("d", BsonValue{ double: 3.141592653589793 }),
        BsonField.init("s", BsonValue{ utf8: "hello bson \xE4\xB8\xAD\xE6\x96\x87" }),
        BsonField.init("b_true", BsonValue{ boolean: true }),
        BsonField.init("b_false", BsonValue{ boolean: false }),
        BsonField.init("n", .null_val),
        BsonField.init("i32", BsonValue{ int32: -123456 }),
        BsonField.init("i64", BsonValue{ int64: 9007199254740993 }),
        BsonField.init("dt", BsonValue{ utc_datetime: 1700000000123 }),
        BsonField.init("ts", BsonValue{ timestamp: BsonTimestamp{ increment: 7, timestamp: 1700000000 } }),
        BsonField.init("bin", BsonValue{ binary: BsonBinary{ subtype: 0x04, data: bin_payload } }),
    ];

    const encoded = encodeDocument(alloc, &fields) catch unreachable;
    defer alloc.free(encoded);

    const decoded = decodeDocument(alloc, encoded) catch unreachable;
    defer deinitDocument(alloc, decoded);

    expect_eq(eqlFields(&fields, decoded), true);

    const root_val = BsonValue{ document: decoded };
    if (v_i32 := root_val.get("i32")) {
        expect_eq(v_i32.eql(BsonValue{ int32: -123456 }), true);
    } else {
        expect_eq(true, false);
    }
    if (v_ts := root_val.get("ts")) {
        expect_eq(v_ts.eql(BsonValue{ timestamp: BsonTimestamp{ increment: 7, timestamp: 1700000000 } }), true);
    } else {
        expect_eq(true, false);
    }
    expect_eq(root_val.get("missing") == null, true);

    // Also test ObjectId field round-trip
    let oid_fields: [1]BsonField = [
        BsonField.init("_id", BsonValue{ object_id: oid }),
    ];
    const oid_enc = encodeDocument(alloc, &oid_fields) catch unreachable;
    defer alloc.free(oid_enc);
    const oid_dec = decodeDocument(alloc, oid_enc) catch unreachable;
    defer deinitDocument(alloc, oid_dec);
    expect_eq(eqlFields(&oid_fields, oid_dec), true);
}

test("2. ObjectId 24-Char Hex Encode/Decode, Timestamp Extraction & InvalidObjectId Guard") {
    const alloc = std.testing.allocator;

    const ts: u32 = 1732100000;
    const oid = ObjectId.generate(ts, [0xAA, 0xBB, 0xCC, 0xDD, 0xEE], 0x123456);
    expect_eq(oid.timestampSec(), ts);

    const hex = oid.toHex(alloc) catch unreachable;
    defer alloc.free(hex);
    expect_eq(hex.len, 24 as usize);

    const parsed = ObjectId.fromHex(hex) catch unreachable;
    expect_eq(parsed.eql(oid), true);
    expect_eq(parsed.timestampSec(), ts);

    // Uppercase hex also valid
    const parsed_upper = ObjectId.fromHex("507F1F77BCF86CD799439011") catch unreachable;
    const lower_hex = parsed_upper.toHex(alloc) catch unreachable;
    defer alloc.free(lower_hex);
    expect_eq(std.mem.eql(u8, lower_hex, "507f1f77bcf86cd799439011"), true);

    // Invalid length
    let hit_short = false;
    * = ObjectId.fromHex("507f1f77") catch |e| {
        if (e == err.InvalidObjectId) hit_short = true;
        ObjectId{}
    };
    expect_eq(hit_short, true);

    // Invalid hex character
    let hit_bad_char = false;
    * = ObjectId.fromHex("507f1f77bcf86cd79943901z") catch |e| {
        if (e == err.InvalidObjectId) hit_bad_char = true;
        ObjectId{}
    };
    expect_eq(hit_bad_char, true);
}

test("3. Nested Document & Array ('0', '1', ...) Complete Round-Trip") {
    const alloc = std.testing.allocator;

    let arr_items: [3]BsonValue = [
        BsonValue{ int32: 10 },
        BsonValue{ utf8: "two" },
        BsonValue{ boolean: true },
    ];

    let inner_fields: [2]BsonField = [
        BsonField.init("flag", BsonValue{ boolean: false }),
        BsonField.init("tags", BsonValue{ array: &arr_items }),
    ];

    let root_fields: [2]BsonField = [
        BsonField.init("meta", BsonValue{ document: &inner_fields }),
        BsonField.init("count", BsonValue{ int64: 42 }),
    ];

    const encoded = encodeDocument(alloc, &root_fields) catch unreachable;
    defer alloc.free(encoded);

    const decoded = decodeDocument(alloc, encoded) catch unreachable;
    defer deinitDocument(alloc, decoded);

    expect_eq(eqlFields(&root_fields, decoded), true);

    // Verify nested lookup via BsonValue.get
    const root_val = BsonValue{ document: decoded };
    if (meta_val := root_val.get("meta")) {
        if (tags_val := meta_val.get("tags")) {
            switch (tags_val) {
                .array items => {
                    expect_eq(items.len, 3 as usize);
                    expect_eq(items[0].eql(BsonValue{ int32: 10 }), true);
                    expect_eq(items[1].eql(BsonValue{ utf8: "two" }), true);
                    expect_eq(items[2].eql(BsonValue{ boolean: true }), true);
                },
                else => expect_eq(true, false),
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("4. Malformed Input Guards & 65-Level MaxDepthExceeded Protection") {
    const alloc = std.testing.allocator;
    let empty_fields: [0]BsonField = [];

    // 1. InvalidDocumentSize: declared size < 5
    let hit_small_size = false;
    const r_small = decodeDocument(alloc, "\x04\x00\x00\x00\x00") catch |e| {
        if (e == err.InvalidDocumentSize) hit_small_size = true;
        &empty_fields
    };
    * = r_small;
    expect_eq(hit_small_size, true);

    // 2. InvalidDocumentSize: declared size > actual buffer
    let hit_over_size = false;
    const r_over = decodeDocument(alloc, "\x10\x00\x00\x00\x00") catch |e| {
        if (e == err.InvalidDocumentSize) hit_over_size = true;
        &empty_fields
    };
    * = r_over;
    expect_eq(hit_over_size, true);

    // 3. MissingTerminator: 5-byte doc whose last byte is not 0x00
    let hit_missing_term = false;
    const r_term = decodeDocument(alloc, "\x05\x00\x00\x00\x01") catch |e| {
        if (e == err.MissingTerminator) hit_missing_term = true;
        &empty_fields
    };
    * = r_term;
    expect_eq(hit_missing_term, true);

    // 4. TrailingBytes: valid empty doc (5 bytes) + 1 trailing byte
    let hit_trailing = false;
    const r_trail = decodeDocument(alloc, "\x05\x00\x00\x00\x00\xFF") catch |e| {
        if (e == err.TrailingBytes) hit_trailing = true;
        &empty_fields
    };
    * = r_trail;
    expect_eq(hit_trailing, true);

    // 5. InvalidCString on encode: key contains embedded \x00
    let bad_key_fields: [1]BsonField = [
        BsonField.init("bad\x00key", BsonValue{ int32: 1 }),
    ];
    let hit_bad_cstring = false;
    * = encodeDocument(alloc, &bad_key_fields) catch |e| {
        if (e == err.InvalidCString) hit_bad_cstring = true;
        ""
    };
    expect_eq(hit_bad_cstring, true);

    // 6. 65-level deep nesting triggers MaxDepthExceeded (default max_depth = 64)
    // Build a 65-level nested BSON document dynamically:
    // Innermost empty doc: 5 bytes (\x05\x00\x00\x00\x00)
    // Each wrapper adds: 4 (size) + 1 (0x03) + 2 ("a\x00") + inner + 1 (0x00) = inner + 8
    let deep_buf: std.ArrayList(u8) = .empty;
    defer deep_buf.deinit(alloc);

    const depth_levels: usize = 65;
    let total_size: i32 = 5 + (depth_levels as i32) * 8;
    let lvl: usize = 0;
    while (lvl < depth_levels) : (lvl += 1) {
        const u: u32 = @bitCast(total_size);
        deep_buf.append(alloc, (u & 0xFF) as u8) catch unreachable;
        deep_buf.append(alloc, ((u >> 8) & 0xFF) as u8) catch unreachable;
        deep_buf.append(alloc, ((u >> 16) & 0xFF) as u8) catch unreachable;
        deep_buf.append(alloc, ((u >> 24) & 0xFF) as u8) catch unreachable;
        deep_buf.append(alloc, 0x03) catch unreachable;
        deep_buf.append(alloc, 'a') catch unreachable;
        deep_buf.append(alloc, 0x00) catch unreachable;
        total_size -= 8;
    }
    // Innermost empty doc
    deep_buf.appendSlice(alloc, "\x05\x00\x00\x00\x00") catch unreachable;
    // Closing 0x00 terminators for all 65 wrappers
    lvl = 0;
    while (lvl < depth_levels) : (lvl += 1) {
        deep_buf.append(alloc, 0x00) catch unreachable;
    }

    let hit_max_depth = false;
    const r_depth = decodeDocument(alloc, deep_buf.items) catch |e| {
        if (e == err.MaxDepthExceeded) hit_max_depth = true;
        &empty_fields
    };
    * = r_depth;
    expect_eq(hit_max_depth, true);

    // Confirm decodeDocumentWithMaxDepth(..., 65) succeeds on the exact same 65-level payload!
    const ok_depth = decodeDocumentWithMaxDepth(alloc, deep_buf.items, 65) catch unreachable;
    deinitDocument(alloc, ok_depth);
}

test("5. Dynamic `cap.bson` Namespace Mounting & Zero-Leak Round-Trip") {
    const alloc = std.testing.allocator;

    const oid = cap.bson.ObjectId.fromHex("656f6a656374696430303031") catch unreachable;
    let fields: [3]cap.bson.BsonField = [
        cap.bson.BsonField.init("_id", cap.bson.BsonValue{ object_id: oid }),
        cap.bson.BsonField.init("service", cap.bson.BsonValue{ utf8: "cap-bson" }),
        cap.bson.BsonField.init("ok", cap.bson.BsonValue{ boolean: true }),
    ];

    const bytes = cap.bson.encodeDocument(alloc, &fields) catch unreachable;
    defer alloc.free(bytes);

    const decoded = cap.bson.decodeDocument(alloc, bytes) catch unreachable;
    defer cap.bson.deinitDocument(alloc, decoded);

    expect_eq(cap.bson.eqlFields(&fields, decoded), true);
}
