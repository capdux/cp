// Source: cp/cap/test/test_json_rfc_production_deep.cp
// Comprehensive Production Test Suite for RFC 6901 JSON Pointer, RFC 6902 JSON Patch,
// RFC 7396 Merge Patch, RFC 8259/8785 Serializer & NDJSON Streaming Reader/Writer.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    JSON,
    JsonValue,
    JsonTag,
    JsonField,
    JsonPointer,
    JsonPatchOpKind,
    JsonPatchOp,
    JsonPatch,
    JsonMergePatch,
    JsonSerializer,
    JsonSerializeOptions,
    NdjsonReader,
    NdjsonWriter,
} from "cap:json";

// ============================================================================
// 1. RFC 6901 JSON Pointer: ~0/~1 escaping, formatting & leading-zero rejection
// ============================================================================
fn testRfc6901Pointer(): !void {
    const allocator = std.heap.page_allocator;

    // Escaping / unescaping exact order (~01 -> ~1, not /)
    const esc1 = try JsonPointer.escapeToken(allocator, "a/b~c");
    defer allocator.free(esc1);
    expect_eq(std.mem.eql(u8, esc1, "a~1b~0c"), true);

    const unesc1 = try JsonPointer.unescapeToken(allocator, "a~1b~0c");
    defer allocator.free(unesc1);
    expect_eq(std.mem.eql(u8, unesc1, "a/b~c"), true);

    const unesc_order = try JsonPointer.unescapeToken(allocator, "~01");
    defer allocator.free(unesc_order);
    expect_eq(std.mem.eql(u8, unesc_order, "~1"), true);

    let bad_esc = false;
    * = JsonPointer.unescapeToken(allocator, "bad~2") catch { bad_esc = true; };
    expect_eq(bad_esc, true);

    // Format & parseTokens roundtrip
    let raw_toks: [3][]const u8 = ["users", "a/b", "c~d"];
    const formatted = try JsonPointer.format(allocator, &raw_toks);
    defer allocator.free(formatted);
    expect_eq(std.mem.eql(u8, formatted, "/users/a~1b/c~0d"), true);

    const parsed_toks = try JsonPointer.parseTokens(allocator, formatted);
    defer JsonPointer.freeTokens(allocator, parsed_toks);
    expect_eq(parsed_toks.len, 3 as usize);
    expect_eq(std.mem.eql(u8, parsed_toks[0], "users"), true);
    expect_eq(std.mem.eql(u8, parsed_toks[1], "a/b"), true);
    expect_eq(std.mem.eql(u8, parsed_toks[2], "c~d"), true);

    // RFC 6901 §5 test document
    const rfc_doc_src =
        "{\"foo\":[\"bar\",\"baz\"],\"\":0,\"a/b\":1,\"c%d\":2,\"e^f\":3,\"g|h\":4,\"i\\\\j\":5,\"k\\\"l\":6,\" \":7,\"m~n\":8,\"~1\":9}";
    const doc = try JSON.parse(allocator, rfc_doc_src);

    expect_eq(JsonPointer.resolve(doc, "").?.isObject(), true);
    expect_eq(std.mem.eql(u8, JsonPointer.resolve(doc, "/foo/0").?.asString().?, "bar"), true);
    expect_eq(JsonPointer.resolve(doc, "/").?.asNumber().?, 0.0);
    expect_eq(JsonPointer.resolve(doc, "/a~1b").?.asNumber().?, 1.0);
    expect_eq(JsonPointer.resolve(doc, "/c%d").?.asNumber().?, 2.0);
    expect_eq(JsonPointer.resolve(doc, "/ ").?.asNumber().?, 7.0);
    expect_eq(JsonPointer.resolve(doc, "/m~0n").?.asNumber().?, 8.0);
    expect_eq(JsonPointer.resolve(doc, "/~01").?.asNumber().?, 9.0);

    // Reject leading zeros ("01", "00"), "-", missing slash, and invalid escapes
    expect_eq(JsonPointer.resolve(doc, "/foo/01") == null, true);
    expect_eq(JsonPointer.resolve(doc, "/foo/00") == null, true);
    expect_eq(JsonPointer.resolve(doc, "/foo/-") == null, true);
    expect_eq(JsonPointer.resolve(doc, "foo") == null, true);
    expect_eq(JsonPointer.resolve(doc, "/~2") == null, true);
}

