// Source: cp/cap/test/test_jsonpath_production_deep.cp (RFC 9535 JSONPath Deep Production Tests)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    JsonKind,
    JsonVal,
    JsonPathQuery,
    JsonPathError,
    QueryError,
    QueryLimits,
    parseJson,
    deinitJsonVal,
    compileJsonPath,
    deinitJsonPathQuery,
    queryJsonPath,
    queryJsonPathWithLimits,
    queryJsonPathValues,
    queryJsonPathStrings,
    deinitMatches,
    deinitStrings,
} from "cap:jsonpath";

const STORE_JSON =
    "{\"store\":{\"book\":[" ++
    "{\"category\":\"reference\",\"author\":\"Nigel Rees\",\"title\":\"Sayings of the Century\",\"price\":8.95,\"tags\":[\"classic\"]}," ++
    "{\"category\":\"fiction\",\"author\":\"Evelyn Waugh\",\"title\":\"Sword of Honour\",\"price\":12.99,\"tags\":[\"war\",\"drama\"]}," ++
    "{\"category\":\"fiction\",\"author\":\"Herman Melville\",\"title\":\"Moby Dick\",\"isbn\":\"0-553-21311-3\",\"price\":8.99,\"tags\":[\"sea\",\"whale\",\"classic\"]}," ++
    "{\"category\":\"fiction\",\"author\":\"J. R. R. Tolkien\",\"title\":\"The Lord of the Rings\",\"isbn\":\"0-395-19395-8\",\"price\":22.99,\"tags\":[\"fantasy\"]}" ++
    "],\"bicycle\":{\"color\":\"red\",\"price\":399}},\"expensive\":10}";

test("jsonpath: member, negative index, forward/reverse slice & union") {
    const alloc = std.testing.allocator;
    let doc = try parseJson(alloc, STORE_JSON);
    defer deinitJsonVal(&doc, alloc);

    // Dot & quoted member + negative index
    let q1 = try compileJsonPath(alloc, "$.store['book'][-1].\"title\"");
    defer deinitJsonPathQuery(&q1, alloc);
    let m1 = try queryJsonPath(alloc, &doc, &q1);
    defer deinitMatches(m1, alloc);
    expect_eq(m1.len, 1);
    expect_eq(std.mem.eql(u8, m1[0].value.str_val, "The Lord of the Rings"), true);
    expect_eq(std.mem.eql(u8, m1[0].path, "$['store']['book'][3]['title']"), true);

    // Step slice [1:4:2] -> indices 1, 3
    let q2 = try compileJsonPath(alloc, "$.store.book[1:4:2].author");
    defer deinitJsonPathQuery(&q2, alloc);
    let v2 = try queryJsonPathValues(alloc, &doc, &q2);
    defer alloc.free(v2);
    expect_eq(v2.len, 2);
    expect_eq(std.mem.eql(u8, v2[0].str_val, "Evelyn Waugh"), true);
    expect_eq(std.mem.eql(u8, v2[1].str_val, "J. R. R. Tolkien"), true);

    // Reverse slice [::-1]
    let arr_doc = try parseJson(alloc, "[10, 20, 30, 40]");
    defer deinitJsonVal(&arr_doc, alloc);
    let q3 = try compileJsonPath(alloc, "$[::-1]");
    defer deinitJsonPathQuery(&q3, alloc);
    let v3 = try queryJsonPathValues(alloc, &arr_doc, &q3);
    defer alloc.free(v3);
    expect_eq(v3.len, 4);
    expect_eq(v3[0].num_val == 40.0, true);
    expect_eq(v3[3].num_val == 10.0, true);

    // Union [0, 2, 'extra']
    let q4 = try compileJsonPath(alloc, "$[0, 2]");
    defer deinitJsonPathQuery(&q4, alloc);
    let v4 = try queryJsonPathValues(alloc, &arr_doc, &q4);
    defer alloc.free(v4);
    expect_eq(v4.len, 2);
    expect_eq(v4[0].num_val == 10.0, true);
    expect_eq(v4[1].num_val == 30.0, true);
}

test("jsonpath: recursive descent & RFC 9535 NormalizedPath") {
    const alloc = std.testing.allocator;
    let doc = try parseJson(alloc, STORE_JSON);
    defer deinitJsonVal(&doc, alloc);

    let q1 = try compileJsonPath(alloc, "$..author");
    defer deinitJsonPathQuery(&q1, alloc);
    let m1 = try queryJsonPath(alloc, &doc, &q1);
    defer deinitMatches(m1, alloc);
    expect_eq(m1.len, 4);
    expect_eq(std.mem.eql(u8, m1[0].path, "$['store']['book'][0]['author']"), true);
    expect_eq(std.mem.eql(u8, m1[2].path, "$['store']['book'][2]['author']"), true);
    expect_eq(std.mem.eql(u8, m1[2].value.str_val, "Herman Melville"), true);

    // Descendant wildcard $..[*] on small nested object
    let small = try parseJson(alloc, "{\"a\":{\"b\":[1,2]}}");
    defer deinitJsonVal(&small, alloc);
    let q2 = try compileJsonPath(alloc, "$..[*]");
    defer deinitJsonPathQuery(&q2, alloc);
    let m2 = try queryJsonPath(alloc, &small, &q2);
    defer deinitMatches(m2, alloc);
    // a, b, 1, 2 => 4 nodes
    expect_eq(m2.len, 4);
    expect_eq(std.mem.eql(u8, m2[0].path, "$['a']"), true);
    expect_eq(std.mem.eql(u8, m2[1].path, "$['a']['b']"), true);
    expect_eq(std.mem.eql(u8, m2[2].path, "$['a']['b'][0]"), true);
    expect_eq(std.mem.eql(u8, m2[3].path, "$['a']['b'][1]"), true);
}

