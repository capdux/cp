// Source: cp/cap/test/test_jsonpath_production_deep.cp
// Deep Production Test Suite for cap:jsonpath & cap.jsonpath
// Covers:
//   1. RFC 9535 classic store.book queries ($.store.book[*].author, $..author, $..book[-1], $..book[0,2], Normalized Paths)
//   2. Array slicing ([1:4], [::2], [::-1] negative step) & wildcard .*
//   3. Compound filter expressions ([?(@.price < 10 && @.category == "fiction")], [?(@.isbn)], !, ||, root $ comparison)
//   4. Malformed JSONPath syntax rejection (non-$, unclosed [, InvalidSliceStep, InvalidFilter, MaxDepthExceeded) & evalJson
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.jsonpath`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { JSON, JsonValue } from "cap:json";
import { JsonPath, JsonPathError, PathMatch } from "cap:jsonpath";
import cap from "cap";

const BOOKSTORE_JSON = "{\"store\":{\"max_price\":15.0,\"book\":[{\"category\":\"reference\",\"author\":\"Nigel Rees\",\"title\":\"Sayings of the Century\",\"price\":8.95},{\"category\":\"fiction\",\"author\":\"Evelyn Waugh\",\"title\":\"Sword of Honour\",\"price\":12.99},{\"category\":\"fiction\",\"author\":\"Herman Melville\",\"title\":\"Moby Dick\",\"isbn\":\"0-553-21311-3\",\"price\":8.99},{\"category\":\"fiction\",\"author\":\"J. R. R. Tolkien\",\"title\":\"The Lord of the Rings\",\"isbn\":\"0-395-19395-8\",\"price\":22.99}],\"bicycle\":{\"color\":\"red\",\"price\":399}}}";

fn strEq(a: string, b: string): bool {
    return std.mem.eql(u8, a, b);
}

fn testRfc9535BookstoreQueries(): ! {
    const alloc = std.heap.page_allocator;
    const root = JSON.parse(alloc, BOOKSTORE_JSON)?;

    // 1. $.store.book[*].author + Normalized Path check
    let jp1 = JsonPath.compile(alloc, "$.store.book[*].author")?;
    defer jp1.deinit();
    const m1 = jp1.query(alloc, root)?;
    defer JsonPath.freeMatches(alloc, m1);

    expect_eq(m1.len, 4);
    expect_eq(strEq(m1[0].path, "$['store']['book'][0]['author']"), true);
    expect_eq(strEq(m1[0].value.asString() ?? "", "Nigel Rees"), true);
    expect_eq(strEq(m1[1].path, "$['store']['book'][1]['author']"), true);
    expect_eq(strEq(m1[1].value.asString() ?? "", "Evelyn Waugh"), true);
    expect_eq(strEq(m1[2].path, "$['store']['book'][2]['author']"), true);
    expect_eq(strEq(m1[2].value.asString() ?? "", "Herman Melville"), true);
    expect_eq(strEq(m1[3].path, "$['store']['book'][3]['author']"), true);
    expect_eq(strEq(m1[3].value.asString() ?? "", "J. R. R. Tolkien"), true);

    // 2. $..author (recursive descent)
    let jp2 = JsonPath.compile(alloc, "$..author")?;
    defer jp2.deinit();
    const vals2 = jp2.queryValues(alloc, root)?;
    defer alloc.free(vals2);
    expect_eq(vals2.len, 4);
    expect_eq(strEq(vals2[2].asString() ?? "", "Herman Melville"), true);

    // 3. $..book[-1] (negative index)
    let jp3 = JsonPath.compile(alloc, "$..book[-1]")?;
    defer jp3.deinit();
    const m3 = jp3.query(alloc, root)?;
    defer JsonPath.freeMatches(alloc, m3);
    expect_eq(m3.len, 1);
    expect_eq(strEq(m3[0].path, "$['store']['book'][3]"), true);
    expect_eq(strEq(m3[0].value.get("title")?.asString() ?? "", "The Lord of the Rings"), true);

    // 4. $..book[0,2] (union of indices)
    let jp4 = JsonPath.compile(alloc, "$..book[0,2]")?;
    defer jp4.deinit();
    const m4 = jp4.query(alloc, root)?;
    defer JsonPath.freeMatches(alloc, m4);
    expect_eq(m4.len, 2);
    expect_eq(strEq(m4[0].path, "$['store']['book'][0]"), true);
    expect_eq(strEq(m4[1].path, "$['store']['book'][2]"), true);
    expect_eq(strEq(m4[1].value.get("author")?.asString() ?? "", "Herman Melville"), true);
}

fn testArraySlicingAndWildcards(): ! {
    const alloc = std.heap.page_allocator;
    const arr_json = "{\"nums\":[10, 20, 30, 40, 50, 60], \"meta\":{\"a\":1,\"b\":2}}";
    const root = JSON.parse(alloc, arr_json)?;

    // [1:4] -> 20, 30, 40
    let jp1 = JsonPath.compile(alloc, "$.nums[1:4]")?;
    defer jp1.deinit();
    const v1 = jp1.queryValues(alloc, root)?;
    defer alloc.free(v1);
    expect_eq(v1.len, 3);
    expect_eq(v1[0].asNumber() ?? 0.0, 20.0);
    expect_eq(v1[1].asNumber() ?? 0.0, 30.0);
    expect_eq(v1[2].asNumber() ?? 0.0, 40.0);

    // [::2] -> 10, 30, 50
    let jp2 = JsonPath.compile(alloc, "$.nums[::2]")?;
    defer jp2.deinit();
    const v2 = jp2.queryValues(alloc, root)?;
    defer alloc.free(v2);
    expect_eq(v2.len, 3);
    expect_eq(v2[0].asNumber() ?? 0.0, 10.0);
    expect_eq(v2[1].asNumber() ?? 0.0, 30.0);
    expect_eq(v2[2].asNumber() ?? 0.0, 50.0);

    // [::-1] -> 60, 50, 40, 30, 20, 10 (negative step reverse)
    let jp3 = JsonPath.compile(alloc, "$.nums[::-1]")?;
    defer jp3.deinit();
    const m3 = jp3.query(alloc, root)?;
    defer JsonPath.freeMatches(alloc, m3);
    expect_eq(m3.len, 6);
    expect_eq(m3[0].value.asNumber() ?? 0.0, 60.0);
    expect_eq(strEq(m3[0].path, "$['nums'][5]"), true);
    expect_eq(m3[5].value.asNumber() ?? 0.0, 10.0);
    expect_eq(strEq(m3[5].path, "$['nums'][0]"), true);

    // .* wildcard on object
    let jp4 = JsonPath.compile(alloc, "$.meta.*")?;
    defer jp4.deinit();
    const v4 = jp4.queryValues(alloc, root)?;
    defer alloc.free(v4);
    expect_eq(v4.len, 2);
    expect_eq(v4[0].asNumber() ?? 0.0, 1.0);
    expect_eq(v4[1].asNumber() ?? 0.0, 2.0);
}

fn testFilterExpressions(): ! {
    const alloc = std.heap.page_allocator;
    const root = JSON.parse(alloc, BOOKSTORE_JSON)?;

    // 1. Compound && filter: price < 10 && category == "fiction" -> Moby Dick
    let jp1 = JsonPath.compile(alloc, "$.store.book[?(@.price < 10 && @.category == \"fiction\")]")?;
    defer jp1.deinit();
    const m1 = jp1.query(alloc, root)?;
    defer JsonPath.freeMatches(alloc, m1);
    expect_eq(m1.len, 1);
    expect_eq(strEq(m1[0].value.get("title")?.asString() ?? "", "Moby Dick"), true);

    // 2. Existence test: [?(@.isbn)] -> Moby Dick & LOTR
    let jp2 = JsonPath.compile(alloc, "$.store.book[?(@.isbn)]")?;
    defer jp2.deinit();
    const v2 = jp2.queryValues(alloc, root)?;
    defer alloc.free(v2);
    expect_eq(v2.len, 2);
    expect_eq(strEq(v2[0].get("title")?.asString() ?? "", "Moby Dick"), true);
    expect_eq(strEq(v2[1].get("title")?.asString() ?? "", "The Lord of the Rings"), true);

    // 3. Logical negation ! and || with parentheses + root path comparison $.store.max_price
    let jp3 = JsonPath.compile(alloc, "$.store.book[?(!@.isbn || @.price > $.store.max_price)]")?;
    defer jp3.deinit();
    const v3 = jp3.queryValues(alloc, root)?;
    defer alloc.free(v3);
    expect_eq(v3.len, 3);
    expect_eq(strEq(v3[0].get("author")?.asString() ?? "", "Nigel Rees"), true);
    expect_eq(strEq(v3[1].get("author")?.asString() ?? "", "Evelyn Waugh"), true);
    expect_eq(strEq(v3[2].get("author")?.asString() ?? "", "J. R. R. Tolkien"), true);

    // 4. queryOne & exists helpers
    expect_eq(jp1.exists(root), true);
    const first = jp1.queryOne(root);
    expect_eq(first != null, true);
    expect_eq(strEq(first?.get("author")?.asString() ?? "", "Herman Melville"), true);

    let jp_none = JsonPath.compile(alloc, "$.store.book[?(@.price > 1000)]")?;
    defer jp_none.deinit();
    expect_eq(jp_none.exists(root), false);
    expect_eq(jp_none.queryOne(root) == null, true);
}

fn expectCompileError(alloc: std.mem.Allocator, expr: string, expected: JsonPathError): ! {
    let matched = false;
    let jp = JsonPath.compile(alloc, expr) catch e => {
        if (e == expected) {
            matched = true;
        }
        expect_eq(matched, true);
        return;
    };
    jp.deinit();
    expect_eq(false, true);
}

fn testMalformedSyntaxAndEvalJson(): ! {
    const alloc = std.heap.page_allocator;

    // 1. Non-$ prefix
    expectCompileError(alloc, "store.book[0]", JsonPathError.InvalidExpression)?;
    expectCompileError(alloc, "", JsonPathError.InvalidExpression)?;

    // 2. Unclosed bracket
    expectCompileError(alloc, "$.store.book[0", JsonPathError.UnclosedBracket)?;
    expectCompileError(alloc, "$.store['book'", JsonPathError.UnclosedBracket)?;

    // 3. Zero slice step -> InvalidSliceStep
    expectCompileError(alloc, "$.store.book[0:2:0]", JsonPathError.InvalidSliceStep)?;

    // 4. Invalid filter expression
    expectCompileError(alloc, "$.store.book[?(@.price < )]", JsonPathError.InvalidFilter)?;
    expectCompileError(alloc, "$.store.book[?()]", JsonPathError.InvalidFilter)?;

    // 5. MaxDepthExceeded (> 32 segments)
    const deep_expr = "$.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a.a";
    expectCompileError(alloc, deep_expr, JsonPathError.MaxDepthExceeded)?;

    // 6. Static convenience entry: JsonPath.evalJson
    const matches = JsonPath.evalJson(alloc, BOOKSTORE_JSON, "$.store.bicycle['color','price']")?;
    defer JsonPath.freeMatches(alloc, matches);
    expect_eq(matches.len, 2);
    expect_eq(strEq(matches[0].path, "$['store']['bicycle']['color']"), true);
    expect_eq(strEq(matches[0].value.asString() ?? "", "red"), true);
    expect_eq(strEq(matches[1].path, "$['store']['bicycle']['price']"), true);
    expect_eq(matches[1].value.asNumber() ?? 0.0, 399.0);
}

fn testCapJsonpathDynamicMount(): ! {
    const alloc = std.heap.page_allocator;
    const payload = "{\"tools\":[{\"name\":\"search\",\"enabled\":true},{\"name\":\"shell\",\"enabled\":false}]}";

    const matches = cap.jsonpath.JsonPath.evalJson(alloc, payload, "$.tools[?(@.enabled == true)].name")?;
    defer cap.jsonpath.JsonPath.freeMatches(alloc, matches);

    expect_eq(matches.len, 1);
    expect_eq(strEq(matches[0].path, "$['tools'][0]['name']"), true);
    expect_eq(strEq(matches[0].value.asString() ?? "", "search"), true);
}

fn main() {
    test("1. RFC 9535 Bookstore Queries, Recursive Descent, Negative Index, Union & Normalized Paths", () => {
        testRfc9535BookstoreQueries()?;
    });

    test("2. Array Slicing ([1:4], [::2], [::-1] Reverse) & Object Wildcard .*", () => {
        testArraySlicingAndWildcards()?;
    });

    test("3. Compound Filter Expressions (&&, ||, !, Existence, Root Comparison) & queryOne/exists", () => {
        testFilterExpressions()?;
    });

    test("4. Malformed JSONPath Syntax Rejection & JsonPath.evalJson Convenience Entry", () => {
        testMalformedSyntaxAndEvalJson()?;
    });

    test("5. Dynamic Lazy Namespace Mounting via `import cap from \"cap\"; cap.jsonpath`", () => {
        testCapJsonpathDynamicMount()?;
    });
}
