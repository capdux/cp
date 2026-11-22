// Origin: Pure-CP Stdlib Test Suite (lib/mcp/arg_validator.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Heap MCP Tool Argument Constraint Validator:
//   1. All 6 types (Str, Int, Float, Bool, Array, Object), negative int bounds, escaped string len & enum CSV
//   2. MissingRequired, TypeMismatch, StringTooShort/Long, IntOutOfRange, EnumMismatch & DuplicateKey
//   3. Nested object/array same-name key isolation (inner keys never shadow or satisfy top-level rules)
//   4. AI-friendly self-healing error hint formatting (`formatMcpArgErrorHint`)
//   5. Small violation/hint buffer safety & truncation telemetry
//   6. Malformed JSON (array root, trailing comma, trailing garbage, leading zero, bad escape, null) guards

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    McpArgType,
    McpArgRule,
    McpArgViolationKind,
    McpArgViolation,
    validateMcpToolArgs,
    formatMcpArgErrorHint,
} from "cap:mcp";

fn containsSubstr(haystack: []const u8, needle: string): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i <= haystack.len - needle.len) : (i += 1) {
        let j: usize = 0;
        let ok: bool = true;
        while (j < needle.len) : (j += 1) {
            if (haystack[i + j] != needle[j]) {
                ok = false;
                break;
            }
        }
        if (ok) return true;
    }
    return false;
}

// ============================================================================
// Test 1: All 6 Types Valid Pass, Boundary Values & Escaped String Length
// ============================================================================

fn testAllTypesValidPass()! {
    const rules: [6]McpArgRule = [
        McpArgRule{ name: "query", kind: McpArgType.Str, required: true, min_len: 2, max_len: 5 },
        McpArgRule{ name: "offset", kind: McpArgType.Int, required: true, min_int: -10, max_int: 100, has_int_bounds: true },
        McpArgRule{ name: "score", kind: McpArgType.Float, required: true },
        McpArgRule{ name: "strict", kind: McpArgType.Bool, required: true },
        McpArgRule{ name: "tags", kind: McpArgType.Array, required: true },
        McpArgRule{ name: "filter", kind: McpArgType.Object, required: true },
    ];
    let viols: [8]McpArgViolation = [McpArgViolation{}; 8];

    // "a\nb" has decoded_len == 3 (fits [2, 5]); offset == -10 (exact min_int); score == 0.85
    const json: string = "{\"query\":\"a\\nb\",\"offset\":-10,\"score\":0.85,\"strict\":true,\"tags\":[\"v1\",2],\"filter\":{\"k\":1}}";
    const rep = cap.mcp.validateMcpToolArgs(json, rules[0..], viols[0..]);
    expect_eq(rep.valid, true);
    expect_eq(rep.violation_count, 0);
    expect_eq(rep.total_violations, 0);
    expect_eq(rep.matched_fields, 6);
    expect_eq(rep.malformed_json, false);

    // Integer literal is also accepted by Float rule
    const json2: string = "{\"query\":\"hi\",\"offset\":100,\"score\":1,\"strict\":false,\"tags\":[],\"filter\":{}}";
    const rep2 = validateMcpToolArgs(json2, rules[0..], viols[0..]);
    expect_eq(rep2.valid, true);
    expect_eq(rep2.matched_fields, 6);
}

// ============================================================================
// Test 2: All Violation Kinds Accurately Detected
// ============================================================================