test("jsonpath: filters, logical ops, existence, length/count & root comparison") {
    const alloc = std.testing.allocator;
    let doc = try parseJson(alloc, STORE_JSON);
    defer deinitJsonVal(&doc, alloc);

    // Filter with && and existence test
    let q1 = try compileJsonPath(alloc, "$.store.book[?@.price < 10 && @.isbn].title");
    defer deinitJsonPathQuery(&q1, alloc);
    let v1 = try queryJsonPathValues(alloc, &doc, &q1);
    defer alloc.free(v1);
    expect_eq(v1.len, 1);
    expect_eq(std.mem.eql(u8, v1[0].str_val, "Moby Dick"), true);

    // Filter with ||, !, length() and root $ comparison
    let q2 = try compileJsonPath(alloc, "$.store.book[?(@.price < $.expensive && !@.isbn) || length(@.tags) >= 3].author");
    defer deinitJsonPathQuery(&q2, alloc);
    let v2 = try queryJsonPathValues(alloc, &doc, &q2);
    defer alloc.free(v2);
    expect_eq(v2.len, 2);
    expect_eq(std.mem.eql(u8, v2[0].str_val, "Nigel Rees"), true);
    expect_eq(std.mem.eql(u8, v2[1].str_val, "Herman Melville"), true);

    // count() & queryJsonPathStrings
    let q3 = try compileJsonPath(alloc, "$.store.book[?count(@.tags[*]) == 2].title");
    defer deinitJsonPathQuery(&q3, alloc);
    let s3 = try queryJsonPathStrings(alloc, &doc, &q3);
    defer deinitStrings(s3, alloc);
    expect_eq(s3.len, 1);
    expect_eq(std.mem.eql(u8, s3[0], "\"Sword of Honour\""), true);
}

fn expectCompileErr(alloc: std.mem.Allocator, expr: []const u8, expected: JsonPathError): bool {
    let q = compileJsonPath(alloc, expr) catch |e| {
        return e == expected;
    };
    deinitJsonPathQuery(&q, alloc);
    return false;
}

fn expectQueryErr(alloc: std.mem.Allocator, root: *const JsonVal, q: *const JsonPathQuery, limits: QueryLimits, expected: QueryError): bool {
    let m = queryJsonPathWithLimits(alloc, root, q, limits) catch |e| {
        return e == expected;
    };
    deinitMatches(m, alloc);
    return false;
}

test("jsonpath: syntax errors, zero slice step & DoS limits") {
    const alloc = std.testing.allocator;

    // Missing '$' root identifier
    expect_eq(expectCompileErr(alloc, "store.book[0]", JsonPathError.MissingRootIdentifier), true);

    // Unterminated bracket
    expect_eq(expectCompileErr(alloc, "$.store['book'", JsonPathError.UnterminatedBracket), true);

    // Zero slice step [0:5:0]
    expect_eq(expectCompileErr(alloc, "$[0:5:0]", JsonPathError.ZeroSliceStep), true);

    // Depth & max nodes DoS protection
    let nested = try parseJson(alloc, "{\"a\":{\"b\":{\"c\":{\"d\":1}}}}");
    defer deinitJsonVal(&nested, alloc);
    let q_desc = try compileJsonPath(alloc, "$..*");
    defer deinitJsonPathQuery(&q_desc, alloc);

    let tight_depth = QueryLimits{ .max_depth = 2, .max_nodes = 100 };
    expect_eq(expectQueryErr(alloc, &nested, &q_desc, tight_depth, QueryError.MaxDepthExceeded), true);

    let tight_nodes = QueryLimits{ .max_depth = 64, .max_nodes = 2 };
    expect_eq(expectQueryErr(alloc, &nested, &q_desc, tight_nodes, QueryError.MaxNodesExceeded), true);
}

test("jsonpath: cap.jsonpath dynamic mount & zero leak") {
    const alloc = std.testing.allocator;
    let doc = try cap.jsonpath.parseJson(alloc, "{\"items\":[{\"id\":1,\"name\":\"alpha\"},{\"id\":2,\"name\":\"beta\"}]}");
    defer cap.jsonpath.deinitJsonVal(&doc, alloc);

    let q = try cap.jsonpath.compileJsonPath(alloc, "$.items[?@.id == 2].name");
    defer cap.jsonpath.deinitJsonPathQuery(&q, alloc);

    let matches = try cap.jsonpath.queryJsonPath(alloc, &doc, &q);
    defer cap.jsonpath.deinitMatches(matches, alloc);
    expect_eq(matches.len, 1);
    expect_eq(matches[0].value.kind == JsonKind.String, true);
    expect_eq(std.mem.eql(u8, matches[0].value.str_val, "beta"), true);
    expect_eq(std.mem.eql(u8, matches[0].path, "$['items'][1]['name']"), true);
}
