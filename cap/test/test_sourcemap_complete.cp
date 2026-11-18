// Source: bun/src/sourcemap/
// cap/test/test_sourcemap_complete.cp
import { test, expect_eq } from "cap:test";
import {
    VLQ,
    LineOffsetTable,
    DynamicLineOffsetTable,
    SourceMapping,
    InternalSourceMap,
    InternalSourceMapBuilder,
    zigzagEncode,
    zigzagDecode,
    writeVarint,
    readVarint,
    Chunk,
    ParsedSourceMapRecord,
    SourceMapGenerator,
    SourceMapConsumer,
} from "cap:sourcemap";
const std = @import("std");

fn testVLQAndZigzag(): !void {
    // Zigzag encoding properties
    expect_eq(zigzagEncode(0), 0);
    expect_eq(zigzagDecode(0), 0);

    expect_eq(zigzagEncode(-1), 1);
    expect_eq(zigzagDecode(1), -1);

    expect_eq(zigzagEncode(1), 2);
    expect_eq(zigzagDecode(2), 1);

    expect_eq(zigzagEncode(-2), 3);
    expect_eq(zigzagDecode(3), -2);

    expect_eq(zigzagEncode(12345), 24690);
    expect_eq(zigzagDecode(24690), 12345);

    // Varint encoding roundtrip
    let varint_buf: [32]u8 = undefined;
    let n1 = writeVarint(&varint_buf, 0, 42);
    let pos: usize = 0;
    let val1 = readVarint(&varint_buf, &pos);
    expect_eq(val1, 42);
    expect_eq(pos, n1);

    let n2 = writeVarint(&varint_buf, 0, -9999);
    pos = 0;
    let val2 = readVarint(&varint_buf, &pos);
    expect_eq(val2, -9999);
    expect_eq(pos, n2);

    // VLQ batch encode
    let nums: [4]i32 = [ 0, 1, -1, 16 ];
    let out_vlq: [32]u8 = undefined;
    let total_vlq = VLQ.encodeValues(&nums, &out_vlq);
    expect_eq(std.mem.eql(u8, out_vlq[0..total_vlq], "ACDgB"), true);
}

fn testDynamicLineOffsetTable(): !void {
    const allocator = std.heap.page_allocator;
    const code = "const a = 1;\nconst b = 2;\nconsole.log(a + b);\n";

    let table = try DynamicLineOffsetTable.build(allocator, code);
    defer table.deinit(allocator);

    expect_eq(table.line_count, 4);

    // Line 0
    const loc0 = table.resolveLocation(6);
    expect_eq(loc0.line, 0);
    expect_eq(loc0.column, 6);
    expect_eq(table.resolveByteOffset(0, 6), 6);

    // Line 1: starts at offset 13 ("const b = 2;\n")
    const loc1 = table.resolveLocation(19);
    expect_eq(loc1.line, 1);
    expect_eq(loc1.column, 6);
    expect_eq(table.resolveByteOffset(1, 6), 19);

    // Line 2: starts at offset 26 ("console.log(a + b);\n")
    const loc2 = table.resolveLocation(34);
    expect_eq(loc2.line, 2);
    expect_eq(loc2.column, 8);
    expect_eq(table.resolveByteOffset(2, 8), 34);
}

fn testInternalSourceMapBinary(): !void {
    const allocator = std.heap.page_allocator;
    let builder = InternalSourceMapBuilder.init();
    defer builder.deinit(allocator);

    // Add multiple mappings
    let m0 = SourceMapping{
        generated_line: 0,
        generated_column: 0,
        source_index: 0,
        original_line: 0,
        original_column: 0,
        name_index: -1,
    };
    try builder.addMapping(allocator, m0);

    let m1 = SourceMapping{
        generated_line: 0,
        generated_column: 10,
        source_index: 0,
        original_line: 0,
        original_column: 5,
        name_index: -1,
    };
    try builder.addMapping(allocator, m1);

    let m2 = SourceMapping{
        generated_line: 1,
        generated_column: 4,
        source_index: 1,
        original_line: 2,
        original_column: 8,
        name_index: -1,
    };
    try builder.addMapping(allocator, m2);

    let ism = try builder.build(allocator);
    defer ism.deinit(allocator);

    expect_eq(ism.total_mappings, 3);

    // Find closest mapping for (0, 5) -> should match m0
    let f0_opt = ism.find(0, 5);
    expect_eq(f0_opt != null, true);
    if (f0 := f0_opt) {
        expect_eq(f0.generated_column, 0);
        expect_eq(f0.original_column, 0);
    }

    // Find closest mapping for (0, 15) -> should match m1
    let f1_opt = ism.find(0, 15);
    expect_eq(f1_opt != null, true);
    if (f1 := f1_opt) {
        expect_eq(f1.generated_column, 10);
        expect_eq(f1.original_column, 5);
    }

    // Find closest mapping for (1, 6) -> should match m2
    let f2_opt = ism.find(1, 6);
    expect_eq(f2_opt != null, true);
    if (f2 := f2_opt) {
        expect_eq(f2.generated_line, 1);
        expect_eq(f2.generated_column, 4);
        expect_eq(f2.source_index, 1);
        expect_eq(f2.original_line, 2);
        expect_eq(f2.original_column, 8);
    }
}

