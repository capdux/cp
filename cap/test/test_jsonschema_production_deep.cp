// Source: cp/cap/test/test_jsonschema_production_deep.cp
// Deep Production Test Suite for cap:jsonschema (JSON Schema Draft-07/2020-12 Validator & Path Diagnostics)
// Covers:
//   1. Nested object properties, required, additionalProperties: false/subschema, min/maxProperties & exact $.user.email path diagnostics
//   2. Numeric boundaries (integer vs number, minimum/maximum/exclusiveMinimum/exclusiveMaximum/multipleOf), UTF-8 codepoint string lengths & array items/uniqueItems
//   3. enum, const, allOf, anyOf, oneOf, not combinators
//   4. Local #/$defs/... & #/definitions/... $ref resolution, circular $ref MaxDepthExceeded guard, InvalidJson/InvalidSchema/InvalidReference
//   5. Dynamic lazy namespace mounting (`import cap from "cap"; cap.jsonschema`), validateValue & `std.testing.allocator` zero leaks

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { JsonParser } from "cap:json";
import {
    JSONSCHEMA_MAX_DEPTH,
    JsonSchemaError,
    ValidationIssue,
    ValidationResult,
    SchemaValidator,
} from "cap:conf";
import cap from "cap";

test("1. Nested Object properties, required, additionalProperties & Path Diagnostics") {
    const alloc = std.testing.allocator;
    const schema_src =
        \\{
        \\  "type": "object",
        \\  "required": ["user"],
        \\  "additionalProperties": false,
        \\  "properties": {
        \\    "user": {
        \\      "type": "object",
        \\      "required": ["name", "email"],
        \\      "additionalProperties": false,
        \\      "minProperties": 2,
        \\      "maxProperties": 4,
        \\      "properties": {
        \\        "name": { "type": "string", "minLength": 1 },
        \\        "email": { "type": "string", "minLength": 3 },
        \\        "age": { "type": "integer", "minimum": 18 }
        \\      }
        \\    }
        \\  }
        \\}
    ;

    let validator = SchemaValidator.compile(alloc, schema_src) catch unreachable;
    defer validator.deinit();

    // Valid nested object
    let ok_res = validator.validate(alloc, "{\"user\":{\"name\":\"Alice\",\"email\":\"a@b.c\",\"age\":25}}") catch unreachable;
    defer ok_res.deinit(alloc);
    expect_eq(ok_res.valid, true);
    expect_eq(ok_res.issues.len, 0);

    // Missing required $.user.email, minimum violation at $.user.age, additionalProperties at $.user.role
    let bad_res = validator.validate(alloc, "{\"user\":{\"name\":\"Bob\",\"age\":16,\"role\":\"admin\"}}") catch unreachable;
    defer bad_res.deinit(alloc);
    expect_eq(bad_res.valid, false);
    expect_eq(bad_res.issues.len, 3);

    let saw_email_req = false;
    let saw_age_min = false;
    let saw_role_add = false;
    for (issue of bad_res.issues) {
        if (std.mem.eql(u8, issue.path, "$.user.email") && std.mem.eql(u8, issue.keyword, "required")) {
            saw_email_req = true;
        }
        if (std.mem.eql(u8, issue.path, "$.user.age") && std.mem.eql(u8, issue.keyword, "minimum")) {
            saw_age_min = true;
        }
        if (std.mem.eql(u8, issue.path, "$.user.role") && std.mem.eql(u8, issue.keyword, "additionalProperties")) {
            saw_role_add = true;
        }
    }
    expect_eq(saw_email_req, true);
    expect_eq(saw_age_min, true);
    expect_eq(saw_role_add, true);
}

