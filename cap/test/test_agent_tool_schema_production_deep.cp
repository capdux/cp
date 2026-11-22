// Source: cp/cap/test/test_agent_tool_schema_production_deep.cp (Production deep tests for cap:agent tool_schema)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    FieldType,
    FieldSpec,
    SchemaErrorKind,
    ValidationReport,
    validateToolArgs,
    formatValidationHint,
} from "cap:agent";

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("1. Valid complex multi-type JSON payload with nested containers, escapes, negatives, and scientific notation", () => {
    const modes: [2]string = ["fast", "deep"];
    const empty_enum: [0]string = [];
    const specs: [7]FieldSpec = [
        FieldSpec { name: "path", kind: FieldType.String, required: true, has_min_num: false, min_num: 0.0, has_max_num: false, max_num: 0.0, min_len: 1, max_len: 64, enum_values: empty_enum[0..] },
        FieldSpec { name: "mode", kind: FieldType.String, required: true, has_min_num: false, min_num: 0.0, has_max_num: false, max_num: 0.0, min_len: 0, max_len: 0, enum_values: modes[0..] },
        FieldSpec { name: "offset", kind: FieldType.Integer, required: true, has_min_num: true, min_num: -100.0, has_max_num: true, max_num: 100.0, min_len: 0, max_len: 0, enum_values: empty_enum[0..] },
        FieldSpec { name: "temp", kind: FieldType.Number, required: false, has_min_num: true, min_num: 0.0, has_max_num: true, max_num: 2.0, min_len: 0, max_len: 0, enum_values: empty_enum[0..] },
        FieldSpec { name: "dry_run", kind: FieldType.Boolean, required: false, has_min_num: false, min_num: 0.0, has_max_num: false, max_num: 0.0, min_len: 0, max_len: 0, enum_values: empty_enum[0..] },
        FieldSpec { name: "tags", kind: FieldType.Array, required: false, has_min_num: false, min_num: 0.0, has_max_num: false, max_num: 0.0, min_len: 0, max_len: 0, enum_values: empty_enum[0..] },
        FieldSpec { name: "meta", kind: FieldType.Object, required: false, has_min_num: false, min_num: 0.0, has_max_num: false, max_num: 0.0, min_len: 0, max_len: 0, enum_values: empty_enum[0..] },
    ];

    const payload: string =
        "{\"path\":\"a/\\\"b\\\"\\nc\",\"mode\":\"deep\",\"offset\":-42,\"temp\":1.5e-1," ++
        "\"dry_run\":true,\"tags\":[\"x\",1,{\"k\":\"}\"}],\"meta\":{\"nested\":[1,2]},\"extra\":null}";

    const rep = validateToolArgs(payload, specs[0..], true);
    expect_eq(rep.ok, true);
    expect_eq(rep.error_kind == SchemaErrorKind.Ok, true);
    expect_eq(rep.checked_fields, 7);
    expect_eq(rep.unknown_fields, 1);

    let hint_buf: [128]u8 = std.mem.zeroes([128]u8);
    let hint_s: []u8 = hint_buf[0..];
    const hlen = formatValidationHint(rep, &hint_s) ?? unreachable;
    expect_eq(strEq(hint_s[0..hlen], "Tool arguments valid"), true);
});

test("2. Missing required field reports MissingRequiredField and formats LLM repair hint", () => {
    const empty_enum: [0]string = [];
    const specs: [2]FieldSpec = [
        FieldSpec { name: "path", kind: FieldType.String, required: true, has_min_num: false, min_num: 0.0, has_max_num: false, max_num: 0.0, min_len: 1, max_len: 100, enum_values: empty_enum[0..] },
        FieldSpec { name: "limit", kind: FieldType.Integer, required: false, has_min_num: false, min_num: 0.0, has_max_num: false, max_num: 0.0, min_len: 0, max_len: 0, enum_values: empty_enum[0..] },
    ];

    const rep = validateToolArgs("{\"limit\": 10}", specs[0..], false);
    expect_eq(rep.ok, false);
    expect_eq(rep.error_kind == SchemaErrorKind.MissingRequiredField, true);
    expect_eq(strEq(rep.field_name, "path"), true);

    let hint_buf: [128]u8 = std.mem.zeroes([128]u8);
    let hint_s: []u8 = hint_buf[0..];
    const n = formatValidationHint(rep, &hint_s) ?? unreachable;
    expect_eq(strEq(hint_s[0..n], "Tool argument validation failed: missing required field 'path'"), true);

    // Undersized hint buffer returns null
    let tiny: [10]u8 = std.mem.zeroes([10]u8);
    let tiny_s: []u8 = tiny[0..];
    expect_eq(formatValidationHint(rep, &tiny_s) == null, true);
});

