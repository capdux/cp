// Source: cp/cap/test/test_schema_production_deep.cp
// Deep Production Test Suite for cap:schema
// Covers:
//   1. Primitive types, numeric bounds, integer checks, string UTF-8 lengths & 5 StringFormats
//   2. Nested objects, required fields, additionalProperties: false & RFC 6901 JSON Pointer paths
//   3. Array recursion, minItems/maxItems/uniqueItems & Schema.enumStrings
//   4. Schema.fromJson parsing real OpenAI/MCP Tool Calling schemas + depth-64 guard
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.schema`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { JsonParser, JsonValue } from "cap:json";
import {
    SchemaType,
    StringFormat,
    PropertySchema,
    Schema,
    ValidationIssue,
    ValidationResult,
    SchemaValidator,
} from "cap:schema";
import cap from "cap";

fn hasIssue(res: ValidationResult, path: []const u8, code: []const u8): bool {
    for (iss of res.issues) {
        if (std.mem.eql(u8, iss.path, path) && std.mem.eql(u8, iss.code, code)) {
            return true;
        }
    }
    return false;
}

fn testPrimitivesAndFormats()! {
    const alloc = std.heap.page_allocator;

    // Integer vs Number & bounds
    const int_schema = Schema.integer().minVal(1.0).maxVal(100.0).multipleOf(5.0);
    let r1 = SchemaValidator.validateJson(alloc, int_schema, "25") catch unreachable;
    defer r1.deinit();
    expect_eq(r1.ok, true);

    let r2 = SchemaValidator.validateJson(alloc, int_schema, "25.5") catch unreachable;
    defer r2.deinit();
    expect_eq(r2.ok, false);
    expect_eq(hasIssue(r2, "", "type_mismatch"), true);

    let r3 = SchemaValidator.validateJson(alloc, int_schema, "102") catch unreachable;
    defer r3.deinit();
    expect_eq(r3.ok, false);
    expect_eq(hasIssue(r3, "", "maximum"), true);
    expect_eq(hasIssue(r3, "", "multiple_of"), true);

    // Nullable number with exclusive bounds
    const num_schema = Schema.number().exclusiveMin(0.0).exclusiveMax(1.0).withNullable(true);
    let r_null = SchemaValidator.validateJson(alloc, num_schema, "null") catch unreachable;
    defer r_null.deinit();
    expect_eq(r_null.ok, true);

    let r_zero = SchemaValidator.validateJson(alloc, num_schema, "0.0") catch unreachable;
    defer r_zero.deinit();
    expect_eq(r_zero.ok, false);
    expect_eq(hasIssue(r_zero, "", "exclusive_minimum"), true);

    // String UTF-8 codepoint lengths
    const str_schema = Schema.string().minLen(2).maxLen(4);
    // "你好" is 6 UTF-8 bytes, 2 codepoints -> passes minLen(2) & maxLen(4)
    let r_utf8 = SchemaValidator.validateJson(alloc, str_schema, "\"你好\"") catch unreachable;
    defer r_utf8.deinit();
    expect_eq(r_utf8.ok, true);

    let r_short = SchemaValidator.validateJson(alloc, str_schema, "\"a\"") catch unreachable;
    defer r_short.deinit();
    expect_eq(r_short.ok, false);
    expect_eq(hasIssue(r_short, "", "min_length"), true);

    // 5 StringFormats: Email, Uri, Uuid, Ipv4, DateTime
    let r_email_ok = SchemaValidator.validateJson(alloc, Schema.string().withFormat(.Email), "\"agent.01@cp-lang.org\"") catch unreachable;
    defer r_email_ok.deinit();
    expect_eq(r_email_ok.ok, true);

    let r_email_bad = SchemaValidator.validateJson(alloc, Schema.string().withFormat(.Email), "\"bad@@example\"") catch unreachable;
    defer r_email_bad.deinit();
    expect_eq(r_email_bad.ok, false);
    expect_eq(hasIssue(r_email_bad, "", "format"), true);

    let r_uri_ok = SchemaValidator.validateJson(alloc, Schema.string().withFormat(.Uri), "\"https://api.cp-lang.org/v1/tools?id=7\"") catch unreachable;
    defer r_uri_ok.deinit();
    expect_eq(r_uri_ok.ok, true);

    let r_uri_bad = SchemaValidator.validateJson(alloc, Schema.string().withFormat(.Uri), "\"not a valid uri\"") catch unreachable;
    defer r_uri_bad.deinit();
    expect_eq(r_uri_bad.ok, false);

    let r_uuid_ok = SchemaValidator.validateJson(alloc, Schema.string().withFormat(.Uuid), "\"550e8400-e29b-41d4-a716-446655440000\"") catch unreachable;
    defer r_uuid_ok.deinit();
    expect_eq(r_uuid_ok.ok, true);

    let r_uuid_bad = SchemaValidator.validateJson(alloc, Schema.string().withFormat(.Uuid), "\"550e8400-e29b-41d4-a716-zzzzzzzzzzzz\"") catch unreachable;
    defer r_uuid_bad.deinit();
    expect_eq(r_uuid_bad.ok, false);

    let r_ip_ok = SchemaValidator.validateJson(alloc, Schema.string().withFormat(.Ipv4), "\"192.168.1.42\"") catch unreachable;
    defer r_ip_ok.deinit();
    expect_eq(r_ip_ok.ok, true);

    let r_ip_bad = SchemaValidator.validateJson(alloc, Schema.string().withFormat(.Ipv4), "\"192.168.01.256\"") catch unreachable;
    defer r_ip_bad.deinit();
    expect_eq(r_ip_bad.ok, false);

    let r_dt_ok = SchemaValidator.validateJson(alloc, Schema.string().withFormat(.DateTime), "\"2026-11-20T12:30:45.123+08:00\"") catch unreachable;
    defer r_dt_ok.deinit();
    expect_eq(r_dt_ok.ok, true);

    let r_dt_bad = SchemaValidator.validateJson(alloc, Schema.string().withFormat(.DateTime), "\"2026-02-30T12:00:00Z\"") catch unreachable;
    defer r_dt_bad.deinit();
    expect_eq(r_dt_bad.ok, false);
}

fn testNestedObjectsAndPointers()! {
    const alloc = std.heap.page_allocator;

    const user_props: [3]PropertySchema = [
        PropertySchema.req("id", Schema.string().withFormat(.Uuid)),
        PropertySchema.req("email", Schema.string().withFormat(.Email)),
        PropertySchema.opt("a/b~c", Schema.integer().minVal(0.0)),
    ];
    const user_schema = Schema.object(&user_props).withAdditionalProperties(false);

    const root_props: [2]PropertySchema = [
        PropertySchema.req("user", user_schema),
        PropertySchema.opt("active", Schema.boolean()),
    ];
    const root_schema = Schema.object(&root_props).withAdditionalProperties(false);

    // Valid nested payload
    const valid_json = "{\"user\":{\"id\":\"550e8400-e29b-41d4-a716-446655440000\",\"email\":\"dev@cp-lang.org\",\"a/b~c\":7},\"active\":true}";
    let ok_res = SchemaValidator.validateJson(alloc, root_schema, valid_json) catch unreachable;
    defer ok_res.deinit();
    expect_eq(ok_res.ok, true);
    expect_eq(ok_res.count(), 0 as usize);

    // Invalid nested payload: missing required `/user/email`, bad `/user/a~1b~0c`, extra `/extra_field`, extra `/user/unknown`
    const bad_json = "{\"user\":{\"id\":\"550e8400-e29b-41d4-a716-446655440000\",\"a/b~c\":-5,\"unknown\":1},\"extra_field\":true}";
    let bad_res = SchemaValidator.validateJson(alloc, root_schema, bad_json) catch unreachable;
    defer bad_res.deinit();
    expect_eq(bad_res.ok, false);
    expect_eq(hasIssue(bad_res, "/user/email", "required"), true);
    expect_eq(hasIssue(bad_res, "/user/a~1b~0c", "minimum"), true);
    expect_eq(hasIssue(bad_res, "/user/unknown", "additional_property"), true);
    expect_eq(hasIssue(bad_res, "/extra_field", "additional_property"), true);
}

fn testArraysAndEnumStrings()! {
    const alloc = std.heap.page_allocator;

    const roles: [3][]const u8 = ["admin", "editor", "viewer"];
    const item_props: [2]PropertySchema = [
        PropertySchema.req("role", Schema.enumStrings(&roles)),
        PropertySchema.req("score", Schema.number().minVal(0.0).maxVal(100.0)),
    ];
    const item_schema = Schema.object(&item_props).withAdditionalProperties(false);
    const items_arr_schema = Schema.array(&item_schema).minItems(1).maxItems(3);

    const tag_item = Schema.string().minLen(2);
    const tags_schema = Schema.array(&tag_item).withUniqueItems(true);

    const payload_props: [2]PropertySchema = [
        PropertySchema.req("members", items_arr_schema),
        PropertySchema.req("tags", tags_schema),
    ];
    const payload_schema = Schema.object(&payload_props);

    const valid = "{\"members\":[{\"role\":\"admin\",\"score\":99.5}],\"tags\":[\"ai\",\"cp\"]}";
    let res_ok = SchemaValidator.validateJson(alloc, payload_schema, valid) catch unreachable;
    defer res_ok.deinit();
    expect_eq(res_ok.ok, true);

    const invalid = "{\"members\":[{\"role\":\"admin\",\"score\":80},{\"role\":\"guest\",\"score\":150}],\"tags\":[\"ai\",\"ai\"]}";
    let res_bad = SchemaValidator.validateJson(alloc, payload_schema, invalid) catch unreachable;
    defer res_bad.deinit();
    expect_eq(res_bad.ok, false);
    expect_eq(hasIssue(res_bad, "/members/1/role", "enum"), true);
    expect_eq(hasIssue(res_bad, "/members/1/score", "maximum"), true);
    expect_eq(hasIssue(res_bad, "/tags/1", "unique_items"), true);
}

fn testFromJsonToolCallingAndDepthGuard()! {
    const alloc = std.heap.page_allocator;

    // Realistic OpenAI / MCP Tool Calling Parameter Schema
    const tool_schema_json =
        "{" ++
        "\"type\":\"object\"," ++
        "\"required\":[\"query\",\"mode\",\"filters\"]," ++
        "\"additionalProperties\":false," ++
        "\"properties\":{" ++
        "  \"query\":{\"type\":\"string\",\"minLength\":1,\"maxLength\":200}," ++
        "  \"mode\":{\"type\":\"string\",\"enum\":[\"fast\",\"deep\",\"hybrid\"]}," ++
        "  \"callback_url\":{\"type\":[\"string\",\"null\"],\"format\":\"uri\"}," ++
        "  \"limit\":{\"type\":\"integer\",\"minimum\":1,\"maximum\":50}," ++
        "  \"filters\":{" ++
        "    \"type\":\"object\"," ++
        "    \"required\":[\"tags\"]," ++
        "    \"additionalProperties\":false," ++
        "    \"properties\":{" ++
        "      \"tags\":{\"type\":\"array\",\"minItems\":1,\"items\":{\"type\":\"string\",\"minLength\":2}}," ++
        "      \"since\":{\"type\":\"string\",\"format\":\"date-time\"}" ++
        "    }" ++
        "  }" ++
        "}" ++
        "}";

    let schema = Schema.fromJson(alloc, tool_schema_json) catch unreachable;
    defer schema.deinit();

    const good_call =
        "{" ++
        "\"query\":\"cp language schema\"," ++
        "\"mode\":\"deep\"," ++
        "\"callback_url\":null," ++
        "\"limit\":10," ++
        "\"filters\":{\"tags\":[\"systems\",\"ai\"],\"since\":\"2026-11-20T00:00:00Z\"}" ++
        "}";
    let ok_res = SchemaValidator.validateJson(alloc, schema, good_call) catch unreachable;
    defer ok_res.deinit();
    expect_eq(ok_res.ok, true);

    const bad_call =
        "{" ++
        "\"query\":\"\"," ++
        "\"mode\":\"invalid_mode\"," ++
        "\"callback_url\":\"bad uri\"," ++
        "\"limit\":99," ++
        "\"filters\":{\"tags\":[\"x\"],\"since\":\"not-a-date\"}," ++
        "\"injected\":true" ++
        "}";
    let bad_res = SchemaValidator.validateJson(alloc, schema, bad_call) catch unreachable;
    defer bad_res.deinit();
    expect_eq(bad_res.ok, false);
    expect_eq(hasIssue(bad_res, "/query", "min_length"), true);
    expect_eq(hasIssue(bad_res, "/mode", "enum"), true);
    expect_eq(hasIssue(bad_res, "/callback_url", "format"), true);
    expect_eq(hasIssue(bad_res, "/limit", "maximum"), true);
    expect_eq(hasIssue(bad_res, "/filters/tags/0", "min_length"), true);
    expect_eq(hasIssue(bad_res, "/filters/since", "format"), true);
    expect_eq(hasIssue(bad_res, "/injected", "additional_property"), true);

    // Depth-64 guard test using recursive array JSON value
    let tmp_arena = std.heap.ArenaAllocator.init(alloc);
    defer tmp_arena.deinit();
    const ta = tmp_arena.allocator();

    // Build a 66-deep nested schema & JsonValue
    let deep_schema = Schema.integer();
    let deep_val = JsonValue{ tag: .Number, num_val: 1.0 };
    let d: usize = 0;
    while (d < 66) : (d += 1) {
        let s_ptr = ta.create(Schema) catch unreachable;
        unsafe {
            s_ptr.* = deep_schema;
        }
        deep_schema = Schema.array(s_ptr);

        let elem = ta.alloc(JsonValue, 1) catch unreachable;
        elem[0] = deep_val;
        deep_val = JsonValue{ tag: .Array, arr_val: elem };
    }

    let depth_res = SchemaValidator.validate(alloc, deep_schema, deep_val) catch unreachable;
    defer depth_res.deinit();
    expect_eq(depth_res.ok, false);
    expect_eq(depth_res.count() > 0, true);
    expect_eq(std.mem.eql(u8, depth_res.first().?.code, "max_depth"), true);
}

fn testCapSchemaLazyMount()! {
    const alloc = std.heap.page_allocator;

    const raw_schema = "{\"type\":\"object\",\"required\":[\"request_id\",\"ip\"],\"additionalProperties\":false,\"properties\":{\"request_id\":{\"type\":\"string\",\"format\":\"uuid\"},\"ip\":{\"type\":\"string\",\"format\":\"ipv4\"}}}";
    let s = cap.schema.Schema.fromJson(alloc, raw_schema) catch unreachable;
    defer s.deinit();

    let ok_res = cap.schema.SchemaValidator.validateJson(
        alloc,
        s,
        "{\"request_id\":\"123e4567-e89b-12d3-a456-426614174000\",\"ip\":\"10.0.0.1\"}",
    ) catch unreachable;
    defer ok_res.deinit();
    expect_eq(ok_res.ok, true);

    let bad_res = cap.schema.SchemaValidator.validateJson(
        alloc,
        s,
        "{\"request_id\":\"bad\",\"ip\":\"999.0.0.1\"}",
    ) catch unreachable;
    defer bad_res.deinit();
    expect_eq(bad_res.ok, false);
    expect_eq(bad_res.count(), 2 as usize);
}

fn main() {
    test("1. Primitive Types, Numeric Bounds, UTF-8 Lengths & 5 Built-in StringFormats", () => {
        testPrimitivesAndFormats()?;
    });

    test("2. Nested Objects, Required Fields, additionalProperties: false & RFC 6901 Pointers", () => {
        testNestedObjectsAndPointers()?;
    });

    test("3. Array Element Recursion, uniqueItems & Schema.enumStrings", () => {
        testArraysAndEnumStrings()?;
    });

    test("4. Schema.fromJson OpenAI/MCP Tool Calling Schema & Depth-64 Guard", () => {
        testFromJsonToolCallingAndDepthGuard()?;
    });

    test("5. Dynamic Lazy Namespace Mounting via `import cap from \"cap\"; cap.schema`", () => {
        testCapSchemaLazyMount()?;
    });
}