// ============================================================================
// 2. RFC 6902 JSON Patch: add/remove/replace/move/copy/test & atomic failure
// ============================================================================
fn testRfc6902Patch(): !void {
    const allocator = std.heap.page_allocator;

    const doc = try JSON.parse(
        allocator,
        "{\"a\":\"foo\",\"arr\":[\"x\",\"z\"],\"nested\":{\"k\":1,\"keep\":true},\"ab\":\"sibling\"}",
    );

    // Apply all 6 operations cleanly
    let ops: [8]JsonPatchOp = [
        JsonPatchOp.testValue("/nested/k", JsonValue{ tag: .Number, num_val: 1.0 }),
        JsonPatchOp.add("/b", JsonValue{ tag: .String, str_val: "added" }),
        JsonPatchOp.add("/arr/1", JsonValue{ tag: .String, str_val: "y" }),
        JsonPatchOp.add("/arr/-", JsonValue{ tag: .String, str_val: "end" }),
        JsonPatchOp.replaceOp("/a", JsonValue{ tag: .String, str_val: "bar" }),
        JsonPatchOp.copy("/nested", "/copied"),
        JsonPatchOp.move("/nested/k", "/moved_k"),
        JsonPatchOp.remove("/ab"),
    ];

    const patched = try JsonPatch.apply(allocator, doc, &ops);
    expect_eq(std.mem.eql(u8, JsonPointer.resolve(patched, "/a").?.asString().?, "bar"), true);
    expect_eq(std.mem.eql(u8, JsonPointer.resolve(patched, "/b").?.asString().?, "added"), true);
    expect_eq(JsonPointer.resolve(patched, "/arr").?.len(), 4 as usize);
    expect_eq(std.mem.eql(u8, JsonPointer.resolve(patched, "/arr/1").?.asString().?, "y"), true);
    expect_eq(std.mem.eql(u8, JsonPointer.resolve(patched, "/arr/3").?.asString().?, "end"), true);
    expect_eq(JsonPointer.resolve(patched, "/moved_k").?.asNumber().?, 1.0);
    expect_eq(JsonPointer.resolve(patched, "/nested/k") == null, true);
    expect_eq(JsonPointer.resolve(patched, "/copied/k").?.asNumber().?, 1.0);
    expect_eq(JsonPointer.resolve(patched, "/ab") == null, true);

    // Original doc MUST remain completely untouched
    expect_eq(std.mem.eql(u8, JsonPointer.resolve(doc, "/a").?.asString().?, "foo"), true);
    expect_eq(JsonPointer.resolve(doc, "/arr").?.len(), 2 as usize);
    expect_eq(JsonPointer.resolve(doc, "/ab") != null, true);

    // JSON-encoded RFC 6902 patch document via parseOps + applyValue
    const patch_json = try JSON.parse(
        allocator,
        "[{\"op\":\"test\",\"path\":\"/nested\",\"value\":{\"keep\":true,\"k\":1}},{\"op\":\"replace\",\"path\":\"/a\",\"value\":42}]",
    );
    const patched2 = try JsonPatch.applyValue(allocator, doc, patch_json);
    expect_eq(JsonPointer.resolve(patched2, "/a").?.asNumber().?, 42.0);

    // Failure protection: test mismatch fails atomically without mutating original doc
    let fail_ops: [2]JsonPatchOp = [
        JsonPatchOp.replaceOp("/a", JsonValue{ tag: .String, str_val: "mutated" }),
        JsonPatchOp.testValue("/a", JsonValue{ tag: .String, str_val: "wrong" }),
    ];
    let test_failed = false;
    * = JsonPatch.apply(allocator, doc, &fail_ops) catch { test_failed = true; };
    expect_eq(test_failed, true);
    expect_eq(std.mem.eql(u8, JsonPointer.resolve(doc, "/a").?.asString().?, "foo"), true);

    // Reject move into proper child (/nested -> /nested/child), but allow sibling prefix (/a -> /ab)
    let bad_move: [1]JsonPatchOp = [JsonPatchOp.move("/nested", "/nested/sub")];
    let move_child_err = false;
    * = JsonPatch.apply(allocator, doc, &bad_move) catch { move_child_err = true; };
    expect_eq(move_child_err, true);

    let sibling_move: [1]JsonPatchOp = [JsonPatchOp.move("/a", "/a_renamed")];
    const sib_doc = try JSON.patch(allocator, doc, &sibling_move);
    expect_eq(JsonPointer.resolve(sib_doc, "/a") == null, true);
    expect_eq(std.mem.eql(u8, JsonPointer.resolve(sib_doc, "/a_renamed").?.asString().?, "foo"), true);
}

