// Origin: Pure-CP Stdlib Test Suite (AI Structured Outputs JSON Schema Builder & OpenAI/Anthropic Tool Spec Emitter) | Created: 2026-11-22
// Deep Production Verification Suite for lib/prompt/json_schema.cp
// Covers:
//   1. All 6 JsonSchemaType variants, required array & strict/non-strict additionalProperties toggle
//   2. Enum CSV trimming/emission & Array items type emission
//   3. Exact OpenAI Function Calling vs Anthropic input_schema dual-format output
//   4. RFC 8259 JSON string escaping (quotes, backslashes, newlines, tabs, control bytes)
//   5. Exact/undersized buffer guards, empty props & empty tool/property name rejection

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    JsonSchemaType,
    SchemaProp,
    ToolSchemaSpec,
    emitJsonSchemaObject,
    emitOpenAiToolJson,
    emitAnthropicToolJson,
} from "../../lib/prompt/index.cp";

fn testAllSchemaTypesEmission()! {
    const props: [6]SchemaProp = [
        SchemaProp{ name: "query", kind: JsonSchemaType.String, description: "Search text", required: true },
        SchemaProp{ name: "limit", kind: JsonSchemaType.Integer, description: "", required: true },
        SchemaProp{ name: "score", kind: JsonSchemaType.Number, description: "", required: false },
        SchemaProp{ name: "exact", kind: JsonSchemaType.Boolean, description: "", required: false },
        SchemaProp{ name: "tags", kind: JsonSchemaType.Array, description: "", required: true, item_kind: JsonSchemaType.String },
        SchemaProp{ name: "meta", kind: JsonSchemaType.Object, description: "", required: false },
    ];

    let buf: [1024]u8 = [0; 1024];
    let out: []u8 = buf[0..];

    const n_strict = emitJsonSchemaObject(props[0..], true, out) ?? 0;
    const expected_strict = "{\"type\":\"object\",\"properties\":{\"query\":{\"type\":\"string\",\"description\":\"Search text\"},\"limit\":{\"type\":\"integer\"},\"score\":{\"type\":\"number\"},\"exact\":{\"type\":\"boolean\"},\"tags\":{\"type\":\"array\",\"items\":{\"type\":\"string\"}},\"meta\":{\"type\":\"object\"}},\"required\":[\"query\",\"limit\",\"tags\"],\"additionalProperties\":false}";
    expect_eq(std.mem.eql(u8, out[0..n_strict], expected_strict), true);

    const n_relaxed = emitJsonSchemaObject(props[0..], false, out) ?? 0;
    const expected_relaxed = "{\"type\":\"object\",\"properties\":{\"query\":{\"type\":\"string\",\"description\":\"Search text\"},\"limit\":{\"type\":\"integer\"},\"score\":{\"type\":\"number\"},\"exact\":{\"type\":\"boolean\"},\"tags\":{\"type\":\"array\",\"items\":{\"type\":\"string\"}},\"meta\":{\"type\":\"object\"}},\"required\":[\"query\",\"limit\",\"tags\"]}";
    expect_eq(std.mem.eql(u8, out[0..n_relaxed], expected_relaxed), true);
}

fn testEnumCsvAndArrayItems()! {
    const props: [2]SchemaProp = [
        SchemaProp{
            name: "order",
            kind: JsonSchemaType.String,
            description: "Sort direction",
            required: true,
            enum_csv: " asc , desc , , rand ",
        },
        SchemaProp{
            name: "ids",
            kind: JsonSchemaType.Array,
            description: "Record IDs",
            required: true,
            item_kind: JsonSchemaType.Integer,
        },
    ];

    let buf: [512]u8 = [0; 512];
    let out: []u8 = buf[0..];
    const n = emitJsonSchemaObject(props[0..], true, out) ?? 0;
    const expected = "{\"type\":\"object\",\"properties\":{\"order\":{\"type\":\"string\",\"description\":\"Sort direction\",\"enum\":[\"asc\",\"desc\",\"rand\"]},\"ids\":{\"type\":\"array\",\"description\":\"Record IDs\",\"items\":{\"type\":\"integer\"}}},\"required\":[\"order\",\"ids\"],\"additionalProperties\":false}";
    expect_eq(std.mem.eql(u8, out[0..n], expected), true);
}