fn testAllViolationKindsDetected()! {
    const rules: [6]McpArgRule = [
        McpArgRule{ name: "req_id", kind: McpArgType.Str, required: true },
        McpArgRule{ name: "short_s", kind: McpArgType.Str, required: false, min_len: 3, max_len: 8 },
        McpArgRule{ name: "long_s", kind: McpArgType.Str, required: false, min_len: 1, max_len: 4 },
        McpArgRule{ name: "limit", kind: McpArgType.Int, required: false, min_int: 1, max_int: 50, has_int_bounds: true },
        McpArgRule{ name: "order", kind: McpArgType.Str, required: false, enum_csv: "asc,desc" },
        McpArgRule{ name: "count", kind: McpArgType.Int, required: false },
    ];
    let viols: [10]McpArgViolation = [McpArgViolation{}; 10];

    // Triggers: StringTooShort(short_s), StringTooLong(long_s), IntOutOfRange(limit=99),
    //           EnumMismatch(order="up"), TypeMismatch(count=3.5), DuplicateKey(order), MissingRequired(req_id)
    const json: string = "{\"short_s\":\"a\",\"long_s\":\"abcdef\",\"limit\":99,\"order\":\"up\",\"count\":3.5,\"order\":\"asc\"}";
    const rep = validateMcpToolArgs(json, rules[0..], viols[0..]);
    expect_eq(rep.valid, false);
    expect_eq(rep.malformed_json, false);
    expect_eq(rep.total_violations, 7);
    expect_eq(rep.violation_count, 7);

    expect_eq(viols[0].kind == McpArgViolationKind.StringTooShort, true);
    expect_eq(viols[1].kind == McpArgViolationKind.StringTooLong, true);
    expect_eq(viols[2].kind == McpArgViolationKind.IntOutOfRange, true);
    expect_eq(viols[3].kind == McpArgViolationKind.EnumMismatch, true);
    expect_eq(viols[4].kind == McpArgViolationKind.TypeMismatch, true);
    expect_eq(viols[5].kind == McpArgViolationKind.DuplicateKey, true);
    expect_eq(viols[6].kind == McpArgViolationKind.MissingRequired, true);
}

// ============================================================================
// Test 3: Nested Object / Array Same-Name Key Isolation
// ============================================================================

fn testNestedSameNameKeyIsolation()! {
    const rules: [2]McpArgRule = [
        McpArgRule{ name: "path", kind: McpArgType.Str, required: true, min_len: 1 },
        McpArgRule{ name: "limit", kind: McpArgType.Int, required: false, min_int: 1, max_int: 10, has_int_bounds: true },
    ];
    let viols: [4]McpArgViolation = [McpArgViolation{}; 4];

    // Nested object and array contain "path" and out-of-range "limit": 9999,
    // but top-level "path" is missing and top-level "limit" is 5 (valid).
    const json: string = "{\"meta\":{\"path\":\"nested.cp\",\"limit\":9999},\"items\":[{\"path\":\"x\"}],\"limit\":5}";
    const rep = validateMcpToolArgs(json, rules[0..], viols[0..]);
    expect_eq(rep.valid, false);
    expect_eq(rep.total_violations, 1);
    expect_eq(viols[0].kind == McpArgViolationKind.MissingRequired, true);
    expect_eq(viols[0].rule_index, 0); // "path"
}

// ============================================================================
// Test 4: AI-Friendly Self-Healing Error Hint Formatting
// ============================================================================

fn testAiFriendlyErrorHintFormatting()! {
    const rules: [3]McpArgRule = [
        McpArgRule{ name: "query", kind: McpArgType.Str, required: true, min_len: 1 },
        McpArgRule{ name: "limit", kind: McpArgType.Int, required: false, min_int: 1, max_int: 100, has_int_bounds: true },
        McpArgRule{ name: "order", kind: McpArgType.Str, required: false, enum_csv: "asc,desc" },
    ];
    let viols: [4]McpArgViolation = [McpArgViolation{}; 4];
    let hint_buf: [256]u8 = [0; 256];

    // 1. Valid case -> "OK"
    const rep_ok = validateMcpToolArgs("{\"query\":\"hello\",\"limit\":10,\"order\":\"desc\"}", rules[0..], viols[0..]);
    const n_ok = formatMcpArgErrorHint(rep_ok, viols[0..], hint_buf[0..]) ?? return error.HintFailed;
    expect_eq(n_ok, 2);
    expect_eq(containsSubstr(hint_buf[0..n_ok], "OK"), true);

    // 2. Multi-violation case -> mentions field names, range [1, 100], enum asc,desc, and missing query
    const rep_bad = validateMcpToolArgs("{\"limit\":250,\"order\":\"random\"}", rules[0..], viols[0..]);
    expect_eq(rep_bad.valid, false);
    const n_bad = formatMcpArgErrorHint(rep_bad, viols[0..], hint_buf[0..]) ?? return error.HintFailed;
    const hint_slice = hint_buf[0..n_bad];
    expect_eq(containsSubstr(hint_slice, "limit"), true);
    expect_eq(containsSubstr(hint_slice, "[1, 100]"), true);
    expect_eq(containsSubstr(hint_slice, "order"), true);
    expect_eq(containsSubstr(hint_slice, "asc,desc"), true);
    expect_eq(containsSubstr(hint_slice, "query"), true);
}

