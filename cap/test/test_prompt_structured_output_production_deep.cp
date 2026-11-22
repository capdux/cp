// Origin: Pure-CP Stdlib Test Suite (lib/prompt/structured_output.cp) | Created: 2026-11-22
// Production Deep Verification Suite for LLM Structured Output Extraction, Repair & Retry Prompting:
//   1. extractJsonCandidate: <think>...</think>, Markdown ```json fences, prose, arrays & truncation detection
//   2. repairLlmJson: trailing commas, single-quoted strings/keys & unescaped raw newlines
//   3. repairLlmJson: truncated string/brackets & dangling colon recovery
//   4. checkRequiredJsonFields: top-level type validation & nested-key non-leakage
//   5. formatRepairRetryPrompt: actionable self-correction prompt generation for missing/mismatch/non-object
//   6. End-to-end pipeline & buffer-too-small / empty / invalid-input defensive guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    JsonCandidateSpan,
    JsonRepairReport,
    JsonValueKind,
    RequiredFieldSpec,
    FieldCheckIssueKind,
    FieldCheckIssue,
    RequiredFieldsReport,
    extractJsonCandidate,
    repairLlmJson,
    checkRequiredJsonFields,
    formatRepairRetryPrompt,
} from "cap:prompt";

fn containsSlice(haystack: string, needle: string): bool {
    return std.mem.indexOf(u8, haystack, needle) != null;
}

// ============================================================================
// Test 1: extractJsonCandidate (<think>, Markdown Fence, Prose, Truncated)
// ============================================================================

fn testExtractJsonCandidate()! {
    const raw1 = "<think>Let me analyze {not_the_answer} first.</think>\nSure! Here is the JSON:\n```json\n{\"status\": \"ok\", \"msg\": \"brace } in string\"}\n```\nHope that helps!";
    const span1 = cap.prompt.extractJsonCandidate(raw1) ?? return error.ExtractFailed;
    expect_eq(span1.skipped_thought, true);
    expect_eq(span1.from_code_fence, true);
    expect_eq(span1.is_truncated, false);
    const slice1 = raw1[span1.start..span1.end];
    expect_eq(std.mem.eql(u8, slice1, "{\"status\": \"ok\", \"msg\": \"brace } in string\"}"), true);

    // Orphan </think> (prefilled prompt) + plain prose without code fence + array root
    const raw2 = "reasoning step...</think> Answer: [1, 2, 3] done.";
    const span2 = extractJsonCandidate(raw2) ?? return error.ExtractFailed;
    expect_eq(span2.skipped_thought, true);
    expect_eq(span2.from_code_fence, false);
    expect_eq(span2.is_truncated, false);
    expect_eq(std.mem.eql(u8, raw2[span2.start..span2.end], "[1, 2, 3]"), true);

    // Truncated JSON inside unclosed code fence
    const raw3 = "```json\n{\"items\": [1, 2";
    const span3 = extractJsonCandidate(raw3) ?? return error.ExtractFailed;
    expect_eq(span3.from_code_fence, true);
    expect_eq(span3.is_truncated, true);
    expect_eq(std.mem.eql(u8, raw3[span3.start..span3.end], "{\"items\": [1, 2"), true);

    // No JSON present or empty input -> null
    expect_eq(extractJsonCandidate("") == null, true);
    expect_eq(extractJsonCandidate("Just plain text without any json.") == null, true);
}

// ============================================================================
// Test 2: repairLlmJson (Trailing Commas, Single Quotes & Raw Newlines)
// ============================================================================

