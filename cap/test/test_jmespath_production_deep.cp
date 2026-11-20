// Source: cp/cap/test/test_jmespath_production_deep.cp (JMESPath production deep tests)
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    JmesError,
    JmesValue,
    parseJson,
    stringifyJson,
    compileJmesPath,
    searchJmesPath,
    searchJmesPathJson,
} from "cap:jmespath";

test("jmespath subexpr, negative index, slice, wildcards and flatten") {
    const alloc = std.testing.allocator;
    const json =
        \\{"a":{"b":{"c":"ok"}},"arr":[10,20,30,40,50],"nested":[[1,2],[3],[4,5]],"objs":{"x":{"v":1},"y":{"v":2}}}
    ;

    const r1 = searchJmesPathJson(alloc, "a.b.c", json)?;
    defer alloc.free(r1);
    try std.testing.expectEqualStrings("\"ok\"", r1);

    const r2 = searchJmesPathJson(alloc, "arr[-1]", json)?;
    defer alloc.free(r2);
    try std.testing.expectEqualStrings("50", r2);

    const r3 = searchJmesPathJson(alloc, "arr[1:4:2]", json)?;
    defer alloc.free(r3);
    try std.testing.expectEqualStrings("[20,40]", r3);

    const r4 = searchJmesPathJson(alloc, "arr[::-2]", json)?;
    defer alloc.free(r4);
    try std.testing.expectEqualStrings("[50,30,10]", r4);

    const r5 = searchJmesPathJson(alloc, "nested[]", json)?;
    defer alloc.free(r5);
    try std.testing.expectEqualStrings("[1,2,3,4,5]", r5);

    const r6 = searchJmesPathJson(alloc, "objs.*.v", json)?;
    defer alloc.free(r6);
    try std.testing.expectEqualStrings("[1,2]", r6);

    const r7 = searchJmesPathJson(alloc, "nested[*][0]", json)?;
    defer alloc.free(r7);
    try std.testing.expectEqualStrings("[1,3,4]", r7);
}

test("jmespath filter, multiselect hash/list and pipe composition") {
    const alloc = std.testing.allocator;
    const json =
        \\{"people":[{"name":"alice","age":30,"role":"eng"},{"name":"bob","age":17,"role":"intern"},{"name":"carol","age":25,"role":"eng"}]}
    ;

    const r1 = searchJmesPathJson(
        alloc,
        "people[?age >= `18` && role == 'eng'].{n: name, a: age} | [1]",
        json,
    )?;
    defer alloc.free(r1);
    try std.testing.expectEqualStrings("{\"n\":\"carol\",\"a\":25}", r1);

    const r2 = searchJmesPathJson(
        alloc,
        "people[?!(age < `20`)].[name, role]",
        json,
    )?;
    defer alloc.free(r2);
    try std.testing.expectEqualStrings("[[\"alice\",\"eng\"],[\"carol\",\"eng\"]]", r2);

    const r3 = searchJmesPathJson(
        alloc,
        "people[?age > `28` || role == 'intern'].name",
        json,
    )?;
    defer alloc.free(r3);
    try std.testing.expectEqualStrings("[\"alice\",\"bob\"]", r3);
}