test("2. Numeric Boundaries, integer vs number, UTF-8 String Length & Array Constraints") {
    const alloc = std.testing.allocator;
    const schema_src =
        \\{
        \\  "type": "object",
        \\  "properties": {
        \\    "count": { "type": "integer", "minimum": 0, "exclusiveMaximum": 10, "multipleOf": 2 },
        \\    "ratio": { "type": ["number", "null"], "exclusiveMinimum": 0.0, "maximum": 1.0 },
        \\    "label": { "type": "string", "minLength": 2, "maxLength": 4 },
        \\    "tags": {
        \\      "type": "array",
        \\      "minItems": 1,
        \\      "maxItems": 3,
        \\      "uniqueItems": true,
        \\      "items": { "type": "string", "minLength": 1 }
        \\    }
        \\  }
        \\}
    ;

    let validator = SchemaValidator.compile(alloc, schema_src) catch unreachable;
    defer validator.deinit();

    // Valid: 4 is integer, ratio is null, "你好" is 2 UTF-8 codepoints, unique tags
    let ok_res = validator.validate(alloc, "{\"count\":4,\"ratio\":null,\"label\":\"你好\",\"tags\":[\"a\",\"b\"]}") catch unreachable;
    defer ok_res.deinit(alloc);
    expect_eq(ok_res.valid, true);

    // Invalid: count 3.5 fails integer & multipleOf, ratio 0.0 fails exclusiveMinimum,
    // label "你好世界!" (5 codepoints) fails maxLength, tags ["x", "x"] fails uniqueItems at $.tags[1]
    let bad_res = validator.validate(alloc, "{\"count\":3.5,\"ratio\":0.0,\"label\":\"你好世界!\",\"tags\":[\"x\",\"x\"]}") catch unreachable;
    defer bad_res.deinit(alloc);
    expect_eq(bad_res.valid, false);

    let saw_count_type = false;
    let saw_count_mult = false;
    let saw_ratio_emin = false;
    let saw_label_max = false;
    let saw_tags_uniq = false;
    for (issue of bad_res.issues) {
        if (std.mem.eql(u8, issue.path, "$.count") && std.mem.eql(u8, issue.keyword, "type")) saw_count_type = true;
        if (std.mem.eql(u8, issue.path, "$.count") && std.mem.eql(u8, issue.keyword, "multipleOf")) saw_count_mult = true;
        if (std.mem.eql(u8, issue.path, "$.ratio") && std.mem.eql(u8, issue.keyword, "exclusiveMinimum")) saw_ratio_emin = true;
        if (std.mem.eql(u8, issue.path, "$.label") && std.mem.eql(u8, issue.keyword, "maxLength")) saw_label_max = true;
        if (std.mem.eql(u8, issue.path, "$.tags[1]") && std.mem.eql(u8, issue.keyword, "uniqueItems")) saw_tags_uniq = true;
    }
    expect_eq(saw_count_type, true);
    expect_eq(saw_count_mult, true);
    expect_eq(saw_ratio_emin, true);
    expect_eq(saw_label_max, true);
    expect_eq(saw_tags_uniq, true);
}

test("3. enum, const, allOf, anyOf, oneOf & not Combinators") {
    const alloc = std.testing.allocator;
    const schema_src =
        \\{
        \\  "type": "object",
        \\  "properties": {
        \\    "version": { "const": 2 },
        \\    "env": { "enum": ["dev", "staging", "prod"] },
        \\    "score": {
        \\      "allOf": [
        \\        { "type": "integer" },
        \\        { "minimum": 0 },
        \\        { "maximum": 100 }
        \\      ]
        \\    },
        \\    "id": {
        \\      "anyOf": [
        \\        { "type": "string", "minLength": 1 },
        \\        { "type": "integer", "minimum": 1 }
        \\      ]
        \\    },
        \\    "exclusive": {
        \\      "oneOf": [
        \\        { "type": "integer", "multipleOf": 3 },
        \\        { "type": "integer", "multipleOf": 5 }
        \\      ]
        \\    },
        \\    "restricted": {
        \\      "not": { "const": "forbidden" }
        \\    }
        \\  }
        \\}
    ;

    let validator = SchemaValidator.compile(alloc, schema_src) catch unreachable;
    defer validator.deinit();

    // Valid: exclusive=9 matches only multipleOf 3
    let ok_res = validator.validate(alloc, "{\"version\":2,\"env\":\"prod\",\"score\":85,\"id\":\"abc\",\"exclusive\":9,\"restricted\":\"ok\"}") catch unreachable;
    defer ok_res.deinit(alloc);
    expect_eq(ok_res.valid, true);

    // Invalid: version=1 (const), env="test" (enum), score=150 (allOf->maximum), id=false (anyOf),
    // exclusive=15 (matches both 3 and 5 -> oneOf fails), restricted="forbidden" (not)
    let bad_res = validator.validate(alloc, "{\"version\":1,\"env\":\"test\",\"score\":150,\"id\":false,\"exclusive\":15,\"restricted\":\"forbidden\"}") catch unreachable;
    defer bad_res.deinit(alloc);
    expect_eq(bad_res.valid, false);
    expect_eq(bad_res.issues.len, 6);
}