test("3. TypeMismatch (Integer vs float/string), NumberOutOfRange, LengthOutOfRange, and EnumMismatch", () => {
    const colors: [2]string = ["red", "blue"];
    const empty_enum: [0]string = [];
    const specs: [3]FieldSpec = [
        FieldSpec { name: "count", kind: FieldType.Integer, required: false, has_min_num: true, min_num: 1.0, has_max_num: true, max_num: 10.0, min_len: 0, max_len: 0, enum_values: empty_enum[0..] },
        FieldSpec { name: "title", kind: FieldType.String, required: false, has_min_num: false, min_num: 0.0, has_max_num: false, max_num: 0.0, min_len: 2, max_len: 3, enum_values: empty_enum[0..] },
        FieldSpec { name: "color", kind: FieldType.String, required: false, has_min_num: false, min_num: 0.0, has_max_num: false, max_num: 0.0, min_len: 0, max_len: 0, enum_values: colors[0..] },
    ];

    // Integer receiving float or string -> TypeMismatch
    const r_flt = validateToolArgs("{\"count\": 3.14}", specs[0..], false);
    expect_eq(r_flt.error_kind == SchemaErrorKind.TypeMismatch, true);
    expect_eq(strEq(r_flt.field_name, "count"), true);

    const r_str = validateToolArgs("{\"count\": \"5\"}", specs[0..], false);
    expect_eq(r_str.error_kind == SchemaErrorKind.TypeMismatch, true);

    // NumberOutOfRange
    const r_low = validateToolArgs("{\"count\": 0}", specs[0..], false);
    expect_eq(r_low.error_kind == SchemaErrorKind.NumberOutOfRange, true);
    const r_high = validateToolArgs("{\"count\": 11}", specs[0..], false);
    expect_eq(r_high.error_kind == SchemaErrorKind.NumberOutOfRange, true);

    // Decoded string length: "a\nb" decodes to 3 bytes (fits max_len=3), "abcd" is 4 bytes (fails)
    expect_eq(validateToolArgs("{\"title\": \"a\\nb\"}", specs[0..], false).ok, true);
    const r_short = validateToolArgs("{\"title\": \"x\"}", specs[0..], false);
    expect_eq(r_short.error_kind == SchemaErrorKind.LengthOutOfRange, true);
    const r_long = validateToolArgs("{\"title\": \"abcd\"}", specs[0..], false);
    expect_eq(r_long.error_kind == SchemaErrorKind.LengthOutOfRange, true);

    // EnumMismatch
    const r_enum = validateToolArgs("{\"color\": \"green\"}", specs[0..], false);
    expect_eq(r_enum.error_kind == SchemaErrorKind.EnumMismatch, true);
    expect_eq(strEq(r_enum.field_name, "color"), true);
});

test("4. UnknownField rejected when allow_unknown_fields is false and DuplicateField always rejected", () => {
    const empty_enum: [0]string = [];
    const specs: [1]FieldSpec = [
        FieldSpec { name: "id", kind: FieldType.Integer, required: true, has_min_num: false, min_num: 0.0, has_max_num: false, max_num: 0.0, min_len: 0, max_len: 0, enum_values: empty_enum[0..] },
    ];

    const r_unk = validateToolArgs("{\"id\": 1, \"unexpected\": true}", specs[0..], false);
    expect_eq(r_unk.ok, false);
    expect_eq(r_unk.error_kind == SchemaErrorKind.UnknownField, true);
    expect_eq(strEq(r_unk.field_name, "unexpected"), true);

    const r_dup = validateToolArgs("{\"id\": 1, \"id\": 2}", specs[0..], true);
    expect_eq(r_dup.ok, false);
    expect_eq(r_dup.error_kind == SchemaErrorKind.DuplicateField, true);
    expect_eq(strEq(r_dup.field_name, "id"), true);
});

test("5. Malformed JSON, non-object payloads, trailing commas/garbage, and empty schema boundaries", () => {
    const empty_specs: [0]FieldSpec = [];

    // Empty object with empty schema -> valid
    expect_eq(validateToolArgs("  { } ", empty_specs[0..], false).ok, true);

    // Non-object / malformed payloads -> InvalidJsonObject
    expect_eq(validateToolArgs("", empty_specs[0..], true).error_kind == SchemaErrorKind.InvalidJsonObject, true);
    expect_eq(validateToolArgs("[1, 2, 3]", empty_specs[0..], true).error_kind == SchemaErrorKind.InvalidJsonObject, true);
    expect_eq(validateToolArgs("{\"a\": 1", empty_specs[0..], true).error_kind == SchemaErrorKind.InvalidJsonObject, true);
    expect_eq(validateToolArgs("{\"a\": 1,}", empty_specs[0..], true).error_kind == SchemaErrorKind.InvalidJsonObject, true);
    expect_eq(validateToolArgs("{\"a\": 1} trailing", empty_specs[0..], true).error_kind == SchemaErrorKind.InvalidJsonObject, true);
    expect_eq(validateToolArgs("{\"a\": [1, 2}", empty_specs[0..], true).error_kind == SchemaErrorKind.InvalidJsonObject, true);
});
