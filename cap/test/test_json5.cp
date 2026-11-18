// cap/test/test_json5.cp
import { test, expect_eq } from "cap:test";
import { JSON5, JSON5Parser, Json5Error } from "../../lib/conf/json5";
import { JsonValue, JsonTag } from "../../lib/json/value";
const std = @import("std");

fn test_comments_and_unquoted_keys(): !void {
    const src = 
        "{\n" +
        "  // This is a single line comment\n" +
        "  foo: 'bar',\n" +
        "  /* This is a \n" +
        "     multi line comment */\n" +
        "  $count: 123,\n" +
        "  _enabled: true,\n" +
        "  trailing: null,\n" +
        "}";

    let val = try JSON5.parse(std.heap.page_allocator, src);
    expect_eq(val.tag == JsonTag.Object, true);
    expect_eq(val.len(), 4);

    let foo_opt = val.get("foo");
    expect_eq(foo_opt != null, true);
    if (foo_opt != null) {
        let foo = foo_opt.?;
        if (s := foo.asString()) {
            expect_eq(std.mem.eql(u8, s, "bar"), true);
        }
    }

    let count_opt = val.get("$count");
    expect_eq(count_opt != null, true);
    if (count_opt != null) {
        let count = count_opt.?;
        if (n := count.asNumber()) {
            expect_eq(n == 123.0, true);
        }
    }

    let enabled_opt = val.get("_enabled");
    expect_eq(enabled_opt != null, true);
    if (enabled_opt != null) {
        let enabled = enabled_opt.?;
        if (b := enabled.asBool()) {
            expect_eq(b, true);
        }
    }

    let trailing_opt = val.get("trailing");
    expect_eq(trailing_opt != null, true);
    if (trailing_opt != null) {
        let trailing = trailing_opt.?;
        expect_eq(trailing.isNull(), true);
    }
}

fn test_numbers_and_trailing_commas(): !void {
    const src = "{\n  hex: 0x2A,\n  pos: +100,\n  neg_hex: -0x10,\n  arr: [1, 2, 3, ],\n}";
    let val = try JSON5.parse(std.heap.page_allocator, src);
    expect_eq(val.tag == JsonTag.Object, true);

    let hex_opt = val.get("hex");
    expect_eq(hex_opt != null, true);
    if (hex_opt != null) {
        let hex = hex_opt.?;
        if (n := hex.asNumber()) {
            expect_eq(n == 42.0, true);
        }
    }

    let pos_opt = val.get("pos");
    expect_eq(pos_opt != null, true);
    if (pos_opt != null) {
        let pos = pos_opt.?;
        if (n := pos.asNumber()) {
            expect_eq(n == 100.0, true);
        }
    }

    let neg_hex_opt = val.get("neg_hex");
    expect_eq(neg_hex_opt != null, true);
    if (neg_hex_opt != null) {
        let neg_hex = neg_hex_opt.?;
        if (n := neg_hex.asNumber()) {
            expect_eq(n == -16.0, true);
        }
    }

    let arr_opt = val.get("arr");
    expect_eq(arr_opt != null, true);
    if (arr_opt != null) {
        let arr = arr_opt.?;
        expect_eq(arr.len(), 3);
        let item0_opt = arr.getIndex(0);
        if (item0_opt != null) {
            let item0 = item0_opt.?;
            if (n := item0.asNumber()) {
                expect_eq(n == 1.0, true);
            }
        }
        let item2_opt = arr.getIndex(2);
        if (item2_opt != null) {
            let item2 = item2_opt.?;
            if (n := item2.asNumber()) {
                expect_eq(n == 3.0, true);
            }
        }
    }
}

fn test_backward_compat_strip(): !void {
    let buf: [128]u8 = undefined;
    const cleaned = JSON5.stripCommentsAndTrailingCommas("{\n  // comment\n  \"a\": 1,\n}", buf[0..128]);
    expect_eq(cleaned.len > 0, true);
    expect_eq(cleaned[cleaned.len - 1] == '}', true);
}

fn main() {
    test("JSON5: comments and unquoted keys parsing", () => {
        try test_comments_and_unquoted_keys();
    });

    test("JSON5: numbers (hex, positive signs, special values)", () => {
        try test_numbers_and_trailing_commas();
    });

    test("JSON5: stripCommentsAndTrailingCommas backward compatibility", () => {
        try test_backward_compat_strip();
    });
}
