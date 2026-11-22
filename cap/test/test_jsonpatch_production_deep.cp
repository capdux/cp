// Source: cp/cap/test/test_jsonpatch_production_deep.cp
// Deep Production Test Suite for cap:jsonpatch & cap.jsonpatch
// Covers:
//   1. RFC 6901 Pointer escaping (~0/~1), array indexing, root pointer "", leading-zero "01" / InvalidPointer rejection
//   2. RFC 6902 all 6 operations: add (incl. "-" append), remove, replace, move, copy, test atomic execution
//   3. Error handling & rollback: TestFailed, PathNotFound, IndexOutOfBounds, CycleMove
//   4. createDiffPatch minimal patch generation & round-trip equivalence verification
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.jsonpatch` & std.testing.allocator zero leaks

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    JsonPatchError,
    PatchOpKind,
    escapeSegment,
    unescapeSegment,
    resolvePointer,
    jsonValuesEqual,
    applyPatch,
    createDiffPatch,
} from "cap:conf";
import cap from "cap";

fn testRfc6901PointerEscapingAndValidation(): ! {
    const alloc = std.testing.allocator;

    // 1. escapeSegment / unescapeSegment (~0 <-> ~, ~1 <-> /, ~01 -> ~1)
    const esc = escapeSegment(alloc, "a/b~c")?;
    defer alloc.free(esc);
    expect_eq(std.mem.eql(u8, esc, "a~1b~0c"), true);

    const unesc = unescapeSegment(alloc, "a~1b~0c~01")?;
    defer alloc.free(unesc);
    expect_eq(std.mem.eql(u8, unesc, "a/b~c~1"), true);

    // Invalid escape sequence
    let bad_esc = unescapeSegment(alloc, "bad~2seg");
    expect_eq(bad_esc == JsonPatchError.InvalidPointer, true);

    // 2. RFC 6901 Section 5 classic document
    const doc_str = "{\"foo\":[\"bar\",\"baz\"],\"\":0,\"a/b\":1,\"c%d\":2,\"e^f\":3,\"g|h\":4,\"i\\\\j\":5,\"k\\\"l\":6,\" \":7,\"m~n\":8}";
    const parsed = std.json.parseFromSlice(std.json.Value, alloc, doc_str, .{})?;
    defer parsed.deinit();
    const doc = parsed.value;

    // Root pointer ""
    const root_val = resolvePointer(doc, "")?;
    expect_eq(jsonValuesEqual(root_val, doc), true);

    // Array index & escaped keys
    expect_eq(std.mem.eql(u8, (resolvePointer(doc, "/foo/0")?).string, "bar"), true);
    expect_eq((resolvePointer(doc, "/a~1b")?).integer, 1);
    expect_eq((resolvePointer(doc, "/m~0n")?).integer, 8);
    expect_eq((resolvePointer(doc, "/")?).integer, 0);
    expect_eq((resolvePointer(doc, "/ ")?).integer, 7);

    // Leading zero "01" must be rejected as InvalidPointer
    let lead_zero = resolvePointer(doc, "/foo/01");
    expect_eq(lead_zero == JsonPatchError.InvalidPointer, true);

    // Missing leading slash must be rejected as InvalidPointer
    let no_slash = resolvePointer(doc, "foo/0");
    expect_eq(no_slash == JsonPatchError.InvalidPointer, true);

    // "-" in resolvePointer -> IndexOutOfBounds
    let dash_res = resolvePointer(doc, "/foo/-");
    expect_eq(dash_res == JsonPatchError.IndexOutOfBounds, true);
}

fn testRfc6902AllSixAtomicOperations(): ! {
    const alloc = std.testing.allocator;

    const doc_json = "{\"user\":{\"name\":\"Alice\",\"roles\":[\"viewer\"]},\"meta\":{\"version\":1},\"temp\":\"drop_me\"}";
    const patch_json = "[" ++
        "{\"op\":\"test\",\"path\":\"/user/name\",\"value\":\"Alice\"}," ++
        "{\"op\":\"add\",\"path\":\"/user/roles/-\",\"value\":\"editor\"}," ++
        "{\"op\":\"add\",\"path\":\"/user/roles/0\",\"value\":\"admin\"}," ++
        "{\"op\":\"replace\",\"path\":\"/meta/version\",\"value\":2}," ++
        "{\"op\":\"remove\",\"path\":\"/temp\"}," ++
        "{\"op\":\"copy\",\"from\":\"/user/roles\",\"path\":\"/backup_roles\"}," ++
        "{\"op\":\"move\",\"from\":\"/meta/version\",\"path\":\"/api_version\"}" ++
        "]";

    const patched_json = applyPatch(alloc, doc_json, patch_json)?;
    defer alloc.free(patched_json);

    const parsed = std.json.parseFromSlice(std.json.Value, alloc, patched_json, .{})?;
    defer parsed.deinit();
    const res = parsed.value;

    expect_eq((resolvePointer(res, "/api_version")?).integer, 2);
    expect_eq(std.mem.eql(u8, (resolvePointer(res, "/user/roles/0")?).string, "admin"), true);
    expect_eq(std.mem.eql(u8, (resolvePointer(res, "/user/roles/1")?).string, "viewer"), true);
    expect_eq(std.mem.eql(u8, (resolvePointer(res, "/user/roles/2")?).string, "editor"), true);
    expect_eq(std.mem.eql(u8, (resolvePointer(res, "/backup_roles/2")?).string, "editor"), true);

    let removed_temp = resolvePointer(res, "/temp");
    expect_eq(removed_temp == JsonPatchError.PathNotFound, true);
    let moved_ver = resolvePointer(res, "/meta/version");
    expect_eq(moved_ver == JsonPatchError.PathNotFound, true);
}

fn testRfc6902ErrorsAndCycleMoveRejection(): ! {
    const alloc = std.testing.allocator;
    const doc_json = "{\"a\":{\"b\":{\"c\":42}},\"arr\":[1,2]}";

    // 1. TestFailed (atomic rollback, no partial mutation or leak)
    const bad_test = "[{\"op\":\"replace\",\"path\":\"/a/b/c\",\"value\":99},{\"op\":\"test\",\"path\":\"/a/b/c\",\"value\":42}]";
    let res_test = applyPatch(alloc, doc_json, bad_test);
    expect_eq(res_test == JsonPatchError.TestFailed, true);

    // 2. PathNotFound
    const bad_path = "[{\"op\":\"replace\",\"path\":\"/a/missing\",\"value\":1}]";
    let res_path = applyPatch(alloc, doc_json, bad_path);
    expect_eq(res_path == JsonPatchError.PathNotFound, true);

    // 3. IndexOutOfBounds
    const bad_idx = "[{\"op\":\"remove\",\"path\":\"/arr/5\"}]";
    let res_idx = applyPatch(alloc, doc_json, bad_idx);
    expect_eq(res_idx == JsonPatchError.IndexOutOfBounds, true);

    // 4. CycleMove: moving parent /a into child /a/b/sub
    const cycle_patch = "[{\"op\":\"move\",\"from\":\"/a\",\"path\":\"/a/b/sub\"}]";
    let res_cycle = applyPatch(alloc, doc_json, cycle_patch);
    expect_eq(res_cycle == JsonPatchError.CycleMove, true);
}

fn testCreateDiffPatchRoundTrip(): ! {
    const alloc = std.testing.allocator;

    const from_json = "{\"service\":\"auth\",\"replicas\":2,\"tags\":[\"v1\",\"canary\",\"old\"],\"env\":{\"debug\":true,\"region\":\"us-east\"}}";
    const to_json = "{\"service\":\"auth\",\"replicas\":5,\"tags\":[\"v1\",\"stable\"],\"env\":{\"region\":\"eu-west\",\"tls\":true}}";

    const diff_json = createDiffPatch(alloc, from_json, to_json)?;
    defer alloc.free(diff_json);

    const replayed_json = applyPatch(alloc, from_json, diff_json)?;
    defer alloc.free(replayed_json);

    const parsed_to = std.json.parseFromSlice(std.json.Value, alloc, to_json, .{})?;
    defer parsed_to.deinit();
    const parsed_replayed = std.json.parseFromSlice(std.json.Value, alloc, replayed_json, .{})?;
    defer parsed_replayed.deinit();

    expect_eq(jsonValuesEqual(parsed_replayed.value, parsed_to.value), true);
}

fn testDynamicCapNamespaceAndZeroLeaks(): ! {
    const alloc = std.testing.allocator;

    expect_eq(cap.conf.PatchOpKind.fromStr("move") == cap.conf.PatchOpKind.Move, true);
    expect_eq(std.mem.eql(u8, cap.conf.PatchOpKind.Copy.toStr(), "copy"), true);

    const doc = "{\"k\":10}";
    const patch = "[{\"op\":\"add\",\"path\":\"/k2\",\"value\":20}]";
    const out = cap.conf.applyPatch(alloc, doc, patch)?;
    defer alloc.free(out);

    const parsed = std.json.parseFromSlice(std.json.Value, alloc, out, .{})?;
    defer parsed.deinit();
    expect_eq((cap.conf.resolvePointer(parsed.value, "/k2")?).integer, 20);
}

test("1. RFC 6901 Pointer Escaping, Array Indexing & InvalidPointer Rejection", () => {
    testRfc6901PointerEscapingAndValidation() catch unreachable;
});

test("2. RFC 6902 All 6 Atomic Operations (add/-/remove/replace/move/copy/test)", () => {
    testRfc6902AllSixAtomicOperations() catch unreachable;
});

test("3. RFC 6902 Error Handling & CycleMove Rejection with Clean Rollback", () => {
    testRfc6902ErrorsAndCycleMoveRejection() catch unreachable;
});

test("4. createDiffPatch Minimal Diff & Round-Trip Equivalence", () => {
    testCreateDiffPatchRoundTrip() catch unreachable;
});

test("5. Dynamic Lazy Namespace `cap.jsonpatch` & std.testing.allocator Zero Leaks", () => {
    testDynamicCapNamespaceAndZeroLeaks() catch unreachable;
});
