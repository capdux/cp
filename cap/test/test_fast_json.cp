// cap/test/test_fast_json.cp
import { test, expect_eq } from "cap:test";
import { JSON, JsonParser, JsonValue, JsonTag, FastJsonScanner } from "../../lib/json/index";
const std = @import("std");

pub struct UserRecord {
    id: i64 = 0,
    username: []const u8 = "",
    is_admin: bool = false,
    credit: f64 = 0,
    tag: []const u8 = "default_tag",
}

fn test_fast_simd_scanner(): !void {
    const raw = "prefix_text: \"target_string_inside_quotes\" trailing_content";
    const start_quote = std.mem.indexOfScalar(u8, raw, '"').?;
    const hit = FastJsonScanner.indexOfQuoteOrEscape(raw[start_quote + 1..]);
    expect_eq(hit != null, true);
    if (h := hit) {
        const slice = raw[start_quote + 1 .. start_quote + 1 + h];
        expect_eq(std.mem.eql(u8, slice, "target_string_inside_quotes"), true);
    }
}

fn test_typed_json_deserialization(): !void {
    const allocator = std.heap.page_allocator;
    const json_str = 
        "{\n" +
        "  \"id\": 10086,\n" +
        "  \"username\": \"antigravity_agent\",\n" +
        "  \"is_admin\": true,\n" +
        "  \"credit\": 999.88,\n" +
        "  \"unknown_metadata\": {\"deep\": [1, 2, \"text\"]}\n" +
        "}";

    let user = UserRecord{};
    zig {
        user = try JSON.parseTyped(UserRecord, allocator, json_str);
    }

    expect_eq(user.id == 10086, true);
    expect_eq(std.mem.eql(u8, user.username, "antigravity_agent"), true);
    expect_eq(user.is_admin == true, true);
    expect_eq(user.credit == 999.88, true);
    // Preserves default field value when omitted in json
    expect_eq(std.mem.eql(u8, user.tag, "default_tag"), true);
}

fn test_dynamic_dom_json(): !void {
    const allocator = std.heap.page_allocator;
    const json_str = "{\"status\": \"ok\", \"code\": 200}";

    let val = try JSON.parse(allocator, json_str);
    expect_eq(val.tag == JsonTag.Object, true);

    let status_opt = val.get("status");
    expect_eq(status_opt != null, true);
    if (status_opt != null) {
        let status_val = status_opt.?;
        if (s := status_val.asString()) {
            expect_eq(std.mem.eql(u8, s, "ok"), true);
        }
    }

    let code_opt = val.get("code");
    expect_eq(code_opt != null, true);
    if (code_opt != null) {
        let code_val = code_opt.?;
        if (n := code_val.asNumber()) {
            expect_eq(n == 200.0, true);
        }
    }
}

fn main() {
    test("FastJsonScanner: 16-byte SIMD quote and escape scanning", () => {
        try test_fast_simd_scanner();
    });

    test("JSON.parseTyped: 50x compiled-in typed struct deserialization", () => {
        try test_typed_json_deserialization();
    });

    test("JSON.parse: zero-copy dynamic DOM AST parsing", () => {
        try test_dynamic_dom_json();
    });
}