// ============================================================================
// 3. RFC 7396 Merge Patch: recursive merge, null key deletion & non-object rules
// ============================================================================
fn testRfc7396MergePatch(): !void {
    const allocator = std.heap.page_allocator;

    // RFC 7396 §1 example
    const target1 = try JSON.parse(
        allocator,
        "{\"title\":\"Goodbye!\",\"author\":{\"givenName\":\"John\",\"familyName\":\"Doe\"},\"tags\":[\"example\",\"sample\"],\"content\":\"This will be unchanged\"}",
    );
    const patch1 = try JSON.parse(
        allocator,
        "{\"title\":\"Hello!\",\"phoneNumber\":\"+01-123-456-7890\",\"author\":{\"familyName\":null},\"tags\":[\"example\"]}",
    );
    const merged1 = try JSON.mergePatch(allocator, target1, patch1);
    const expected1 = try JSON.parse(
        allocator,
        "{\"title\":\"Hello!\",\"phoneNumber\":\"+01-123-456-7890\",\"author\":{\"givenName\":\"John\"},\"tags\":[\"example\"],\"content\":\"This will be unchanged\"}",
    );
    expect_eq(JSON.equals(merged1, expected1), true);
    // Target remains unmodified
    expect_eq(std.mem.eql(u8, target1.pointer("/title").?.asString().?, "Goodbye!"), true);
    expect_eq(target1.pointer("/author/familyName") != null, true);

    // Non-object target is overwritten by empty object before applying object patch
    const arr_target = try JSON.parse(allocator, "[1, 2, 3]");
    const obj_patch = try JSON.parse(allocator, "{\"a\":\"b\",\"c\":null}");
    const merged_non_obj = try JsonMergePatch.apply(allocator, arr_target, obj_patch);
    const expected_non_obj = try JSON.parse(allocator, "{\"a\":\"b\"}");
    expect_eq(JSON.equals(merged_non_obj, expected_non_obj), true);

    // Non-object patch replaces object target wholesale
    const scalar_patch = try JSON.parse(allocator, "\"replaced\"");
    const merged_scalar = try JsonMergePatch.apply(allocator, target1, scalar_patch);
    expect_eq(std.mem.eql(u8, merged_scalar.asString().?, "replaced"), true);
}

// ============================================================================
// 4. RFC 8259 Pretty & RFC 8785 Canonical JSON Serializer
// ============================================================================
fn testPrettyAndCanonicalSerializer(): !void {
    const allocator = std.heap.page_allocator;

    // Canonical key sorting + nested objects + `-0` normalization + escapes
    const src = "{\"z\":1,\"a\":2,\"b\":{\"d\":4,\"c\":3},\"neg_zero\":-0.0,\"ctrl\":\"\\b\\f\\n\\r\\t\\\"\\\\\\u0001\"}";
    const doc = try JSON.parse(allocator, src);

    const canon = try JSON.stringifyCanonical(allocator, doc);
    defer allocator.free(canon);
    const expected_canon = "{\"a\":2,\"b\":{\"c\":3,\"d\":4},\"ctrl\":\"\\b\\f\\n\\r\\t\\\"\\\\\\u0001\",\"neg_zero\":0,\"z\":1}";
    expect_eq(std.mem.eql(u8, canon, expected_canon), true);

    // UTF-16 code unit ordering check (RFC 8785 §3.2.3): U+10000+ (surrogate 0xD83D) sorts before U+E000
    const u_doc = try JSON.parse(allocator, "{\"\xEE\x80\x80\":1,\"\xF0\x9F\x98\x80\":2}");
    const u_canon = try JsonSerializer.stringifyCanonical(allocator, u_doc);
    defer allocator.free(u_canon);
    expect_eq(std.mem.eql(u8, u_canon, "{\"\xF0\x9F\x98\x80\":2,\"\xEE\x80\x80\":1}"), true);

    // Pretty + sort_keys + escape_html_safe options
    const html_doc = try JSON.parse(allocator, "{\"tag\":\"<script>&</script>\",\"alpha\":true}");
    const html_safe = try JSON.stringifyWithOptions(allocator, html_doc, JsonSerializeOptions{
        indent_spaces: 2,
        sort_keys: true,
        escape_html_safe: true,
    });
    defer allocator.free(html_safe);
    const expected_html = "{\n  \"alpha\": true,\n  \"tag\": \"\\u003cscript\\u003e\\u0026\\u003c/script\\u003e\"\n}";
    expect_eq(std.mem.eql(u8, html_safe, expected_html), true);

    // Re-parse verifies semantic equality
    const reparsed = try JSON.parse(allocator, html_safe);
    expect_eq(JSON.equals(html_doc, reparsed), true);
}