// ============================================================================
// Test 5: Small Violation Buffer & Small Hint Buffer Guards
// ============================================================================

fn testSmallBufferGuards()! {
    const rules: [3]McpArgRule = [
        McpArgRule{ name: "a", kind: McpArgType.Str, required: true },
        McpArgRule{ name: "b", kind: McpArgType.Int, required: true },
        McpArgRule{ name: "c", kind: McpArgType.Bool, required: true },
    ];
    // Only 1 slot in out_violations, yet 3 required fields are missing
    let small_viols: [1]McpArgViolation = [McpArgViolation{}; 1];
    const rep = validateMcpToolArgs("{}", rules[0..], small_viols[0..]);
    expect_eq(rep.valid, false);
    expect_eq(rep.violation_count, 1);
    expect_eq(rep.total_violations, 3);

    // Tiny hint buffer returns null without overflow
    let tiny_buf: [8]u8 = [0; 8];
    expect_eq(formatMcpArgErrorHint(rep, small_viols[0..], tiny_buf[0..]) == null, true);
}

// ============================================================================
// Test 6: Malformed JSON & Null Value Defensive Guards
// ============================================================================

fn testMalformedJsonGuards()! {
    const rules: [1]McpArgRule = [
        McpArgRule{ name: "x", kind: McpArgType.Int, required: false },
    ];
    let viols: [2]McpArgViolation = [McpArgViolation{}; 2];

    // Non-object root, truncated object, trailing comma, trailing garbage, leading zero, bad escape
    expect_eq(validateMcpToolArgs("[1,2,3]", rules[0..], viols[0..]).malformed_json, true);
    expect_eq(validateMcpToolArgs("{\"x\":1", rules[0..], viols[0..]).malformed_json, true);
    expect_eq(validateMcpToolArgs("{\"x\":1,}", rules[0..], viols[0..]).malformed_json, true);
    expect_eq(validateMcpToolArgs("{\"x\":1} extra", rules[0..], viols[0..]).malformed_json, true);
    expect_eq(validateMcpToolArgs("{\"x\":01}", rules[0..], viols[0..]).malformed_json, true);
    expect_eq(validateMcpToolArgs("{\"x\":\"\\q\"}", rules[0..], viols[0..]).malformed_json, true);

    // Malformed hint renders descriptive message
    const rep_m = validateMcpToolArgs("not-json", rules[0..], viols[0..]);
    let hint_buf: [128]u8 = [0; 128];
    const n_m = formatMcpArgErrorHint(rep_m, viols[0..], hint_buf[0..]) ?? return error.HintFailed;
    expect_eq(containsSubstr(hint_buf[0..n_m], "JSON object"), true);

    // Valid JSON null fails Int type check with TypeMismatch (not MalformedJson)
    const rep_null = validateMcpToolArgs("{\"x\":null}", rules[0..], viols[0..]);
    expect_eq(rep_null.valid, false);
    expect_eq(rep_null.malformed_json, false);
    expect_eq(viols[0].kind == McpArgViolationKind.TypeMismatch, true);
}

// ============================================================================
// Test Runner Registration
// ============================================================================

test("mcp/arg_validator: all 6 types valid pass, bounds & escaped string len", testAllTypesValidPass);
test("mcp/arg_validator: missing/type/length/range/enum/duplicate violations", testAllViolationKindsDetected);
test("mcp/arg_validator: nested object/array same-name key isolation", testNestedSameNameKeyIsolation);
test("mcp/arg_validator: AI-friendly self-healing error hint formatting", testAiFriendlyErrorHintFormatting);
test("mcp/arg_validator: small violation & hint buffer safety guards", testSmallBufferGuards);
test("mcp/arg_validator: malformed JSON & null value defensive guards", testMalformedJsonGuards);