test("4. Local $ref Resolution, Circular $ref MaxDepthExceeded & Error Guards") {
    const alloc = std.testing.allocator;
    const ref_schema =
        \\{
        \\  "$defs": {
        \\    "Address": {
        \\      "type": "object",
        \\      "required": ["city"],
        \\      "properties": {
        \\        "city": { "type": "string", "minLength": 1 }
        \\      }
        \\    }
        \\  },
        \\  "type": "object",
        \\  "required": ["home"],
        \\  "properties": {
        \\    "home": { "$ref": "#/$defs/Address" }
        \\  }
        \\}
    ;

    let v_ref = SchemaValidator.compile(alloc, ref_schema) catch unreachable;
    defer v_ref.deinit();

    let ok_ref = v_ref.validate(alloc, "{\"home\":{\"city\":\"Sydney\"}}") catch unreachable;
    defer ok_ref.deinit(alloc);
    expect_eq(ok_ref.valid, true);

    let bad_ref = v_ref.validate(alloc, "{\"home\":{}}") catch unreachable;
    defer bad_ref.deinit(alloc);
    expect_eq(bad_ref.valid, false);
    expect_eq(bad_ref.issues.len, 1);
    expect_eq(std.mem.eql(u8, bad_ref.issues[0].path, "$.home.city"), true);
    expect_eq(std.mem.eql(u8, bad_ref.issues[0].keyword, "required"), true);

    // Circular $ref guard -> MaxDepthExceeded
    const circ_schema =
        \\{
        \\  "$defs": {
        \\    "Loop": { "$ref": "#/$defs/Loop" }
        \\  },
        \\  "$ref": "#/$defs/Loop"
        \\}
    ;
    let v_circ = SchemaValidator.compile(alloc, circ_schema) catch unreachable;
    defer v_circ.deinit();
    let hit_max_depth = false;
    * = v_circ.validate(alloc, "123") catch |e| {
        if (e == JsonSchemaError.MaxDepthExceeded) {
            hit_max_depth = true;
        }
    };
    expect_eq(hit_max_depth, true);

    // InvalidReference guard
    const missing_ref_schema = "{\"$ref\":\"#/$defs/DoesNotExist\"}";
    let v_missing = SchemaValidator.compile(alloc, missing_ref_schema) catch unreachable;
    defer v_missing.deinit();
    let hit_inv_ref = false;
    * = v_missing.validate(alloc, "{}") catch |e| {
        if (e == JsonSchemaError.InvalidReference) {
            hit_inv_ref = true;
        }
    };
    expect_eq(hit_inv_ref, true);

    // InvalidSchema & InvalidJson guards
    let hit_inv_schema = false;
    * = SchemaValidator.compile(alloc, "{\"type\":\"not_a_type\"}") catch |e| {
        if (e == JsonSchemaError.InvalidSchema) {
            hit_inv_schema = true;
        }
    };
    expect_eq(hit_inv_schema, true);

    let hit_inv_json = false;
    * = v_ref.validate(alloc, "{bad json") catch |e| {
        if (e == JsonSchemaError.InvalidJson) {
            hit_inv_json = true;
        }
    };
    expect_eq(hit_inv_json, true);
}

test("5. Dynamic cap.jsonschema Mounting, validateValue & Zero Memory Leaks") {
    const alloc = std.testing.allocator;
    expect_eq(JSONSCHEMA_MAX_DEPTH, 64);

    const schema_src = "{\"type\":\"object\",\"required\":[\"ok\"],\"properties\":{\"ok\":{\"type\":\"boolean\"}}}";
    let validator = cap.conf.SchemaValidator.compile(alloc, schema_src) catch unreachable;
    defer validator.deinit();

    let inst_arena = std.heap.ArenaAllocator.init(alloc);
    defer inst_arena.deinit();
    const val = JsonParser.parse(inst_arena.allocator(), "{\"ok\":true}") catch unreachable;

    let res = validator.validateValue(alloc, val) catch unreachable;
    defer res.deinit(alloc);
    expect_eq(res.valid, true);
    expect_eq(res.issues.len, 0);
}
