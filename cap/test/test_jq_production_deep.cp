// Source: cp/cap/test/test_jq_production_deep.cp (Deep Production Tests for cap:jq)
import { test, expect_eq } from "cap:test";
import * as jq from "cap:jq";
import cap from "cap";
const std = @import("std");

test("jq nested fields, negative index, slice, optional and alternative operator") {
    const alloc = std.testing.allocator;
    const data = "{\"user\":{\"name\":\"alice\",\"tags\":[\"a\",\"b\",\"c\",\"d\"]},\"flag\":false}";

    const r1 = try jq.runJqJson(alloc, ".user.name", data);
    defer alloc.free(r1);
    expect_eq(std.mem.eql(u8, r1, "\"alice\""), true);

    const r2 = try jq.runJqJson(alloc, ".user.tags[-1]", data);
    defer alloc.free(r2);
    expect_eq(std.mem.eql(u8, r2, "\"d\""), true);

    const r3 = try jq.runJqJson(alloc, ".user.tags[1:3]", data);
    defer alloc.free(r3);
    expect_eq(std.mem.eql(u8, r3, "[\"b\",\"c\"]"), true);

    const r4 = try jq.runJqJson(alloc, ".user.name.sub? // \"fallback\"", data);
    defer alloc.free(r4);
    expect_eq(std.mem.eql(u8, r4, "\"fallback\""), true);

    const r5 = try jq.runJqJson(alloc, ".flag // \"default_on_false\"", data);
    defer alloc.free(r5);
    expect_eq(std.mem.eql(u8, r5, "\"default_on_false\""), true);

    const r6 = try jq.runJqJson(alloc, "empty // \"recovered\"", data);
    defer alloc.free(r6);
    expect_eq(std.mem.eql(u8, r6, "\"recovered\""), true);
}

test("jq stream expansion, select pipeline, object shorthand and array collect") {
    const alloc = std.testing.allocator;
    const data = "[{\"name\":\"alice\",\"score\":92,\"role\":\"eng\"},{\"name\":\"bob\",\"score\":74,\"role\":\"ops\"},{\"name\":\"carol\",\"score\":88,\"role\":\"eng\"}]";

    const r1 = try jq.runJqJson(alloc, "[.[] | select(.score >= 80 and .role == \"eng\") | {name, score}]", data);
    defer alloc.free(r1);
    expect_eq(std.mem.eql(u8, r1, "[{\"name\":\"alice\",\"score\":92},{\"name\":\"carol\",\"score\":88}]"), true);

    const r2 = try jq.runJqJson(alloc, ".[] | .name", data);
    defer alloc.free(r2);
    expect_eq(std.mem.eql(u8, r2, "\"alice\"\n\"bob\"\n\"carol\""), true);

    const r3 = try jq.runJqJson(alloc, "[1, 2] | [.[] * 10]", "null");
    defer alloc.free(r3);
    expect_eq(std.mem.eql(u8, r3, "[10,20]"), true);
}