test("jmespath builtins and backtick literals") {
    const alloc = std.testing.allocator;
    const json =
        \\{"nums":[3,1,4,1,5],"words":["cp","zig","rust"],"obj":{"k1":10,"k2":20},"maybe":null}
    ;

    const r_len = searchJmesPathJson(alloc, "length(nums)", json)?;
    defer alloc.free(r_len);
    try std.testing.expectEqualStrings("5", r_len);

    const r_keys = searchJmesPathJson(alloc, "keys(obj)", json)?;
    defer alloc.free(r_keys);
    try std.testing.expectEqualStrings("[\"k1\",\"k2\"]", r_keys);

    const r_vals = searchJmesPathJson(alloc, "values(obj)", json)?;
    defer alloc.free(r_vals);
    try std.testing.expectEqualStrings("[10,20]", r_vals);

    const r_sort = searchJmesPathJson(alloc, "sort(nums)", json)?;
    defer alloc.free(r_sort);
    try std.testing.expectEqualStrings("[1,1,3,4,5]", r_sort);

    const r_rev = searchJmesPathJson(alloc, "reverse(words)", json)?;
    defer alloc.free(r_rev);
    try std.testing.expectEqualStrings("[\"rust\",\"zig\",\"cp\"]", r_rev);

    const r_contains = searchJmesPathJson(alloc, "contains(words, 'zig')", json)?;
    defer alloc.free(r_contains);
    try std.testing.expectEqualStrings("true", r_contains);

    const r_sw = searchJmesPathJson(alloc, "starts_with('hello_cp', 'hello')", json)?;
    defer alloc.free(r_sw);
    try std.testing.expectEqualStrings("true", r_sw);

    const r_ew = searchJmesPathJson(alloc, "ends_with('hello_cp', 'cp')", json)?;
    defer alloc.free(r_ew);
    try std.testing.expectEqualStrings("true", r_ew);

    const r_join = searchJmesPathJson(alloc, "join('-', words)", json)?;
    defer alloc.free(r_join);
    try std.testing.expectEqualStrings("\"cp-zig-rust\"", r_join);

    const r_max = searchJmesPathJson(alloc, "max(nums)", json)?;
    defer alloc.free(r_max);
    try std.testing.expectEqualStrings("5", r_max);

    const r_min = searchJmesPathJson(alloc, "min(nums)", json)?;
    defer alloc.free(r_min);
    try std.testing.expectEqualStrings("1", r_min);

    const r_type = searchJmesPathJson(alloc, "type(obj)", json)?;
    defer alloc.free(r_type);
    try std.testing.expectEqualStrings("\"object\"", r_type);

    const r_nn = searchJmesPathJson(alloc, "not_null(maybe, `\"fallback\"`)", json)?;
    defer alloc.free(r_nn);
    try std.testing.expectEqualStrings("\"fallback\"", r_nn);
}

test("jmespath syntax errors, type errors, max_depth and zero step") {
    const alloc = std.testing.allocator;

    // Zero slice step
    try std.testing.expectError(JmesError.InvalidSliceStep, compileJmesPath(alloc, "arr[::0]"));

    // Unclosed bracket / invalid syntax
    try std.testing.expectError(JmesError.InvalidSyntax, compileJmesPath(alloc, "arr[0"));
    try std.testing.expectError(JmesError.InvalidSyntax, compileJmesPath(alloc, "a..b"));

    // Invalid function / argument type
    try std.testing.expectError(JmesError.InvalidFunction, searchJmesPathJson(alloc, "unknown_fn(a)", "{\"a\":1}"));
    try std.testing.expectError(JmesError.InvalidArgumentType, searchJmesPathJson(alloc, "keys(a)", "{\"a\":[1,2]}"));
    try std.testing.expectError(JmesError.InvalidArity, searchJmesPathJson(alloc, "length(a, b)", "{\"a\":1,\"b\":2}"));

    // max_depth (> 64) interception in JSON parser
    let deep_buf: [140]u8 = undefined;
    let i: usize = 0;
    while (i < 70) : (i += 1) {
        deep_buf[i] = '[';
        deep_buf[139 - i] = ']';
    }
    try std.testing.expectError(JmesError.MaxDepthExceeded, parseJson(alloc, deep_buf[0..140]));
}

test("cap.jmespath dynamic mount and zero memory leak") {
    const alloc = std.testing.allocator;
    const json = "{\"items\":[{\"id\":1,\"tags\":[\"a\",\"b\"]},{\"id\":2,\"tags\":[\"c\"]}]}";

    let expr = cap.jmespath.compileJmesPath(alloc, "items[].tags[]")?;
    defer expr.deinit(alloc);

    let data = cap.jmespath.parseJson(alloc, json)?;
    defer data.deinit(alloc);

    let result = cap.jmespath.searchJmesPath(alloc, &expr, data)?;
    defer result.deinit(alloc);

    const out = cap.jmespath.stringifyJson(alloc, result)?;
    defer alloc.free(out);

    try std.testing.expectEqualStrings("[\"a\",\"b\",\"c\"]", out);
    expect_eq(result.Array.len, 3);
}
