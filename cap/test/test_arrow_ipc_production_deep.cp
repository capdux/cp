// Source: cp/cap/test/test_arrow_ipc_production_deep.cp (Production deep tests for cap:arrow_ipc)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    ArrowError,
    ArrowField,
    ArrowSchema,
    ArrowColumn,
    RecordBatch,
    writeStream,
    readStream,
    writeFile,
    readFile,
} from "cap:arrow_ipc";

test("arrow_ipc: Int32/Int64/Float64/Bool/Utf8 RecordBatch with validity bitmap") {
    const alloc = std.testing.allocator;

    const fields: [5]ArrowField = [
        ArrowField.init("c_i32", .Int32, true),
        ArrowField.init("c_i64", .Int64, false),
        ArrowField.init("c_f64", .Float64, true),
        ArrowField.init("c_bool", .Bool, true),
        ArrowField.init("c_utf8", .Utf8, true),
    ];
    let schema = try ArrowSchema.init(alloc, &fields);
    defer schema.deinit(alloc);

    const v_i32: [4]i32 = [10, 0, -30, 40];
    const m_i32: [4]bool = [true, false, true, true];
    const v_i64: [4]i64 = [1000, 2000, 3000, 4000];
    const v_f64: [4]f64 = [1.5, 2.5, 0.0, -4.25];
    const m_f64: [4]bool = [true, true, false, true];
    const v_bool: [4]bool = [true, false, true, false];
    const m_bool: [4]bool = [true, true, true, false];
    const v_utf8: [4][]const u8 = ["alpha", "", "gamma", "delta"];
    const m_utf8: [4]bool = [true, false, true, true];

    let cols: [5]ArrowColumn = [
        try ArrowColumn.fromInt32(alloc, "c_i32", &v_i32, &m_i32),
        try ArrowColumn.fromInt64(alloc, "c_i64", &v_i64, null),
        try ArrowColumn.fromFloat64(alloc, "c_f64", &v_f64, &m_f64),
        try ArrowColumn.fromBool(alloc, "c_bool", &v_bool, &m_bool),
        try ArrowColumn.fromUtf8(alloc, "c_utf8", &v_utf8, &m_utf8),
    ];
    let batch = try RecordBatch.init(alloc, schema, &cols);
    defer batch.deinit(alloc);

    expect_eq(batch.numRows(), 4);
    expect_eq(batch.numCols(), 5);

    const col0 = try batch.column(0);
    expect_eq((try col0.getInt32(0)).?, 10);
    try std.testing.expect((try col0.getInt32(1)) == null);
    expect_eq((try col0.getInt32(2)).?, -30);

    const col1 = try batch.column(1);
    expect_eq((try col1.getInt64(3)).?, 4000);

    const col2 = try batch.column(2);
    try std.testing.expect((try col2.getFloat64(2)) == null);
    try std.testing.expect((try col2.getFloat64(3)).? == -4.25);

    const col3 = try batch.column(3);
    expect_eq((try col3.getBool(0)).?, true);
    try std.testing.expect((try col3.getBool(3)) == null);

    const col4 = try batch.column(4);
    try std.testing.expectEqualStrings("alpha", (try col4.getUtf8(0)).?);
    try std.testing.expect((try col4.getUtf8(1)) == null);
    try std.testing.expectEqualStrings("delta", (try col4.getUtf8(3)).?);
}

test("arrow_ipc: IPC Streaming multi-batch roundtrip and EOS") {
    const alloc = std.testing.allocator;

    const fields: [2]ArrowField = [
        ArrowField.init("id", .Int32, false),
        ArrowField.init("tag", .Utf8, true),
    ];
    let schema = try ArrowSchema.init(alloc, &fields);
    defer schema.deinit(alloc);

    const b1_ids: [2]i32 = [1, 2];
    const b1_tags: [2][]const u8 = ["one", "two"];
    let b1_cols: [2]ArrowColumn = [
        try ArrowColumn.fromInt32(alloc, "id", &b1_ids, null),
        try ArrowColumn.fromUtf8(alloc, "tag", &b1_tags, null),
    ];
    let batch1 = try RecordBatch.init(alloc, schema, &b1_cols);
    defer batch1.deinit(alloc);

    const b2_ids: [2]i32 = [3, 4];
    const b2_tags: [2][]const u8 = ["", "four"];
    const b2_mask: [2]bool = [false, true];
    let b2_cols: [2]ArrowColumn = [
        try ArrowColumn.fromInt32(alloc, "id", &b2_ids, null),
        try ArrowColumn.fromUtf8(alloc, "tag", &b2_tags, &b2_mask),
    ];
    let batch2 = try RecordBatch.init(alloc, schema, &b2_cols);
    defer batch2.deinit(alloc);

    const batches: [2]RecordBatch = [batch1, batch2];
    const stream_bytes = try writeStream(alloc, &schema, &batches);
    defer alloc.free(stream_bytes);

    // Verify EOS marker at end of stream (0xFFFFFFFF 0x00000000)
    try std.testing.expect(stream_bytes.len >= 8);
    const eos_tok = std.mem.readInt(u32, stream_bytes[stream_bytes.len - 8 ..][0..4], .little);
    const eos_len = std.mem.readInt(u32, stream_bytes[stream_bytes.len - 4 ..][0..4], .little);
    expect_eq(eos_tok, 0xFFFFFFFF);
    expect_eq(eos_len, 0);

    let res = try readStream(alloc, stream_bytes);
    defer res.deinit(alloc);

    expect_eq(res.batches.len, 2);
    expect_eq(res.batches[0].numRows(), 2);
    expect_eq(res.batches[1].numRows(), 2);
    expect_eq((try (try res.batches[0].column(0)).getInt32(1)).?, 2);
    try std.testing.expect((try (try res.batches[1].column(1)).getUtf8(0)) == null);
    try std.testing.expectEqualStrings("four", (try (try res.batches[1].column(1)).getUtf8(1)).?);
}