fn testRepairTrailingCommasQuotesAndNewlines()! {
    let out_buf: [256]u8 = [0; 256];
    let out_s: []u8 = out_buf[0..];

    // 1. Trailing commas in object & array
    const malformed1 = "{\"a\": 1, \"arr\": [10, 20, ], }";
    const rep1 = repairLlmJson(malformed1, out_s) ?? return error.RepairFailed;
    expect_eq(rep1.was_modified, true);
    expect_eq(rep1.removed_trailing_commas, 2);
    expect_eq(rep1.closed_brackets, 0);
    expect_eq(std.mem.eql(u8, out_buf[0..rep1.bytes_written], "{\"a\": 1, \"arr\": [10, 20 ] }"), true);

    // 2. Single-quoted keys & strings with inner double quote + raw newline
    const malformed2 = "{'msg': 'line1\nwith \"quote\"',}";
    const rep2 = repairLlmJson(malformed2, out_s) ?? return error.RepairFailed;
    expect_eq(rep2.was_modified, true);
    expect_eq(rep2.normalized_single_quotes, 2);
    expect_eq(rep2.escaped_newlines, 1);
    expect_eq(rep2.removed_trailing_commas, 1);
    expect_eq(
        std.mem.eql(u8, out_buf[0..rep2.bytes_written], "{\"msg\":\"line1\\nwith \\\"quote\\\"\"}"),
        false,
    );
    // Exact check preserving space after ':'
    expect_eq(
        std.mem.eql(u8, out_buf[0..rep2.bytes_written], "{\"msg\": \"line1\\nwith \\\"quote\\\"\"}"),
        true,
    );

    // 3. Clean valid JSON passes through unmodified
    const clean = "{\"ok\": true}";
    const rep3 = repairLlmJson(clean, out_s) ?? return error.RepairFailed;
    expect_eq(rep3.was_modified, false);
    expect_eq(std.mem.eql(u8, out_buf[0..rep3.bytes_written], clean), true);
}

// ============================================================================
// Test 3: repairLlmJson (Truncated String, Nested Brackets & Dangling Colon)
// ============================================================================

fn testRepairTruncatedJson()! {
    let out_buf: [256]u8 = [0; 256];
    let out_s: []u8 = out_buf[0..];

    // 1. Truncated mid-string inside nested array and object
    const trunc1 = "{\"user\": {\"name\": \"alice\", \"roles\": [\"admin";
    const rep1 = repairLlmJson(trunc1, out_s) ?? return error.RepairFailed;
    expect_eq(rep1.was_modified, true);
    expect_eq(rep1.closed_string, true);
    expect_eq(rep1.closed_brackets, 3);
    expect_eq(
        std.mem.eql(u8, out_buf[0..rep1.bytes_written], "{\"user\": {\"name\": \"alice\", \"roles\": [\"admin\"]}}"),
        true,
    );

    // 2. Truncated right after trailing comma
    const trunc2 = "{\"a\": 1,";
    const rep2 = repairLlmJson(trunc2, out_s) ?? return error.RepairFailed;
    expect_eq(rep2.removed_trailing_commas, 1);
    expect_eq(rep2.closed_brackets, 1);
    expect_eq(std.mem.eql(u8, out_buf[0..rep2.bytes_written], "{\"a\": 1}"), true);

    // 3. Truncated right after key colon
    const trunc3 = "{\"a\": 1, \"pending\":";
    const rep3 = repairLlmJson(trunc3, out_s) ?? return error.RepairFailed;
    expect_eq(rep3.closed_brackets, 1);
    expect_eq(std.mem.eql(u8, out_buf[0..rep3.bytes_written], "{\"a\": 1, \"pending\":null}"), true);
}

// ============================================================================
// Test 4: checkRequiredJsonFields (Types & Nested-Key Isolation)
// ============================================================================