// ============================================================================
// 5. NDJSON / JSONL Streaming Reader/Writer & Existing cap:json Zero Regression
// ============================================================================
fn testNdjsonAndRegression(): !void {
    const allocator = std.heap.page_allocator;

    // Cross-chunk NDJSON streaming with \r\n, \n, blank lines, and EOF without trailing \n
    let reader = NdjsonReader.init(allocator);
    defer reader.deinit();

    const r1 = try reader.feed("{\"id\":1,\"msg\":\"hel");
    defer { if (r1.len > 0) allocator.free(r1); }
    expect_eq(r1.len, 0 as usize);
    expect_eq(reader.bufferedBytes() > 0, true);

    const r2 = try reader.feed("lo\"}\r\n   \t  \n{\"id\":2,\"arr\":[10,20]}\n{\"id\":3");
    defer { if (r2.len > 0) allocator.free(r2); }
    expect_eq(r2.len, 2 as usize);
    expect_eq(r2[0].pointer("/id").?.asNumber().?, 1.0);
    expect_eq(std.mem.eql(u8, r2[0].pointer("/msg").?.asString().?, "hello"), true);
    expect_eq(r2[1].pointer("/arr/1").?.asNumber().?, 20.0);

    const r3 = try reader.feed(",\"ok\":true}");
    defer { if (r3.len > 0) allocator.free(r3); }
    expect_eq(r3.len, 0 as usize);

    const r4 = try reader.finish();
    defer { if (r4.len > 0) allocator.free(r4); }
    expect_eq(r4.len, 1 as usize);
    expect_eq(r4[0].pointer("/id").?.asNumber().?, 3.0);
    expect_eq(r4[0].pointer("/ok").?.asBool().?, true);
    expect_eq(reader.totalRecords(), 3 as usize);
    expect_eq(reader.currentLine(), 4 as usize);

    // Line-length DoS guard enforcement
    let limited = NdjsonReader.initWithOptions(allocator, 16, 32);
    defer limited.deinit();
    let too_long = false;
    * = limited.feed("{\"payload\":\"this_exceeds_16_bytes\"}\n") catch { too_long = true; };
    expect_eq(too_long, true);

    // NdjsonWriter -> NdjsonReader.parseAll roundtrip
    let items: [3]JsonValue = [r2[0], r2[1], r4[0]];
    const ndjson_bytes = try JSON.stringifyNdjson(allocator, &items);
    defer allocator.free(ndjson_bytes);
    expect_eq(
        std.mem.eql(u8, ndjson_bytes, "{\"id\":1,\"msg\":\"hello\"}\n{\"id\":2,\"arr\":[10,20]}\n{\"id\":3,\"ok\":true}\n"),
        true,
    );

    const roundtrip = try JSON.parseNdjson(allocator, ndjson_bytes);
    defer allocator.free(roundtrip);
    expect_eq(roundtrip.len, 3 as usize);
    expect_eq(JSON.equals(roundtrip[0], items[0]), true);
    expect_eq(JSON.equals(roundtrip[1], items[1]), true);
    expect_eq(JSON.equals(roundtrip[2], items[2]), true);

    // Single-record writer
    const single_line = try NdjsonWriter.stringifyRecord(allocator, items[0]);
    defer allocator.free(single_line);
    expect_eq(std.mem.eql(u8, single_line, "{\"id\":1,\"msg\":\"hello\"}\n"), true);

    // Existing JSON.stringifyValue / stringifyPretty / pointer zero regression
    const compact = try JSON.stringifyValue(allocator, items[1]);
    defer allocator.free(compact);
    expect_eq(std.mem.eql(u8, compact, "{\"id\":2,\"arr\":[10,20]}"), true);
    expect_eq(JSON.pointer(items[1], "/arr/0").?.asNumber().?, 10.0);
}

fn main() {
    test("1. RFC 6901 JSON Pointer: ~0/~1 escaping, format & leading-zero rejection", () => {
        try testRfc6901Pointer();
    });
    test("2. RFC 6902 JSON Patch: add/remove/replace/move/copy/test & atomic failure", () => {
        try testRfc6902Patch();
    });
    test("3. RFC 7396 Merge Patch: recursive merge, null deletion & non-object rules", () => {
        try testRfc7396MergePatch();
    });
    test("4. RFC 8259 Pretty & RFC 8785 Canonical JSON Serializer", () => {
        try testPrettyAndCanonicalSerializer();
    });
    test("5. NDJSON / JSONL Streaming Reader & Writer + cap:json Zero Regression", () => {
        try testNdjsonAndRegression();
    });
}