test("jq builtins: map, keys, length, has, contains, unique, sort, reverse, type, values") {
    const alloc = std.testing.allocator;

    const r_map = try jq.runJqJson(alloc, "map(. + 1)", "[1,2,3]");
    defer alloc.free(r_map);
    expect_eq(std.mem.eql(u8, r_map, "[2,3,4]"), true);

    const r_keys = try jq.runJqJson(alloc, "keys", "{\"z\":1,\"a\":2,\"m\":3}");
    defer alloc.free(r_keys);
    expect_eq(std.mem.eql(u8, r_keys, "[\"a\",\"m\",\"z\"]"), true);

    const r_len = try jq.runJqJson(alloc, "length", "[10,20,30,40]");
    defer alloc.free(r_len);
    expect_eq(std.mem.eql(u8, r_len, "4"), true);

    const r_has = try jq.runJqJson(alloc, "has(\"a\"), has(\"missing\")", "{\"a\":1}");
    defer alloc.free(r_has);
    expect_eq(std.mem.eql(u8, r_has, "true\nfalse"), true);

    const r_cont = try jq.runJqJson(alloc, "contains({\"a\":[2]})", "{\"a\":[1,2,3],\"b\":4}");
    defer alloc.free(r_cont);
    expect_eq(std.mem.eql(u8, r_cont, "true"), true);

    const r_uniq = try jq.runJqJson(alloc, "unique", "[3,1,2,1,3]");
    defer alloc.free(r_uniq);
    expect_eq(std.mem.eql(u8, r_uniq, "[1,2,3]"), true);

    const r_sort = try jq.runJqJson(alloc, "sort | reverse", "[3,1,2]");
    defer alloc.free(r_sort);
    expect_eq(std.mem.eql(u8, r_sort, "[3,2,1]"), true);

    const r_type = try jq.runJqJson(alloc, "[null,true,1,\"x\",[],{}] | map(type)", "null");
    defer alloc.free(r_type);
    expect_eq(std.mem.eql(u8, r_type, "[\"null\",\"boolean\",\"number\",\"string\",\"array\",\"object\"]"), true);

    const r_vals = try jq.runJqJson(alloc, "[.[] | values]", "[1,null,2,false,null,3]");
    defer alloc.free(r_vals);
    expect_eq(std.mem.eql(u8, r_vals, "[1,2,false,3]"), true);
}

test("jq error handling: syntax error, type error, division by zero, max_depth and output limit") {
    const alloc = std.testing.allocator;

    // Syntax error
    try std.testing.expectError(jq.JqError.InvalidSyntax, jq.compileJq(alloc, ".foo | "));

    // Type error: array minus number
    try std.testing.expectError(jq.JqError.TypeError, jq.runJqJson(alloc, ". - 1", "[1,2]"));

    // Division by zero
    try std.testing.expectError(jq.JqError.DivisionByZero, jq.runJqJson(alloc, "10 / 0", "null"));

    // Max depth JSON guard (> 64)
    let deep_buf = std.ArrayList(u8).empty;
    defer deep_buf.deinit(alloc);
    let i: usize = 0;
    while (i < 70) {
        try deep_buf.append(alloc, '[');
        i += 1;
    }
    try deep_buf.append(alloc, '1');
    i = 0;
    while (i < 70) {
        try deep_buf.append(alloc, ']');
        i += 1;
    }
    try std.testing.expectError(jq.JqError.MaxDepthExceeded, jq.parseJson(alloc, deep_buf.items));

    // Output limit DoS guard (> 65536 outputs via comma cartesian explosion)
    const boom = "[1,2,3,4] | (.,.) | (.,.) | (.,.) | (.,.) | (.,.) | (.,.) | (.,.) | (.,.) | (.,.) | (.,.) | (.,.) | (.,.) | (.,.) | (.,.) | (.,.)";
    try std.testing.expectError(jq.JqError.OutputLimitExceeded, jq.runJqJson(alloc, boom, "null"));
}

test("jq cap.jq dynamic mount and zero-leak roundtrip with testing.allocator") {
    const alloc = std.testing.allocator;
    const raw = "{\"items\":[{\"id\":1,\"ok\":true},{\"id\":2,\"ok\":false}],\"meta\":{\"v\":2}}";

    let parsed = try cap.jq.parseJson(alloc, raw);
    defer parsed.deinit(alloc);

    let expr = try cap.jq.compileJq(alloc, "{ids: [.items[] | select(.ok) | .id], ver: .meta.v}");
    defer expr.deinit(alloc);

    let out = try cap.jq.runJq(alloc, &expr, parsed);
    defer cap.jq.freeValues(alloc, &out);

    expect_eq(out.len, 1);
    const json_str = try cap.jq.stringifyJson(alloc, out[0]);
    defer alloc.free(json_str);
    expect_eq(std.mem.eql(u8, json_str, "{\"ids\":[1],\"ver\":2}"), true);
}