fn testCheckRequiredFieldsAndIsolation()! {
    const json_ok = "{\"title\": \"CP\", \"count\": 42, \"ratio\": 0.95, \"active\": true, \"tags\": [\"ai\"], \"meta\": {\"secret\": 1}, \"note\": null}";
    const specs: [7]RequiredFieldSpec = [
        RequiredFieldSpec{ name: "title", expected_kind: JsonValueKind.String },
        RequiredFieldSpec{ name: "count", expected_kind: JsonValueKind.Integer },
        RequiredFieldSpec{ name: "ratio", expected_kind: JsonValueKind.Number },
        RequiredFieldSpec{ name: "active", expected_kind: JsonValueKind.Boolean },
        RequiredFieldSpec{ name: "tags", expected_kind: JsonValueKind.Array },
        RequiredFieldSpec{ name: "meta", expected_kind: JsonValueKind.Object },
        RequiredFieldSpec{ name: "note", expected_kind: JsonValueKind.Null },
    ];
    let issues_buf: [8]FieldCheckIssue = [FieldCheckIssue{}; 8];
    let issues_s: []FieldCheckIssue = issues_buf[0..];

    const r1 = checkRequiredJsonFields(json_ok, specs[0..], issues_s);
    expect_eq(r1.passed, true);
    expect_eq(r1.is_valid_object, true);
    expect_eq(r1.checked_fields, 7);
    expect_eq(r1.missing_count, 0);
    expect_eq(r1.type_mismatch_count, 0);
    expect_eq(r1.issue_count, 0);

    // Integer also satisfies Number, and nested "secret" must NOT satisfy top-level "secret".
    const specs_nested: [2]RequiredFieldSpec = [
        RequiredFieldSpec{ name: "count", expected_kind: JsonValueKind.Number },
        RequiredFieldSpec{ name: "secret", expected_kind: JsonValueKind.Integer },
    ];
    const r2 = checkRequiredJsonFields(json_ok, specs_nested[0..], issues_s);
    expect_eq(r2.passed, false);
    expect_eq(r2.missing_count, 1);
    expect_eq(r2.issue_count, 1);
    expect_eq(issues_buf[0].issue_kind == FieldCheckIssueKind.Missing, true);
    expect_eq(std.mem.eql(u8, issues_buf[0].field_name, "secret"), true);
}

// ============================================================================
// Test 5: formatRepairRetryPrompt (Missing, TypeMismatch & NotAnObject)
// ============================================================================

fn testFormatRepairRetryPrompt()! {
    const bad_json = "{\"title\": \"CP\", \"tags\": \"not-an-array\"}";
    const specs: [3]RequiredFieldSpec = [
        RequiredFieldSpec{ name: "title", expected_kind: JsonValueKind.String },
        RequiredFieldSpec{ name: "confidence", expected_kind: JsonValueKind.Number },
        RequiredFieldSpec{ name: "tags", expected_kind: JsonValueKind.Array },
    ];
    let issues_buf: [4]FieldCheckIssue = [FieldCheckIssue{}; 4];
    let issues_s: []FieldCheckIssue = issues_buf[0..];

    const chk = checkRequiredJsonFields(bad_json, specs[0..], issues_s);
    expect_eq(chk.passed, false);
    expect_eq(chk.missing_count, 1);
    expect_eq(chk.type_mismatch_count, 1);
    expect_eq(chk.issue_count, 2);

    let prompt_buf: [512]u8 = [0; 512];
    let prompt_s: []u8 = prompt_buf[0..];
    const n = formatRepairRetryPrompt(
        issues_buf[0..],
        chk.issue_count,
        "{title: string, confidence: number, tags: string[]}",
        prompt_s,
    ) ?? return error.FormatFailed;

    const prompt = prompt_buf[0..n];
    expect_eq(containsSlice(prompt, "Missing required field \"confidence\" (expected number)"), true);
    expect_eq(containsSlice(prompt, "Field \"tags\" has wrong type: expected array, got string"), true);
    expect_eq(containsSlice(prompt, "Expected schema: {title: string, confidence: number, tags: string[]}"), true);

    // Non-object root produces NotAnObject issue & prompt
    const chk_arr = checkRequiredJsonFields("[1, 2, 3]", specs[0..], issues_s);
    expect_eq(chk_arr.passed, false);
    expect_eq(chk_arr.is_valid_object, false);
    expect_eq(chk_arr.issue_count, 1);
    expect_eq(issues_buf[0].issue_kind == FieldCheckIssueKind.NotAnObject, true);

    const n2 = formatRepairRetryPrompt(issues_buf[0..], chk_arr.issue_count, "", prompt_s) ?? return error.FormatFailed;
    expect_eq(containsSlice(prompt_buf[0..n2], "Root value must be a JSON object"), true);
}

