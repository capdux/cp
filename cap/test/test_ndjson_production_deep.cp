// Source: cp/cap/test/test_ndjson_production_deep.cp
// Deep Production Test Suite for cap:ndjson & cap.ndjson
// Covers:
//   1. Incremental chunked feed() across split/coalesced frames (\n, \r\n, blank line skipping) & finish() trailing record
//   2. NdjsonWriter structured value serialization + parseAll & stringifyAll round-trip consistency
//   3. Resource exhaustion guard (MaxLineLengthExceeded), InvalidJsonLine with line numbers, UnterminatedFinalLine & InvalidLimit
//   4. Frame boundary protection in NdjsonWriter.writeRawJson rejecting embedded raw \n / \r
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.ndjson` & std.testing.allocator zero leaks

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    NdjsonError,
    NdjsonRecord,
    NdjsonStreamParser,
    NdjsonWriter,
    parseAll,
    stringifyAll,
} from "cap:conf";
import cap from "cap";

fn freeRecords(alloc: std.mem.Allocator, recs: []NdjsonRecord) {
    let i: usize = 0;
    while (i < recs.len) : (i += 1) {
        recs[i].deinit();
    }
    alloc.free(recs);
}

fn testIncrementalChunkedFeedAndFinish(): ! {
    const alloc = std.testing.allocator;
    let parser = NdjsonStreamParser.init(alloc, 4096)?;
    defer parser.deinit();

    // Chunk 1: line 1 complete (\r\n), line 2 whitespace-only (\n), line 3 partial
    const recs1 = parser.feed("{\"id\":1,\"name\":\"alpha\"}\r\n   \t  \n{\"id\":2,")?;
    expect_eq(recs1.len, 1);
    expect_eq(recs1[0].line_number, 1);
    expect_eq(recs1[0].parsed.value.object.get("id").?.integer, 1);

    // Chunk 2: completes line 3, adds blank line 4 (\r\n), and starts line 5 without trailing newline
    const recs2 = parser.feed("\"tags\":[\"a\",\"b\"]}\r\n\r\n{\"id\":3,\"tail\":true}")?;
    expect_eq(recs2.len, 1);
    expect_eq(recs2[0].line_number, 3);
    expect_eq(recs2[0].parsed.value.object.get("tags").?.array.items.len, 2);

    // Finish with allow_trailing_without_newline = true flushes line 5
    const recs3 = parser.finish(true)?;
    expect_eq(recs3.len, 1);
    expect_eq(recs3[0].line_number, 5);
    expect_eq(recs3[0].parsed.value.object.get("tail").?.bool, true);

    expect_eq(parser.parsedCount(), 3);
    expect_eq(parser.skippedEmptyLines(), 2);

    // Pull all records via nextRecord() cursor
    let pulled: usize = 0;
    while (rec := parser.nextRecord()) {
        * = rec;
        pulled += 1;
    }
    expect_eq(pulled, 3);
    expect_eq(parser.nextRecord() == null, true);
}

fn testWriterAndParseAllRoundTrip(): ! {
    const alloc = std.testing.allocator;
    let writer = NdjsonWriter.init(alloc);
    defer writer.deinit();

    let obj1 = std.json.ObjectMap.init(alloc);
    defer obj1.deinit();
    obj1.put("seq", std.json.Value{ .integer = 10 }) catch return error.OutOfMemory;
    obj1.put("msg", std.json.Value{ .string = "line1\nescaped" }) catch return error.OutOfMemory;

    let arr2 = std.json.Array.init(alloc);
    defer arr2.deinit();
    arr2.append(std.json.Value{ .bool = true }) catch return error.OutOfMemory;
    arr2.append(std.json.Value{ .integer = 42 }) catch return error.OutOfMemory;

    writer.writeValue(std.json.Value{ .object = obj1 })?;
    writer.writeValue(std.json.Value{ .array = arr2 })?;
    writer.writeRawJson("{\"seq\":30,\"ok\":true}")?;

    const out_bytes = writer.bytes();
    const parsed_recs = parseAll(alloc, out_bytes)?;
    defer freeRecords(alloc, parsed_recs);

    expect_eq(parsed_recs.len, 3);
    expect_eq(parsed_recs[0].parsed.value.object.get("seq").?.integer, 10);
    expect_eq(std.mem.eql(u8, parsed_recs[0].parsed.value.object.get("msg").?.string, "line1\nescaped"), true);
    expect_eq(parsed_recs[1].parsed.value.array.items[1].integer, 42);
    expect_eq(parsed_recs[2].parsed.value.object.get("seq").?.integer, 30);

    // stringifyAll round-trip
    let vals: [2]std.json.Value = .{
        std.json.Value{ .object = obj1 },
        std.json.Value{ .array = arr2 },
    };
    const batch = stringifyAll(alloc, &vals)?;
    defer alloc.free(batch);
    const batch_recs = parseAll(alloc, batch)?;
    defer freeRecords(alloc, batch_recs);
    expect_eq(batch_recs.len, 2);
}

fn testGuardsAndLineErrors(): ! {
    const alloc = std.testing.allocator;

    // 1. InvalidLimit when max_line_bytes == 0
    let bad_init = NdjsonStreamParser.init(alloc, 0);
    expect_eq(bad_init == NdjsonError.InvalidLimit, true);

    // 2. MaxLineLengthExceeded guard
    let p_limit = NdjsonStreamParser.init(alloc, 16)?;
    defer p_limit.deinit();
    let too_long = p_limit.feed("{\"key\":\"this_exceeds_sixteen_bytes\"}\n");
    expect_eq(too_long == NdjsonError.MaxLineLengthExceeded, true);
    expect_eq(p_limit.lastErrorLine(), 1);

    // 3. InvalidJsonLine with accurate physical line number (line 3 after blank line 2)
    let p_bad = NdjsonStreamParser.init(alloc, 1024)?;
    defer p_bad.deinit();
    * = p_bad.feed("{\"ok\":1}\n\n")?;
    let bad_json = p_bad.feed("{broken_json: true}\n");
    expect_eq(bad_json == NdjsonError.InvalidJsonLine, true);
    expect_eq(p_bad.lastErrorLine(), 3);

    // 4. UnterminatedFinalLine in strict mode
    let p_strict = NdjsonStreamParser.init(alloc, 1024)?;
    defer p_strict.deinit();
    * = p_strict.feed("{\"ok\":1}\n{\"unterminated\":2}")?;
    let fin_err = p_strict.finish(false);
    expect_eq(fin_err == NdjsonError.UnterminatedFinalLine, true);
    expect_eq(p_strict.lastErrorLine(), 2);
}

fn testWriterFrameBoundaryProtection(): ! {
    const alloc = std.testing.allocator;
    let writer = NdjsonWriter.init(alloc);
    defer writer.deinit();

    let lf_err = writer.writeRawJson("{\"a\":1}\n{\"b\":2}");
    expect_eq(lf_err == NdjsonError.InvalidJsonLine, true);

    let cr_err = writer.writeRawJson("{\"a\":1}\r");
    expect_eq(cr_err == NdjsonError.InvalidJsonLine, true);

    let bad_json_err = writer.writeRawJson("{not_json}");
    expect_eq(bad_json_err == NdjsonError.InvalidJsonLine, true);

    let empty_err = writer.writeRawJson("   ");
    expect_eq(empty_err == NdjsonError.InvalidJsonLine, true);

    expect_eq(writer.bytes().len, 0);
}

fn testDynamicCapNamespaceAndZeroLeaks(): ! {
    const alloc = std.testing.allocator;

    let parser = cap.conf.NdjsonStreamParser.init(alloc, 2048)?;
    defer parser.deinit();

    * = parser.feed("{\"ns\":\"cap.ndjson\"}\n")?;
    * = parser.finish(false)?;
    expect_eq(parser.parsedCount(), 1);

    let writer = cap.conf.NdjsonWriter.init(alloc);
    defer writer.deinit();
    writer.writeRawJson("[1,2,3]")?;

    const recs = cap.conf.parseAll(alloc, writer.bytes())?;
    defer freeRecords(alloc, recs);
    expect_eq(recs.len, 1);
    expect_eq(recs[0].parsed.value.array.items.len, 3);
}

test("1. Incremental Chunked feed() & finish() with CRLF and Blank Line Skipping", () => {
    try testIncrementalChunkedFeedAndFinish();
});

test("2. NdjsonWriter Serialization & parseAll / stringifyAll Round-Trip", () => {
    try testWriterAndParseAllRoundTrip();
});

test("3. MaxLineLengthExceeded, InvalidJsonLine Line Numbers & UnterminatedFinalLine", () => {
    try testGuardsAndLineErrors();
});

test("4. NdjsonWriter Frame Boundary Protection Against Raw Newlines", () => {
    try testWriterFrameBoundaryProtection();
});

test("5. Dynamic Lazy Namespace `cap.ndjson` & std.testing.allocator Zero Leaks", () => {
    try testDynamicCapNamespaceAndZeroLeaks();
});