fn testChunkAndParsedSourceMap(): !void {
    const allocator = std.heap.page_allocator;
    let chunk = Chunk.init();
    defer chunk.deinit(allocator);

    const s0_idx = try chunk.addSource(allocator, "app.ts", "console.log('hello');");
    expect_eq(s0_idx, 0);

    const n0_idx = try chunk.addName(allocator, "log");
    expect_eq(n0_idx, 0);

    let m0 = SourceMapping{
        generated_line: 0,
        generated_column: 0,
        source_index: 0,
        original_line: 0,
        original_column: 0,
        name_index: -1,
    };
    try chunk.addMapping(allocator, m0);

    let m1 = SourceMapping{
        generated_line: 0,
        generated_column: 8,
        source_index: 0,
        original_line: 0,
        original_column: 8,
        name_index: 0,
    };
    try chunk.addMapping(allocator, m1);

    const mappings_str = try chunk.generateMappingsString(allocator);
    defer allocator.free(mappings_str);

    // Mappings string should start with "AAAA"
    expect_eq(std.mem.startsWith(u8, mappings_str, "AAAA"), true);

    const json_str = try chunk.printSourceMapJSON(allocator, "bundle.js", true);
    defer allocator.free(json_str);

    expect_eq(std.mem.indexOf(u8, json_str, "\"version\": 3") != null, true);
    expect_eq(std.mem.indexOf(u8, json_str, "\"file\": \"bundle.js\"") != null, true);
    expect_eq(std.mem.indexOf(u8, json_str, "\"app.ts\"") != null, true);

    // Parse the generated JSON back using ParsedSourceMapRecord
    let parsed = try ParsedSourceMapRecord.parse(allocator, json_str);
    defer parsed.deinit(allocator);

    expect_eq(parsed.version, 3);
    expect_eq(std.mem.eql(u8, parsed.file, "bundle.js"), true);
    expect_eq(parsed.sources.len, 1);
    expect_eq(std.mem.eql(u8, parsed.sources[0], "app.ts"), true);
    expect_eq(parsed.mappings.len, 2);

    // Query position
    let pos_opt = parsed.findOriginalPosition(0, 10);
    expect_eq(pos_opt != null, true);
    if (pos := pos_opt) {
        expect_eq(std.mem.eql(u8, pos.source, "app.ts"), true);
        expect_eq(pos.line, 0);
        expect_eq(pos.column, 8);
    }
}

fn testSourceMapGeneratorAndConsumer(): !void {
    const allocator = std.heap.page_allocator;

    // 1. Generate SourceMap
    let gen = SourceMapGenerator.init(allocator, "out.js");
    defer gen.deinit();

    const s_idx = try gen.addSourceWithContent("src/math.cp", "pub fn add(a: i32, b: i32): i32 { return a + b; }");
    const name_idx = try gen.addName("add");

    try gen.addMapping(0, 0, 0, 0, s_idx, -1);
    try gen.addMapping(0, 7, 0, 7, s_idx, name_idx);

    const map_json = try gen.generateJSON(allocator, true);
    defer allocator.free(map_json);

    // 2. Consume SourceMap
    let consumer = try SourceMapConsumer.init(allocator, map_json);
    defer consumer.deinit();

    let orig_pos_opt = consumer.originalPositionFor(0, 8);
    expect_eq(orig_pos_opt != null, true);
    if (orig_pos := orig_pos_opt) {
        expect_eq(std.mem.eql(u8, orig_pos.source, "src/math.cp"), true);
        expect_eq(orig_pos.line, 0);
        expect_eq(orig_pos.column, 7);
        expect_eq(std.mem.eql(u8, orig_pos.name, "add"), true);
    }
}

fn main() {
    test("1. VLQ, Zigzag and Varint binary encodings", () => {
        try testVLQAndZigzag();
    });
    test("2. DynamicLineOffsetTable dynamic line and column resolution", () => {
        try testDynamicLineOffsetTable();
    });
    test("3. InternalSourceMap binary storage and binary search", () => {
        try testInternalSourceMapBinary();
    });
    test("4. Chunk assembly and ParsedSourceMapRecord JSON roundtrip", () => {
        try testChunkAndParsedSourceMap();
    });
    test("5. SourceMapGenerator and SourceMapConsumer end-to-end", () => {
        try testSourceMapGeneratorAndConsumer();
    });
}