// ============================================================================
// Test 6: End-to-End Pipeline & Defensive Buffer/Empty Guards
// ============================================================================

fn testEndToEndAndDefensiveGuards()! {
    // Full pipeline: <think> + fenced malformed truncated JSON -> extract -> repair -> validate
    const llm_raw = "<think>checking</think>\nOutput:\n```json\n{'action': 'search', 'limit': 5, 'filters': ['rust', 'zig',";
    const span = extractJsonCandidate(llm_raw) ?? return error.ExtractFailed;
    expect_eq(span.skipped_thought, true);
    expect_eq(span.from_code_fence, true);
    expect_eq(span.is_truncated, true);

    let repaired_buf: [256]u8 = [0; 256];
    let repaired_s: []u8 = repaired_buf[0..];
    const rep = repairLlmJson(llm_raw[span.start..span.end], repaired_s) ?? return error.RepairFailed;
    const clean = repaired_buf[0..rep.bytes_written];
    expect_eq(
        std.mem.eql(u8, clean, "{\"action\": \"search\", \"limit\": 5, \"filters\": [\"rust\", \"zig\"]}"),
        true,
    );

    const specs: [3]RequiredFieldSpec = [
        RequiredFieldSpec{ name: "action", expected_kind: JsonValueKind.String },
        RequiredFieldSpec{ name: "limit", expected_kind: JsonValueKind.Integer },
        RequiredFieldSpec{ name: "filters", expected_kind: JsonValueKind.Array },
    ];
    let issues_buf: [4]FieldCheckIssue = [FieldCheckIssue{}; 4];
    let issues_s: []FieldCheckIssue = issues_buf[0..];
    const chk = checkRequiredJsonFields(clean, specs[0..], issues_s);
    expect_eq(chk.passed, true);

    // --- Defensive Guards ---
    // Empty/whitespace input to repairLlmJson -> null
    expect_eq(repairLlmJson("   \n\t ", repaired_s) == null, true);

    // Undersized out_buf for repairLlmJson -> null (no out-of-bounds write)
    let tiny_buf: [4]u8 = [0; 4];
    let tiny_s: []u8 = tiny_buf[0..];
    expect_eq(repairLlmJson("{\"key\": \"value\"}", tiny_s) == null, true);

    // Undersized out_buf & out-of-range issue_count for formatRepairRetryPrompt -> null
    expect_eq(formatRepairRetryPrompt(issues_buf[0..], 5, "", repaired_s) == null, true);
    expect_eq(formatRepairRetryPrompt(issues_buf[0..], 0, "", tiny_s) == null, true);
}

fn main() {
    test("1. prompt.structured_output: extractJsonCandidate (<think>, fence, prose, truncated)", () => {
        testExtractJsonCandidate()?;
    });
    test("2. prompt.structured_output: repairLlmJson (trailing commas, single quotes, raw newlines)", () => {
        testRepairTrailingCommasQuotesAndNewlines()?;
    });
    test("3. prompt.structured_output: repairLlmJson (truncated strings, brackets & colon)", () => {
        testRepairTruncatedJson()?;
    });
    test("4. prompt.structured_output: checkRequiredJsonFields (types & nested-key isolation)", () => {
        testCheckRequiredFieldsAndIsolation()?;
    });
    test("5. prompt.structured_output: formatRepairRetryPrompt (missing, mismatch, non-object)", () => {
        testFormatRepairRetryPrompt()?;
    });
    test("6. prompt.structured_output: end-to-end pipeline & defensive guards", () => {
        testEndToEndAndDefensiveGuards()?;
    });
}
