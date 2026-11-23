// Source: native-cp-stdlib (RFC 6902 JSON Patch & RFC 6901 Pointer)
import std from "std";
import cap from "cap";
import { test, expect_eq } from "cap:test";
import {
    PatchOpKind,
    Rfc6902PatchOp as JsonPatchOp,
    resolvePointer,
    applyPatch,
    applyPatchJson,
    mergePatch,
    diff,
    freeOps,
    freeValue,
    valuesEqual,
    stringifyValue
} from "cap:json";

test("RFC 6901 JSON Pointer ~0/~1 unescaping and array indexing") {
    const alloc = std.testing.allocator;
    const raw = "{\"a/b\":1,\"c~d\":2,\"\":0,\"arr\":[\"x\",\"y\"],\"a~1b\":3}";
    let parsed = try std.json.parseFromSlice(std.json.Value, alloc, raw, .{});
    defer parsed.deinit();

    const v1 = try resolvePointer(parsed.value, "/a~1b");
    expect_eq(v1.integer, 1);

    const v2 = try resolvePointer(parsed.value, "/c~0d");
    expect_eq(v2.integer, 2);

    const v3 = try resolvePointer(parsed.value, "/a~01b");
    expect_eq(v3.integer, 3);

    const v4 = try resolvePointer(parsed.value, "/arr/1");
    expect_eq(std.mem.eql(u8, v4.string, "y"), true);

    const root_val = try resolvePointer(parsed.value, "");
    expect_eq(root_val == .object, true);

    let failed_dash = false;
    * = resolvePointer(parsed.value, "/arr/-") catch {
        failed_dash = true;
    };
    expect_eq(failed_dash, true);
}

test("RFC 6902 all 6 operations and array '-' append") {
    const alloc = std.testing.allocator;
    const doc_json = "{\"a\":\"foo\",\"arr\":[1,2],\"rm\":true,\"obj\":{\"x\":10}}";
    const patch_json = "[" ++
        "{\"op\":\"test\",\"path\":\"/a\",\"value\":\"foo\"}," ++
        "{\"op\":\"replace\",\"path\":\"/a\",\"value\":\"bar\"}," ++
        "{\"op\":\"add\",\"path\":\"/arr/-\",\"value\":3}," ++
        "{\"op\":\"add\",\"path\":\"/arr/0\",\"value\":0}," ++
        "{\"op\":\"remove\",\"path\":\"/rm\"}," ++
        "{\"op\":\"copy\",\"from\":\"/obj/x\",\"path\":\"/copied_x\"}," ++
        "{\"op\":\"move\",\"from\":\"/obj/x\",\"path\":\"/moved_x\"}" ++
    "]";

    const out = try applyPatchJson(alloc, doc_json, patch_json);
    defer alloc.free(out);

    let out_parsed = try std.json.parseFromSlice(std.json.Value, alloc, out, .{});
    defer out_parsed.deinit();

    const a_val = try resolvePointer(out_parsed.value, "/a");
    expect_eq(std.mem.eql(u8, a_val.string, "bar"), true);

    const arr_val = try resolvePointer(out_parsed.value, "/arr");
    expect_eq(arr_val.array.items.len, 4);
    expect_eq(arr_val.array.items[0].integer, 0);
    expect_eq(arr_val.array.items[3].integer, 3);

    const copied = try resolvePointer(out_parsed.value, "/copied_x");
    expect_eq(copied.integer, 10);

    const moved = try resolvePointer(out_parsed.value, "/moved_x");
    expect_eq(moved.integer, 10);

    let rm_missing = false;
    * = resolvePointer(out_parsed.value, "/rm") catch {
        rm_missing = true;
    };
    expect_eq(rm_missing, true);
}

test("RFC 6902 atomic rollback on test failure and zero memory leaks") {
    const alloc = std.testing.allocator;
    let parsed = try std.json.parseFromSlice(std.json.Value, alloc, "{\"a\":1,\"b\":[10,20]}", .{});
    defer parsed.deinit();

    let ops = std.ArrayList(JsonPatchOp).empty;
    defer ops.deinit(alloc);
    try ops.append(alloc, .{ .op = .Replace, .path = "/a", .value = .{ .integer = 99 } });
    try ops.append(alloc, .{ .op = .Add, .path = "/b/-", .value = .{ .integer = 30 } });
    try ops.append(alloc, .{ .op = .Test, .path = "/a", .value = .{ .integer = 1 } });

    let got_test_failed = false;
    let maybe_ok: std.json.Value? = applyPatch(alloc, parsed.value, ops.items) catch |e| blk: {
        got_test_failed = (e == error.TestFailed);
        break :blk null;
    };
    if (ok_val := maybe_ok) {
        let mv = ok_val;
        freeValue(alloc, &mv);
    }
    expect_eq(got_test_failed, true);

    const orig_a = try resolvePointer(parsed.value, "/a");
    expect_eq(orig_a.integer, 1);
    const orig_b = try resolvePointer(parsed.value, "/b");
    expect_eq(orig_b.array.items.len, 2);
}

test("diff and round-trip applyPatch consistency") {
    const alloc = std.testing.allocator;
    const src_str = "{\"keep\":1,\"drop\":\"bye\",\"nested\":{\"x\":10,\"y\":20},\"list\":[1,2,3]}";
    const dst_str = "{\"keep\":1,\"add\":true,\"nested\":{\"x\":10,\"y\":99},\"list\":[1,4]}";

    let src_p = try std.json.parseFromSlice(std.json.Value, alloc, src_str, .{});
    defer src_p.deinit();
    let dst_p = try std.json.parseFromSlice(std.json.Value, alloc, dst_str, .{});
    defer dst_p.deinit();

    const ops = try diff(alloc, src_p.value, dst_p.value);
    defer freeOps(alloc, ops);

    let applied = try applyPatch(alloc, src_p.value, ops);
    defer freeValue(alloc, &applied);

    expect_eq(valuesEqual(applied, dst_p.value), true);
}

test("RFC 7396 mergePatch and cap.json_patch dynamic mount") {
    const alloc = std.testing.allocator;
    const target_str = "{\"title\":\"Goodbye!\",\"author\":{\"givenName\":\"John\",\"familyName\":\"Doe\"},\"tags\":[\"example\",\"sample\"],\"content\":\"This will be unchanged\"}";
    const patch_str = "{\"title\":\"Hello!\",\"phoneNumber\":\"+01-123-456-7890\",\"author\":{\"familyName\":null},\"tags\":[\"example\"]}";

    let target_p = try std.json.parseFromSlice(std.json.Value, alloc, target_str, .{});
    defer target_p.deinit();
    let patch_p = try std.json.parseFromSlice(std.json.Value, alloc, patch_str, .{});
    defer patch_p.deinit();

    let merged = try mergePatch(alloc, target_p.value, patch_p.value);
    defer freeValue(alloc, &merged);

    const title_val = try cap.json.resolvePointer(merged, "/title");
    expect_eq(std.mem.eql(u8, title_val.string, "Hello!"), true);

    let family_removed = false;
    * = cap.json.resolvePointer(merged, "/author/familyName") catch {
        family_removed = true;
    };
    expect_eq(family_removed, true);

    const tags_val = try cap.json.resolvePointer(merged, "/tags");
    expect_eq(tags_val.array.items.len, 1);

    const out_json = try stringifyValue(alloc, merged);
    defer alloc.free(out_json);
    expect_eq(out_json.len > 0, true);
}