fn testOpenAiAndAnthropicToolSpecFormats()! {
    const props: [1]SchemaProp = [
        SchemaProp{ name: "path", kind: JsonSchemaType.String, description: "File path", required: true },
    ];
    const spec = ToolSchemaSpec{
        tool_name: "read_file",
        description: "Read a workspace file",
        strict: true,
        props: props[0..],
    };

    let buf: [512]u8 = [0; 512];
    let out: []u8 = buf[0..];

    const n_oai = emitOpenAiToolJson(spec, out) ?? 0;
    const exp_oai = "{\"type\":\"function\",\"function\":{\"name\":\"read_file\",\"description\":\"Read a workspace file\",\"strict\":true,\"parameters\":{\"type\":\"object\",\"properties\":{\"path\":{\"type\":\"string\",\"description\":\"File path\"}},\"required\":[\"path\"],\"additionalProperties\":false}}}";
    expect_eq(std.mem.eql(u8, out[0..n_oai], exp_oai), true);

    const n_ant = emitAnthropicToolJson(spec, out) ?? 0;
    const exp_ant = "{\"name\":\"read_file\",\"description\":\"Read a workspace file\",\"input_schema\":{\"type\":\"object\",\"properties\":{\"path\":{\"type\":\"string\",\"description\":\"File path\"}},\"required\":[\"path\"],\"additionalProperties\":false}}";
    expect_eq(std.mem.eql(u8, out[0..n_ant], exp_ant), true);
}

fn testJsonStringEscapingInDescriptions()! {
    const ctrl_desc: [12]u8 = ['a', '"', '\\', '\n', '\r', '\t', 0x08, 0x0C, 0x01, 'b', 'c', 'd'];
    const props: [1]SchemaProp = [
        SchemaProp{
            name: "msg",
            kind: JsonSchemaType.String,
            description: ctrl_desc[0..],
            required: true,
        },
    ];
    let buf: [512]u8 = [0; 512];
    let out: []u8 = buf[0..];
    const n = emitJsonSchemaObject(props[0..], false, out) ?? 0;
    const expected = "{\"type\":\"object\",\"properties\":{\"msg\":{\"type\":\"string\",\"description\":\"a\\\"\\\\\\n\\r\\t\\b\\f\\u0001bcd\"}},\"required\":[\"msg\"]}";
    expect_eq(std.mem.eql(u8, out[0..n], expected), true);
}

fn testBufferAndValidationBoundaryGuards()! {
    // 1. Empty props list produces valid empty schema object
    const empty_props: [0]SchemaProp = undefined;
    let buf: [256]u8 = [0; 256];
    let out: []u8 = buf[0..];
    const n_empty = emitJsonSchemaObject(empty_props[0..], true, out) ?? 0;
    const exp_empty = "{\"type\":\"object\",\"properties\":{},\"required\":[],\"additionalProperties\":false}";
    expect_eq(std.mem.eql(u8, out[0..n_empty], exp_empty), true);

    // 2. Exact-size buffer succeeds, 1-byte-short buffer returns null
    expect_eq(n_empty, exp_empty.len);
    let exact_buf: [76]u8 = [0; 76];
    let exact_out: []u8 = exact_buf[0..];
    expect_eq(emitJsonSchemaObject(empty_props[0..], true, exact_out), 76);

    let short_buf: [75]u8 = [0; 75];
    let short_out: []u8 = short_buf[0..];
    expect_eq(emitJsonSchemaObject(empty_props[0..], true, short_out) == null, true);

    // 3. Empty tool_name or empty prop.name rejected with null
    const bad_spec = ToolSchemaSpec{
        tool_name: "",
        description: "desc",
        strict: true,
        props: empty_props[0..],
    };
    expect_eq(emitOpenAiToolJson(bad_spec, out) == null, true);
    expect_eq(emitAnthropicToolJson(bad_spec, out) == null, true);

    const bad_props: [1]SchemaProp = [
        SchemaProp{ name: "", kind: JsonSchemaType.String, required: true },
    ];
    expect_eq(emitJsonSchemaObject(bad_props[0..], true, out) == null, true);

    // 4. Tiny buffer on OpenAI & Anthropic emitters returns null
    const ok_spec = ToolSchemaSpec{
        tool_name: "ping",
        description: "Ping",
        strict: false,
        props: empty_props[0..],
    };
    let tiny_buf: [16]u8 = [0; 16];
    let tiny_out: []u8 = tiny_buf[0..];
    expect_eq(emitOpenAiToolJson(ok_spec, tiny_out) == null, true);
    expect_eq(emitAnthropicToolJson(ok_spec, tiny_out) == null, true);
}

fn main() {
    test("1. prompt json_schema: all 6 types, required array & strict toggle", () => {
        testAllSchemaTypesEmission()?;
    });
    test("2. prompt json_schema: enum CSV trimming & array items emission", () => {
        testEnumCsvAndArrayItems()?;
    });
    test("3. prompt json_schema: OpenAI vs Anthropic dual-format tool specs", () => {
        testOpenAiAndAnthropicToolSpecFormats()?;
    });
    test("4. prompt json_schema: RFC 8259 JSON string escaping in descriptions", () => {
        testJsonStringEscapingInDescriptions()?;
    });
    test("5. prompt json_schema: buffer boundaries, empty props & invalid name guards", () => {
        testBufferAndValidationBoundaryGuards()?;
    });
}