test("arrow_ipc: IPC File Format ARROW1 roundtrip") {
    const alloc = std.testing.allocator;

    const fields: [3]ArrowField = [
        ArrowField.init("k", .Int64, false),
        ArrowField.init("v", .Float64, false),
        ArrowField.init("flag", .Bool, false),
    ];
    let schema = try ArrowSchema.init(alloc, &fields);
    defer schema.deinit(alloc);

    const ks: [3]i64 = [100, 200, 300];
    const vs: [3]f64 = [3.14, 2.71, 1.41];
    const fs: [3]bool = [true, false, true];
    let cols: [3]ArrowColumn = [
        try ArrowColumn.fromInt64(alloc, "k", &ks, null),
        try ArrowColumn.fromFloat64(alloc, "v", &vs, null),
        try ArrowColumn.fromBool(alloc, "flag", &fs, null),
    ];
    let batch = try RecordBatch.init(alloc, schema, &cols);
    defer batch.deinit(alloc);

    const batches: [1]RecordBatch = [batch];
    const file_bytes = try writeFile(alloc, &schema, &batches);
    defer alloc.free(file_bytes);

    try std.testing.expectEqualStrings("ARROW1", file_bytes[0..6]);
    try std.testing.expectEqualStrings("ARROW1", file_bytes[file_bytes.len - 6 ..]);

    let res = try readFile(alloc, file_bytes);
    defer res.deinit(alloc);

    expect_eq(res.schema.fields.len, 3);
    expect_eq(res.batches.len, 1);
    expect_eq((try (try res.batches[0].column(0)).getInt64(2)).?, 300);
    try std.testing.expect((try (try res.batches[0].column(1)).getFloat64(0)).? == 3.14);
    expect_eq((try (try res.batches[0].column(2)).getBool(1)).?, false);
}

test("arrow_ipc: malformed magic, footer/buffer bounds, row mismatch, and invalid utf8 offsets") {
    const alloc = std.testing.allocator;

    // 1. Invalid magic
    const bad_magic = "BADMAG\x00\x00\x04\x00\x00\x00BADMAG";
    try std.testing.expectError(ArrowError.InvalidMagic, readFile(alloc, bad_magic));

    // 2. Footer length out of bounds
    let bad_footer: [22]u8 = undefined;
    @memcpy(bad_footer[0..8], "ARROW1\x00\x00");
    @memset(bad_footer[8..12], 0);
    std.mem.writeInt(i32, bad_footer[12..16], 9999, .little);
    @memcpy(bad_footer[16..22], "ARROW1");
    try std.testing.expectError(ArrowError.InvalidFooter, readFile(alloc, &bad_footer));

    // 3. Row count mismatch in RecordBatch
    const fields: [2]ArrowField = [
        ArrowField.init("a", .Int32, false),
        ArrowField.init("b", .Int32, false),
    ];
    let schema = try ArrowSchema.init(alloc, &fields);
    defer schema.deinit(alloc);

    const a_vals: [2]i32 = [1, 2];
    const b_vals: [3]i32 = [1, 2, 3];
    let c_a = try ArrowColumn.fromInt32(alloc, "a", &a_vals, null);
    let c_b = try ArrowColumn.fromInt32(alloc, "b", &b_vals, null);
    let mismatch_cols: [2]ArrowColumn = [c_a, c_b];
    const rb_res = RecordBatch.init(alloc, schema, &mismatch_cols);
    c_a.deinit(alloc);
    c_b.deinit(alloc);
    try std.testing.expectError(ArrowError.RowCountMismatch, rb_res);

    // 4. Invalid UTF-8 bytes rejected
    const bad_utf8: [1][]const u8 = ["\xff\xfe"];
    try std.testing.expectError(ArrowError.InvalidUtf8, ArrowColumn.fromUtf8(alloc, "s", &bad_utf8, null));
}

test("arrow_ipc: cap.arrow_ipc dynamic mount and zero-leak verification") {
    const alloc = std.testing.allocator;
    const arrow = cap.arrow_ipc;

    const fields: [1]arrow.ArrowField = [
        arrow.ArrowField.init("x", .Int32, false),
    ];
    let schema = try arrow.ArrowSchema.init(alloc, &fields);
    defer schema.deinit(alloc);

    const xs: [2]i32 = [42, 99];
    let cols: [1]arrow.ArrowColumn = [
        try arrow.ArrowColumn.fromInt32(alloc, "x", &xs, null),
    ];
    let batch = try arrow.RecordBatch.init(alloc, schema, &cols);
    defer batch.deinit(alloc);

    const batches: [1]arrow.RecordBatch = [batch];
    const bytes = try arrow.writeStream(alloc, &schema, &batches);
    defer alloc.free(bytes);

    let res = try arrow.readStream(alloc, bytes);
    defer res.deinit(alloc);

    expect_eq((try (try res.batches[0].column(0)).getInt32(0)).?, 42);
    expect_eq((try (try res.batches[0].column(0)).getInt32(1)).?, 99);
}
